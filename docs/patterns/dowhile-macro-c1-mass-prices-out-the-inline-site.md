# A do{...}while(0) macro wrapper's C1-only mass prices the callee out of its /Ob1 site

tags: cpp:inline cpp:dtor cpp:macro | asm:call | topic:codegen-idiom topic:wall
symptoms: diagnose says INLINE/CALL-SET; retail EXPANDS an inline-visible body at
a site where our build CALLS it (and, budget freed, our build then inlines a
LATER cheap callee retail calls); the callee's own standalone COMDAT is
byte-exact both ways; the callee's source wraps statement groups in a
do{...}while(0) hygiene macro
confidence: 9/10 (??1CWwdGameObject 0x15bd10, 71.54 -> 95.27 on the one deletion)

## Mechanism

cl 5.0's /Ob1 budget is spent against the callee's PRE-OPTIMIZATION (C1) form
(inline-callee-frontend-cost-drives-ob1-budget.md). A `do { ... } while (0)`
wrapper contributes loop/condition nodes that /O2 deletes unconditionally - so
it is INVISIBLE in the callee's own bytes, but each wrapped group inflates the
C1 cost every inline SITE must afford. Four wrapped frees were enough to flip
`CGameObject::Unload` (the WORKER_FREE macro) from expand to call inside
`~CWwdGameObject`'s base-dtor slot; with the budget then unspent, cl spent it
inlining `??1CWapObj`'s three stores that retail CALLS - one macro produced BOTH
inversions the wall documented.

## Reconstruction rule

When a reconstruction macro exists purely as shorthand (ours introduced
WORKER_FREE; nothing proves retail had it), write the statements plainly:

```cpp
if (m_animWorker) { delete m_animWorker; m_animWorker = 0; }   // not WORKER_FREE(p)
```

The standalone body's bytes cannot adjudicate the spelling - only the CALLERS'
expand/call decisions can, and they vote for the leaner C1 form.

## Bound observed

The lever moves one budget step. The next nested step (retail also expands
`~CResolveNode` then calls `??1CWapObj`) stayed out of reach: inlining the
member `Reset()` stores by hand into the dtor did not flip it and perturbed an
exact sibling dtor, and the member's user-declared empty `~WwdDirtyRect() {}`
is pinned to a real retail body (0x15b290), so its site mass is authentic.
