# vtable-conversion log

Tracks the "make all manual vtable stamps real C++ virtuals" mandate. A manual
`*(void**)this = &g_*Vtbl` / `m_vtbl = &g_*Vtbl` stamp (+ placeholder
`struct{virtual void v0();}`) is usually a decomp hack: the devs wrote a normal
C++ class with `virtual` methods and the compiler emitted the vptr store. Where
that is true and converting is **matching-neutral**, we convert (let `cl`
auto-emit `??_7`). Where converting **regresses** or the stamp is **foreign**
(a vtable owned/defined by another class), we keep the hand-roll and record why.

Empirical decision rule per stamp:

- **CONVERTED** — modeled the class real-polymorphic, removed the stamp +
  `g_*Vtbl` extern, `gruntz build` stayed neutral (no fn's fuzzy/exact dropped).
- **KEPT-hand-rolled** — the class's OWN vtable, but converting regresses
  (vptr-last, unmatched-virtuals → divergent `??_7`, or the stamp lives in a
  non-ctor helper `cl` cannot auto-emit). Logic stays; revisit in the final
  sweep when the blocker lifts.
- **KEPT-foreign** — `g_*Vtbl` is a vtable DEFINED in another/unmatched TU that
  this class merely stamps into an object it constructs (or a shared base
  subobject vtable). Converting would emit a divergent/duplicate `??_7`.

`??_7...@@6B@` sizes/names below are from `config/vtable_names.csv` where known.

## Batch 1 (dedicated-header classes) — 2026-07-01

Net effect on this batch: **0 neutral conversions** (all stamps are foreign,
vptr-last, or not compiler-auto-emittable). exact/fuzzy delta: **0** (the one
tested conversion regressed and was reverted). One empirical test (CContainerErr)
confirms the vptr-last rule; the rest are decided structurally (foreign / a
non-ctor install site / already-real).

| Class / stamp | vtable RVA (VA) | Outcome | %-effect | Reason |
| :-- | :-- | :-- | :-- | :-- |
| `CContainerErr` / `g_containerErrVtbl` | (undef extern, VA 0x?) | KEPT-hand-rolled (vptr-last) | tested: ctor 100%→non-exact (gametext 3/4→2/4, −1 overall) → reverted | OWN vtable, real ctor. Retail stores `m_msg`(+0x04) FIRST then vptr(+0x00) LAST; a `virtual` decl forces MSVC's implicit vptr-first store at ctor entry. **Empirically confirmed + reverted.** |
| `CRemusNode` / `g_remusNodeVtbl` | 0x1efbc0 (0x5efbc0) | KEPT-hand-rolled | none (not attempted) | OWN vtable, real ctors, but the node's virtuals are UNMATCHED (live in other TUs; PMF dispatch uses slot 9 @+0x24, total slot count unknown) → a polymorphic model emits a divergent `??_7`. Ctors are already `@early-stop` on the sentinel-seed store-schedule wall; a compiler-implicit vptr-first store cannot beat the walled hand-placed store. Revisit when the node virtuals are matched. |
| `CRemusNode` / `g_remusBaseDtorVtbl` | 0x1e8cb4 (0x5e8cb4) | KEPT-foreign | — | `??_7CObject@@6B@` (MFC base). Restamped at `~CRemusNode` exit = base-subobject teardown; not this class's vtable. |
| `CZDArrayDerived` / `g_zDArrayVtbl` | 0x1e70fc (0x5e70fc) | KEPT-hand-rolled | none (not attemptable) | OWN vtable (`??_7?$zDArray@P8CUserLogic@@AEHXZ@@6B@`, 1 slot), 100% now. The vptr install lives in the NON-ctor helper `CZDArrayDerived::Construct(int,int)` (return-`this` in-place-construction idiom); `cl` only auto-emits the implicit vptr store inside a real `??0` ctor, so the manual stamp is the faithful shape. Same idiom as `CMenuItem::Construct`. |
| `CWorldSoundSet` / `g_ambientSoundVtbl` | 0x1e710c (0x5e710c) | KEPT-foreign | — | `??_7CAmbientSound@@6B@`. The sound object CWorldSoundSet RezAllocs + placement-constructs; class defined in another TU. |
| `CWorldSoundSet` / `g_posSoundVtbl` | 0x1e7124 (0x5e7124) | KEPT-foreign | — | `??_7CAmbientPosSound@@6B@`, ditto. |
| `CWorldSoundSet` / `g_randomSoundVtbl` | 0x1e713c (0x5e713c) | KEPT-foreign | — | `??_7CRandomAmbientSound@@6B@`, ditto. |
| `MenuPage` / `g_menuItemVtbl` | 0x1f08c0 (0x5f08c0) | KEPT-foreign | — | `CMenuItem` (child item) vtable; its real class/virtuals live in the 0x184670+ TU. MenuPage only reproduces the inlined `CMenuItem::Construct` leaf helper. |
| `MenuPage` / `g_menuItem2Vtbl` | 0x1f08f8 (0x5f08f8) | KEPT-foreign | — | second `CMenuItem` variant vtable, same TU-foreign. |
| `CGruntzSoundZ` / `g_innerSoundVtbl` | 0x1ef700 (0x5ef700) | KEPT-foreign | — | Inner sound object's vtable (another TU). `CGruntzSoundZ` itself is ALREADY real-polymorphic (derives from MFC `CMapStringToOb`, reuses its vtable; the `~dtor` `@early-stop` notes the polymorphic base is required). |
| `CWwdObjMgrFactories` / 6 wwd vtables | 0x1f0020,0x1effd0,0x1f0060,0x1f0128,0x1f00a8,0x1f00e8 | KEPT-foreign | — | Intermediate/final vtables of the wide game objects each factory RezAllocs + placement-constructs; object classes live in other TUs. (Unit also `@early-stop` on the rezalloc-placement-new-no-EH-frame wall.) |
| `CAniAdvanceCursor` / `g_wwdSubVtbl` | 0x1f0128 (0x5f0128) | KEPT-foreign (shared) | — | SHARED WWD sub-object vtable (also stamped by WwdFile's +0x1A0 sub-object; DATA pinned in WwdFile.cpp). A per-class polymorphic model would emit a distinct `??_7CAniAdvanceCursor` ≠ the shared 0x5f0128 (divergent). Stamp is vptr-not-at-entry anyway. 100% now. |
| `WwdFile` / `g_wwdObjVtbl`,`g_wwdSubVtbl`,`g_planeRenderVtbl` | 0x1f00a8,0x1f0128,0x1f02a8 | KEPT-foreign | — | Vtables of plane/sub/worker objects WwdFile's ReadPlaneObjects/RebuildPlanes construct; classes in other TUs. (Unit `@early-stop` on the throwing-new EH-frame wall.) |
| `WwdFile` / `g_severusWorkerDtorVtbl` | 0x1e8cb4 (0x5e8cb4) | KEPT-foreign | — | `??_7CObject@@6B@` (MFC base) restamped in the worker teardown. |
| `Image` (`CImageSurfaceItemInit`) / `g_fileImageVtbl` | 0x1ef7f0 (0x5ef7f0) | KEPT-foreign | — | The remaining stamp is for a foreign inline-built 0xc0 surface item the factory files into the image cache (class elsewhere). `CFileImage`/`CFileImageSurface` are ALREADY real-polymorphic (virtual dtor; compiler-implicit stamp-first) — the shared surface vtable reloc-masks. |
| `ShadeTableCache` (`CShadeTableArray`) / `g_shadeArrayVtbl` | 0x1efb28 (0x5efb28) | KEPT-foreign | — | Element-array base's vtable (a `CObArray`-like base in another TU); the embedded subobject stamps it during ctor. |
| `ShadeTableCache` (`CShadeTableArray`) / `g_remusBaseDtorVtbl` | 0x1e8cb4 (0x5e8cb4) | KEPT-foreign | — | `??_7CObject@@6B@` re-stamped during the standard MFC array-base dtor teardown. |

### Rules learned / reinforced

- **vptr-last is a hard wall for real-`virtual` conversion.** If retail stores a
  member (e.g. `m_msg`) before the vptr, MSVC's implicit vptr-first store at ctor
  entry cannot reproduce it — keep the explicit `void* m_vtbl` member + manual
  stamp. (CContainerErr, empirically.) The vptr-last MIGHT be an /O2 reordering
  reproducible with the right member-init shape — a later test, not now.
- **A vptr install inside a non-ctor `Construct`/`Init` helper is NOT
  compiler-auto-emittable** — `cl` only emits the implicit vptr store inside a
  real `??0` ctor. The manual stamp is the faithful shape. (CZDArrayDerived,
  CMenuItem::Construct.)
- **Foreign / shared-base vtables must stay hand-rolled** — converting would emit
  a divergent or duplicate `??_7`. Most "class-owns-a-vtable" stamps in factory
  / manager TUs are actually the built sub-object's vtable, not the manager's.
- **`0x5e8cb4` = `??_7CObject@@6B@`** (the recurring "remus/severus base dtor
  vtable"); always foreign.

## Batch 2 (CDDraw / engine WORKER family) — 2026-07-01

Net effect: **1 new neutral conversion (aniElem, fully realized)**, 1 already-real
(albus, prior), 6 KEPT (all inline-manual vptr-middle / two-vptr / CObArray-base).
exact/fuzzy delta: **0** (`gruntz build` "no regressions vs baseline"). The key new
finding: a worker vtable realizes **only when the object is built via a real
`new Class` with a vptr-FIRST ctor** — the whole family is otherwise built by INLINE
manual `operator new + field init + *(void**)w=&g_*Vtbl` in a factory, where the vptr
stamp lands vptr-MIDDLE and cl's implicit vptr-first ctor cannot reproduce it.

| Class / stamp | vtable RVA (VA) | Outcome | %-effect | Reason |
| :-- | :-- | :-- | :-- | :-- |
| `CAniElementObj` / `g_aniElemVtbl` | 0x1efba8 (0x5efba8) | **CONVERTED (realized)** | 0 (cddrawsubmgrani 1/3, factories 96.87% unchanged) | Built via `new CAniElementObj` with a vptr-FIRST ctor. Modeled `CAniElementBase` (grand-base, 5 CObject-interface slots, virtual dtor) + `CAniElementObj` (overrides dtor slot) real-polymorphic; **cl auto-emits `??_7CAniElementBase@@6B@` (masks 0x5e8cb4) + `??_7CAniElementObj@@6B@` (masks 0x5efba8, paired via `vtable_names.csv` 0x1efba8/0x14)** — confirmed emitted `sec 4/5` + base-then-derived `DIR32 ??_7…` stamps in the objdiff base obj. Removed 2 manual `*(void**)this=` stamps, the `g_aniElemVtbl`+`g_remusBaseDtorVtbl` externs, and the `CAniElemView` placeholder; failure path is now `delete el` (virtual scalar-deleting dtor = same `mov eax,[el]; call [eax+4]`). Synthesized `??_G/??_E/??1` thunks are emitted but **unpaired (no `RVA()`) → not counted** → neutral. |
| `CAniRecordBase2` / `g_albusWorkerVtbl` | 0x1f02d8 (0x5f02d8) | **ALREADY-real (prior)** | — | `CAniRecord.cpp` already models the albus worker as real-polymorphic `CAniRecordBase2` (14 slots, `~CAniRecordBase2` @0x165dd0 = 100%). No action. `CDDrawWorkerMapSmall`'s `AlbusWorkerObj` foreign-stamps a *distinct* raw block it constructs inline — that stamp stays. |
| `CShadeTableArray` / `g_shadeArrayVtbl` | 0x1efb28 (0x5efb28) | **KEPT-foreign** (final-sweep candidate) | — | CObArray-like element-array base; vptr at subobject +0 with a vptr-FIRST member store (`m_vtbl=&g_shadeArrayVtbl`) — structurally realizable like aniElem — BUT its inline ctor/dtor fold into `CShadeTableCache` ctor/dtor (0x14de30/0x14de50), the cache is 1.9%, and the CObArray base semantics risk a divergent `??_7`. Revisit in the final sweep once the cache ctor/dtor are reconstructed. |
| `CSiriusWorker` / `g_siriusWorkerVtbl` | 0x1efb80 (0x5efb80) | **KEPT-hand-rolled (vptr-middle)** | — | Built by INLINE manual construction in `CGameObject::EnsureWorker80/88/90` (UserBaseLink.cpp): vptr stamp lands AFTER m_04/m_08/m_0c → vptr-middle. Those factories are already `@early-stop` on the zero-register-pinning wall. Also member-stored (`m_vptr=`) in SiriusWorkerHandlers/CDDrawWorkerCache. Worker virtuals unmatched. |
| `HagridWorkerA` / `g_hagridWorkerVtblA` | 0x1efea0 (0x5efea0) | **KEPT-hand-rolled (vptr-middle)** | — | `MakeWorkerA` (CDDrawWorkerList.cpp) inline construction stamps vptr AFTER field inits. |
| `HagridWorkerB` / `g_hagridWorkerVtblB` | 0x1efed0 (0x5efed0) | **KEPT-hand-rolled (vptr-middle)** | — | `MakeWorkerB`, ditto. |
| `SeverusWorkerObj` / `g_severusWorkerVtbl` | 0x1efbe8 (0x5efbe8) | **KEPT-hand-rolled (inline two-vptr)** | — | `MakeSeverusWorker` (CDDrawWorkerRegistry.cpp) stamps the BASE vtbl first, then (after m_04/m_08/m_0c + CByteArray ctor) the DERIVED vtbl (vptr-middle for derived), then m_64/m_68. Worker virtuals unmatched. |
| `SeverusWorkerBase` / `g_severusWorkerBaseVtbl` | 0x1efc30 (0x5efc30) | **KEPT-hand-rolled/foreign** | — | Base subobject vtbl stamped first in `MakeSeverusWorker`; also in `GameLevel.h`'s inline ctor + `CSeverusWorkerHost`. Same inline two-vptr wall; base virtuals unmatched. |
| `LeafElementObj` / `g_leafElemVtbl` | 0x1eff08 (0x5eff08) | **KEPT-hand-rolled (vptr-middle factory)** | — | `~LeafElementObj` (0x158680) IS reconstructed with a vptr-first stamp, but `CreateEntry_157d70` (0x157d70, 99.81%) constructs the element by INLINE manual raw-alloc with a vptr-MIDDLE stamp. Making the class polymorphic forces vptr-first in the factory too → regresses. A class can't be split (real dtor + manual factory), so it stays. |

### Rules learned / reinforced (Batch 2)

- **Realization is neutral iff the object is built via `new Class` with a vptr-FIRST
  ctor** (aniElem). Then cl auto-emits the `??_7` (base + derived) with byte-identical
  reloc-masked stamps, AND — mapped in `vtable_names.csv` — the emitted vtable data
  symbol pairs with the delinked target. The compiler-synthesized `??_G/??_E/??1` dtor
  thunks are emitted but carry **no `RVA()`** → they aren't paired/counted → the unit's
  function % is unchanged.
- **INLINE-manual construction (`operator new` + field init + `*(void**)w=&g_*Vtbl`)
  with a vptr-MIDDLE stamp is a hard wall** — the factory cannot reference the compiler
  `??_7` without switching to `new`, and `new` forces the implicit vptr-first store,
  which regresses the vptr-middle factory. This is the WHOLE worker family
  (sirius/hagrid/severus/leaf) — the manual stamp is the faithful shape.
- **Two-vptr inline construction (base-first then derived-middle) is a wall** for the
  same reason (severus).
- **A vptr-first embedded-subobject member store (shade) is structurally realizable
  like aniElem**, but a CObArray-like base folded into a barely-reconstructed cache is
  deferred to the final sweep.

## Batch 3 (image / wwd / fileMem families) — 2026-07-01

Net effect: **0 neutral conversions** — every eligible-looking stamp in these
SETTLED modules fails the Batch-2 decisive rule (realize NEUTRALLY only when the
object is built via a real `new Class` with a compiler vptr-FIRST ctor **and** the
whole retail vtable can be reproduced). All are KEPT (documented wall), SKIPPED
(owner in an off-limits file), or ALREADY-REALIZED. exact/fuzzy delta: **0** (no
source change; `gruntz build` 1873/3394 = 55.2% exact / 64.05% fuzzy, "no
regressions vs baseline"). No UnknownVTables struct deleted (catalog stays 74).

Key finding: two candidates *pass the `new Class`/vptr-first shape* yet still cannot
realize because the vtable is a **SHARED base vtable** — a per-class `??_7` would
diverge from the one shared retail vtable (0x5ef7f0 is CPoolItemA's base, stamped by
CFileImage/CImageSurfaceItem/CPoolItemA/CDDSurface alike). The `new Class` test is
necessary but **not sufficient**: the vtable must also be *class-private*, and its
slots reproducible.

| Class / stamp | vtable RVA (VA) | Outcome | Reason |
| :-- | :-- | :-- | :-- |
| `CImageSurfaceItemInit` / `g_fileImageVtbl` (Image.cpp) | 0x1ef7f0 (0x5ef7f0) | KEPT-foreign (shared base) | Built via `new CImageSurfaceItemInit` — a **real `new` with a vptr-FIRST ctor** (passes the rule's shape) — BUT 0x5ef7f0 is a **shared CPoolItemA base vtable** (also stamped by `CFileImage::~CFileImage`, `CPoolItemA` in CDDrawPtrCollections.cpp, and CDDSurfaceDtor.cpp; not in `vtable_names.csv`). A per-class `??_7CImageSurfaceItem` would diverge from the shared 9-slot retail vtable (8/9 slots are unmatched `sub_`/no-body). `CFileImage` itself is ALREADY real-polymorphic (`virtual ~CFileImage`, cl-implicit stamp reloc-masks). The `Build_13e9a0` factory is also `@early-stop` on the rezalloc/throwing-new EH-frame wall. Realization owner = `CPoolItemA` (off-limits CDDraw* file). |
| `CFileMemBase` / `g_fileMemBaseVtbl` (FileMem.cpp) | 0x1efe68 (0x5efe68) | KEPT-hand-rolled (unmatched-vtable-contents) | `m_vtbl` @+0x00 is stored vptr-FIRST in `CFileMemBase::CFileMemBase` (structurally realizable), but the base vtable's slots (`scalar_deleting_destructor` 0x157960, `slot2` 0x157910) are **UNMATCHED** in-TU → cl would emit a divergent `??_7CFileMemBase`. |
| `CFileMem` / `g_fileMemVtbl` (FileMem.cpp) | 0x1efe30 (0x5efe30) | KEPT-hand-rolled (unmatched-vtable-contents) | Same class family. Derived vtable (0x5efe30, 3 slots) + the CFile-interface vtable (0x5efe3c, 10 slots) reference mostly-UNMATCHED methods (scalar-dtors 0x157a20, `slot2` 0x157a70, the CFile slots). `~CFileMem` is already `@early-stop` on the manual-vtable EH-dtor wall. Revisit when the vtable-slot methods (the `sub_`/scalar-dtor thunks) are matched. |
| `WwdGameObj`(+sub) / `g_wwdObjVtbl` (WwdFile.cpp ReadPlaneObjects) | 0x1f00a8 (0x5f00a8) | KEPT-foreign (inline re-stamp) | `operator new(0x1dc)` + base `Construct` + **re-stamp** the derived vtable (base ctor leaves the base vtable, ReadPlaneObjects promotes) → vptr not at ctor entry, not cl-implicit-first. Foreign `WwdGameObj` (class in another TU). Unit `@early-stop` on the throwing-new EH-frame wall. |
| `WwdObjAnimInit` / `g_wwdSubVtbl` (WwdFile.cpp +0x1A0 sub-object) | 0x1f0128 (0x5f0128) | KEPT-foreign (shared, inline re-stamp) | Inline sub-object `Construct` + re-stamp (vptr-middle); **SHARED** with CAniAdvanceCursor (a per-class `??_7` would diverge). |
| `CAniAdvanceCursor` / `g_wwdSubVtbl` (CAniAdvanceCursor.cpp ctor) | 0x1f0128 (0x5f0128) | KEPT-hand-rolled (vptr-middle, shared) | Ctor stores m_4/m_8/m_c **then** `*(void**)this=&g_wwdSubVtbl` (vptr-MIDDLE, arg-store-order steered) → cl's implicit vptr-first would reorder + regress; also the SHARED sub-object vtable. Confirmed in Batch 1. |
| — / `g_wwdGameObjectVtbl` | 0x1f0020 (0x5f0020) | SKIP (owner off-limits) | All stamp sites are in off-limits Gruntz files (CDDrawSubMgr.cpp, CWwdObjMgrFactories.cpp, WwdGameObjectEh.cpp) — no stamp in any Image/Wwd/Io file. |
| — / `g_imageSet1Vtbl` `g_imageSet2Vtbl` `g_imageSet3Vtbl` | 0x1f0198 / 0x1f01e0 / 0x1f0228 | SKIP (owner off-limits) | Stamped in GameLevel.cpp + CImageSet3.cpp (off-limits). |
| — / `g_wwdGridIterVtbl` | 0x1f02a8 (0x5f02a8) | SKIP (owner off-limits) | Stamped in WwdSpatialMgr.cpp (off-limits); `m_vptr = g_wwdGridIterVtbl` member store. |
| — / `g_wwdGridVtbl` | 0x1f0328 (0x5f0328) | SKIP (abstract base, no stamp) | Only referenced by `include/Gruntz/WwdGrid.h` (abstract base with a pure virtual @slot 5); no object is constructed in an allowed file. |
| `CDDrawSubMgrLeaf` / `g_catalogVtbl` | 0x1efc78 (0x5efc78) | ALREADY-REALIZED | cl already stamps `??_7CDDrawSubMgrLeaf` (masks 0x5efc78) in CDDrawSubMgrLeaf.cpp — no `g_catalogVtbl` extern/stamp remains. No action. |
| `CPoolItemA` / `g_poolItemAVtbl` | 0x1efa58 (0x5efa58) | SKIP (owner off-limits) | `new CPoolItemA(&g_poolItemAVtbl)` in CDDrawPtrCollections.cpp (CDDraw*, off-limits) — the vtbl is passed as a **ctor argument** → the ctor does a manual `m_vptr = arg` member store, not a compiler-implicit stamp. |
| — / `g_deviceConfigVtbl*` | 0x5ef628 … | SKIP (out of module) | Owner in DinMgr2/Boundary — not an image/wwd/fileMem module. |

### Rules learned / reinforced (Batch 3)

- **The `new Class` / vptr-first test is necessary but NOT sufficient.** Two stamps
  here (`CImageSurfaceItemInit` via `new`, `CFileMemBase` vptr-first member) pass the
  shape yet still can't realize: a **shared base vtable** (0x5ef7f0) would emit a
  divergent per-class `??_7`, and **unmatched vtable-slot contents** (fileMem) would
  emit a vtable of the wrong function pointers. Add two more gates to the rule: the
  vtable must be **class-private** AND its **slots reproducible** (every slot a matched
  fn at its exact RVA).
- **0x5ef7f0 is CPoolItemA's shared base vtable** (the recurring DirectDraw
  surface-pool-item base) — stamped by CFileImage / CImageSurfaceItem / CPoolItemA /
  CDDSurface alike; always foreign in any single TU. Its realization owner is
  `CPoolItemA` (CDDrawPtrCollections.cpp), and CDDSurfaceDtor.cpp already documents that
  cl's implicit stamp is unavailable there because the vtable is shared/unmatched.
- **Inline `operator new(size)` + `Construct` + vtable re-stamp is a wall** (the whole
  wwd game-object/sub-object family) — the re-stamp lands after the base ctor, so it is
  never at ctor entry and cl's implicit vptr-first store cannot reproduce it.
- **These modules are settled: the walls were already found in Batch 1.** Batch 3
  re-confirms them with the Batch-2 decisive rule and adds the fileMem determination.
  Nothing here yields today; revisit fileMem in the final sweep once its vtable-slot
  scalar-dtor/`sub_` thunks are reconstructed.
## Batch 3 (Dsndmgr / sound family) — 2026-07-01

Net effect: **0 neutral conversions** (documentation only; no source change).
exact/fuzzy delta: **0** (`gruntz build` "no regressions vs baseline",
1873/3394 exact / 64.05% fuzzy before and after). The whole Dsndmgr sound family
is modeled **non-polymorphically** (manual `g_*Vtbl` stamps) because every
candidate class's virtual slots live in OTHER TUs (unmatched `sub_XXXXXX`) — a
real-`virtual` model would make `cl` auto-emit a **divergent `??_7`** — and the
family's ctors/dtors are already `@early-stop` on the `/GX` EH-state walls (the
"whole family must be modeled first" deferral). The `new`-vptr-first
realization criterion (batch 2, aniElem) is met by **none** of these: no
`new StreamFeeder` / `new StreamVoice` / `new CGruntzSoundInnerZ` exists — all are
embedded members / `RezAlloc`+inline-field-init. Two of the listed stamps are
owned by **non-Dsndmgr** classes (a Bute tree node / a DinMgr2 device-config) →
out of scope for this batch.

Baseline sound units (unchanged): directsoundmgr 30/41 (69.5%), streamfeeder 7/9
(47.9%), streamvoice 3/5 (50.9%), gruntzsoundz 14/15 (93.1%), sounddevice 8/16,
soundstream 3/7, dsoundvoice 1/2, soundvoicelist 3/4.

| Class / stamp | vtable RVA (VA) | Outcome | Reason |
| :-- | :-- | :-- | :-- |
| `CGruntzSoundInnerZ` / `g_innerSoundVtbl` | 0x1ef700 (0x5ef700) | **KEPT (inline-manual, no `new`)** | 16-slot vtbl; slots mostly `sub_`/thirdparty-AIL in other TUs. The inner bank is built by INLINE manual construction in `CGruntzSoundZ::CreateBank`/`CreateBank2` — `RezAlloc(0x60)` + `*(void**)raw=&g_innerSoundVtbl` (vptr-first) + field seed — NOT `new CGruntzSoundInnerZ`. `cl` only emits the implicit vptr store inside a real ctor reached via `new`/member-construction; the inline `RezAlloc`+field-init can't reference the compiler `??_7` without switching to `new` (which would reshape the create helpers). The class already declares its 13 virtuals **declared-only** (no `??_7` emitted). Same idiom as the worker family (batch 2). |
| `DSoundBaseSub` / `g_DirectSoundBaseVtbl` | 0x1ef6c0 (0x5ef6c0) | **KEPT (family non-poly + EH wall)** | DSoundBaseSub's OWN vtbl, stamped **vptr-first in the derived ctor body** (0x136230) after chaining the `DirectSoundMgr` base ctor — the textbook derived-restamp shape. BUT the base `DirectSoundMgr` is modeled **non-polymorphic** (manual `g_DirectSoundMgrVtbl` @0x5ef6b8; virtuals external/unmatched, see `DirectSoundMgr.h` — `DSoundCloneBase` is only a thin slot-0 dispatch view). Realizing the 3-level hierarchy (DirectSoundMgr→DSoundBaseSub→DSoundCloneInst) would emit divergent `??_7DirectSoundMgr`/`??_7DSoundBaseSub`, and `~DirectSoundMgr` (0x135bb0) + `DSoundCloneInst::DSoundCloneInst` (0x135b10) are **already `@early-stop` on the /GX EH-state-count wall** ("modeling the base hierarchy re-shapes the ctor + emits a `??_7/??_G` and risks the 20 exact siblings; defer to the final sweep"). |
| `StreamFeeder` / `g_StreamFeederVtbl` | 0x1ef6f0 (0x5ef6f0) | **KEPT (pure slot-0 + external virtuals)** | Has a real **vptr-first ctor** (0x137cd0) BUT: (a) slot 0 = `__purecall` (a PURE virtual) while the object is instantiated as a **concrete embedded member** (`StreamFeeder m_feeder` @ StreamVoice+0x6c) — `cl` cannot both put `__purecall` in slot 0 (needs `=0` ⇒ abstract) AND permit a concrete member embedding; (b) slots 1/2 (0x137e10/0x137e20) are external/unmatched ⇒ divergent `??_7`; (c) never built via `new StreamFeeder`. Realizing would also reshape the `@early-stop` `StreamVoice` ctor/dtor (0x1375b0/0x137650). The derived override provides the real slot-0 (`CopyWindow` 0x137380) — see next row. |
| `StreamVoiceFeeder` (embedded) / `g_StreamVoiceFeederVtbl` | 0x1ef6e0 (0x5ef6e0) | **KEPT-foreign (embedded-member override)** | The DERIVED feeder-override vtbl the `StreamVoice` ctor stamps OVER its embedded `StreamFeeder` member (`*(void**)&m_feeder=&g_StreamVoiceFeederVtbl`, after the member ctor stamped the base 0x5ef6f0). `cl` can only auto-stamp a member's OWN class vtbl, never a different override into an embedded member — the restamp is inherently manual. Slots 0/1/2 (`CopyWindow` 0x137380 + `sub_137490`/`sub_1374b0`) mixed matched/external; `StreamVoice` is `@early-stop` on EH. |
| (Bute node) / `g_streamVtbl`, `g_streamData` | 0x1f0510 / 0x1f0514 | **OUT-OF-SCOPE (Bute-owned)** | Not a Dsndmgr class: a Bute tree-node's primary vftable + its +0x08 sub-object vftable (`g_streamData` = `&g_streamVtbl+4`), stamped by the node ctor in `src/Gruntz/CButeSectionCtor.cpp` (forbidden file). Owner lives in the Bute TU. |
| (DinMgr2 device-config) / `g_deviceConfigVtbl` (VtblA) | 0x1ef628 (0x5ef628) | **OUT-OF-SCOPE (DinMgr2-owned)** | A DirectInput device-config class vtbl, stamped in `src/DinMgr2/DirectInputMgr2.cpp` + `src/Gruntz/BoundaryUpper2*.cpp` (forbidden files). Owner is a DinMgr2 class, not Dsndmgr. |

### Rules learned / reinforced (Batch 3)

- **The whole Dsndmgr sound family is a "non-poly base with external virtuals"
  family** (like the CDDraw worker family in batch 2): realization stays blocked
  until the base classes' virtuals are matched — the compiler-emitted `??_7` would
  otherwise diverge. Deferred to the final sweep en masse.
- **`__purecall` in slot 0 of an instantiated-concrete class is a hard C++
  contradiction for `cl`'s implicit vptr emission** — a pure virtual makes the class
  abstract and un-embeddable as a concrete member, yet retail stamps the base
  (`__purecall`-carrying) vtbl into a live member and only later overrides it. Keep
  the manual stamp. (StreamFeeder / StreamVoiceFeeder.)
- **An embedded-member vtable OVERRIDE can never be `cl`-auto-emitted** — the
  compiler stamps the member's own class vtbl, not a caller-chosen override. The
  voice's `*(void**)&m_feeder = &g_StreamVoiceFeederVtbl` is the faithful shape.
- **No sound-family object is built via `new`** (all embedded members or
  `RezAlloc`+inline-field-init), so the batch-2 `new`-vptr-first realization gate is
  unmet across the board.
