# Your frame is one dword wider than retail's: a named loop bound retail RE-READ from memory
tags: cpp:loop cpp:local | asm:sub asm:mov asm:test | topic:codegen-idiom
symptoms: sub esp differs by 4, extra esp slot, frame size mismatch, test edx,edx vs test dl,dl, spill reload, esp+0x1c
confidence: 8/10

A `do … while (k < n)` whose bound is a named local keeps that local live across the whole loop,
so cl spills it — one extra dword of frame and a store/reload pair. Retail often has no such local:
it re-reads the bound from memory at the loop test, which is free because the memory is already
addressable. The tell is the FRAME SIZE, which no per-instruction view mentions, plus a
zero-extended dword test (`xor edx,edx; mov dl,[..]; test edx,edx`) where retail tests the byte
(`mov dl,[..]; test dl,dl`) — the named `u32 n` forced the widening too.

```cpp
// spills: `n` is live across the loop
u32 n = src[srcidx];
if (n > 0) { i32 k = 0; do { … } while (k < (i32)n); }

// retail: no local, re-read at both the guard and the test
if (src[srcidx] > 0) { i32 k = 0; do { … } while (k < src[srcidx]); }
```
```asm
; ours                              ; retail
sub  esp,0x210                      sub  esp,0x20c
xor  edx,edx                        mov  dl,BYTE PTR [esi+ebx]
mov  dl,BYTE PTR [ebp+ebx]          test dl,dl
test edx,edx                        jbe  <skip>
mov  DWORD PTR [esp+0x1c],edx       ; …
```

STEERABLE. `CDDrawShadeBlit::EncodeRle16` 0x1495d0: 77.50 -> **91.96%**, frame 0x210 -> 0x20c,
147/147 instructions, and it dropped off `jcc_sieve`'s SIGNEDNESS list as a side effect (the
byte-vs-dword test was the signedness twin). Cross-check the frame with
`python -m gruntz.audit.base_size` — a length mismatch is a structural bug, never noise.
