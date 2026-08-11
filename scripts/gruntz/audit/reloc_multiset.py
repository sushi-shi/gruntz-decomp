#!/usr/bin/env python3
"""reloc_multiset.py - per-function relocation-referent MULTISET diff, base vs target.

`gruntz sema disasm --diff` MASKS address operands, so a call to the WRONG callee,
a missing statement, or one inline expansion too many all print as "identical" or
as an unremarkable register shuffle. The referent multiset does not mask: if retail
calls `??0CString@@QAE@ABV0@@Z` twice and we call it once, that is a source defect
with a name on it, and it survives every scheduling and register difference.

This is the complement of `gruntz.audit.assert_relocs`, which audits only >=99.5%
functions and only looks for referents present on one side. Here the COUNT is the
signal and every function is in scope.

Measured (Bute module, one pass): `??0CString@@QAE@PBD@Z` base 2 / target 1 named a
`char*` return type that should have been `CString*`; `_isdigit` base 9 / target 10
named a missing loop test; `??0CButeValue@...` base 3 / target 4 named the /Ob1
expansion-count wall; `??1zErrHandling@@UAE@XZ` base 0 / target 1 was the delinker's
jump-table packing artifact (a documented FALSE positive - see below).

Filtered out, because they are structural to how the two sides are produced and
never a source defect:

  * `$L<n>` / `$<label>$<n>` local labels - MSVC splits switch arms into them, and
    they are also why the referent list must NOT be cut at the jump table: a naive
    per-symbol walk truncates every switch-carrying function (`ParseAttributeFile`
    read 7 referents instead of 114).
  * `__except_list`, `__ehreg$`, `__ehunwind$`, `__ehfuncinfo$` - the base obj takes
    relocations against the EH scaffolding the delinker carves into its own band.
  * self-references - our base takes a reloc against a `$L` arm, the delinked target
    against the function symbol itself, so every jump table shows up as N spurious
    self-referents.

KNOWN FALSE POSITIVE: the delinker packs an unclaimed neighbour into the preceding
symbol, so a referent that appears ONLY on the target side, ONCE, in a function
whose RVA span abuts an unclaimed gap, is usually the neighbour's. Check the span
before believing it.
"""
import argparse
import collections
import re
import subprocess
import sys
from pathlib import Path

from gruntz.core.branches import is_local_label


# Resolve REPO from the CWD first, not __file__: in a worktree the shell's PYTHONPATH
# can point at MAIN's scripts/, so `python -m ...` would mis-resolve to main.
def _find_repo() -> Path:
    for base in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (base, *base.parents):
            if (p / "flake.nix").exists() and (p / "build" / "objdiff").exists():
                return p
    return Path(__file__).resolve().parents[3]


REPO = _find_repo()

RELOC_RE = re.compile(r"^[0-9a-f]{8}:\s+IMAGE_REL_I386_(\w+)\s+(.*)$")
NOISE = ("__except_list", "__eh", "__unwindtable$", "__tryblocktable$", "$L", "$")


def referents(obj: Path) -> dict:
    """{function -> [referent, ...]} for one COFF, switch arms folded into their fn."""
    out = subprocess.run(["llvm-objdump", "-dr", "--x86-asm-syntax=intel", str(obj)],
                         capture_output=True, text=True).stdout
    res, cur = {}, None
    for ln in out.splitlines():
        s = ln.strip()
        if s.endswith(">:") and "<" in s:
            name = s.split("<", 1)[1][:-2]
            if cur is not None and is_local_label(name):
                continue          # a switch arm belongs to the function above it
            cur = name
            res.setdefault(cur, [])
            continue
        m = RELOC_RE.match(s)
        if m and cur is not None:
            res[cur].append(m.group(2).strip())
    return res


def audit(unit: str) -> list:
    base = REPO / "build/objdiff/base" / (unit + ".obj")
    target = REPO / "build/objdiff/target" / (unit + ".c.obj")
    if not base.is_file() or not target.is_file():
        return []
    b, t = referents(base), referents(target)
    findings = []
    for fn in sorted(b):
        if fn not in t:
            continue
        def keep(n):
            return not (n.startswith(NOISE) or n == fn)
        cb = collections.Counter(x for x in b[fn] if keep(x))
        ct = collections.Counter(x for x in t[fn] if keep(x))
        rows = [(k, cb.get(k, 0), ct.get(k, 0)) for k in sorted(set(cb) | set(ct))
                if cb.get(k, 0) != ct.get(k, 0)]
        if rows:
            findings.append((unit, fn, rows))
    return findings


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("units", nargs="*", help="unit names (default: every base obj)")
    ap.add_argument("--summary", action="store_true", help="counts only")
    args = ap.parse_args(argv if argv is not None else sys.argv[1:])

    units = args.units or sorted(p.stem for p in (REPO / "build/objdiff/base").glob("*.obj"))
    total = 0
    for unit in units:
        for u, fn, rows in audit(unit):
            total += 1
            if args.summary:
                continue
            print(f"-- {u}  {fn}")
            for k, x, y in rows:
                print("     %-58s base %2d target %2d" % (k[:58], x, y))
    print(f"reloc-multiset: {total} function(s) whose referent counts differ "
          f"across {len(units)} unit(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
