# A `goto` whose label is just the CODE THAT FOLLOWS is a transcription bug, not a lever
tags: cpp:branch cpp:goto cpp:return | asm:jcc asm:jmp asm:ret | topic:codegen-idiom
symptoms: DUP-EXIT, base has MORE ret blocks than target, base's shared block sits mid-function where retail's sits after the whole if/else, `goto dispatch`/`goto L_xxx` label immediately follows the if/else it is jumped out of, block skeleton aligns for the first N blocks then permutes, the if-BODY is our fall-through but retail's `jcc` target
confidence: 9/10

Two different source shapes both "jump past the guarded arms to the common tail":

```cpp
// (a) a real shared block - the label is a DESTINATION, reached only by goto
if (a) goto fail;
if (b) goto fail;
...
fail: return 0;

// (b) NOT a shared block - the label is simply THE NEXT STATEMENT
if (rem > win) {
    if (!fired)  goto dispatch;      // <-- these two are not "merging" anything
    ...
} else {
    if (fired)   goto dispatch;
    if (damage)  goto dispatch;
    ...
}
dispatch:                            // <-- falls here anyway when the else ends
    Advance(); ...
```

Only (a) is the [`goto fail`](goto-fail-shares-one-exit-block.md) lever. In (b) the goto
is redundant: control reaches the label by falling out of the `if/else` too. **cl treats
it as a real jump target and HOISTS the labelled block to the last goto's position**, so
the whole second half of the function is laid out in the wrong order and every surplus
`ret` after it is a knock-on. Deleting the gotos - turning each guard into a positive
gate and letting the arm fall through - restores retail's order exactly.

```cpp
// YES - no label, no goto; the guards are positive gates and the tail is the fall-through
if (rem > win) {
    if (fired) {                     // was: if (!fired) goto dispatch;
        if (damage == 0) { ...; return 0; }
        ...; return 0;
    }
} else if (!fired && damage == 0) {  // was: two `goto dispatch` guards
    ...; return 0;
}
Advance(); ...                       // was: dispatch:
```

`CStaticHazard::LoadAttributes` 0xfc1a0 is the worked example: **52.65 -> 98.91** in three
steps, of which this was the largest (70.40 -> 98.10, and it is what made blocks B0-B13
align). The other two steps are the companions below.

## Companion 1: read WHICH SIDE of the guard retail falls through to

Same function, first step (52.65 -> 58.82): we had
`if (m_object->m_damage != 0) { IDLE... } GO...` and retail has `jne` **away** to the
IDLE arm, i.e. the source is `if (m_object->m_damage == 0) { GO...; return 0; } IDLE...`.
Swapping which arm is the if-BODY is free and it re-aligns every block after it.

The same inversion moved two more: `CGrunt::StepGooSuckerBehavior` 0xf0e20 (46.38 ->
47.85) and `CGrunt::StepBrickLayerBehavior` 0xecc90 (57.18 -> 58.92), where retail
out-lines `{ m_neighborValid = 0; return 1; }` as the `jcc` target and falls through into
the long arm - i.e. the source nests the long arm inside `if (m_neighborValid == 0)` and
puts the short one after it, not the other way round.

**Read it off `--blocks --diff --lite`:** when base says `Bn 3i [jcc Bshort | fall Blong]`
and target says `Bn 3i [jcc Blong | fall Bshort]`, the arms are swapped in the source.

## Companion 2: `&&` short-circuits a load retail issues unconditionally

Also `LoadAttributes` (58.82 -> 70.40). Retail loads BOTH tile coordinates before the
first bounds compare:

```
mov eax,ds:g_gameReg / mov ecx,[esi+0x68] / mov esi,[esi+0x64] / mov eax,[eax+0x70]
cmp esi,[eax+0xc] / jae exit / cmp ecx,[eax+0x10] / jae exit
```

Written as `if ((u32)m_tileCol < (u32)g->m_width && (u32)m_tileRow < (u32)g->m_height)`
the second member load sinks below the first `jae` (that is what `&&` MEANS), the `this`
register stays live, and the block can no longer share a tail with the identical guard
elsewhere in the function. Hoisting `i32 row = m_tileRow; i32 col = m_tileCol;` above the
`if` reproduces retail at all four sites. Retail does this at *every* site of the
bounds-checked tile write, which is the signature of an inlined `CMapMgr` member taking
`(x, y)` by value.

related: [goto-fail-shares-one-exit-block.md](goto-fail-shares-one-exit-block.md) (the (a) case - a REAL shared block), [dup-exit-means-a-shared-goto-label.md](dup-exit-means-a-shared-goto-label.md), [one-shared-return-tail-is-a-positive-gate-nest.md](one-shared-return-tail-is-a-positive-gate-nest.md)
