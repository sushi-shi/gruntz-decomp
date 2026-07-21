#!/usr/bin/env python3
"""
Final scan + classify (v3).

Match each library signature (masked) at every KNOWN function-start in
GRUNTZ.EXE .text. Then classify each (rva,name) using global statistics:

  - name_rvas[name]   = how many distinct EXE starts this name matches.
                        A *distinctive* library body matches few starts;
                        a *generic shape* (thunk/virtual stub) matches many.
  - rva_names[rva]     = distinct names matching at this start (identical bodies).

Confidence:
  HIGH  iff  name matches exactly ONE rva  (unique body, no ambiguity)
         AND  rva is claimed by exactly ONE name
         AND  the discriminating signature is substantial:
              sig_len-trailing_pad >= HIGH_LEN  and  n_fixed >= HIGH_FIXED
         AND  sig does not overrun into the next function start.
  LOW   otherwise (generic shape, multi-name collision, or short/weak sig)
         -- still emitted, but flagged with the reason.

Trailing alignment padding (0x90 NOP / 0xCC INT3) is trimmed from the END of a
signature for length/overrun accounting (it is not part of the function body).
"""
import os, sys, struct, pickle, json, csv
from collections import defaultdict
from gruntz.audit.fid.common import pe_text, trim_pad

MIN_LEN    = 8     # minimum sig length to attempt primary matching
MIN_FIXED  = 6     # minimum fixed (non-wildcard) bytes to attempt
HIGH_LEN   = 16    # trimmed-length floor for HIGH
HIGH_FIXED = 10    # fixed-byte floor for HIGH

def _zlib_units(toml_path):
    """Unit stems whose source is a vendored zlib TU (per config/units.toml)."""
    import tomllib
    with open(toml_path,'rb') as f:
        data=tomllib.load(f)
    return {u['unit'] for u in data.get('unit',[])
            if str(u.get('source','')).startswith('vendor/zlib')}

