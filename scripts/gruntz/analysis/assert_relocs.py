#!/usr/bin/env python3
"""assert_relocs.py - OPT-IN reloc-TARGET audit. Ported from the sibling homm2-decomp project
(scripts/homm2/build/assert_relocs.py); same toolchain, same delink+objdiff pipeline.

WHY THIS EXISTS (and why reloc_fidelity.py is not enough):

objdiff MASKS relocations when scoring, so it never checks a reloc's TARGET. A "100% match" can
silently reference the WRONG global/const/field/function - or CALL A FABRICATED FUNCTION that is
declared, never defined, and (because we compile-but-don't-link) never caught as an unresolved
external. A wrong reloc costs ~0.005%, which rounds to "100.00%" in the display.

This complements the two tools we already have:
  * reloc_fidelity.py  - offset-EXACT, per-site, and only over byte-EXACT functions. Precise, but
                         blind to everything below 100% and brittle under instruction reordering.
  * link_defects.py    - "can the linker resolve this symbol at all" (reads the real .LIB tables).
  * assert_relocs.py   - "does this function point at the addresses RETAIL points at" - ORDER-
                         INDEPENDENT, so it works on NEAR-exact (>=99.5%) functions too, which is
                         most of the tree. Plus a FAKE check for unresolvable '?'-mangled symbols.

Order-INDEPENDENT: resolve each side's relocs to a MULTISET of final RVAs (symbol RVA + instruction
addend; REL32 -> the symbol's own RVA) and flag any address the BASE references that retail never
does (or references fewer times). A multiset - not a set - so an injected wrong ref is still caught
when that address is ALSO legitimately referenced elsewhere in the same function.

Data offsets come from symbol_names.csv (authoritative CodeView RVAs); globals with no CodeView
symbol are read from their DATA(0x..) annotation in src/ or include/.

OPT-IN, not a hard build gate: it also surfaces unreproducible link artifacts (chiefly the
delinker's COMDAT-folded empty stubs), which are a delinker-side concern, not a source bug.

    python -m gruntz.analysis.assert_relocs                 # audit every near-exact function
    python -m gruntz.analysis.assert_relocs --unit butemgr  # one unit
    python -m gruntz.analysis.assert_relocs 0x0008dc60      # review ONE function (works at a wall)

Exits 1 on any wrong/fabricated reloc target.
"""
import sys, os, re, csv, json, glob, argparse, subprocess
from collections import Counter
from pathlib import Path


