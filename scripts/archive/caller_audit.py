#!/usr/bin/env python3
"""gruntz.analysis.caller_audit - two-sided caller audit (WS5).

Builds TWO call graphs over the ~3209 `RVA()`-labelled functions and diffs them:

  * TARGET graph - the retail truth. One linear scan of GRUNTZ.EXE's `.text`
    for direct `call`/`jmp rel32` (E8/E9) whose destination is a labelled RVA;
    each site is resolved to its containing function (ghidra fstarts) + unit.
    (This is `xref.py`'s machinery run over every function at once.)

  * BASE graph - what OUR reconstruction actually emits. Each src TU that
    carries `RVA()` functions is compiled to LLVM IR with clang (`labels.emit_ir`,
    the same clang-cl invocation the label build uses), and every `call`/`invoke`
    to a mangled symbol we know is an edge. clang's MS mangling reproduces the
    VC5 symbols, so the IR `@"?callee@..."` names key straight into
    symbol_names.csv - the caller's define and the callee reference both resolve
    to RVAs, name-independently comparable to the target graph.

Diffing the two yields THREE worklists that each cap match %:

  (a) HEADER-PROMOTION - a function with a reconstructed cross-TU (different unit)
      retail caller, whose class/struct is declared only inside a .cpp (or, for a
      free function, has no prototype in include/). The cross-TU caller cannot
      make a real typed call until the class moves to a shared header. This is the
      CLightEffect::Setup / PaletteLerp::Tick / ProjectWallQuad shape.

  (b) ORPHANS - a retail edge caller->callee that the BASE graph does NOT emit:
      the reconstructed caller (its IR was analysed) makes no call to the callee
      in our source. The caller is a stub/@early-stop, or fakes the call through a
      view / raw-offset, so the call reloc is missing and the caller's % is capped.
      Precise (no grep): the clang IR either has the edge or it does not.

  (c) ACCESS-MISMATCH - a function with a reconstructed cross-TU caller in a
      DIFFERENT class, whose symbol access (the char after `@@` in the mangled
      name: A-H=private, I-P=protected, Q-X=public) is not public. A cross-class
      direct call needs a public target; a non-public decl mangles to a different
      symbol than the caller emits, so the reloc never pairs. The CLightEffect
      lesson (Setup must be public=Q). The src class decl is parsed too, and a
      decl-vs-mangled disagreement is reported (a stale SYMBOL()/pin).

Usage (inside `nix develop` - needs clang/llvm-undname + $GRUNTZ_EXE):
    python3 -m gruntz.analysis.caller_audit                 # all 3 -> config/*.md + summary
    python3 -m gruntz.analysis.caller_audit --kind orphan   # one list to stdout
    python3 -m gruntz.analysis.caller_audit --csv           # machine-readable to stdout
    python3 -m gruntz.analysis.caller_audit --no-ir         # skip the clang base pass
                                                            # (promote+access only; fast)
    python3 -m gruntz.analysis.caller_audit --check 0x1804a0 # explain one function
"""
import os
import re
import sys
import csv
import bisect
import struct
import argparse
import subprocess
import concurrent.futures
from pathlib import Path

from gruntz.analysis import xref
from gruntz.build import labels

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
SRC = REPO / "src"
INC = REPO / "include"
SYMCSV = REPO / "build/gen/symbol_names.csv"
OUT_DIR = REPO / "config"

# --- mangled-name access + class extraction --------------------------------
# The char after the qualified-name terminator `@@` encodes access+storage in
# groups of eight: A-H private, I-P protected, Q-X public, Y/Z global (free fn).
def access_group(c):
    if not c:
        return None
    if "A" <= c <= "H":
        return "private"
    if "I" <= c <= "P":
        return "protected"
    if "Q" <= c <= "X":
        return "public"
    return "global"  # Y, Z


# special member codes we can still attribute to a class (ctor/dtor/deleting
# dtors/operators). `?_7`/`?_8`/`?_R*` are vftable/RTTI DATA - skip.
_CTORDTOR = {"?0": "ctor", "?1": "dtor", "?_G": "vec-dtor", "?_E": "scalar-dtor"}


