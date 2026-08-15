"""gruntz.graph.cc - the `cl` edge driver: compile, stabilise, write-if-changed.

    python3 -m gruntz.graph.cc --out <obj> --src <src> [--unit U] -- <cl flags>

gruntz.tool.cl runs the compiler (and already owns the wine plumbing: the
persistent wineserver, the temp-FILE capture and process-group kill that keep
an unreaped grandchild from wedging ninja's pipe, and "the produced .obj is
the success signal, never the return code"). This module is what makes the
EDGE incremental, which needs two more things cl 5.0 does not give:

  * cl stamps every COFF header with the wall-clock TimeDateStamp, so two
    compiles of one unchanged TU differ in bytes 4..7 AND NOWHERE ELSE
    (measured on this toolchain). Left alone, every rebuild dirties every
    object and everything downstream of it re-runs for no reason.
  * ninja prunes a subtree only when an output's mtime does not move, so an
    object whose content did not change must not be rewritten at all.

So: compile into `<base-dir>/.tmp/`, compare against the installed object
with the TimeDateStamp field masked out, and install only on a real
difference - zeroing the stamp on the way in, so objects converge to
byte-reproducible content. With `restat = 1` on the `cl` rule that makes an
unchanged recompile a genuine no-op for labels / normalize / report.

Zeroing is matching-NEUTRAL: TimeDateStamp is COFF header metadata, lives in
no section, is named by no relocation, and neither objdiff, the delinker nor
link.exe reads it. The temp directory is a dotted subdirectory of the object
tree on purpose - the model's `build/objdiff/base` readers glob `*.obj`, and
a sibling temp file would enrol into the data manifest as a phantom unit.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

from gruntz.tool import ToolError

#: COFF file header: Machine(2) NumberOfSections(2) TimeDateStamp(4) ...
_MACHINE_I386 = 0x14C
_TIMESTAMP_OFFSET = 4


def stabilise(data: bytes) -> bytes:
    """`data` with the COFF TimeDateStamp zeroed; non-COFF input unchanged."""
    if len(data) < 20 or struct.unpack_from("<H", data, 0)[0] != _MACHINE_I386:
        return data
    buf = bytearray(data)
    struct.pack_into("<I", buf, _TIMESTAMP_OFFSET, 0)
    return bytes(buf)


def install(new: bytes, out: Path) -> bool:
    """Write `new` to `out` if its stable form differs; True when it changed."""
    stable = stabilise(new)
    if out.exists() and stabilise(out.read_bytes()) == stable:
        return False
    tmp = out.with_name(out.name + ".install")
    tmp.write_bytes(stable)
    tmp.replace(out)
    return True


def compile_unit(src: Path | str, out: Path | str, flags: list[str]) -> bool:
    """Compile one TU into `out`. Returns True when the object changed."""
    from gruntz.tool import cl

    src, out = Path(src), Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    scratch = out.parent / ".tmp"
    scratch.mkdir(parents=True, exist_ok=True)
    staged = scratch / out.name
    try:
        cl.compile(src, staged, flags)
        return install(staged.read_bytes(), out)
    finally:
        staged.unlink(missing_ok=True)


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True)
    ap.add_argument("--src", required=True)
    ap.add_argument("--unit", help="manifest unit name (diagnostics only)")
    ap.add_argument("flags", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    flags = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    try:
        compile_unit(a.src, a.out, flags)
    except ToolError as e:
        print(f"[cl] {a.unit or a.src}: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
