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

## Retail itself makes opposite choices at two sites in ONE TU (2026-08-07)

`gruntz sema xref 0x0015b390` settles the visibility question: retail's
`CreateContainerObject` (`0x1598d0`, WwdObjMgr.cpp) **calls** `CGameObject::CGameObject`
while `CreateDotObject` (`0x159250`, same .cpp, 0x680 bytes earlier) **expands** it -
two stores of `??_7CResolveNode`/`??_7CGameObject` inline plus the nested
`new AnimWorkerObj`. One TU, one constructor, both shapes. So the constructor is
inline-in-header (a `.cpp`-out-of-line definition could produce only the call form),
and the call/expand choice is pure accumulated budget.

The cut point is the *innermost* level reached, and it is not steerable from source:

| site | retail cuts after | ours cuts after |
|---|---|---|
| `CreateDotObject` / `CreateDeferredObject` | CGameObject (level 2); calls `??0CResolveNode`, `WwdRegion`, `WwdDirtyRect`, `CString`, `??0AnimWorkerObj` | CLoadable (level 4); calls `??0CWapObj` |
| `CreateSpriteObject` | CGameObject; calls `FUN_0055b2b0` (`WwdGridNode`) | inlines the whole chain |
| `??0CGameObject` standalone `0x15b390` | nothing - expands CResolveNode, CLoadable, CWapObj, AnimWorkerObj | same |

Because our cut lands one level deeper, cl materializes `??0CWapObj@@QAE@XZ` and with
it `??_7CWapObj@@6B@`, which retail never emits - the `class_vtables` vtbl-absent
violation. The chain, the field-store sets and even the EH state table (5 states = the
5 destructible sub-objects) are byte-for-byte the same on both sides; only the cut
differs. Disproved as causes, each by a rebuild of `wwdobjmgr.obj`:

- making `CWapObj`'s default ctor compiler-generated (removing both user ctors);
- moving the three member stores into a `CWapObj(CDDrawSurfaceMgr*,i32,i32)` so the
  declined body has a different size (cl then emits *that* ctor instead);
- deleting the TU's unused placement `operator new`;
- deleting the fabricated `char _p18d[]` tail padding of `CWwdGameObjectC`;
- `#pragma inline_depth(2)` (no effect at all; `(1)` works, so the pragma is live).

`inline_depth(3)` would put the cut on `??0CLoadable` - the symbol retail's obj
actually references - but that is exactly the banned per-TU device. Treat the
vtbl-absent row on `wwdobjmgr` as this wall's readout, not as a hierarchy bug.

## Related

- `docs/patterns/base-trio-in-ctor-body-misplaces-vptr.md`
- `docs/patterns/msvc5-variable-ctor-inline-depth.md`
- `docs/patterns/rezalloc-placement-new-no-eh-frame.md`