def parse_mangled(m):
    """(class_or_None, member_or_None, access_char_or_None) for a MSVC function
    mangling. No templates in this binary, so `find('@@')` reliably terminates
    the qualified name. Returns None for non-C++ / data-ish symbols."""
    if not m.startswith("?"):
        return None                        # extern "C" (_foo, _foo@N)
    b = m[1:]
    special = None
    if b.startswith("?"):
        b = b[1:]
        if b.startswith("_"):
            special = "?_" + b[1:2]
            b = b[2:]
        else:
            special = "?" + b[:1]
            b = b[1:]
        if special.startswith("?_") and special not in _CTORDTOR:
            return None                    # ?_7 vftable, ?_R0 RTTI, ... = data
    end = b.find("@@")
    if end < 0:
        return None
    tokens = b[:end].split("@")
    rest = b[end + 2:]
    access = rest[0] if rest else None
    if special is not None:
        cls = tokens[0] if tokens else None
        member = _CTORDTOR.get(special, "op")
        return (cls, member, access)
    member = tokens[0] if tokens else None
    cls = tokens[1] if len(tokens) > 1 else None   # None => free function
    return (cls, member, access)


# --- data sources ----------------------------------------------------------
def load_symbols():
    """rva -> (mangled, unit, size, kind) for func rows; plus mangled -> rva."""
    syms, m2rva = {}, {}
    with open(SYMCSV) as f:
        for r in csv.DictReader(f):
            try:
                rva = int(r["rva"], 16)
            except (ValueError, KeyError):
                continue
            syms[rva] = (r["name"], r.get("unit", ""), r.get("size", ""),
                         r.get("kind", ""))
            m2rva.setdefault(r["name"], rva)
    return syms, m2rva


_RVA_MACRO = re.compile(r"\bRVA\s*\(\s*(0x[0-9a-fA-F]+)")


def build_rva2file():
    """rva -> src file (relative), from every RVA() macro across src/."""
    r2f = {}
    for p in SRC.rglob("*.cpp"):
        try:
            txt = p.read_text(errors="replace")
        except OSError:
            continue
        for m in _RVA_MACRO.finditer(txt):
            r2f[int(m.group(1), 16)] = p.relative_to(REPO).as_posix()
    return r2f


# class/struct BODY definitions (`class X {` / `struct X : ...`) - not forward
# decls (`class X;`). A class is header-visible iff any body lives in a .h.
_DECL_RE = re.compile(r"^\s*(?:class|struct)\s+([A-Za-z_]\w*)\b\s*(?:final\b\s*)?(?::|\{)",
                      re.M)


def build_decl_index():
    """(decl_in_header:set[class], header_blob:str). header_blob is the
    concatenation of include/ headers, for free-function prototype presence."""
    in_header = set()
    header_parts = []
    files = list(SRC.rglob("*.cpp")) + list(SRC.rglob("*.h")) + list(INC.rglob("*.h"))
    for p in files:
        try:
            txt = p.read_text(errors="replace")
        except OSError:
            continue
        is_header = p.suffix == ".h"
        if is_header and INC in p.parents:
            header_parts.append(txt)
        if is_header:
            for m in _DECL_RE.finditer(txt):
                in_header.add(m.group(1))
    return in_header, "\n".join(header_parts)


# --- TARGET graph: one .text pass ------------------------------------------
def target_edges(callees, d, secs, fstarts):
    """{callee_rva: set(caller_start_rva)} for every labelled callee, via a
    single E8/E9 scan resolved to the containing function start."""
    _n, tva, tvsz, trp, trsz = xref._text(secs)
    tb = d[trp:trp + trsz]
    cset = set(callees)
    hits = []                                # (caller_site, callee_rva)
    n = len(tb) - 5
    i = 0
    while i < n:
        op = tb[i]
        if op == 0xE8 or op == 0xE9:
            rel = struct.unpack_from("<i", tb, i + 1)[0]
            tgt = tva + i + 5 + rel
            if tgt in cset:
                hits.append((tva + i, tgt))
        i += 1

    def owner(rva):
        k = bisect.bisect_right(fstarts, rva) - 1
        return fstarts[k] if k >= 0 else None

    edges = {}
    for site, callee in hits:
        o = owner(site)
        if o is None:
            continue
        edges.setdefault(callee, set()).add(o)
    return edges


# --- BASE graph: clang IR per TU -------------------------------------------
_DEF_RE = re.compile(r'^define\b[^@\n]*@("(?:[^"\\]|\\.)*"|[-\w.$?@]+)\s*\(', re.M)
# an instruction line that is a call/invoke (labels sit at col 0; instructions
# are indented and start with an optional `%res =` then call/invoke).
_CALL_RE = re.compile(r'^\s+(?:%\S+\s*=\s*)?(?:tail |musttail |notail )?'
                      r'(?:call|invoke)\b')
_CALL_TGT_RE = re.compile(r'@("(?:[^"\\]|\\.)*"|[-\w.$?@]+)\s*(?:\(|to\b)')


