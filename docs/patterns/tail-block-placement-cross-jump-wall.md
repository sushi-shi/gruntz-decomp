# Two identical `return` tails cross-jumped: the epilogue moves and the layout follows
tags: cpp:branch cpp:goto | asm:jcc asm:push asm:pop asm:ret | topic:wall topic:regalloc
symptoms: `push esi` at the top of the base where retail shrink-wraps it past the first `ret`; base `je <shared tail>` where retail has `jne <cont>` + an inline `mov eax,1; ret`; a goto-label body inline in the base and far in retail with the SAME predecessor count on both sides; register roles swapped between the two callee-saved registers
confidence: 7/10

Three measured functions show the same shape and none of them has a source lever:
cl5 cross-jumps two source-identical `return` tails, and because the merged tail now
needs the callee-saved `pop`, the matching `push` is forced up to the entry block —
which then re-colours the whole function. Retail made the opposite choice from the
same source. Recognize it and stop; do not restructure the source to chase it.

## The three sites

- `CPlay::AdvanceCursorAnimation` 0x000d0a60 (66.67). Bodies are byte-identical from the fourth
  block on. Retail: `test eax,eax / jne L1 / mov eax,1 / ret 4` — an early `return 1`
  with **no** `pop esi` — then `push esi` shrink-wrapped into L1, after the
  `mov edx,[esp+0x4]` parameter read. Base merges that `return 1` with the function's
  final one, so the merged tail pops `esi`, so `push esi` must dominate the entry
  block, so the parameter reads shift to `[esp+0x8]`.
- `CGrunt::SetFacing` 0x0004ac10 (66.76). Blocks B0..B32 are byte-exact. The `idle:`
  label has exactly two `goto` predecessors on BOTH sides; retail emits it at 0x4af33
  (after all seven strcmp arms), the base inverts the fourth arm's branch
  (`je <skip>` where retail has `jne <idle>`) and lays `idle:` inline as its
  fall-through, which displaces the fifth arm.
- `CGameObject::ResolveLinkedObject` 0x00151b90 (74.71). Retail's lookup-failed arm is
  `mov [esi+0x98],eax` (reusing the known-zero result register) and is therefore a
  DISTINCT block from the outer `m_carrier = NULL` arm, which is
  `mov [esi+0x98],0x0`. The base picks the immediate for both and cross-jumps them,
  which also flips `test eax,eax; jne` to `je`.

## What was tried and did NOT move it

- the retired permuter on 0x151b90 and 0x18e40: 9 and 6 candidates,
  no improvement (these functions have too few atomic mutation sites to search).
- Nesting the following test into the arm's `else` (SetFacing arm K) — byte-identical.
- Reordering the label bodies / moving the far label past the function's natural end
  (`StepBehavior` 0x0005d210, the `combatTimeoutTail` arm): cl produced a byte-identical
  object, because the arm's exit is a BACKWARD jump to the join and cl rotates it back
  inline regardless of where the label sits in source.

## What DOES move it

**BROKEN for the shrink-wrap half (2026-08-08).** Collapsing the body to a SINGLE tail
`return` (guard + one trailing return, nested `if`/`else` in between) is exactly the
predecessor-count change this wall wanted, and it makes cl sink the callee-saved pushes
past the entry guard - see
[`shrink-wrapped-prologue-needs-one-tail-return`](shrink-wrapped-prologue-needs-one-tail-return.md).
`CPlay::AdvanceCursorAnimation` 0xd0a60, listed below as unmovable, went **66.67 -> 100.00 EXACT**
with that one edit.

Only a change in predecessor count - see
[`single-predecessor-tail-block-gets-replicated`](single-predecessor-tail-block-gets-replicated.md).
When the counts already agree with retail, the remaining difference is the cross-jump
decision itself and there is no known source spelling for it.

## Related

- [`negated-condition-far-block`](negated-condition-far-block.md)
- [`allocate-check-then-body-is-the-then-block`](allocate-check-then-body-is-the-then-block.md)
- [`redundant-local-becomes-the-zero-register`](redundant-local-becomes-the-zero-register.md)
