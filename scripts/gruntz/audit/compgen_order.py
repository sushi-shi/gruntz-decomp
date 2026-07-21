#!/usr/bin/env python3
"""compgen_order.py - RVA_COMPGEN placement ratchet.

RVA_COMPGEN(<rva>, <size>, <mangled>) pins a COMPILER-GENERATED function (a
``??_G`` scalar-deleting dtor, a ``??_D`` vbase dtor, an ``_$E`` EH funclet, an
out-of-line MFC-inline COMDAT, ...) that has no source body to hang an RVA() on.
The convention (rva.h) is that every invocation sits in RVA ORDER among the TU's
other labeled functions, so a reader scanning a TU top-to-bottom walks the retail
address space monotonically - the same property tu_order_check ratchets for
RVA() bodies. (COMDAT copies are LINKER-POOLED away from their TU's contiguous
.text run, so they are deliberately NOT part of tu_order_check's inter-TU
contiguity invariant - this check is purely intra-file, against each
invocation's nearest labeled neighbors.)

  python -m gruntz.audit.compgen_order          # report violations
  python -m gruntz.audit.compgen_order --gate   # exit 1 on any violation (build tail)
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from gruntz.audit.tu_layout import RVA_RE, RVA_COMPGEN_RE  # noqa: E402

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parents[3])


def collect(path: Path):
    """[(line_no, rva, is_compgen)] for every labeled line, in file order."""
    seq = []
    for i, ln in enumerate(path.read_text(errors="replace").splitlines(), 1):
        m = RVA_RE.search(ln)
        if m:
            seq.append((i, int(m.group(1), 16), False))
            continue
        m = RVA_COMPGEN_RE.search(ln)
        if m:
            seq.append((i, int(m.group(1), 16), True))
            continue
    return seq


def violations():
    out = []
    for path in sorted((REPO / "src").rglob("*.cpp")):
        seq = collect(path)
        for k, (ln, rva, is_cg) in enumerate(seq):
            if not is_cg:
                continue
            prev = seq[k - 1][1] if k > 0 else None
            nxt = seq[k + 1][1] if k + 1 < len(seq) else None
            if (prev is not None and rva < prev) or (nxt is not None and rva > nxt):
                rel = path.relative_to(REPO)
                out.append(
                    f"{rel}:{ln}: RVA_COMPGEN 0x{rva:06x} out of order (between "
                    f"{'0x%06x' % prev if prev is not None else 'BOF'} and "
                    f"{'0x%06x' % nxt if nxt is not None else 'EOF'})")
    return out


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gate", action="store_true",
                    help="exit 1 on any violation (build-tail ratchet)")
    args = ap.parse_args()
    viol = violations()
    for v in viol:
        print(v)
    if viol:
        print(f"compgen-order: {len(viol)} RVA_COMPGEN invocation(s) out of RVA order")
        return 1 if args.gate else 0
    print("compgen-order: OK - every RVA_COMPGEN sits in RVA order among its TU's labels")
    return 0


if __name__ == "__main__":
    sys.exit(main())
