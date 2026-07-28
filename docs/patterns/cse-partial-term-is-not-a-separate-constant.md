# A stack slot holding ONE channel of a packed constant is a CSE, not a separate constant

tags: cpp:local cpp:const | asm:sar asm:shl asm:or | topic:mis-model topic:tooling
symptoms: a long straight-line run of `mov reg,imm / sar reg,cl / shl reg,cl / or`,
partial results parked in `[esp+N]` and re-ORed into a LATER value, a table of
`Pack(r,g,b)`-style constants where suspiciously many have a `0x00` channel,
recompile has FEWER instructions than retail on a constant-table prologue
confidence: 10/10
variants: masked-immediate-hides-a-wrong-divisor.md

When a body seeds a table of composed constants — `Pack(r,g,b) = ((r>>rDown)<<rUp) |
((g>>gDown)<<gUp) | (b>>bDown)` with the shift amounts in globals — cl5 /O2 CSEs each
**per-channel term** that appears in more than one constant, parks it in a stack slot,
and ORs it back in later. Reading the disassembly top-down, that slot looks exactly
like a finished constant with two channels zero. Transcribe it that way and you get a
**double** error: a fabricated extra constant, and every real constant that consumed
the CSE loses that channel to `0x00`.

```asm
mov ebp,0x37 | sar ebp,[gDown] | shl ebp,[gUp] | mov [esp+0x1c],ebp   ; NOT Pack(0,0x37,0)
mov ebp,0x13 | sar ebp,[bDown]                 | mov [esp+0x14],ebp   ; NOT Pack(0,0,0x13)
mov ebp,0x63 | sar ebp,[rDown] | shl ebp,[rUp]
or  ebp,[esp+0x1c] | or ebp,[esp+0x14] | mov [esp+0x30],ebp   ; = Pack(0x63,0x37,0x13)
```

**A zero channel is VISIBLE — it is computed, not folded.** `Pack(0,0,0)` emits
`xor ebp,ebp; sar ebp,cl; shl ebp,cl` per channel; cl5 does not fold `0 >> var`. So a
channel that is genuinely zero costs three instructions, and a channel that arrives
from `or reg,[esp+N]` is genuinely NON-zero. That is the decisive test, and it is why
the recompile is SHORTER than retail: our table computed ~30 fabricated zero channels
that CSE into three, retail computed the real ones.

Decode it mechanically instead of by eye — a ~150-line symbolic interpreter over
`sema disasm --target --lite` (track `mov reg,imm`, `sar/shl reg,cl` tagged by WHICH
global `ecx` holds, `or`, and the `[esp+N]` slots) recovers both the constant list and
the store program exactly, including the dword-duplication (`mov ax,bx; mov cx,ax; shl
ecx,0x10; mov cx,ax`) and `rep stos` runs. Two cross-checks that caught the bug:
**siblings** (seven near-identical generators agreed on `Pack(0xff,0xd9,0x13)` where
one said `0x00`) and the **frame size** (retail 21 dwords vs our 20 for what we had
written as 24 constants).

Evidence (2026-07-28, `src/Gruntz/LightFxRender.cpp`): `CLightFxRender::Shape1..Shape8`
had 21-24 hand-read `Pack()`s each, most with a spurious zero channel, and three
fabricated single-channel colours per generator; buf[9..16], buf[120..123] and
buf[160..163] were painted from the wrong ones. Re-decoding took the eight from
67.6-77.0% to 82.7-85.4%, all eight having been filed as one "RGB-pack CSE +
scheduling wall".