# Resolve REPO from the CWD first, not __file__: in a worktree the shell's PYTHONPATH can point at
# MAIN's scripts/, so `python -m ...` loads main's module and __file__ would mis-resolve to main.
def _find_repo():
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists() and (p / "build" / "objdiff").exists():
                return p
    return next(
        (p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
        Path(__file__).resolve().parents[3],
    )


REPO = str(_find_repo())
IMAGE_BASE = 0x400000
THRESHOLD = 99.5  # a wrong reloc costs ~0.005% -> audit NEAR-exact, not just ==100


def _norm(s):
    # llvm-objdump renders each non-printable mangled byte as one '_'; symbol_names.csv stores it
    # as the UTF-8 replacement char.
    s = s.replace("\xef\xbf\xbd", "_")
    return "".join(c if 0x20 <= ord(c) < 0x7F else "_" for c in s)


def _key(s):
    # encoding-stable key for ??_C@ string names: the ASCII "??_C@_<len>@<hash>@" prefix
    m = re.match(r"(\?\?_C@_\w+@\w+@)", s)
    return m.group(1) if m else _norm(s)


def load_symbols():
    sym, dups = {}, {}
    csv_path = os.path.join(REPO, "build/gen/symbol_names.csv")
    for r in csv.reader(open(csv_path, encoding="latin-1")):
        if len(r) >= 2:
            try:
                v = int(r[0], 16)
            except ValueError:
                continue
            for k in (r[1], _norm(r[1]), _key(r[1])):
                # content-hashed string names (??_C@) COLLIDE - two distinct addresses can share one
                # name (e.g. two "!" literals). Record every address per name so an ambiguous target
                # reloc may match ANY of them (they are value-identical), not just the last loaded.
                sym.setdefault(k, v)
                dups.setdefault(k, set()).add(v)
    data = {}
    for pat in ("src/**/*.cpp", "include/**/*.h"):
        for f in glob.glob(os.path.join(REPO, pat), recursive=True):
            for ln in open(f, encoding="latin-1"):
                m = re.search(r"DATA\(0x([0-9a-fA-F]+)\).*?\b([A-Za-z_]\w*)\s*(?:\[|;|=)", ln)
                if m:
                    data[m.group(2)] = int(m.group(1), 16) - IMAGE_BASE
    return sym, data, dups


def resolve(sym, data, typ, s, add):
    mc = re.match(r"const_([0-9a-fA-F]+)$", s)  # delinker names unlabeled data addrs const_<RVA>
    if mc:
        b = int(mc.group(1), 16)
    else:
        b = sym.get(s) or sym.get(_norm(s)) or sym.get(_key(s))
        if b is None:
            mm = re.match(r"\?(\w+)@@", s)  # ?<name>@@... -> a DATA()-pinned global
            if mm:
                b = data.get(mm.group(1))
    if b is None:
        return None
    return b if typ == "REL32" else (b + add) & 0xFFFFFFFF


def _lib_symbols():
    """Every symbol the real toolchain archives (CRT, MFC, Win32, DirectX) can supply. Without this
    the FAKE check false-positives on every statically-linked MFC/CRT call - `CString::GetBuffer`,
    `operator new`, ... - which resolve at link perfectly well. link_defects.py reads the .LIB symbol
    tables (the linker's own answer to "can this resolve"); reuse it rather than re-deriving."""
    try:
        from gruntz.analysis.link_defects import lib_symbols

        return lib_symbols()
    except Exception:
        return set()


LIBS = None


def is_fake(sym, data, s):
    """A '?'-mangled symbol that resolves to NEITHER CodeView, NOR a DATA() global, NOR any
    toolchain .LIB is FABRICATED: declared, never defined. We compile but never link, so nothing
    else in the pipeline catches it - it is a guaranteed `unresolved external symbol`."""
    if s.startswith("_") or s.startswith("??_C@") or s.startswith("$SG") or not s.startswith("?"):
        return False  # CRT / import / string / local -> outside CodeView, fine
    if resolve(sym, data, "DIR32", s, 0) is not None:
        return False
    global LIBS
    if LIBS is None:
        LIBS = _lib_symbols()
    return not (s in LIBS or s.lstrip("_") in LIBS)  # a real CRT/MFC body is not fabricated


def _addend(insn):
    insn = insn.split("#")[0]  # drop llvm-objdump's "# imm = 0xNN" annotation
    m = re.search(r"\[(0x[0-9a-f]+)\]", insn)  # absolute-memory operand [0xN] -> addend N
    if m:
        return int(m.group(1), 16)
    imms = re.findall(r"(?<![\w\]])(-?0x[0-9a-f]+)", insn)  # last standalone SIGNED imm/disp
    return int(imms[-1], 16) & 0xFFFFFFFF if imms else 0  # -0x4c disp -> 0xffffffb4 addend


def parse_obj(obj):
    """llvm-objdump -dr -> {func_name: [(type, symbol, addend), ...]} in order; __imp__ skipped."""
    out = subprocess.run(["llvm-objdump", "-dr", obj], capture_output=True, text=True).stdout
    funcs, cur, prev = {}, None, ""
    for ln in out.splitlines():
        m = re.match(r"^[0-9a-f]+ <(.+?)>:", ln)
        if m:
            cur = m.group(1)
            funcs.setdefault(cur, [])
            prev = ""
            continue
        if cur is None:
            continue
        mr = re.search(r"IMAGE_REL_I386_(\w+)\s+(\S+)", ln)
        if mr:
            s = mr.group(2)
            if not s.startswith("__imp__"):
                funcs[cur].append((mr.group(1), s, _addend(prev)))
            continue
        mi = re.match(r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+)+(.+)$", ln)
        if mi:
            prev = mi.group(1).replace("\t", " ").strip()
    return funcs


# Pre-existing discrepancies to triage, keyed (unit, function, base-symbol). Keep EMPTY if possible:
# an entry here is a known-wrong reloc we have chosen not to fix yet, not an exemption on principle.
ALLOWLIST = set()


def _cands(dups, s, add):
    """Every address a target reloc name could denote, +addend. Content-hashed (??_C@) names collide:
    one name, several value-identical addresses; the reloc may mean ANY, so accept all."""
    c = dups.get(s) or dups.get(_norm(s)) or dups.get(_key(s)) or set()
    return {(a + add) & 0xFFFFFFFF for a in c}


def _tvas(sym, data, dups, tgt_relocs):
    """Multiset of every RVA retail references. An ambiguous (dup) name contributes ALL its candidate
    addrs per occurrence, so a base ref to any one of them clears."""
    tvas = Counter()
    for r in tgt_relocs:
        v = resolve(sym, data, *r)
        if v is None:
            continue
        cs = _cands(dups, r[1], r[2]) if r[0] != "REL32" else set()
        if len(cs) > 1:
            for c in cs:
                tvas[c] += 1
        else:
            tvas[v] += 1
    return tvas


def check_fn(sym, data, dups, unit, name, base_relocs, tgt_relocs):
    probs = []
    for r in base_relocs:  # (1) fabricated function/data symbol -> would be an unresolved external
        if is_fake(sym, data, r[1]) and (unit, name, r[1]) not in ALLOWLIST:
            probs.append("FAKE ref '%s' - in neither CodeView nor a DATA() global" % r[1])
    tvas = _tvas(sym, data, dups, tgt_relocs)
    bvas, va_sym = Counter(), {}
    for r in base_relocs:
        v = resolve(sym, data, *r)
        if v is not None:
            bvas[v] += 1
            va_sym.setdefault(v, r[1])
    for v, n in (bvas - tvas).items():  # (2) base references an addr retail never does (or fewer)
        bs = va_sym[v]
        if "_00A@" in bs or (unit, name, bs) in ALLOWLIST:
            continue
        probs.append(
            "WRONG: base references 0x%x (%s) x%d - retail never does (or fewer)" % (v, bs, n)
        )
    return probs


def _objs(unit):
    return (
        os.path.join(REPO, "build/objdiff/base/%s.obj" % unit),
        os.path.join(REPO, "build/objdiff/target/%s.c.obj" % unit),
    )


def review(rva):
    """Single-function review: find the unit+name owning `rva`, then diff its two reloc multisets."""
    sym, data, dups = load_symbols()
    want = int(rva, 16) if isinstance(rva, str) else rva
    report = json.load(open(os.path.join(REPO, "build/objdiff/report.json")))
    for u in report["units"]:
        base_obj, tgt_obj = _objs(u["name"])
        if not (os.path.exists(base_obj) and os.path.exists(tgt_obj)):
            continue
        for f in u.get("functions", []):
            if f.get("address") not in (want, want + IMAGE_BASE):
                continue
            nm = f["name"]
            bf, tf = parse_obj(base_obj), parse_obj(tgt_obj)
            if nm not in bf or nm not in tf:
                print("  %s: no COMDAT on one side" % nm)
                return 0
            print("%s  %s  (fuzzy %.2f%%)" % (u["name"], nm, f.get("fuzzy_match_percent", 0)))
            for p in check_fn(sym, data, dups, u["name"], nm, bf[nm], tf[nm]):
                print("  !! " + p)
            print("  base relocs=%d  target relocs=%d" % (len(bf[nm]), len(tf[nm])))
            return 0
    print("no near-exact function found at %s" % rva)
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("rva", nargs="?", help="review ONE function at this RVA")
    ap.add_argument("--unit", help="audit only this unit")
    a = ap.parse_args()
    if a.rva:
        return review(a.rva)

    sym, data, dups = load_symbols()
    report = json.load(open(os.path.join(REPO, "build/objdiff/report.json")))
    bad, seen = [], 0
    for u in report["units"]:
        unit = u["name"]
        if a.unit and unit != a.unit:
            continue
        near = {
            f["name"]
            for f in u.get("functions", [])
            if f.get("fuzzy_match_percent", 0) >= THRESHOLD
        }
        if not near:
            continue
        base_obj, tgt_obj = _objs(unit)
        if not (os.path.exists(base_obj) and os.path.exists(tgt_obj)):
            continue
        bf, tf = parse_obj(base_obj), parse_obj(tgt_obj)
        for name in sorted(near):
            if name not in bf or name not in tf:
                continue
            seen += 1
            for p in check_fn(sym, data, dups, unit, name, bf[name], tf[name]):
                bad.append((unit, name, p))
    for unit, name, p in bad:
        print("  %-22s %-46s %s" % (unit, name[:46], p))
    fake = sum(1 for _, _, p in bad if p.startswith("FAKE"))
    print(
        "\nassert_relocs: %d near-exact fn(s) audited (>=%.1f%%), %d defect(s) "
        "[%d FAKE, %d WRONG]" % (seen, THRESHOLD, len(bad), fake, len(bad) - fake)
    )
    if bad:
        print("objdiff MASKS relocs, so none of these cost %. They WILL break the link.")
        return 1
    print("relocs OK: every near-exact function's reloc targets resolve to the retail address.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
