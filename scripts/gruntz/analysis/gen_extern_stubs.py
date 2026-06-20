#!/usr/bin/env python3
"""gen_extern_stubs.py - generate compilable src/Stub stubs from the extern harvest.

Reads the extern_harvest TSV (callers,kind,state,actionable,rva,size,name) and
emits empty-body stubs (each with its `RVA(rva, size)`) for the actionable engine
FUNCTIONS, so the delinker names their entry instead of a FUN_<rva> placeholder
and every matched caller's `call` reloc resolves to the real symbol.

The mangled name IS the contract: we demangle (llvm-undname) to a readable
signature, then re-emit C++ that reproduces the SAME mangling (access + calling
convention + const + return/param types). A stub that mangled differently would
define a symbol nobody references, so the build + a re-harvest are the integration
check (a wrong mangling leaves its target still unresolved).

This phase handles C++ `?`-names only (methods / free functions / ctors / dtors)
with pointer/primitive signatures - the clean, mechanical tier. It SKIPS (and
reports) nested-class methods, by-value class params/returns, and any class that a
stub file already included by All.cpp declares (would double-declare in that TU).
C-linkage `_name`/`_name@N` functions and DATA globals are later phases.

Output: one file `src/Stub/GenExterns.cpp` grouped by class, plus the `#include`
line to add to src/Stub/All.cpp. Run: python -m gruntz.analysis.gen_extern_stubs
"""

import argparse
import csv
import re
import subprocess
from collections import OrderedDict, defaultdict
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
REPO = next((p for p in SCRIPT_DIR.parents if (p / "flake.nix").exists()), SCRIPT_DIR)

CC_RE = re.compile(r"\b(__cdecl|__thiscall|__stdcall|__fastcall)\b")
CLASS_REF_RE = re.compile(r"\b(class|struct|enum)\s+([A-Za-z_]\w*)")
PRIMITIVES = {
    "int", "unsigned int", "char", "unsigned char", "signed char", "short",
    "unsigned short", "long", "unsigned long", "bool", "float", "double",
    "__int64", "unsigned __int64", "wchar_t", "void",
}

# Exclude: the operator-delete alias + Win32 API the classifier over-claims as eng.
SKIP_NAMES = {"_MapFree"}
WIN32_API = {"GetFileVersionInfoA", "GetFileVersionInfoSizeA", "VerQueryValueA"}
CLINK_STDCALL_RE = re.compile(r"^_([A-Za-z_]\w*)@(\d+)$")   # _name@N -> __stdcall
CLINK_CDECL_RE = re.compile(r"^_([A-Za-z_]\w*)$")           # _name   -> __cdecl


def demangle(name, tool="llvm-undname"):
    out = subprocess.run([tool, name], capture_output=True, text=True).stdout
    lines = [ln for ln in out.splitlines() if ln.strip()]
    return lines[-1] if lines else ""


def parse_sig(dem):
    """Demangled signature -> dict, or None if not a handleable C++ function."""
    s = dem.strip()
    m = re.match(r"(public|protected|private):\s*", s)
    access = m.group(1) if m else "public"
    if m:
        s = s[m.end():]
    for kw in ("static ", "virtual "):
        while s.startswith(kw):
            s = s[len(kw):]
    is_static = "static " in dem and dem.index("static ") < dem.find("(")
    is_const = s.endswith(" const")
    if is_const:
        s = s[: -len(" const")]
    mcc = CC_RE.search(s)
    if not mcc:
        return None
    cc = mcc.group(1)
    ret = s[: mcc.start()].strip()
    rest = s[mcc.end():].strip()
    if "(" not in rest:
        return None
    qual = rest[: rest.index("(")].strip()
    params = rest[rest.index("(") + 1: rest.rindex(")")].strip()
    cls, meth = (qual.rsplit("::", 1) + [None])[:2] if "::" in qual else (None, qual)
    if cls is None:
        cls, meth = None, qual
    else:
        cls, meth = qual.rsplit("::", 1)
    return dict(access=access, static=is_static, const=is_const, cc=cc,
                ret=ret, cls=cls, meth=meth, params=params)


def split_params(params):
    if params in ("", "void"):
        return []
    out, depth, cur = [], 0, ""
    for ch in params:
        if ch in "<(":
            depth += 1
        elif ch in ">)":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip()); cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def cc_kw(cc):
    return cc + " " if cc in ("__stdcall", "__fastcall") else ""


def by_value_class(t):
    """A param/return that is a class/struct value (no * or &) -> needs full type."""
    if "*" in t or "&" in t:
        return False
    return bool(CLASS_REF_RE.search(t))


def collect_type_refs(types, exclude):
    refs = OrderedDict()
    for t in types:
        for kind, nm in CLASS_REF_RE.findall(t):
            if nm not in exclude:
                refs[(kind, nm)] = None
    return list(refs.keys())


def body_for(ret):
    return "{}" if ret in ("", "void") else "{ return 0; }"


