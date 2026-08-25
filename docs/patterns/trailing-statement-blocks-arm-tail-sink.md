# cl sinks identical TRAILING STATEMENTS out of sibling arms — a differing last statement blocks it
tags: cpp:branch cpp:if cpp:local cpp:return | asm:jmp asm:call | topic:codegen-idiom topic:tail-merge
symptoms: retail emits two if/else arms in FULL (equal-length blocks, ~20i each) where the base leaves 4i stubs and a fat join block; `gruntz walls diagnose --asm` shows base `Ni [jmp Bj]` / `Ni [fall Bj]` with a join much LARGER than the target's, and the arm sizes short by the same amount the join is long; total instruction count matches, only the placement differs
confidence: 9/10
variants: identical-arms-need-distinct-locals.md, statement-order-decides-the-tail-merge.md, single-predecessor-tail-block-gets-replicated.md

cl 5.0 has **two independent** exit/tail-merging mechanisms, and confusing them
wastes the whole investigation:

1. **Statement-list sinking (early, IL level).** Identical *trailing statements*
   of sibling `if`/`else` arms are sunk into the join block. The unit of
   comparison is the **IL statement list**, not machine code — see
   [identical-arms-need-distinct-locals](identical-arms-need-distinct-locals.md).
2. **Machine-level cross-jump (late, layout).** Identical instruction *suffixes*
   are cross-jumped — this is what the `goto fail` / `||` regimes steer, see
   [goto-fail-shares-one-exit-block](goto-fail-shares-one-exit-block.md).

**This file is about (1), and the lever is the LAST statement of each arm.**

## The knob

If the arms' last statements are the same statement, cl sinks; make the last
statement differ and cl emits both arms in full. Moving an existing arm-only
statement from the head of an arm to its tail is enough — it does not have to be
a new statement.

```cpp
// NO - both arms end `...Setup(m_poseWalk);`, so cl sinks the whole run into the
//      join: arms 4i/4i, join 27i  (retail: 22i/22i, join 10i)
if (!(targetCellFlags & 0x20000000)) {
    m_previousAnimationActId = m_logicRecord->m_eventCode;
    m_logicRecord->m_eventCode = ActFindId(s_codeD);
    m_value            = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->m_animCursor.Setup(m_poseWalk);
} else {
    if (!(currentCellFlags & 0x80)) { return 0; }
    m_entranceActive = 1;                       // arm-only, at the HEAD
    m_previousAnimationActId = m_logicRecord->m_eventCode;
    /* ...identical... */
    m_wwdObject->m_animCursor.Setup(m_poseWalk);
}
GruntDirectionCell cell = m_entranceCell;

// YES - the arm-only store moves to the TAIL, so the last statements differ and
//       nothing sinks; then the join's own head statement is written into both
//       arms, which is where retail has it too
GruntDirectionCell cell;
if (!(targetCellFlags & 0x20000000)) {
    /* ...the four shared statements... */
    cell = m_entranceCell;
} else {
    if (!(currentCellFlags & 0x80)) { return 0; }
    /* ...the four shared statements... */
    cell = m_entranceCell;
    m_entranceActive = 1;                       // arm-only, at the TAIL
}
```

## Micro-study (standalone TU, `cl /nologo /c /O2 /MT /GX /GR`)

Four cells, identical bodies, differing only in arm shape. Smaller = cl shared.

| cell | arm shape | insns | verdict |
|---|---|---|---|
| `Join` | arms fall into a join, tails identical | 43 | **sank** |
| `Ret`  | arms each `return`, tails identical | 43 | **sank** |
| `JoinLocals` | each arm names its own block-scope local | 49 | duplicated (partially) |
| `JoinExtraStmt` | one arm has one EXTRA trailing statement | 57 | **duplicated in full** |

Two things this settles:

- **`Join` == `Ret` (43 == 43).** Whether the arms end the function is
  **irrelevant** — the sink is not a return-epilogue phenomenon, so do not
  reach for the `goto fail` / `||` levers here; they steer mechanism (2).
- **A differing last statement is stronger than a differing local.** The
  block-scope local of
  [identical-arms-need-distinct-locals](identical-arms-need-distinct-locals.md)
  only partially blocks the sink when the local is a pure alias (cl CSEs it back);
  a genuinely different trailing statement blocks it completely.

## Retail proof that this is NOT machine-level cross-jumping

`CGrunt::StepEntranceReinit` 0x637a0, retail blocks B34 and B40 (22i each) share a
**13-instruction byte-identical suffix** and retail does **not** cross-jump it —
their first ~8 instructions differ only by register colouring (one arm already has
`m_logicRecord` in `eax`). So identical machine tails alone do not trigger merging; the
statement list is what mechanism (1) reads.

Measured 2026-08-08 on `CGrunt::StepEntranceReinit` 0x637a0:
79.56 -> 83.37 (moving `m_entranceActive = 1` to the arm tail; arms 4i -> 16i)
-> 87.60 (writing `cell = m_entranceCell` into both arms; **join B41 exact at 10i**)
-> 90.18 (folding the two hand-copied bounds-checked cell reads onto the real
`CMapMgr::CellFlagsAt` inline, which spells the guard `<`-positive, not `>=`/`||`).

## Screening

`gruntz walls diagnose <rva> --asm` and look for arm blocks SHORTER
than the target's with a join LONGER by the same total. The exit-merge sieve does
**not** catch this family — ret counts are equal, because nothing about the
function's exits changed.

related: [identical-arms-need-distinct-locals.md](identical-arms-need-distinct-locals.md),
[statement-order-decides-the-tail-merge.md](statement-order-decides-the-tail-merge.md),
[goto-fail-shares-one-exit-block.md](goto-fail-shares-one-exit-block.md) (mechanism 2),
[if-else-both-arms-return-is-the-or-regime.md](if-else-both-arms-return-is-the-or-regime.md)
