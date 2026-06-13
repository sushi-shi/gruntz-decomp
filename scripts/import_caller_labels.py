#!/usr/bin/env python3
"""import_caller_labels.py — generate engine LABELS from import-caller analysis.

A function that calls a *discriminating* imported function attributes (with
confidence) to that import's subsystem/TU. This parses GRUNTZ.EXE's PE imports,
computes each import's IAT slot VA (ImageBase + FirstThunk + idx*4), finds the
functions that reach it — both the direct `FF15 [IAT]` form (Win32 dllimport) and
the `FF25 [IAT]` thunk + its `E8` callers (non-dllimport, e.g. DirectX) — maps the
owning function (Ghidra functions.csv), and MERGES new rows into
config/engine_labels.csv (deduping by RVA against existing labels).

Confidence tiers reflect how discriminating the import is:
  high — DirectX *creators* → a specific manager TU (DirectDrawCreate=CDirectDrawMgr…)
  med  — registry (ADVAPI32 Reg*) → config code (RegistryHelper cluster)
  low  — file I/O (KERNEL32 CreateFile/ReadFile…) → broad (RezMgr/save-load/wrappers)

Run inside `nix develop` (needs python3): python3 scripts/import_caller_labels.py
"""
import struct, csv, bisect
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
EXE = REPO / "binaries/retail_en/GRUNTZ.EXE"
FUNCS = REPO / "build/ghidra/exports/functions.csv"
LABELS = REPO / "config/engine_labels.csv"

d = EXE.read_bytes()
e = struct.unpack_from('<I', d, 0x3c)[0]
nsec = struct.unpack_from('<H', d, e+6)[0]; optsz = struct.unpack_from('<H', d, e+20)[0]; opt = e+24
imagebase = struct.unpack_from('<I', d, opt+28)[0]
imp_rva = struct.unpack_from('<I', d, opt+96+8)[0]
secs = [struct.unpack_from('<IIII', d, opt+optsz+i*40+8) for i in range(nsec)]  # vsz,va,rsz,rp

def off(rva):
    for vsz, va, rsz, rp in secs:
        if va <= rva < va+max(vsz, rsz): return rva-va+rp

def cstr(rva):
    o = off(rva); return d[o:d.index(b'\0', o)].decode('latin1', 'replace')

funcs = []
with open(FUNCS) as f:
    for r in csv.DictReader(f):
        try:
            if r.get('in_text', '1') != '1': continue
            funcs.append((int(r['entry_rva'], 16), int(r['byte_size'])))
        except Exception: pass
funcs.sort(); starts = [x[0] for x in funcs]

def owner(rva):
    i = bisect.bisect_right(starts, rva)-1
    return funcs[i][0] if i >= 0 and funcs[i][0] <= rva < funcs[i][0]+funcs[i][1] else None

# slot VA per import (named + ordinal)
slot_of = {}; p = off(imp_rva)
while True:
    oft, ts, fc, nm, ft = struct.unpack_from('<IIIII', d, p)
    if oft == 0 and nm == 0 and ft == 0: break
    dll = cstr(nm); idx = 0
    while True:
        t = struct.unpack_from('<I', d, off(oft or ft)+idx*4)[0]
        if t == 0: break
        key = f"{dll}!#{t & 0xffff}" if t & 0x80000000 else cstr(t+2)
        slot_of[key] = imagebase + ft + idx*4
        idx += 1
    p += 20

def callers_of(key):
    if key not in slot_of: return set()
    slot = slot_of[key]; ow = set()
    p15 = b'\xff\x15'+struct.pack('<I', slot); p25 = b'\xff\x25'+struct.pack('<I', slot); thunks = set()
    for vsz, va, rsz, rp in secs:
        blob = d[rp:rp+rsz]
        i = blob.find(p15)
        while i >= 0:
            w = owner(va+i);  ow.add(w) if w else None; i = blob.find(p15, i+1)
        j = blob.find(p25)
        while j >= 0:
            thunks.add(va+j); j = blob.find(p25, j+1)
    if thunks:
        for vsz, va, rsz, rp in secs:
            blob = d[rp:rp+rsz]; i = -1
            while True:
                i = blob.find(b'\xe8', i+1)
                if i < 0 or i+5 > len(blob): break
                if va+i+5+struct.unpack_from('<i', blob, i+1)[0] in thunks:
                    w = owner(va+i); ow.add(w) if w else None
                i += 1
    return ow

RANK = {'high': 3, 'med': 2, 'low': 1, None: 0}
# (import key, class-attribution, confidence)
TARGETS = (
    [(k, m, 'high') for k, m in (('DirectDrawCreate', 'CDirectDrawMgr'), ('DirectDrawEnumerateA', 'CDirectDrawMgr'),
      ('DirectInputCreateA', 'DirectInputMgr2'), ('DSOUND.dll!#1', 'DirectSoundMgr'),
      ('DPLAYX.dll!#1', 'CNetMgr'), ('DPLAYX.dll!#2', 'CNetMgr'), ('DPLAYX.dll!#4', 'CNetMgr'))] +
    [(k, '', 'med') for k in ('RegOpenKeyExA', 'RegQueryValueExA', 'RegCreateKeyExA', 'RegSetValueExA', 'RegCloseKey')] +
    [(k, '', 'low') for k in ('CreateFileA', 'ReadFile', 'WriteFile', 'FindFirstFileA', 'SetFilePointer', 'OpenFile', 'GetFileSize')]
)
seeds = {}  # rva -> {class, conf, imports:set}
for key, cls, conf in TARGETS:
    for w in callers_of(key):
        s = seeds.setdefault(w, {'class': '', 'conf': None, 'imports': set()})
        s['imports'].add(key)
        if RANK[conf] > RANK[s['conf']]: s['conf'] = conf; s['class'] = cls

# merge into engine_labels.csv
raw = LABELS.read_text().splitlines()
comments = [l for l in raw if l.startswith('#')]
body = [l for l in raw if not l.startswith('#')]
header = body[0]; data = body[1:]
have = {row.split(',', 1)[0] for row in data if row.strip()}
added = 0; overlap = 0
for rva, s in sorted(seeds.items()):
    key = f"0x{rva:06x}"
    if key in have: overlap += 1; continue
    src = "import:" + ",".join(sorted(s['imports']))
    # csv-safe (source has commas) -> quote
    data.append(f'{key},,{s["class"]},,import-caller,"{src}",{s["conf"]}')
    added += 1
# sort data rows by rva
data = [l for l in data if l.strip()]
data.sort(key=lambda l: int(l.split(',', 1)[0], 16))
LABELS.write_text("\n".join(comments + [header] + data) + "\n")
print(f"import-caller seeds: {len(seeds)} functions; added {added}, skipped {overlap} already-labeled (cross-validated).")
from collections import Counter
print("by confidence (added):", dict(Counter(s['conf'] for rva, s in seeds.items() if f"0x{rva:06x}" not in have)))