def _ir_name(tok):
    if tok.startswith('"') and tok.endswith('"'):
        tok = tok[1:-1]
    if tok.startswith("\\01"):
        tok = tok[3:]
    return tok


def _tu_edges(args):
    """(caller_mangled -> set(callee_mangled)) for one TU, or None if IR failed.

    A function body spans `define ... {` .. the closing `}` at column 0; basic-block
    labels also sit at column 0, so scope is tracked by define/`}` only."""
    clang, tu, cl_flags = args
    ir = labels.emit_ir(clang, tu, [], cl_flags)
    if not ir:
        return tu, None
    out = {}
    cur = None
    for line in ir.splitlines():
        dm = _DEF_RE.match(line)
        if dm:
            cur = _ir_name(dm.group(1))
            out.setdefault(cur, set())
            continue
        if cur is None:
            continue
        if line.startswith("}"):
            cur = None                       # end of the function body
            continue
        if _CALL_RE.match(line):
            for tok in _CALL_TGT_RE.findall(line):
                out[cur].add(_ir_name(tok))
    return tu, out


def base_edges(m2rva, r2f, jobs=None):
    """({(caller_rva, callee_rva)}, defined:set[caller_rva], failed:list[tu]).

    Only TUs that carry RVA() functions are compiled (the caller universe)."""
    clang = os.environ.get("GRUNTZ_CLANG") or "clang"
    compdb = labels.load_compdb(str(REPO / "build/clangd/compile_commands.json"))
    tus = sorted({os.path.realpath(REPO / f) for f in set(r2f.values())})
    # only TUs the compdb knows: their /imsvc case-mirror includes let clang's
    # header lookup resolve on Linux (a bare-flags compile just fails).
    work = [(clang, tu, compdb[tu]) for tu in tus if tu in compdb]
    skipped = [os.path.relpath(tu, REPO) for tu in tus if tu not in compdb]
    edges, defined, failed = set(), set(), list(skipped)
    jobs = jobs or min(16, (os.cpu_count() or 4) * 2)
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as ex:
        for tu, res in ex.map(_tu_edges, work):
            if res is None:
                failed.append(os.path.relpath(tu, REPO))
                continue
            for caller, callees in res.items():
                cr = m2rva.get(caller)
                if cr is None:
                    continue                 # not one of our labelled functions
                defined.add(cr)
                for callee in callees:
                    ce = m2rva.get(callee)
                    if ce is not None and ce != cr:
                        edges.add((cr, ce))
    return edges, defined, failed


