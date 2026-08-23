# One OR expression lets cl reassociate and hoist the loop-invariant pair; accumulate instead

tags: cpp:loop cpp:expression cpp:local | asm:or asm:jmp | topic:codegen-idiom
symptoms: retail recomputes every term of an `a | b | c` inside the innermost loop, our
base computes two of them once in a loop preheader, homes the pair in the frame, and
reloads it per iteration - plus one unconditional `jmp` entering the loop past the
preheader
confidence: 10/10

An `a | b | c` written as ONE expression is a reassociation candidate. When two of the
three terms are invariant in the innermost loop, cl 5.0 re-associates the chain so the
invariant pair is adjacent, hoists it into a preheader it creates for the purpose, and
homes the result. Retail's loop has no preheader because its terms are never adjacent:
it computes all three per iteration, left to right.

Writing the accumulation one term at a time removes the freedom:

```cpp
// hoists (rn>>g_rDown)<<g_rUp | (gn>>g_gDown)<<g_gUp into a b-loop preheader
*out++ = static_cast<u16>( static_cast<u8>(bn >> g_bDown)
                         | (static_cast<u8>(rn >> g_rDown) << g_rUp)
                         | (static_cast<u8>(gn >> g_gDown) << g_gUp) );

// retail: three terms per iteration, no preheader, no entry jump
u16 v = static_cast<u8>(bn >> g_bDown);
v |= static_cast<u16>(static_cast<u8>(rn >> g_rDown) << g_rUp);
v |= static_cast<u16>(static_cast<u8>(gn >> g_gDown) << g_gUp);
*out++ = v;
```

`CShadeTableCache::SubTable` 0x14f310: 218 -> 214 instructions, **7 -> 6** unconditional
jumps against retail's 6, size 0x2ab -> 0x29e against retail's 0x297, **78.72 -> 89.76**
fuzzy (above its 78.79 historical high). Four spellings of the accumulation - a `u16`
seed, an explicit `static_cast<u16>` chain, `v = v | ...`, and a `u16` seeded from the
byte term - are byte-identical, so the plateau is the shape rather than one wording.
The five sibling functions in that TU are byte-unchanged.

## What does NOT work, measured on the same instrument

* **Parenthesising** `((a | b) | c)` - byte-identical to the bare chain. cl reassociates
  through the parentheses; only the statement boundary stops it.
* **Splitting the store** (`*out = ...; out++;`) or **indexing** (`out[b] = ...`) -
  byte-identical / worse.
* **Naming the three terms** as `u8` locals first - 216 instructions and a LOWER score;
  the named u8s change the mask/extend shape.
* Recomputing one invariant inside the loop by hand removes the hoist too, and scores
  slightly better again - but it is a transcription of the effect, not the cause, and
  it is not what a dev writes.

## Reverse reading

`walls loopscan` reporting `ours FATTER` on the preheader-carrying loop with the
matching `retail+movxN` inside it, and a `jmp` count one higher than retail, is this
pattern. If the invariant pair is NOT loop-invariant, the extra jump is allocation
instead - see
[loop-entry-trampoline-is-a-readout-not-a-shape.md](loop-entry-trampoline-is-a-readout-not-a-shape.md).

related: [wall-reasons-globalopt.md](../relevations/wall-reasons-globalopt.md) §6