def stub_header(callers):
    """The verify_stubs `// @confidence / @source / @stub` block. Consensus across
    >=2 callers is high confidence; a single caller is unverified (med)."""
    conf = "high" if callers >= 2 else "med"
    plural = "" if callers == 1 else "s"
    return ["// @confidence: %s" % conf,
            "// @source: reloc-correlation (%d caller%s)" % (callers, plural),
            "// @stub"]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--tsv", default="/tmp/externs.tsv")
    ap.add_argument("--out", default=str(REPO / "src/Stub/GenExterns.cpp"))
    ap.add_argument("--stub-dir", default=str(REPO / "src/Stub"))
    ap.add_argument("--names", default=str(REPO / "build/gen/symbol_names.csv"))
    args = ap.parse_args()

    # RVAs already labeled (in some unit). An extern that correlates to one of
    # these is the SAME retail function under a different name - our caller
    # misnames an already-named function. Stubbing would duplicate the RVA (the
    # labels.py guard refuses); the real fix is to rename the caller. Skip+report.
    # An RVA is an ALIAS only when labeled under a DIFFERENT name (the same retail
    # function our caller misnames). If the existing label IS our extern's name,
    # that's our own prior stub - regenerate it (idempotent across builds).
    labeled = {}
    with open(args.names, newline="") as f:
        for r in csv.DictReader(ln for ln in f if not ln.lstrip().startswith("#")):
            if r.get("rva") and r.get("name"):
                labeled[int(r["rva"], 16)] = r["name"].strip()

    # actionable engine functions: C++ `?`-names + C-linkage `_name`/`_name@N`.
    rows, clink = [], []
    with open(args.tsv) as f:
        for r in csv.DictReader(f, delimiter="\t"):
            if r["kind"] != "eng" or r["actionable"] != "1" or r["name"] in SKIP_NAMES:
                continue
            name, rva, size = r["name"], int(r["rva"], 16), int(r["size"])
            callers = int(r["callers"])
            if name.startswith("?"):
                rows.append((name, rva, size, callers))
            elif not name.startswith("_g_"):     # _g_* are data globals (later phase)
                clink.append((name, rva, size, callers))

    # classes already declared by a stub file included in All.cpp -> avoid double-decl.
    existing = set()
    for p in Path(args.stub_dir).rglob("*.cpp"):
        if p.name == "GenExterns.cpp":
            continue
        for kind, nm in CLASS_REF_RE.findall(p.read_text()):
            existing.add(nm)

    by_class = defaultdict(list)     # cls (or None) -> [parsed + rva/size/name]
    skipped = []
    aliases = []                     # (extern, rva, existing canonical name)
    for name, rva, size, callers in sorted(rows, key=lambda x: x[0]):
        if rva in labeled and labeled[rva] != name:
            aliases.append((name, rva, labeled[rva])); continue
        dem = demangle(name)
        sig = parse_sig(dem)
        if not sig:
            skipped.append((name, "unparsed: " + dem)); continue
        cls = sig["cls"]
        if cls and "::" in cls:
            skipped.append((name, "nested class " + cls)); continue
        if cls and cls in existing:
            skipped.append((name, "class %s already declared in a stub" % cls)); continue
        params = split_params(sig["params"])
        bad = [t for t in [sig["ret"]] + params if by_value_class(t)]
        if bad:
            skipped.append((name, "by-value class type %r" % bad)); continue
        sig.update(name=name, rva=rva, size=size, callers=callers, paramlist=params)
        by_class[cls].append(sig)

    # C-linkage `_name`/`_name@N` engine functions -> extern "C" stubs.
    clink_specs = []                 # (symbol, funcname, cc, nargs, rva, size, callers)
    for name, rva, size, callers in sorted(clink):
        if rva in labeled and labeled[rva] != name:
            aliases.append((name, rva, labeled[rva])); continue
        m = CLINK_STDCALL_RE.match(name)
        if m:
            fn, nargs = m.group(1), int(m.group(2)) // 4
            cc = "__stdcall"
        else:
            m = CLINK_CDECL_RE.match(name)
            if not m:
                skipped.append((name, "unrecognized C-linkage name")); continue
            fn, nargs, cc = m.group(1), 0, "__cdecl"
        if fn in WIN32_API:
            skipped.append((name, "Win32 API (library)")); continue
        if size <= 0:
            skipped.append((name, "no Ghidra boundary (size 0) - needs @size")); continue
        clink_specs.append((name, fn, cc, nargs, rva, size, callers))

    # emit.
    L = []
    L.append("#include <rva.h>")
    L.append("// GenExterns.cpp - GENERATED by gruntz.analysis.gen_extern_stubs.")
    L.append("//")
    L.append("// Empty-body stubs for engine functions that matched code calls but")
    L.append("// we have not reconstructed. Each RVA() was recovered by")
    L.append("// reloc-correlation (extern_harvest): matched callers agree on the")
    L.append("// entry address. Naming the entry makes the callers' `call` relocs")
    L.append("// resolve to the real symbol instead of FUN_<rva>. @stub: move each")
    L.append("// into its real class's TU as it is reconstructed.")
    L.append("")

    # forward declarations needed across all groups (pointer/ref params only).
    defined_classes = {c for c in by_class if c}
    all_types = []
    for sigs in by_class.values():
        for s in sigs:
            all_types.append(s["ret"])
            all_types.extend(s["paramlist"])
    # struct-vs-class key per type name (mangles U vs V). A receiver-only class
    # never appears as a U/V operand so its key is irrelevant -> default class;
    # a class used as a param/return type gets the key llvm-undname showed.
    class_key = {}
    for t in all_types:
        for kind, nm in CLASS_REF_RE.findall(t):
            class_key.setdefault(nm, kind)
    fwds = collect_type_refs(all_types, exclude=defined_classes | PRIMITIVES)
    if fwds:
        L.append("// forward declarations (incomplete types are fine for pointer/ref args)")
        for kind, nm in fwds:
            L.append("%s %s;" % (kind, nm))
        L.append("")

    count = 0
    # C-linkage engine functions (extern "C"; __stdcall arity from the @N decoration).
    if clink_specs:
        L.append("// C-linkage engine functions (reloc-correlation).")
        L.append('extern "C" {')
        for sym, fn, cc, nargs, rva, size, callers in clink_specs:
            args_s = ", ".join(["int"] * nargs)
            ccs = (cc + " ") if cc == "__stdcall" else ""
            L.extend(stub_header(callers))
            L.append("RVA(0x%06x, 0x%x)" % (rva, size))
            # clang's IR names an extern "C" fn `fn`; the i386 obj symbol is `_fn`
            # (`_fn@N` stdcall). Pin the real symbol so labels.py's authority check
            # (against the base obj) matches.
            L.append("SYMBOL(%s)" % sym)
            L.append("int %s%s(%s) { return 0; }" % (ccs, fn, args_s))
        L.append("}")
        L.append("")
        count += len(clink_specs)

    # free functions first.
    for s in by_class.get(None, []):
        ret = s["ret"]
        ps = ", ".join(s["paramlist"]) if s["paramlist"] else ""
        L.extend(stub_header(s["callers"]))
        L.append("RVA(0x%06x, 0x%x)" % (s["rva"], s["size"]))
        L.append("%s %s%s(%s) %s" % (ret, cc_kw(s["cc"]), s["meth"], ps, body_for(ret)))
        L.append("")
        count += 1

    for cls in sorted(c for c in by_class if c):
        sigs = by_class[cls]
        L.append("// ---- %s ----" % cls)
        # group declarations by access
        decls = defaultdict(list)
        for s in sigs:
            ps = ", ".join(s["paramlist"]) if s["paramlist"] else ""
            is_ctor = (s["ret"] == "" and s["meth"] == cls)
            is_dtor = (s["ret"] == "" and s["meth"] == "~" + cls)
            if is_ctor:
                d = "%s%s(%s);" % (cc_kw(s["cc"]), cls, ps)
            elif is_dtor:
                d = "%s~%s();" % (cc_kw(s["cc"]), cls)
            else:
                const = " const" if s["const"] else ""
                stat = "static " if s["static"] else ""
                d = "%s%s%s %s(%s)%s;" % (stat, cc_kw(s["cc"]), s["ret"], s["meth"], ps, const)
            decls[s["access"]].append(d)
        L.append("%s %s {" % (class_key.get(cls, "class"), cls))
        for acc in ("public", "protected", "private"):
            if decls.get(acc):
                L.append("%s:" % acc)
                for d in decls[acc]:
                    L.append("    " + d)
        L.append("};")
        # out-of-line definitions with RVA.
        for s in sigs:
            ps = ", ".join(s["paramlist"]) if s["paramlist"] else ""
            const = " const" if s["const"] else ""
            is_ctor = (s["ret"] == "" and s["meth"] == cls)
            is_dtor = (s["ret"] == "" and s["meth"] == "~" + cls)
            L.extend(stub_header(s["callers"]))
            L.append("RVA(0x%06x, 0x%x)" % (s["rva"], s["size"]))
            if is_ctor:
                L.append("%s::%s(%s) {}" % (cls, cls, ps))
            elif is_dtor:
                L.append("%s::~%s() {}" % (cls, cls))
            else:
                L.append("%s %s::%s(%s)%s %s"
                         % (s["ret"], cls, s["meth"], ps, const, body_for(s["ret"])))
            count += 1
        L.append("")

    Path(args.out).write_text("\n".join(L) + "\n")
    print("[gen] wrote %d stub(s) across %d class group(s) -> %s"
          % (count, len([c for c in by_class if c]) + (1 if None in by_class else 0), args.out))
    print("[gen] add to src/Stub/All.cpp:  #include \"GenExterns.cpp\"")
    if skipped:
        print("[gen] skipped %d (later phases / manual):" % len(skipped))
        for nm, why in skipped:
            print("    %-50s %s" % (nm, why))
    if aliases:
        print("[gen] %d ALIAS(es) - our caller misnames an already-labeled function "
              "(rename the caller, do NOT stub):" % len(aliases))
        for nm, rva, canon in sorted(aliases):
            print("    0x%06x  %-44s == %s" % (rva, nm, canon))


if __name__ == "__main__":
    main()