def main():
    sigs_pkl,exe,funcs_csv,out_csv = sys.argv[1:5]
    text,trva,base=pe_text(exe)
    func_size={}; offs=[]
    for row in csv.DictReader(open(funcs_csv)):
        rva=int(row['entry_rva'],16); off=rva-trva
        if 0<=off<len(text):
            func_size[rva]=int(row['byte_size']); offs.append(off)
    start_set=set(offs); sorted_starts=sorted(start_set)
    next_start={o:(sorted_starts[i+1] if i+1<len(sorted_starts) else len(text))
                for i,o in enumerate(sorted_starts)}
    print(f"# starts={len(start_set)}",file=sys.stderr)

    sigs=pickle.load(open(sigs_pkl,'rb'))
    # prepare
    prepared=[]
    for s in sigs:
        body=bytes.fromhex(s['bytes']); mask=bytes.fromhex(s['mask'])
        fi=[i for i in range(s['length']) if mask[i]==0xff]
        if s['length']<MIN_LEN or len(fi)<MIN_FIXED:
            continue
        tlen=trim_pad(body,mask)
        prepared.append(dict(name=s['name'],lib=s['lib'],member=s['member'],
                             length=s['length'],tlen=tlen,nfixed=len(fi),
                             body=body,fi=fi,f0=fi[0]))
    print(f"# usable sigs={len(prepared)}",file=sys.stderr)

    by_f0=defaultdict(dict)
    for s in prepared:
        by_f0[s['f0']].setdefault(s['body'][s['f0']],[]).append(s)
    f0_list=sorted(by_f0.keys())

    # match
    hits=defaultdict(list)  # rva -> list of sig dict
    for off in start_set:
        for f0 in f0_list:
            p=off+f0
            if p>=len(text): break
            cand=by_f0[f0].get(text[p])
            if not cand: continue
            for s in cand:
                if off+s['length']>len(text): continue
                body=s['body']
                if all(text[off+i]==body[i] for i in s['fi']):
                    hits[off+trva].append(s)

    # global stats
    name_rvas=defaultdict(set)
    rva_names=defaultdict(set)
    for rva,lst in hits.items():
        for s in lst:
            name_rvas[s['name']].add(rva)
            rva_names[rva].add(s['name'])

    rows=[]
    for rva,lst in hits.items():
        off=rva-trva; room=next_start[off]-off; fs=func_size.get(rva,0)
        names=rva_names[rva]
        # representative: prefer a name that is itself unique (claims only this rva),
        # then longest trimmed length, then most fixed bytes, then the name itself
        # (final tiebreaker so identical-body collisions resolve deterministically,
        # independent of .lib member ingest order).
        def keyf(s):
            uniq = (len(name_rvas[s['name']])==1)
            return (0 if uniq else 1, -s['tlen'], -s['nfixed'], s['name'])
        rep=sorted(lst,key=keyf)[0]
        name,lib=rep['name'],rep['lib']
        name_n=len(name_rvas[name])
        notes=[]
        # base predicates
        ambiguous = (len(names)>1 or name_n>1)
        overrun   = (rep['tlen']>room)
        substantial = (rep['tlen']>=HIGH_LEN and rep['nfixed']>=HIGH_FIXED)
        if len(names)>1:  notes.append(f'rva_multiname={len(names)}')
        if name_n>1:      notes.append(f'name_multimatch={name_n}')
        if rep['tlen']<HIGH_LEN:   notes.append(f'shorttrim={rep["tlen"]}')
        if rep['nfixed']<HIGH_FIXED:notes.append(f'fewfixed={rep["nfixed"]}')
        if overrun:       notes.append(f'overrun(room={room},tlen={rep["tlen"]})')
        # tiering:
        #   HIGH   = unambiguous + substantial + no overrun (byte-exact unique body)
        #   MEDIUM = unambiguous (name- AND rva-unique) + no overrun, but short body
        #            (uniqueness across 14,411 starts is strong even for short funcs)
        #   LOW    = ambiguous (generic shape / identical-body collision) or overrun
        if not ambiguous and not overrun and substantial:
            conf='HIGH'
        elif not ambiguous and not overrun:
            conf='MEDIUM'
        elif substantial and not overrun:
            # ambiguous name but unmistakably a (substantial) library body:
            # the RVA is library code; only the exact sibling-name is uncertain.
            conf='AMBIG'
        else:
            conf='LOW'
        rows.append((rva,name,lib,conf,rep['tlen'],rep['length'],fs,room,rep['nfixed'],
                     name_n,len(names),rep['member'],';'.join(notes)))

    rows.sort()
    with open(out_csv,'w',newline='') as f:
        w=csv.writer(f)
        w.writerow(['rva','name','lib','confidence','trim_len','sig_len','func_size',
                    'room','n_fixed','name_match_count','rva_name_count','member','notes'])
        for r in rows:
            w.writerow([f'0x{r[0]:06x}',r[1],r[2],r[3],r[4],r[5],r[6],r[7],r[8],r[9],r[10],r[11],r[12]])

    # deliverable CSV: rva,name,lib,confidence  (one row per matched RVA)
    deliv = out_csv.rsplit('/',1)[0] + '/library_labels.csv'
    with open(deliv,'w',newline='') as f:
        w=csv.writer(f); w.writerow(['rva','name','lib','confidence'])
        # zlib (already hand-matched) for completeness, if present. Only label rows
        # whose unit is an actual vendored-zlib TU -- symbol_names.csv now also
        # carries engine/game units (CFileIO, dialog procs, ...) that are NOT zlib.
        try:
            zlib_units=_zlib_units('config/units.toml')
            for line in open('build/gen/symbol_names.csv'):
                line=line.strip()
                if line.startswith('0x'):
                    rva_s,name,unit=line.split(',')[:3]
                    if unit not in zlib_units:
                        continue
                    w.writerow([f'0x{int(rva_s,16):06x}',name,'zlib','HIGH'])
        except Exception: pass
        for r in rows:
            w.writerow([f'0x{r[0]:06x}',r[1],r[2],r[3]])

    from collections import Counter
    tier=Counter(r[3] for r in rows)
    by_lib=defaultdict(lambda:Counter())
    for r in rows: by_lib[r[2]][r[3]]+=1
    print(json.dumps(dict(
        matched_rvas=len(rows),
        tiers=dict(tier),
        by_lib={k:dict(v) for k,v in by_lib.items()},
    ),indent=2))

if __name__=='__main__':
    main()
