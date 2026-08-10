# The same `[esp+N]` names TWO different locals when an argument was pushed early
tags: cpp:local cpp:call cpp:struct | asm:push asm:mov | topic:correctness topic:audit
symptoms: `[esp+0x14]` read back after a call it was stored before; `push esi` far above its `call`; wrong field of a copied struct; runtime defect at ~100% match
confidence: 10/10

An `esp`-relative displacement is only meaningful together with the `esp` at *that*
instruction. /O2 hoists a constant argument push (a `NULL` out-param, a `0` flag) many
instructions above its `call`, so `esp` is 4 (or 8, 12...) lower for the whole block in
between. Read the store and the load as "the same local" and you have silently swapped
one field of a copied struct for the next — a **semantic** bug that costs almost nothing
in match % because both spellings encode as `8b 54 24 XX`.

Walk the frame from the prologue and re-derive `esp` at every load. Anchor it on the
epilogue: the `mov ecx,[esp+K]` that restores `fs:0` must name the saved `__except_list`
slot, which pins `esp` for the whole tail.

```asm
; SoundDevice::CreateBuffer 0x1366f0 - wf is at E-0x34, esp is E-0x48 here
13674b: push esi                     ; pUnkOuter=NULL, hoisted ~0x6c bytes above its call
13674e: mov  [esp+0x14],ecx          ; E-0x34  <- wf.wFormatTag|nChannels
136755: mov  [esp+0x18],edx          ; E-0x30  <- wf.nSamplesPerSec
1367b7: call [ecx+0xc]               ; CreateSoundBuffer; stdcall eats that push -> esp = E-0x44
136808: mov  edx,[esp+0x14]          ; E-0x30  == wf.nSamplesPerSec, NOT the first dword
136816: mov  [esi+0x18],edx          ; voice->m_freq = fmt->nSamplesPerSec
```

```cpp
voice->m_freq = wf.nSamplesPerSec;   // not wf's first dword
```

Steerable, and a correctness oracle rather than a scoring one. Misreading exactly this
put `wFormatTag|nChannels` (0x00010001 = 65537) into `DirectSoundMgr::m_freq`, so every
`SetFrequencyOffsetPercent(0)` retuned each buffer to 65537 Hz — all game audio played
~3x fast, and `CreateBuffer` still scored 65.56% either way. Screen for it wherever a
struct is copied to a local and a field of that local is read back **after** an
intervening call.