# --- classification --------------------------------------------------------
class Audit:
    def __init__(self, use_ir=True, jobs=None):
        self.syms, self.m2rva = load_symbols()
        self.r2f = build_rva2file()
        self.in_header, self.header_blob = build_decl_index()
        d, secs = xref._load()
        _names, _byname, fstarts, _fsize = xref._names()
        self.names = _names
        func_rvas = [r for r, (_, _, _, k) in self.syms.items() if k == "func"]
        self.tgt = target_edges(func_rvas, d, secs, fstarts)
        self.base, self.base_defined, self.ir_failed = set(), set(), []
        if use_ir:
            self.base, self.base_defined, self.ir_failed = base_edges(
                self.m2rva, self.r2f, jobs)

    def unit(self, rva):
        s = self.syms.get(rva)
        if s:
            return s[1]
        return self.names.get(rva, ("", "?"))[1]

    def name(self, rva):
        s = self.syms.get(rva)
        if s:
            return s[0]
        return self.names.get(rva, (f"FUN_{rva:x}", ""))[0]

    def header_visible(self, cls, member):
        if cls:
            return cls in self.in_header
        return bool(member) and re.search(r"\b" + re.escape(member) + r"\b",
                                          self.header_blob) is not None

    def promote(self):
        """[(callee_rva, name, def_file, cls, [(caller_rva, caller_unit, emitted)])]"""
        rows = []
        for ce, callers in self.tgt.items():
            nm, cunit, _sz, kind = self.syms.get(ce, ("", "", "", ""))
            if kind != "func":
                continue
            pm = parse_mangled(nm)
            if not pm:
                continue
            cls, member, _acc = pm
            if self.header_visible(cls, member):
                continue                     # already shareable
            cross = []
            for cr in sorted(callers):
                if cr not in self.r2f:       # caller not reconstructed
                    continue
                if self.unit(cr) == cunit:   # same TU - no cross-TU need
                    continue
                emitted = (cr, ce) in self.base
                cross.append((cr, self.unit(cr), emitted))
            if cross:
                rows.append((ce, nm, self.r2f.get(ce, "?"), cls or "(free)", cross))
        rows.sort(key=lambda r: r[0])
        return rows

    def _emits_any(self, cr):
        return any(c == cr for c, _ in self.base)

    def orphans(self):
        """[(callee_rva, callee_name, [(caller_rva, caller_name, caller_unit,
        is_real_body)])] - retail edges whose reconstructed+analysed caller emits
        no such call. `is_real_body` = the caller is not a src/Stub/ stub and does
        emit other calls (a partial body missing THIS call - the actionable kind,
        vs a stub whose whole body is unmatched). Real-body misses sort first."""
        emitters = {c for c, _ in self.base}
        rows = []
        for ce, callers in self.tgt.items():
            if self.syms.get(ce, ("", "", "", ""))[3] != "func":
                continue
            miss = []
            for cr in sorted(callers):
                if cr not in self.base_defined:   # caller IR not analysed
                    continue
                if (cr, ce) in self.base:
                    continue
                is_stub = self.r2f.get(cr, "").startswith("src/Stub/")
                real = (not is_stub) and cr in emitters
                miss.append((cr, self.name(cr), self.unit(cr), real))
            if miss:
                rows.append((ce, self.name(ce), miss))
        # actionable first: callees whose miss list has real-body callers, then
        # by number of missing callers.
        rows.sort(key=lambda r: (-sum(1 for m in r[2] if m[3]), -len(r[2])))
        return rows

    def access(self):
        """[(callee_rva, name, access_group, [(caller_rva, caller_cls, caller_unit)])]
        cross-class cross-TU retail callers of a non-public target."""
        rows = []
        for ce, callers in self.tgt.items():
            nm, cunit, _sz, kind = self.syms.get(ce, ("", "", "", ""))
            if kind != "func":
                continue
            pm = parse_mangled(nm)
            if not pm:
                continue
            cls, _member, acc = pm
            grp = access_group(acc)
            if grp in (None, "public", "global"):
                continue
            bad = []
            for cr in sorted(callers):
                if cr not in self.r2f:
                    continue
                cpm = parse_mangled(self.name(cr))
                ccls = cpm[0] if cpm else None
                if ccls == cls:              # same class may legitimately be non-pub
                    continue
                bad.append((cr, ccls or "(free)", self.unit(cr)))
            if bad:
                rows.append((ce, nm, grp, bad))
        rows.sort(key=lambda r: r[0])
        return rows

    def explain(self, rva):
        nm = self.name(rva)
        pm = parse_mangled(nm)
        print(f"0x{rva:08x}  {nm}")
        print(f"  unit={self.unit(rva)}  def_file={self.r2f.get(rva,'?')}")
        if pm:
            cls, member, acc = pm
            print(f"  class={cls}  member={member}  access={acc}({access_group(acc)})"
                  f"  header_visible={self.header_visible(cls, member)}")
        callers = sorted(self.tgt.get(rva, set()))
        print(f"  retail callers ({len(callers)}):")
        for cr in callers:
            recon = cr in self.r2f
            emitted = (cr, rva) in self.base if self.base_defined else None
            tag = []
            if not recon:
                tag.append("unreconstructed")
            elif self.unit(cr) != self.unit(rva):
                tag.append("CROSS-TU")
            if emitted is False and cr in self.base_defined:
                tag.append("ORPHAN(not emitted)")
            elif emitted is True:
                tag.append("emitted")
            print(f"    0x{cr:08x} {self.name(cr)} [{self.unit(cr)}] "
                  f"{' '.join(tag)}")


# --- rendering -------------------------------------------------------------
def md_promote(rows):
    out = [f"# Header-promotion candidates ({len(rows)})", "",
           "Function has a reconstructed cross-TU retail caller but its class is "
           "declared only in a .cpp (free fn: no include/ prototype). Move the "
           "class to a shared header so the caller can make a real typed call.",
           "`emit=no` means our source does not yet emit the call (also an orphan).",
           "", "| callee rva | callee | class | def file | cross-TU callers (unit, emit) |",
           "|---|---|---|---|---|"]
    for ce, nm, deffile, cls, cross in rows:
        cc = "; ".join(f"0x{cr:08x} [{u}] emit={'yes' if e else 'no'}"
                       for cr, u, e in cross)
        out.append(f"| 0x{ce:08x} | `{nm}` | {cls} | {deffile} | {cc} |")
    return "\n".join(out) + "\n"


