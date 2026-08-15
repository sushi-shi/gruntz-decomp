"""gruntz.tool.objdump - the raw-binary i386 disassembler.

    gruntz tool objdump <file.bin> [--vma 0x153810]

In-process (same function):
    from gruntz.tool import objdump
    text = objdump.disassemble(blob, vma=0x153810)

binutils `objdump` decodes a flat byte blob (`-b binary -m i386`) with the
addresses biased to the blob's real load address, which is what a retail
function's bytes need: they are carved out of the image, not out of an object
file. This module RUNS the program and hands back its stdout; deciding what an
instruction MEANS - which operand is a relocated address, which call reaches
which function - is the caller's business (gruntz.sema.disasm).

The era toolchain has no part in this: it is a native host tool, no wine.
"""

from __future__ import annotations

import shutil
import subprocess
import tempfile
from pathlib import Path

from gruntz.tool import ToolError

OBJDUMP = "objdump"


def version(exe: str = OBJDUMP) -> str:
    if shutil.which(exe) is None:
        raise ToolError(f"{exe} not found on PATH - run inside `nix develop`")
    proc = subprocess.run([exe, "--version"], capture_output=True, text=True)
    return (proc.stdout or proc.stderr).strip().splitlines()[0]


def disassemble(blob: bytes, vma: int = 0, *, arch: str = "i386",
                intel: bool = True, exe: str = OBJDUMP) -> str:
    """objdump's stdout for `blob` decoded as flat `arch` code loaded at `vma`."""
    if shutil.which(exe) is None:
        raise ToolError(f"{exe} not found on PATH - run inside `nix develop`")
    with tempfile.TemporaryDirectory(prefix="gruntz-objdump-") as tmp:
        path = Path(tmp) / "blob.bin"
        path.write_bytes(blob)
        argv = [exe, "-D", "-b", "binary", "-m", arch,
                f"--adjust-vma=0x{vma:x}", str(path)]
        if intel:
            argv.insert(1, "-Mintel")
        proc = subprocess.run(argv, capture_output=True, text=True)
    if proc.returncode != 0:
        tail = "\n".join((proc.stderr or proc.stdout).strip().splitlines()[-8:])
        raise ToolError(f"{exe} failed (rc {proc.returncode}):\n{tail}")
    return proc.stdout


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="gruntz tool objdump",
                                 description=__doc__.splitlines()[0])
    ap.add_argument("file", help="raw byte blob to decode")
    ap.add_argument("--vma", default="0", help="load address of byte 0 (hex)")
    ap.add_argument("--arch", default="i386")
    args = ap.parse_args(argv)
    print(disassemble(Path(args.file).read_bytes(), int(args.vma, 16),
                      arch=args.arch), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
