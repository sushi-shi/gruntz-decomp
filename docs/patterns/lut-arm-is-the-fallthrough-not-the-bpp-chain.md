# In a `useLut` / per-bpp renderer, the LUT arm is retail's FALL-THROUGH

tags: cpp:branch | asm:jcc asm:test | topic:codegen-idiom
symptoms: a large per-pixel-format renderer whose first ~17 basic blocks align exactly and
then diverge at the loop body: the recompile's first in-loop `jcc` goes to a block ~25 blocks
away while retail's goes to the very next test, and every arm afterwards is `!!` in the
skeleton diff
confidence: 9/10

A shading/warping inner loop that supports both a colour LUT and a chain of per-`bpp`
specializations is naturally written the "cheap case first" way:

```cpp
// NO - the bpp chain becomes the fall-through and the LUT body is exiled to the far end
if (m_useLut == 0) {
    if (bpp == 1) { ... } else if (bpp == 2) { ... } else if (bpp == 3) { ... }
} else {
    u8* lut = m_table->m_data;
    ...
}
```

Retail's is the positive form, so the LUT body falls through and the `bpp` ladder is the
`jcc` target:

```cpp
// YES
if (m_useLut != 0) {
    u8* lut = m_table->m_data;
    ...
} else {
    if (bpp == 1) { ... } else if (bpp == 2) { ... } else if (bpp == 3) { ... }
}
```

The tell is one instruction: retail ends the loop-body prologue with
`mov edx,[esi+<m_useLut>] ; test edx,edx ; je <bpp-chain>`, i.e. the ZERO case jumps away.
The recompile emits `jne <lut>` instead and the whole arm ordering inverts, which the
`gruntz walls diagnose --asm` skeleton reports as a wall of `!!` rows even though every arm's body is
byte-for-byte right.

A second, smaller lever in the same bodies: the run-start index belongs in the `for` that
uses it, not hoisted above the preceding copy loop.

```cpp
// NO - materializes `t` before the first loop (2 extra instructions per arm)
i32 i = 0;
i32 t = colBase;
if (colBase > 0) { do { ... } while (i < colBase); }
for (; t < stride; t++) { ... }

// YES
i32 i = 0;
if (colBase > 0) { do { ... } while (i < colBase); }
for (i32 t = colBase; t < stride; t++) { ... }
```

STEERABLE. `CFaderShape::RenderWarpTile` 0x181e50: 43.43 -> 58.57 on the arm swap alone
(both of its two sweep-direction copies), 58.57 -> 60.52 with the five `t` folds.

related: [allocate-check-then-body-is-the-then-block.md](allocate-check-then-body-is-the-then-block.md),
[if-body-owns-the-fallthrough.md](if-body-owns-the-fallthrough.md)
