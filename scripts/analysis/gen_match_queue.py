#!/usr/bin/env python3
"""gen_match_queue.py - build docs/match-queue.md, the prioritized matching worklist.

The autonomous matching campaign grinds labeled-but-unmatched engine functions
into byte-exact src/. This picks WHAT to match next: middle-small, leaf-first.

Inputs (all already in the repo):
  config/engine_labels.csv         - labeled engine funcs (names/classes/protos)
  config/symbol_names.csv          - the byte-MATCHED set (exclude: already done)
  build/ghidra/exports/functions.csv - function boundaries + sizes
  config/library_labels.csv        - library funcs (exclude: not engine)
  build/patch-diff/validated_changed.pkl - 52 v1.01-changed funcs (exclude)
  $GRUNTZ_EXE (flake)              - scanned for E8 call edges (leaf-readiness)

Readiness = number of an engine callee functions not yet matched/library. 0 = a
leaf (or all callees already matched) -> match it now. We order by readiness, then
prefer the middle-small size band (~64-512 B), then by size. Re-run any time to
refill the queue as matches land (idempotent).

Run inside nix develop: nix develop .#build --command python3 scripts/gen_match_queue.py
"""
import os, struct, csv, bisect, pickle
from pathlib import Path

REPO = next((p for p in Path(__file__).resolve().parents if (p / "flake.nix").exists()),
            Path(__file__).resolve().parent)
EXE = Path(os.environ.get("GRUNTZ_EXE") or REPO / "build/exe/GRUNTZ.EXE")
FUNCS = REPO / "build/ghidra/exports/functions.csv"
LABELS = REPO / "config/engine_labels.csv"
MATCHED = REPO / "build/gen/symbol_names.csv"   # generated (was config/symbol_names.csv)
FID = REPO / "config/library_labels.csv"
CHANGED = REPO / "build/patch-diff/validated_changed.pkl"
OUT = REPO / "docs/match-queue.md"

def rint(s):
    return int(s, 16) if isinstance(s, str) else int(s)

# ---- PE sections (for rva->file offset on E8 scan) ----
d = EXE.read_bytes()
e = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, e+6)[0]; optsz = struct.unpack_from('<H', d, e+20)[0]; opt = e+24
secs = [struct.unpack_from('<IIII', d, opt+optsz+i*40+8) for i in range(nsec)]  # vsz,va,rsz,rp

def off(rva):
    for vsz, va, rsz, rp in secs:
        if va <= rva < va+max(vsz, rsz): return rva-va+rp
    return None

# ---- functions: boundaries + sizes (in_text, non-thunk) ----
size_of = {}; name_of = {}; funcs = []
with open(FUNCS) as f:
    for r in csv.DictReader(f):
        try:
            rva = rint(r['entry_rva']); sz = int(r['byte_size'])
        except Exception:
            continue
        size_of[rva] = sz; name_of[rva] = r['name']
        if r.get('in_text', '1') == '1' and r.get('is_thunk', '0') != '1':
            funcs.append((rva, sz))
funcs.sort(); starts = [x[0] for x in funcs]
startset = set(starts)

def owner(rva):
    i = bisect.bisect_right(starts, rva)-1
    return funcs[i][0] if i >= 0 and funcs[i][0] <= rva < funcs[i][0]+funcs[i][1] else None

# ---- call graph: scan E8 rel32 within each function body ----
callees = {}  # func_rva -> set(callee_rva that is a known func start)
for frva, sz in funcs:
    o = off(frva)
    if o is None: continue
    body = d[o:o+sz]; cs = set(); i = 0
    while True:
        i = body.find(b'\xe8', i)
        if i < 0 or i+5 > len(body): break
        tgt = frva + i + 5 + struct.unpack_from('<i', body, i+1)[0]
        if tgt in startset and tgt != frva: cs.add(tgt)
        i += 1
    callees[frva] = cs

# ---- exclusion / status sets ----
matched = set()
with open(MATCHED) as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith('#') or line.startswith('rva,'): continue
        matched.add(rint(line.split(',', 1)[0]))
library = set()
if FID.exists():
    with open(FID) as f:
        for r in csv.DictReader(f):
            try: library.add(rint(r['rva']))
            except Exception: pass
