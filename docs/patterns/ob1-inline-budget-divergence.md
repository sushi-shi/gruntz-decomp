# /Ob1 inline-budget divergence: the same constructor is inlined and called

**Tags:** `topic:wall` `cpp:ctor` `cpp:inline` `asm:call` `topic:eh`

## Symptom

Retail contains both a standalone COMDAT for a small inline constructor and a mix
of callers: some call that body while others expand it. With MSVC 5.0 `/O2`
(`/Ob1`), the result depends on the inliner's accumulated per-call-site budget.
A single source definition can therefore produce both shapes.

The wide WWD object factories are the clearest example. Retail keeps
`CGameObject::CGameObject` at `0x15b390`; some factories expand it, others call it,
and its own body expands constructors that callers sometimes leave out of line.
The declarations are still most consistently explained by one shared inline
definition for each constructor.

## Reconstruction rule

Keep one authentic inline definition in the owning header. Do not vary constructor
visibility per translation unit, provide a second source body, or add an artificial
caller merely to materialize a COMDAT. Those devices can reproduce selected bytes,
but they encode the desired compiler result in source and make the match denominator
depend on invented code.

When a real caller naturally causes the reconstructed compiler to emit the body,
place its `RVA_COMPGEN` binding in that emitting TU. When no real caller emits it,
the retail function remains in `config/retail/functions.tsv` and in historical MAX
evidence, but is unclaimed by the current source build. A deliberate label-count
drop must be acknowledged in `config/labels_manifest.tsv`; it is not repaired with
scaffolding.

This policy removed the former WWD placement switches in August 2026. The rebuild
then showed that `CResolveNode`, `AnimWorkerObj`, and `CAniAdvanceCursor` are emitted
naturally by `LevelPlane.cpp`; their annotations moved there. `WwdDirtyRect`,
`WwdGridNode`, `WwdRegion`, and `CGameObject` currently have no natural standalone
emission and are intentionally unclaimed.

## What remains a wall

Two call sites in one TU can require opposite call/expand choices. MSVC 5.0 has no
supported per-call-site no-inline spelling, and `inline_depth` is depth-based rather
than a faithful control for the accumulated expansion budget. Recover surrounding
source structure and remeasure; do not steer this with visibility changes.

An exhausted budget can also choose a different nested constructor to leave out of
line, potentially emitting a vtable absent from retail. That is evidence of a wider
declaration or caller-shape mismatch, not permission to choose the pruned constructor
with a macro.

## Related

- `docs/patterns/base-trio-in-ctor-body-misplaces-vptr.md`
- `docs/patterns/msvc5-variable-ctor-inline-depth.md`
- `docs/patterns/rezalloc-placement-new-no-eh-frame.md`
