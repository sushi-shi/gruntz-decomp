#!/usr/bin/env python3
"""gruntz.sema.dump_target - dump a target function for matching: bytes + disasm + relocs.

For a matcher: given an RVA (or a resolvable name), print the function's size,
raw bytes, an RVA-aligned x86 disassembly (objdump), and the relocation sites
in range resolved to symbol names (the load-bearing address operands: vftables,
globals, strings, imports). Runs in-process over gruntz.core.

Usage (inside nix develop):
    python3 -m gruntz.sema.dump_target 0x13d8c0 [0x13dc20 ...]
    python3 -m gruntz.sema.dump_target CGameApp::CloseResources
    python3 -m gruntz.sema.dump_target --no-disasm 0x13d8c0      # bytes+relocs only
"""
import shutil
import subprocess
import sys

from gruntz.core import get_context
from gruntz.core.pe import IMAGEBASE, REPO

OBJDUMP = shutil.which("objdump") or "objdump"


def _disasm(ctx, rva, size):
    o = ctx.pe.off(rva)
    blob = ctx.pe.data[o:o + size]
    tmp = REPO / "build" / f".dt_{rva:06x}.bin"
    tmp.parent.mkdir(exist_ok=True)
    tmp.write_bytes(blob)
    try:
        out = subprocess.run([OBJDUMP, "-D", "-b", "binary", "-m", "i386",
                              "-Mintel", f"--adjust-vma=0x{rva:x}", str(tmp)],
                             capture_output=True, text=True).stdout
    finally:
        tmp.unlink(missing_ok=True)
    # keep only the instruction lines
    return "\n".join(l for l in out.splitlines() if ":\t" in l)


def _resolve_name(ctx, rva):
    """Best label for a reloc target: ghidra symbol, then any known fn name."""
    db = ctx.symbols
    return db.gsyms.get(rva) or db.names.get(rva, (None,))[0]


def _boundary(ctx, rva):
    """(name, size) for the fn at `rva`: ghidra boundary first, the src RVA()+size
    claim as the authority for functions Ghidra never carved."""
    db = ctx.symbols
    nm, unit = db.names.get(rva, (None, None))
    size = db.fsize.get(rva, 0)
    if nm is None:
        nm = _resolve_name(ctx, rva) or "?"
    return nm, size


def dump_text(ctx, target, no_disasm=False) -> str:
    """The full dump for one target (RVA int or resolvable name string)."""
    rva = target if isinstance(target, int) else ctx.symbols.resolve(target)
    name, size = _boundary(ctx, rva)
    out = [f"\n{'=' * 72}\n{name}  @ RVA 0x{rva:06x}  (VA 0x{rva + IMAGEBASE:08x})  "
           f"size {size} B\n{'=' * 72}"]
    if size == 0:
        out.append("  (no boundary in functions.csv / symbol_names.csv — recovery gap; "
                   "size unknown)")
        return "\n".join(out)
    rl = []
    for site, val in ctx.pe.relocs_in(rva, rva + size):
        tgt = val - IMAGEBASE
        nm = _resolve_name(ctx, tgt) or _resolve_name(ctx, val) or f"DAT_{tgt:08x}"
        rl.append((site, val, nm))
    if rl:
        out.append("Relocations (address operands — reloc-masked in objdiff):")
        for site, val, nm in rl:
            out.append(f"  @0x{site:06x}  -> 0x{val:08x}  {nm}")
    else:
        out.append("Relocations: none (self-contained / rel32 calls only)")
    if not no_disasm:
        out.append("\nDisassembly (RVA-aligned):")
        out.append(_disasm(ctx, rva, size))
    return "\n".join(out)


def main():
    args = sys.argv[1:]
    no_dis = False
    if "--no-disasm" in args:
        no_dis = True
        args.remove("--no-disasm")
    if not args:
        print(__doc__)
        return
    ctx = get_context()
    for a in args:
        print(dump_text(ctx, a, no_disasm=no_dis))


if __name__ == "__main__":
    main()
