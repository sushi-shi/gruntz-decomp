# One role at two frame slots names WHICH header local the loop reuses

tags: cpp:local cpp:loop cpp:decl | asm:lea asm:cmp | topic:codegen-idiom topic:regalloc
symptoms: a parse function is otherwise byte-exact, its frame size matches, and
the ONLY residue is that two address-escaped scalars have each other's `[esp+N]`
displacement - the same semantic role (`&chunkId`) is at one offset in the
prologue and a DIFFERENT offset inside the loop on the retail side, while our
side uses one offset for both
confidence: 9/10
variants: escaped-local-scope-decides-frame-slot-packing.md

## Mechanism

`escaped-local-scope-decides-frame-slot-packing` reads two offsets for one role
as *two variables*, and splitting them is usually right. It is the WRONG move
when the frame is already the right size: cl 5.0 will not overlay an inner-scope
local onto a dead outer one, so splitting adds a slot and grows the frame.

The other reading is the one to try first here: retail has the SAME number of
variables we do, and the loop simply REUSES a different one of the header locals.
The displacement tells you which. Map every `lea`/`cmp` displacement to a slot,
then ask which header word ends up sharing the loop's slot - that word's variable
is the one the loop reuses, and the header word with a slot of its own is the one
that is genuinely single-use.

```cpp
// retail: the RIFF tag is single-use; the FORM tag and the RIFF size are the
// pair the chunk loop reuses.
u32 riffTag;
u32 chunkId;                        // holds the WAVE form tag first
u32 chunkSize;                      // holds the RIFF size first
src->Read(&riffTag, 4, -1);
src->Read(&chunkSize, 4, -1);
src->Read(&chunkId, 4, -1);
if (riffTag != mmioFOURCC('R','I','F','F')) { return 0; }
if (chunkId != mmioFOURCC('W','A','V','E')) { return 0; }
u32 end = src->m_nCurPos + chunkSize - 4;
while (src->m_nCurPos < end) {
    src->Read(&chunkId, 4, -1);     // same slot as the form tag
    src->Read(&chunkSize, 4, -1);   // same slot as the RIFF size
    ...
}
```
```asm
; retail prologue: first header word high, third header word low
lea  eax,[esp+0x1c]        ; riffTag
lea  ecx,[esp+0x24]        ; chunkSize  (recycled `src` argument home)
lea  edx,[esp+0x14]        ; chunkId <- WAVE tag
; retail loop: the SAME two low/recycled slots come back
lea  ecx,[esp+0x14]
lea  edx,[esp+0x24]
```

Steerable. `SoundStream::ParseWave` 0x137b70 99.9682 -> **100.00 EXACT**: our
version reused the FIRST header word's variable for the loop, retail reuses the
third's. Everything else was measured inert first - six declaration orders
(including one-line and `end` hoisted to the top) are byte-identical, and moving
the loop pair into the `while` body's scope grows the frame `0xc` -> `0x10` and
costs 0.14 (99.968 -> 99.824), which is the negative control for the split
reading.
