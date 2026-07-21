"""Find THUNK-ALIAS DUPLICATE declarations: two source names for ONE retail function.

THE DEFECT
----------
A method is declared (and RVA()-annotated, or referenced by comment) at an address that
is really an **incremental-link thunk** (`jmp rel32`), and the thunk's TARGET already
carries a src claim under a DIFFERENT name. The result is one retail function wearing
two source identities:

  * the duplicate name is a PHANTOM - nothing defines it, so it is a guaranteed
    `unresolved external symbol` at whole-game link;
  * and it is a KNOWLEDGE defect - the next reader models one function as two.

Found by hand twice (2026-07-13, Fable lane):
    CFontConfig::Draw258b    -> ILT 0x258b jmps to 0x21f20 == CFontConfig::MeasureLabel
    CChatBoxOwner::HitTest43e0 -> ILT 0x43e0 jmps to 0x21140 == CChatBoxOwner::HitTest
Both were the SAME CLASS as their target - i.e. a method declared twice on one class.

THE DETECTION (mechanical)
--------------------------
For every rva claimed by src (build/gen/symbol_names.csv):
  1. read the retail bytes at that rva; if it is `e9 <rel32>` (a jmp thunk), resolve the
     target;
  2. if the TARGET also carries a src claim, under a different mangled name -> DEFECT.

Usage:  python -m gruntz.analysis.thunk_alias_dups
Exit 0 always (a report, not a gate) - the fix is a source merge, per hit.
"""

import csv
import os
import struct
import sys
from pathlib import Path

REPO = Path(os.environ.get("REPO", Path(__file__).resolve().parents[3]))
SYMS = REPO / "build" / "gen" / "symbol_names.csv"
EXE = Path(os.environ["GRUNTZ_EXE"])


def _rva2off(exe: bytes):
    pe = struct.unpack_from("<I", exe, 0x3C)[0]
    nsec = struct.unpack_from("<H", exe, pe + 6)[0]
    optsz = struct.unpack_from("<H", exe, pe + 20)[0]
    secs = pe + 24 + optsz
    table = []
    for i in range(nsec):
        s = secs + i * 40
        vsz = struct.unpack_from("<I", exe, s + 8)[0]
        va = struct.unpack_from("<I", exe, s + 12)[0]
        raw = struct.unpack_from("<I", exe, s + 20)[0]
        table.append((va, vsz, raw))

    def conv(rva: int):
        for va, vsz, raw in table:
            if va <= rva < va + vsz:
                return raw + (rva - va)
        return None

    return conv


def main() -> int:
    if not SYMS.exists():
        print(f"thunk-alias-dups: {SYMS} missing - run `gruntz build` first.")
        return 0
    exe = EXE.read_bytes()
    conv = _rva2off(exe)

    # rva -> (name, kind)
    claims: dict[int, tuple[str, str]] = {}
    with SYMS.open() as fh:
        for row in csv.DictReader(fh):
            rva, name = row.get("rva"), row.get("name")
            if not rva or not name:
                continue
            try:
                claims[int(rva, 16)] = (name, (row.get("kind") or "").strip(),
                                        (row.get("size") or "").strip())
            except ValueError:
                continue

    code_dups, data_aliases = [], []
    for rva, (name, kind, size) in sorted(claims.items()):
        off = conv(rva)
        if off is None or off + 5 > len(exe) or exe[off] != 0xE9:
            continue  # not a jmp rel32 thunk
        # A claim that carries its own SIZE is a real RVA()-annotated body that merely
        # happens to start with a jmp - i.e. a genuine one-instruction TAIL-CALL
        # forwarder (e.g. a virtual override that tail-jumps to a shared helper), not an
        # ILT thunk aliasing another name. Those are correct code; do not flag them.
        if size:
            continue
        tgt = (rva + 5 + struct.unpack_from("<i", exe, off + 1)[0]) & 0xFFFFFFFF
        hit = claims.get(tgt)
        if not hit or hit[0] == name:
            continue
        (code_dups if kind != "data" else data_aliases).append((rva, name, kind, tgt, hit[0]))

    # (A) the TRUE duplicate-declaration defect: a FUNCTION declared at a thunk rva
    #     whose target is already a function under another name -> one fn, two decls,
    #     and the thunk-side name is a phantom (nothing defines it).
    print(f"=== (A) DUPLICATE FUNCTION DECLARATIONS: {len(code_dups)} "
          f"(one retail fn, two src names; the thunk-side name is an unresolvable phantom)\n")
    for rva, name, _kind, tgt, other in code_dups:
        print(f"  0x{rva:08x}  {name}")
        print(f"    -> jmp 0x{tgt:08x}  ALREADY CLAIMED AS  {other}")
        print(f"    FIX: delete the 0x{rva:08x} decl; call {other} at its sites.\n")
    if not code_dups:
        print("  (none)\n")

    # (B) a DATA label on a thunk (a callback/factory fn-pointer: taking a function's
    #     address yields its ILT thunk, so this row is LEGITIMATE). But it still NAMES
    #     the target: if the body carries a placeholder name, the data label is the
    #     REAL name - and if the two names contradict, one of the claims is WRONG.
    print(f"=== (B) DATA labels on thunks: {len(data_aliases)} "
          f"(legitimate fn-pointer rows - but they NAME the body they jump to)\n")
    for rva, name, _kind, tgt, other in data_aliases:
        print(f"  0x{rva:08x}  data {name:<42s} -> body 0x{tgt:08x}  {other}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