changed = {t[0] for t in pickle.load(open(CHANGED, 'rb'))}

# ---- labeled candidates ----
rows = []
with open(LABELS) as f:
    for r in csv.DictReader((l for l in f if not l.startswith('#'))):
        try: rva = rint(r['rva'])
        except Exception: continue
        rows.append((rva, r))

queue = []
for rva, r in rows:
    if rva in matched or rva in library or rva in changed: continue
    nm = (r.get('name') or '').strip()
    cls = (r.get('class') or '').strip()
    if not nm and not cls: continue                 # pure unlabeled import-caller seed
    sz = size_of.get(rva)
    in_text = rva in startset
    cs = callees.get(rva, set())
    # engine callees still needing work = not matched, not library, not self
    blockers = sorted(c for c in cs if c not in matched and c not in library)
    queue.append({
        'rva': rva, 'name': nm, 'class': cls, 'size': sz, 'in_text': in_text,
        'kind': (r.get('kind') or '').strip(), 'source': (r.get('source') or '').strip(),
        'conf': (r.get('confidence') or '').strip(),
        'nblock': len(blockers), 'blockers': blockers,
    })

def size_rank(sz):
    if sz is None: return 9          # unknown boundary (recovery gap) -> defer
    if 64 <= sz <= 512: return 0     # middle-small: campaign priority band
    if sz < 64: return 1             # tiny leaves
    if sz <= 1024: return 2
    return 3                          # big -> Pareto later

queue.sort(key=lambda q: (q['nblock'], size_rank(q['size']), q['size'] if q['size'] else 1<<30, q['rva']))

ready = [q for q in queue if q['nblock'] == 0 and q['in_text'] and q['size'] is not None]
blocked = [q for q in queue if q not in ready]

def fmt(q):
    nm = q['name'] or f"({q['class']})"
    deps = '' if not q['blockers'] else ' '.join(f"0x{b:06x}" for b in q['blockers'][:4]) + ('…' if len(q['blockers'])>4 else '')
    sz = q['size'] if q['size'] is not None else '?'
    st = 'ready' if (q['nblock']==0 and q['in_text'] and q['size'] is not None) else ('gap' if not q['in_text'] or q['size'] is None else f"blk:{q['nblock']}")
    return f"| 0x{q['rva']:06x} | {nm} | {q['class']} | {sz} | {st} | {deps} | {q['source']} | {q['conf']} |"

hdr = "| rva | name | class | size | status | blockers | source | conf |\n|---|---|---|---|---|---|---|---|"
band = lambda lo, hi: [q for q in ready if (lo <= (q['size'] or 0) <= hi)]
mid = band(64, 512)
lines = []
lines.append("# Match queue (auto-generated by scripts/gen_match_queue.py — re-run to refill)\n")
lines.append(f"Totals: {len(queue)} labeled-unmatched candidates · {len(ready)} READY (leaf, sized, in .text) · {len(blocked)} blocked/gap.")
lines.append(f"Excluded: {len(matched)} matched · {len(library)} library(FID) · {len(changed)} v1.01-changed.\n")
lines.append("Order: readiness (unmatched engine callees) → middle-small band (64–512 B) → size. Match top-down.\n")
lines.append(f"## READY — middle-small first ({len(mid)} in 64–512 B band)\n")
lines.append(hdr)
for q in mid: lines.append(fmt(q))
rest = [q for q in ready if q not in mid]
lines.append(f"\n## READY — other sizes ({len(rest)})\n")
lines.append(hdr)
for q in rest: lines.append(fmt(q))
lines.append(f"\n## BLOCKED / boundary-gap ({len(blocked)}) — unlock as deps match\n")
lines.append(hdr)
for q in blocked[:120]: lines.append(fmt(q))
if len(blocked) > 120: lines.append(f"\n…and {len(blocked)-120} more (re-run after matches land).")
OUT.write_text("\n".join(lines) + "\n")
print(f"wrote {OUT.relative_to(REPO)}: {len(ready)} ready ({len(mid)} middle-small), {len(blocked)} blocked, {len(queue)} total candidates.")