def md_orphans(rows):
    real = sum(1 for _c, _n, miss in rows if any(m[3] for m in miss))
    out = [f"# Orphan call edges ({len(rows)} callees, {real} with a real-body caller)",
           "",
           "Retail calls these functions, but the reconstructed caller emits no "
           "such call in our LLVM IR - the caller is a stub/@early-stop or fakes "
           "the call via a view/raw-offset. Each missing call reloc caps the "
           "caller's %. **[real]** = the caller is a matched body (not a src/Stub/ "
           "stub) that emits other calls but not this one - fix these first.", "",
           "| callee rva | callee | non-emitting retail callers |", "|---|---|---|"]
    for ce, nm, miss in rows:
        mc = "; ".join(f"0x{cr:08x} {cn} [{u}]{' **[real]**' if real_body else ''}"
                       for cr, cn, u, real_body in miss)
        out.append(f"| 0x{ce:08x} | `{nm}` | {mc} |")
    return "\n".join(out) + "\n"


def md_access(rows):
    out = [f"# Access-mismatch candidates ({len(rows)})", "",
           "A cross-class, cross-TU retail caller calls these directly, but the "
           "symbol's access (mangled name) is not public - a non-public target "
           "mangles to a symbol the caller never emits, so the reloc will not "
           "pair. Make it public (the CLightEffect::Setup = Q lesson).", "",
           "| callee rva | callee | current access | cross-class callers |",
           "|---|---|---|---|"]
    for ce, nm, grp, bad in rows:
        bc = "; ".join(f"0x{cr:08x} {ccls} [{u}]" for cr, ccls, u in bad)
        out.append(f"| 0x{ce:08x} | `{nm}` | {grp} | {bc} |")
    return "\n".join(out) + "\n"


def csv_dump(a):
    w = csv.writer(sys.stdout)
    w.writerow(["kind", "callee_rva", "callee", "detail", "callers"])
    for ce, nm, deffile, cls, cross in a.promote():
        w.writerow(["promote", f"0x{ce:08x}", nm, f"{cls}|{deffile}",
                    ";".join(f"0x{cr:08x}:{u}:{'emit' if e else 'noemit'}"
                             for cr, u, e in cross)])
    for ce, nm, miss in a.orphans():
        w.writerow(["orphan", f"0x{ce:08x}", nm, "",
                    ";".join(f"0x{cr:08x}:{u}:{'real' if rb else 'stub'}"
                             for cr, _cn, u, rb in miss)])
    for ce, nm, grp, bad in a.access():
        w.writerow(["access", f"0x{ce:08x}", nm, grp,
                    ";".join(f"0x{cr:08x}:{u}" for cr, _c, u in bad)])


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--kind", choices=["promote", "orphan", "access"],
                    help="print one worklist to stdout (default: all 3 -> config/*.md)")
    ap.add_argument("--csv", action="store_true", help="machine-readable to stdout")
    ap.add_argument("--no-ir", action="store_true",
                    help="skip the clang base-graph pass (promote+access only)")
    ap.add_argument("--check", help="explain one function (rva or mangled name)")
    ap.add_argument("--jobs", type=int, default=None, help="clang IR parallelism")
    args = ap.parse_args()

    a = Audit(use_ir=not args.no_ir, jobs=args.jobs)

    if args.check:
        try:
            rva = int(args.check, 16)
        except ValueError:
            rva = a.m2rva.get(args.check)
            if rva is None:
                sys.exit(f"[caller_audit] unknown symbol {args.check}")
        a.explain(rva)
        return

    if args.csv:
        csv_dump(a)
        return

    renderers = {"promote": (a.promote, md_promote),
                 "orphan": (a.orphans, md_orphans),
                 "access": (a.access, md_access)}
    if args.kind:
        getf, mdf = renderers[args.kind]
        print(mdf(getf()), end="")
        return

    OUT_DIR.mkdir(exist_ok=True)
    counts = {}
    for kind, (getf, mdf) in renderers.items():
        rows = getf()
        counts[kind] = len(rows)
        (OUT_DIR / f"caller-audit-{kind}.md").write_text(mdf(rows))
    print(f"[caller_audit] promote={counts['promote']} "
          f"orphan={counts['orphan']} access={counts['access']}")
    if not args.no_ir:
        print(f"[caller_audit] base IR: {len(a.base_defined)} funcs analysed, "
              f"{len(a.base)} edges, {len(a.ir_failed)} TU IR failures")
    print(f"[caller_audit] wrote config/caller-audit-{{promote,orphan,access}}.md")


if __name__ == "__main__":
    main()
