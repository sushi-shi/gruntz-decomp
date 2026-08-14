"""Gate: every RVA_COMPGEN pin must be emitted by its own TU's base obj.

`RVA_COMPGEN(<rva>, <size>, <mangled>)` names a body cl generates and no source
line defines -- an inline dtor's COMDAT, an implicit default ctor, a `??_G`
scalar-deleting destructor.  `labels.py` binds one only when the HOSTING TU's
base obj actually defines that symbol; otherwise it records a
`RVA_COMPGEN not in base obj` miss and moves on.

That miss is logged from `write_symbol_names`, which only runs when the labels
fragment CHANGES -- so in a steady-state build a dropped pin prints nothing at
all, and the function silently leaves the scored universe.  Two pins sat dropped
across 25+ commits that way (`??1CLoadable@@UAE@XZ` at 0xd5d70, emitted by 11
objs but not by the `cimagecomdats` TU that pins it, and
`??0CSBI_RectOnly@@QAE@XZ` at 0x101fa0, which cl declines to emit anywhere).
They surface in the README only as an anonymous `(unmatched) 0 / 2` row.

A dropped pin is not a matching problem: the body is compiler-generated and is
byte-correct the moment it exists.  It is an EMISSION problem -- cl inlined the
callee everywhere and produced no COMDAT for the pin to bind to.

    python -m gruntz.audit.compgen_pins [--fatal]
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
SRC = REPO / "src"
BASE = REPO / "build" / "objdiff" / "base"
ACK = REPO / "config" / "cleanliness" / "compgen-pin-unemitted.tsv"
PIN_RE = re.compile(
    r"\bRVA_COMPGEN\s*\(\s*(0x[0-9a-fA-F]+)\s*,\s*(?:0x[0-9a-fA-F]+|\d+)\s*,\s*([^\s,)]+)\s*\)")


def _obj_syms(obj: Path) -> set[str]:
    out = subprocess.run(["llvm-objdump", "-t", str(obj)],
                         capture_output=True, text=True).stdout
    return {ln.split()[-1] for ln in out.splitlines() if ln.strip()}


def _unit_of(tu: Path) -> str | None:
    """The units.toml unit whose `source` is this TU."""
    txt = (REPO / "config" / "units.toml").read_text()
    rel = tu.relative_to(REPO).as_posix()
    m = re.search(r'unit\s*=\s*"([^"]+)"\s*\nsource\s*=\s*"%s"' % re.escape(rel), txt)
    return m.group(1) if m else None


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--why", action="store_true",
                    help="also scan the tree to say which objs DO emit an "
                         "acknowledged pin (slow)")
    ap.add_argument("--gate", "--fatal", action="store_true", dest="gate",
                    help="exit 1 on any drop not acknowledged in the floor file")
    args = ap.parse_args(argv)

    cache: dict[str, set[str]] = {}
    pins = dropped = 0
    rows = []
    for tu in sorted(SRC.rglob("*.cpp")):
        text = tu.read_text(errors="ignore")
        if "RVA_COMPGEN" not in text:
            continue
        unit = _unit_of(tu)
        if unit is None:
            continue
        obj = BASE / f"{unit}.obj"
        if not obj.exists():
            continue
        syms = cache.setdefault(unit, _obj_syms(obj))
        for m in PIN_RE.finditer(text):
            pins += 1
            rva, sym = m.group(1), m.group(2)
            if sym not in syms:
                dropped += 1
                rows.append((rva, sym, unit, None))

    print(f"compgen-pins: {pins} pin(s), {dropped} NOT emitted by their own TU")

    def emitters(sym):
        """Which objs DO emit it - the answer that names the fix. Costs a
        whole-tree symtab scan, so only asked for actionable rows."""
        return [o.stem for o in sorted(BASE.glob("*.obj"))
                if sym in cache.setdefault(o.stem, _obj_syms(o))]
    ack = {}
    if ACK.is_file():
        for ln in ACK.read_text().splitlines():
            if ln.startswith("#") or not ln.strip():
                continue
            rva_s, sym, _reason = ln.split("\t", 2)
            ack[(rva_s.lower(), sym)] = True
    new = [r for r in rows if (r[0].lower(), r[1]) not in ack]
    for rva, sym, unit, _ in rows:
        if args.why or (rva.lower(), sym) not in ack:
            e = emitters(sym)
            print(f"  DROPPED {rva} [{unit}] {sym}\n      emitted by: "
                  + (", ".join(e) if e else "NO obj emits it"))
        else:
            print(f"  DROPPED {rva} [{unit}] {sym} (acknowledged)")
    stale = [k for k in ack if not any((r[0].lower(), r[1]) == k for r in rows)]
    for rva_s, sym in stale:
        print(f"  STALE FLOOR ROW {rva_s} {sym} - now emitted; delete it from "
              f"{ACK.relative_to(REPO)}")
    if (new or stale) and args.gate:
        for rva_s, sym, unit, _e in new:
            print(f"  UNACKNOWLEDGED {rva_s} [{unit}] {sym}")
        print("\nA dropped pin leaves a compiler-generated body OUT of the scored "
              "universe; it shows up only as the README's anonymous (unmatched) row. "
              f"Fix the pin, or acknowledge it with a reason in "
              f"{ACK.relative_to(REPO)}.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
