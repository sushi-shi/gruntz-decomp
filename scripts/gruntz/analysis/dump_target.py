#!/usr/bin/env python3
"""gruntz.analysis.dump_target - dump a target function for matching: bytes + disasm + relocs.

For a matcher: given an RVA (or a name in functions.csv/symbols.csv), print the
function's size, raw bytes, an RVA-aligned x86 disassembly (objdump), and the
relocation sites in range resolved to symbol names (the load-bearing address
operands: vftables, globals, strings, imports). Calls (E8/E9 rel32) are shown by
objdump with absolute RVA targets thanks to --adjust-vma.

Usage (inside nix develop .#build):
    python3 -m gruntz.analysis.dump_target 0x13d8c0 [0x13dc20 ...]
    python3 -m gruntz.analysis.dump_target CGameApp::CloseResources
    python3 -m gruntz.analysis.dump_target --no-disasm 0x13d8c0      # bytes+relocs only
"""
import os, sys, struct, csv, subprocess, shutil
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
EXE = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")
FUNCS = REPO / "build/ghidra-enrich/exports/functions.csv"
SYMS = REPO / "build/ghidra-enrich/exports/symbols.csv"
IMAGEBASE = 0x400000

d = EXE.read_bytes()
e = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, e+6)[0]; optsz = struct.unpack_from('<H', d, e+20)[0]; opt = e+24
secs = [struct.unpack_from('<IIII', d, opt+optsz+i*40+8) for i in range(nsec)]

def off(rva):
    for vsz, va, rsz, rp in secs:
        if va <= rva < va+max(vsz, rsz): return rva-va+rp
    return None

# functions: rva->(name,size); also name->rva
frows = {}; byname = {}
with open(FUNCS) as f:
    for r in csv.DictReader(f):
        try: rva = int(r['entry_rva'], 16)
        except Exception: continue
        frows[rva] = (r['name'], int(r['byte_size']))
        byname[r['name']] = rva

# symbols: rva->name (first/primary wins)
syms = {}
with open(SYMS) as f:
    for r in csv.DictReader(f):
        a = r['address_rva']
        try: rva = int(a, 16) if not a.startswith('0x-') else -int(a[3:], 16)
        except Exception: continue
        if rva not in syms or r.get('is_primary') == '1':
            syms[rva] = r['name']
        byname.setdefault(r['name'], rva)

# PE base relocations (IMAGE_REL_BASED_HIGHLOW=3) = the real DIR32 address-operand
# sites. data dir index 5 = base reloc table.
import bisect
reloc_rva = struct.unpack_from('<I', d, opt+96+5*8)[0]
reloc_sz = struct.unpack_from('<I', d, opt+96+5*8+4)[0]
reloc_sites = []
if reloc_rva:
    base = off(reloc_rva); end = base + reloc_sz; p = base
    while p < end:
        page, blk = struct.unpack_from('<II', d, p)
        if blk == 0: break
        for i in range((blk-8)//2):
            ent = struct.unpack_from('<H', d, p+8+i*2)[0]
            if ent >> 12 == 3: reloc_sites.append(page + (ent & 0xfff))
        p += blk
reloc_sites.sort()

def resolve(rva):
    return syms.get(rva) or frows.get(rva, (None,))[0]

def relocs_in(lo, hi):
    out = []
    i = bisect.bisect_left(reloc_sites, lo)
    while i < len(reloc_sites) and reloc_sites[i] < hi:
        site = reloc_sites[i]; o = off(site)
        if o is not None:
            val = struct.unpack_from('<I', d, o)[0]
            tgt = val - IMAGEBASE
            out.append((site, val, resolve(tgt) or resolve(val) or f"DAT_{tgt:08x}"))
        i += 1
    return out

OBJDUMP = shutil.which("objdump") or "objdump"

def disasm(rva, size):
    o = off(rva); blob = d[o:o+size]
    tmp = REPO / "build" / f".dt_{rva:06x}.bin"; tmp.parent.mkdir(exist_ok=True); tmp.write_bytes(blob)
    try:
        out = subprocess.run([OBJDUMP, "-D", "-b", "binary", "-m", "i386",
                              "-Mintel", f"--adjust-vma=0x{rva:x}", str(tmp)],
                             capture_output=True, text=True).stdout
    finally:
        tmp.unlink(missing_ok=True)
    # keep only the instruction lines
    return "\n".join(l for l in out.splitlines() if ":\t" in l)

def main():
    args = sys.argv[1:]; no_dis = False
    if "--no-disasm" in args: no_dis = True; args.remove("--no-disasm")
    if not args:
        print(__doc__); return
    for a in args:
        rva = int(a, 16) if a.lower().startswith("0x") else byname.get(a)
        if rva is None:
            print(f"!! unknown target: {a}"); continue
        name, size = frows.get(rva, (resolve(rva) or "?", 0))
        print(f"\n{'='*72}\n{name}  @ RVA 0x{rva:06x}  (VA 0x{rva+IMAGEBASE:08x})  size {size} B\n{'='*72}")
        if size == 0:
            print("  (no boundary in functions.csv — recovery gap; size unknown)"); continue
        rl = relocs_in(rva, rva+size)
        if rl:
            print("Relocations (address operands — reloc-masked in objdiff):")
            for site, val, nm in rl:
                print(f"  @0x{site:06x}  -> 0x{val:08x}  {nm}")
        else:
            print("Relocations: none (self-contained / rel32 calls only)")
        if not no_dis:
            print("\nDisassembly (RVA-aligned):")
            print(disasm(rva, size))

if __name__ == "__main__":
    main()
