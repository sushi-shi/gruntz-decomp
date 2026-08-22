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

The same arithmetic decides what a WRITE means, and getting it wrong turns a missing
statement into an imaginary compiler quirk. `CStatusBarMgr::LoadChipMachineConfig`
0x106bb0 stores `edi` (=1) into `[esp+0x1c]` inside a three-push `GetDwordDef` setup,
i.e. E+0x10 — the same slot the epilogue reads with `cmp [esp+0x10],ebx` at depth 0.
Read at face value those five stores look like a write-only fourth local that no source
spelling can produce, and that is how the row was parked. They are the `rectFlag = 1`
that belongs in every belt-advance arm that moves the item rect; the reconstruction had
it only in the clamp arms, so the belt item's widget rect was never refreshed while it
travelled. 94.15 -> 97.97 on the five added assignments.

```asm
push 0x64                    ; three args pushed, esp = E-0xc
push <str> / push <str>
mov  DWORD PTR [esp+0x1c],edi   ; E+0x10  == rectFlag, NOT a fourth slot
call ?GetDwordDef@CButeMgr@@...
...
cmp  DWORD PTR [esp+0x10],ebx   ; E+0x10  again, at depth 0
```

Steerable, and a correctness oracle rather than a scoring one. Misreading exactly this
put `wFormatTag|nChannels` (0x00010001 = 65537) into `DirectSoundMgr::m_freq`, so every
`SetFrequencyOffsetPercent(0)` retuned each buffer to 65537 Hz — all game audio played
~3x fast, and `CreateBuffer` still scored 65.56% either way. Screen for it wherever a
struct is copied to a local and a field of that local is read back **after** an
intervening call.
