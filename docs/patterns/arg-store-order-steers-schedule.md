# Tiny multi-arg ctor/setter (no interleaved sentinels): assignment order fully steers the store schedule
tags: cpp:ctor cpp:local | asm:mov | topic:codegen-idiom topic:scheduling
symptoms: small __thiscall fn storing 2-3 args (+ a constant/vptr/trailing zeros) into fields; same instruction multiset, only ~2 arg-stores + the edx-held value permute vs retail; 99.5-99.8%
confidence: 9/10

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

## Larger-body extension: the emitted store order need not be the source order

`StreamFeeder::Initialize` @0x137d10 proves the same lever survives at the head
of a 171-byte function with five calls and seven branches. Base and retail had
identical instruction, store, constant, CFG, and four-referent multisets, but the
first argument preload used the wrong register and the choice cascaded into the
virtual `ResetSource` receiver load and the final `PrimeBuffer` argument load.

An exhaustive 24-order matrix over the four independent initial assignments
produced 15 compiler islands and exactly two byte-exact states. The retained
source follows semantic/member-layout order:

```cpp
m_owner = owner;
m_bufferBytes = bufferBytes;
m_refillThresholdBytes = refillThresholdBytes;
m_playing = false;
```

That order moved `bufferBytes` into EAX before the callee-save pushes and kept
`refillThresholdBytes` in EDX, closing the whole function from 96.158730% to
100%. Retail nevertheless emits the stores as refill-threshold, owner,
buffer-size, playing. Therefore do not transcribe a retail store run back into
source order mechanically: for a larger body, authored assignment order can
steer value creation and register lifetimes while the scheduler emits a different
store order. The second exact state puts `m_playing` before the refill-threshold
assignment; bytes cannot distinguish those two independent source orders, so
member-layout order is the structural tie-break.
