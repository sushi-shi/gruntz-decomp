#!/usr/bin/env python3
# string_xref_labels.py - string-xref labeling aid for config/engine_labels.csv.
#
# Mechanically recovers, for every Ghidra-recognised function, the .rdata/.data
# string literals it directly references (an immediate 4-byte LE VA equal to a
# string start, i.e. a `push offset`/`mov reg,offset` operand in .text). The
# function->strings table turns the Gruntz/WAP32 string taxonomy (see
# docs/strings-analysis.md) into per-function role evidence: a bare FUN_ that
# loads every GRUNTZ_PICKUPS_* key is a pickup-sprite loader; one that self-IDs
# 'DirectDrawMgr' + every DDERR_* is the DDraw error formatter; etc.
#
# This is the reproducible extraction half of the "string-xref" rows in
# config/engine_labels.csv. The name/confidence judgment is human (the script
# only surfaces ranked candidates).
#
# Inputs : binaries/retail_en/GRUNTZ.EXE  (v1.0 EN, md5 81c7f648...)
#          build/ghidra-named/exports/functions.csv  (Ghidra function boundaries)
# Usage  : nix develop --command python3 scripts/string_xref_labels.py [--rva 0x141400 ...]
#          (no args -> ranked report of bare FUN_ funcs with distinctive strings)
import struct, csv, re, bisect, sys, os

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN  = os.path.join(ROOT, "binaries/retail_en/GRUNTZ.EXE")
FUNCS= os.path.join(ROOT, "build/ghidra-named/exports/functions.csv")
IMAGE_BASE = 0x400000

def load():
    data = open(BIN, "rb").read()
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    nsec = struct.unpack_from("<H", data, pe+6)[0]
    opt_size = struct.unpack_from("<H", data, pe+20)[0]
    sec_off = pe + 24 + opt_size
    secs = []
    for i in range(nsec):
        o = sec_off + i*40
        name = data[o:o+8].rstrip(b"\0").decode("latin1")
        vsize, vaddr, rawsz, rawp = struct.unpack_from("<IIII", data, o+8)
        secs.append((name, vaddr, vsize, rawp, rawsz))
    text = next(s for s in secs if s[0] == ".text")
    tbytes = data[text[3]:text[3]+text[4]]; trva = text[1]
    # strings (start VA -> text) from non-.text sections
    str_at = {}
    pat = re.compile(rb"[\x20-\x7e]{4,}")
    for name, vaddr, vsize, rawp, rawsz in secs:
        if name == ".text": continue
        blob = data[rawp:rawp+rawsz]
        for m in pat.finditer(blob):
            str_at[IMAGE_BASE+vaddr+m.start()] = m.group().decode("latin1")
    # function ranges
    starts=[]; fname={}; fsize={}
    with open(FUNCS) as f:
        for row in csv.DictReader(f):
            rva=int(row["entry_rva"],16); starts.append(rva)
            fname[rva]=row["name"]; fsize[rva]=int(row["byte_size"])
    starts.sort()
    def func_of(rva):
        i=bisect.bisect_right(starts,rva)-1
        return starts[i] if i>=0 and rva<starts[i]+fsize[starts[i]] else None
    # scan .text for string-VA immediates
    func_strs={}
    lo,hi=min(str_at),max(str_at)
    n=len(tbytes)
    for o in range(n-3):
        v=tbytes[o]|(tbytes[o+1]<<8)|(tbytes[o+2]<<16)|(tbytes[o+3]<<24)
        if v<lo or v>hi: continue
        s=str_at.get(v)
        if s is None: continue
        fn=func_of(trva+o)
        if fn is not None:
            func_strs.setdefault(fn,set()).add(s)
    return func_strs, fname, fsize

def main():
    func_strs, fname, fsize = load()
    args=sys.argv[1:]
    if "--rva" in args:
        for tok in args[args.index("--rva")+1:]:
            rva=int(tok,16)
            print(f"### 0x{rva:06x} {fname.get(rva,'?')} sz={fsize.get(rva,0)}")
            for s in sorted(func_strs.get(rva,[])): print("   "+repr(s))
        return
    DIST=re.compile(r"GRUNTZ|AREA|STAGE|WORLDZ?|QUESTZ|TOOLZ|TOYZ|WARLORDZ|POWERUPZ|"
                    r"\.wwd|\.rez|\.vob|\.SF2|BOOTY|MULTI|STATEZ|SECRET|TELEPORT|CHEAT|"
                    r"DDERR_|DIERR_|DSERR_|CURSORZ|DEATHZ",re.I)
    rows=[]
    for rva,strs in func_strs.items():
        nm=fname.get(rva,"")
        if not (nm.startswith("FUN_") or nm.startswith("thunk_FUN_")): continue
        good=[s for s in strs if len(s)>=4]
        score=sum(3 if DIST.search(s) else 1 for s in good)
        if score: rows.append((score,rva,fsize.get(rva,0),sorted(good)))
    rows.sort(key=lambda r:(-r[0],r[1]))
    for score,rva,sz,good in rows[:80]:
        head=" | ".join(good[:6])+(f" (+{len(good)-6})" if len(good)>6 else "")
        print(f"[{score:4d}] 0x{rva:06x} sz={sz:<5d} {head}")

if __name__=="__main__":
    main()
