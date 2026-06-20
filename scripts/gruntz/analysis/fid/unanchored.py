#!/usr/bin/env python3
"""
Unanchored sweep: find substantial, distinctive library bodies that occur at
.text offsets NOT registered as Ghidra function starts (Ghidra missed them).

Strictness to avoid false positives:
  - only sigs with trim_len >= MIN_TRIM and n_fixed >= MIN_FIXED
  - a candidate offset is accepted only if it is NOT in the interior of an
    existing function (it may be in a gap, or exactly at a known start which we
    skip since the anchored scan already handled those)
  - the matched body must not overrun into a known function start that lies
    strictly after the candidate (overlap => reject)
  - require the byte right before the candidate (if in a gap) to be padding
    (0x90/0xCC) OR the candidate to directly abut the previous function end,
    i.e. a plausible function boundary.
Report newly-found (rva,name,lib) with the same HIGH/AMBIG logic.
"""
import os,sys,struct,pickle,csv,json
from collections import defaultdict
from gruntz.analysis.fid.common import pe_text, trim_pad

MIN_TRIM=24
MIN_FIXED=16

def main():
    sigs_pkl,exe,funcs_csv,out_csv=sys.argv[1:5]
    text,trva,_=pe_text(exe)
    rows=list(csv.DictReader(open(funcs_csv)))
    starts={}
    ints=[]
    for r in rows:
        rva=int(r['entry_rva'],16); off=rva-trva
        if 0<=off<len(text):
            starts[off]=int(r['byte_size']); ints.append((off,int(r['byte_size'])))
    ints.sort()
    start_set=set(starts)
    sorted_starts=sorted(start_set)
    import bisect
    # interior intervals (off, off+size) for "inside" test using byte_size
    # build sorted start list for overrun check
    def next_start_after(off):
        i=bisect.bisect_right(sorted_starts,off)
        return sorted_starts[i] if i<len(sorted_starts) else len(text)
    def inside_interior(off):
        # is off strictly inside some function body [s, s+size)? (s<off)
        i=bisect.bisect_right(sorted_starts,off)-1
        if i<0: return False
        s=sorted_starts[i]; sz=starts[s]
        return s<off<s+sz

    sigs=pickle.load(open(sigs_pkl,'rb'))
    sel=[]
    for s in sigs:
        body=bytes.fromhex(s['bytes']); mask=bytes.fromhex(s['mask'])
        fi=[i for i in range(s['length']) if mask[i]==0xff]
        tl=trim_pad(body,mask)
        if tl>=MIN_TRIM and len(fi)>=MIN_FIXED:
            s['_body']=body; s['_fi']=fi; s['_tl']=tl; s['_f0']=fi[0]; s['_nf']=len(fi)
            sel.append(s)
    print(f"# substantial sigs: {len(sel)}",file=sys.stderr)

    # index by first-fixed byte value for whole-text search
    found=defaultdict(list)  # off -> [(name,lib,tl,nf,member)]
    for s in sel:
        body=s['_body']; fi=s['_fi']; f0=s['_f0']; b0=body[f0]; L=s['length']
        pos=0
        target=bytes([b0])
        while True:
            p=text.find(target,pos)
            if p<0: break
            pos=p+1
            off=p-f0
            if off<0 or off+L>len(text): continue
            if off in start_set: continue          # anchored scan covers these
            if inside_interior(off): continue        # reject mid-body
            # overrun: trimmed body must not cross the next known start
            ns=next_start_after(off)
            if off+s['_tl']>ns: continue
            if all(text[off+i]==body[i] for i in fi):
                found[off].append((s['name'],s['lib'],s['_tl'],s['_nf'],s['member']))

    # classify like before
    name_offs=defaultdict(set); off_names=defaultdict(set)
    for off,lst in found.items():
        for (nm,lib,tl,nf,mem) in lst:
            name_offs[nm].add(off); off_names[off].add(nm)

    out=[]
    for off,lst in found.items():
        rva=off+trva
        names=off_names[off]
        # name (x[0]) is the final tiebreaker so identical-body collisions resolve
        # deterministically, independent of .lib member ingest order.
        rep=sorted(lst,key=lambda x:(0 if len(name_offs[x[0]])==1 else 1,-x[2],-x[3],x[0]))[0]
        nm,lib,tl,nf,mem=rep
        name_n=len(name_offs[nm])
        ambiguous=(len(names)>1 or name_n>1)
        conf='HIGH' if not ambiguous else 'AMBIG'
        out.append((rva,nm,lib,conf,tl,nf,name_n,len(names),mem))
    out.sort()
    with open(out_csv,'w',newline='') as f:
        w=csv.writer(f)
        w.writerow(['rva','name','lib','confidence','trim_len','n_fixed','name_match_count','off_name_count','member'])
        for r in out: w.writerow([f'0x{r[0]:06x}']+list(r[1:]))
    from collections import Counter
    print(json.dumps(dict(new_offstart_rvas=len(out),
        tiers=dict(Counter(r[3] for r in out)),
        by_lib=dict(Counter(r[2] for r in out))),indent=2))

if __name__=='__main__': main()
