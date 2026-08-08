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

## RESOLVED for `??_7CWapObj` (2026-08-08): it was a missing entity, not a budget wall

The `??_7CWapObj` vtbl-absent row was read as this wall for two units in a row
(`wwdobjmgr`, then `levelplane`, after the wwdobjmgr emission moved). It was neither.
`CDDrawWorkerHost::ReadPlaneObjects` (0x162af0) inlined `CLoadable::CLoadable` and then
declined `CWapObj::CWapObj`, materialising `??0CWapObj@@QAE@XZ` and its vtable, where
retail **calls** `??0CLoadable` (0x156cb0) and never reaches `CWapObj` at all.

`gruntz sema xref 0x00156cb0` lists exactly four retail `call` sites. Reading the
call/expand choice off each construction's vtable stamp turns that into a per-CLASS
split, and the split IS expressible - it is the tagged-sibling recipe of
[two-shapes-need-two-entities.md](two-shapes-need-two-entities.md). Inside
`CDDrawSurfaceMgr::Init` alone retail calls `??0CLoadable` for four children and
expands it for four others, and the eight are eight distinct classes:

| child (by its `??_7` stamp) | retail |
|---|---|
| `CDDrawSubMgrPages` 0x1efe08, `CDDrawChildGroup` 0x1efdc0, `CDDrawWorkerList` 0x1efd88, `CDDrawWorkerMapSmall` 0x1efcc8 | **call** |
| `CDDrawWorkerRegistry` 0x1efd28, `CDDrawWorkerCache` 0x1efd00, `CDDrawSubMgrLeafScan` 0x1efca0, `CDDrawSubMgrLeaf` 0x1efc78 | expand |

Pinning `CLoadable(CDDrawSurfaceMgr*,i32,i32)` out of line in DDrawSubMgr.cpp and
tagging the expansion sites `CLoadable(owner, a, b, CLoadable::NO_SEED)` measured
+6 exact / 91.155 -> 91.178 whole-tree, no denominator change, and **no base obj emits
`??0CWapObj@@QAE@XZ` any more** - `class_vtables` is clean. `CDDrawSurfaceMgr::Init`
91.34 -> 100.00 EXACT, `CreateContainerObject` 85.39 -> 100.00 EXACT, `ReadPlaneObjects`
83.05 -> 86.15, `CAniPlayer::RenderCel` 75.64 -> 98.12, `CSBI_ImageSet::Render`
87.05 -> 98.58, `CSpriteRef::Build` 85.46 -> 96.70, three more rows to 100.00.

So the five disproofs recorded on the old ack row were all aimed at the wrong object:
they perturbed `CWapObj`, which is downstream of the real defect. **Before recording a
budget wall, run the xref on the callee one level UP the chain and check whether retail
splits it by class.** A residual per-site conflict survives - `CWwdGameObject::CreateObject`
(0x166640) expands the same `CWwdGameObjectA::m_animCursor` chain that
`CreateContainerObject`/`ReadPlaneObjects` call, and `CDDrawSurfaceChildA` expands where
its `CDrawSubWorker` sibling's other user `CDDrawSurfacePair` calls - those two are the
genuine budget residue, and they cost 10.50 and ~5 points respectively.

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

## Same shape for a DESTRUCTOR, and the sieve is what surfaces it (2026-08-08)

`gruntz.audit.global_refs --rel32` reads `??1CFileMemBase@@UAE@XZ` **3 times** in
retail's `CDDrawSurfaceMgr::SnapshotChildren` and **11 times** in `RestoreChildren`
against 1 and 6 in ours - and, in the other direction, `??1CFile@@UAE@XZ` 3 vs our 11.
That reads like a dtor-chain modelling error and is not one. Both sides agree on the
chain: `~CFileMem` = vptr `??_7CFileMem` + `Close()` through the vtable slot,
`~CFile` on the `CFile m_file` member at `+0x10`, then `~CFileMemBase` = vptr
`??_7CFileMemBase` + `Close()` + `~CString` on `m_name` at `+0xc`. Only the cut differs:
retail CALLS `~CFileMemBase` at all three cleanup sites in those two big functions, we
expand it, and the extra vptr stamp per site is why our body is 0x620 bytes against
retail's 0x505.

Retail refutes the out-of-line reading itself. `??1CFileMem` at `0x157980` is 116 bytes
and **expands** `~CFileMemBase` inline (`mov [esi],0x5efe68` / `call [0x5efe74]` /
`lea ecx,[esi+0xc]` / `call ??1CString`); a `.cpp`-side definition could only have
produced a call there. So `~CFileMemBase(){ Close(); }` is inline-in-header and the
call/expand split is budget, exactly as for the constructors above.

