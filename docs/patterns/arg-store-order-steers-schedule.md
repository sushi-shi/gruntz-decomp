# Tiny multi-arg ctor/setter (no interleaved sentinels): assignment order fully steers the store schedule
tags: cpp:ctor cpp:local | asm:mov | topic:codegen-idiom topic:scheduling
symptoms: small __thiscall fn storing 2-3 args (+ a constant/vptr/trailing zeros) into fields; same instruction multiset, only ~2 arg-stores + the edx-held value permute vs retail; 99.5-99.8%
confidence: 8/10

The steerable counterpart to [[sentinel-seed-ctor-store-schedule]] (a WALL): when
the body is just arg-stores + a vptr/constant + a block of trailing `=0` stores —
with **no `-1`/`0x80000000` sentinels interleaved among the arg-stores** — the
assignment ORDER fully reproduces retail's schedule and closes to 100%. MSVC 5.0
/O2 pins the **2nd-referenced arg in `edx`** (loaded first, held across the other
stores); the remaining args stream through `eax`/`ecx` in source order. Pick the
assignment order so the arg retail holds in edx is written **second**.

```cpp
// retail holds `c` in edx across the m_0c store  (b,a,c stored: m_4,m_c,m_8)
m_04 = b;   // 1st ref -> eax/ecx
m_08 = c;   // 2nd ref -> held in edx
m_0c = a;   // 3rd ref
// NOT m_04=b; m_0c=a; m_08=c;  -> holds `a`, ascending-sorts the stores (1 byte off)
```
```asm
mov edx,0xc(esp)  ; c (2nd-referenced) loaded first, held
mov ecx,0x8(esp)  ; b
mov 0x4(eax),ecx  ; m_4=b
mov ecx,0x4(esp)  ; a
mov 0xc(eax),ecx  ; m_c=a
mov 0x8(eax),edx  ; m_8=c (held)
```
Steerable. WorkerFull::WorkerFull @0x15b300 99.8%→100%, CFaderMgr::SetConfig
@0x17d980 99.6%→100%. Distinct from sentinel-seed: there the interleaved `-1`
store ALSO floats (wall); here nothing floats, so the edx-arg reorder is sufficient.