Measured, so nobody re-runs it: declaring `virtual ~CFileMemBase();` and defining it in
`DDrawSubMgr.cpp` moves `SnapshotChildren` 63.71 -> 72.89 and costs `RestoreChildren`
-5.03, `??1CFileMem` -19.26 and `LoadRecordFile` -12.54 in the same unit. Reverted: the
+9 is the wall paying out at one site while the shape it needs is wrong everywhere else.

## A 4-of-5 control group, which is what makes the verdict cheap (2026-08-08)

`CUserLogic::BuildLogicTypeTable` is expanded into exactly five derived constructors, and
each expansion contains three `CDDrawWorkerCache::Find` sites. Retail calls all three in
`CGuardPoint`, `CLevelTime` (twice - `leveltimedtor` and `statedispatch`) and `CWayPoint`,
and in `CLightFx` alone it EXPANDS the first one, which is why that site reads
`?Lookup@CMapStringToOb@@QBEHPBDAAPAVCObject@@@Z` on the target side and
`?Find@CDDrawWorkerCache@@QAEPAVCObject@@PBD@Z` on ours.

Four identical siblings that already agree settle it in one command: the divergence is one
site's budget, not the visibility of `Find`. Making `Find` an in-class inline to buy back
`CLightFx` would expand it in the other four, which currently match. Before spending a
header change on a single-site row, run
`gruntz.audit.global_refs`-style whole-tree counting for the symbol and look for the
siblings - if most of them agree, there is nothing to model.

Note also that the sieve can only show HALF of this row. It drops a name only one side
mentions, so the `CMapStringToOb::Lookup` half is invisible and the row reads as a bare
"we invented a `Find`". The ordered relocation SEQUENCE is what names the substitution.

## Related

- `docs/patterns/base-trio-in-ctor-body-misplaces-vptr.md`
- `docs/patterns/msvc5-variable-ctor-inline-depth.md`
- `docs/patterns/rezalloc-placement-new-no-eh-frame.md`

## The one-level-up test, applied to every recorded budget wall (2026-08-09)

The rule is: **before recording a budget wall, `sema xref` the callee one level UP and
check whether retail splits it by CLASS.** Ran on all of them. It is not a universal
solvent - it cracked four and left four standing, and the ones it leaves standing have a
shared shape worth naming.

CRACKED (the wall was a missing entity):

| recorded wall | what the xref showed | result |
|---|---|---|
| `??0CLoadable` 0x156cb0 | 4 callers; `CDDrawSurfaceMgr::Init` alone calls it for 4 child classes and expands it for 4 | +7 exact tree-wide |
| `CAniAdvanceCursor` in `CWwdGameObject::CreateObject` 0x166640 | 3 users of ONE `CWwdGameObjectA` ctor; two call, one expands | 76.90 -> 87.40 |
| `??0CStatusBarItem` 0x1005d0 | 4 callers, 38 `new` sites; depth-2 classes 6/6 inline, `CSBI_Image` 10/14 call | BuildStatusBarTabs 71.58 -> 78.21 + 4 more |
| `??0CSBI_RectOnly` 0x101fa0 | the depth-5 chains are 8/8 `call` | previously-unclaimed COMDAT -> 100.00 EXACT |

SURVIVED, and all four for the same reason - **the callee has ONE caller, so there is no
population to find a per-class majority in**:

| recorded wall | xref | why it stands |
|---|---|---|
| `??1CWwdGameObject` 0x15bd10 / `??1CLoadable` 0xd5d70 | 2 callers (`??_GCLoadable` + this dtor); every other derived dtor expands it | a base DESTRUCTOR is invoked implicitly and cannot carry a tag parameter, so the recipe has no spelling |
| `CGruntzMgr::ChangeState` 0x8fab0 / `??1CMoviePlayer` 0x38fc0 | 1 caller, `CCreditsState::ReleaseResources`' `delete vh` | ChangeState EXPANDS it; out-of-lining it measured 74.88 -> 70.54 |
| `SnapshotChildren` / `RestoreChildren` / `LoadRecordFile` / `??1CFileMemBase` | corroborated independently: retail's own `??1CFileMem` expands it | already measured, see above |
| `FontRenderer::DrawWrapped` 0x17a460 / `CRect::Width` 0x17b500 | 1 caller - DrawWrapped itself, 4 sites inside it | the callee is an MFC header inline; a second entity would mean editing MFC |

So the test's discriminator is cheap and mechanical: **count the DISTINCT callers, and if
there is more than one, tabulate the call/expand choice by the constructed class.** One
caller means the split is inside a single function and there is nothing to model. Several
callers with a unanimous or near-unanimous per-class column means an entity is missing.
