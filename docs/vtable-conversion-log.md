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

## Batch 4 (Gruntz N-Z + Image, all-vtables override) — 2026-07-02

Mandate override: apply EVERY manual vtable stamp as a real C++ polymorphic class,
accept divergence + regressions; ONLY hard bar is `gruntz build` GREEN. Scope: all
`src/Gruntz/` files whose basename starts N-Z + all `src/Image/`.

### CONVERTED (5 sites — real-ctor stamps, cl now auto-emits `??_7` + vptr)

| Class / stamp | vtable | File | Notes |
| :-- | :-- | :-- | :-- |
| `Obj_11e4d0` / `g_vtbl_5ed0e4` | 0x5ed0e4 (5 slots) | ReconBatch2O1.cpp | Real ctor; 5 declared-only virtuals; extern removed. |
| `WorkerFull` / `g_siriusWorkerVtbl` | 0x5efb80 (10 slots) | SiriusWorkerHandlers.cpp | Real 3-arg ctor; now `@early-stop` (vptr-last: retail stamps vptr AFTER m_04/m_08/m_0c). |
| `CImageFrame` / `g_imageFrameVtbl` | 0x5eaa2c (13 slots, CImage) | Image/ImageSet.{h,cpp} | Merged `CImageFrameLoader` slots onto the class; factories dispatch `nf->Load30/Delete` directly; extern + placeholder struct removed. |
| `CImageSurfaceItemInit` / `g_fileImageVtbl` | 0x5ef7f0 (2 slots used) | Image/Image.cpp | Real ctor; vptr already stamped FIRST so store position preserved; extern removed; UnknownVTables.h entry removed. |
| `CFrameWorker` / `g_imageVtbl` | 0x5eaa2c (12 slots, CImage) | Gruntz/SpriteResource.cpp | Replaced the manual `CFrameWorkerVtbl` PMF table with real virtuals (Destroy@04, Resolve@2c); extern + PMF typedefs removed. |

Effect: **-5 exact overall** (vptr-first regressions on the converted ctors + 2
neighbour drops in the image/imageset TUs: `CImageSet::GetMemoryUsage` 100->99.96
and `CFileImage::DecodePcx` 55.78->55.68). Accepted per the mandate. Build GREEN.

### NOT compiler-auto-emittable (kept manual + documented — a hard `cl` wall, not a preference)

`cl` only emits `??_7` + the implicit vptr store inside a REAL `??0` ctor / `??1`
dtor. The following stamps live where no such ctor/dtor exists, so a polymorphic
model cannot reproduce them without inventing a fake ctor/hierarchy (large
divergence + build-break risk on already-`@early-stop` functions):

- **Return-`this` `Construct` helpers** — `ZDArrayDerived.cpp` `CZDArrayDerived::
  Construct(lo,hi)` / `g_zDArrayVtbl` (0x5e70fc, own 1-slot vtable, currently 100%).
  A method that returns `this` after an in-place base-build is NOT a ctor; turning
  it into one loses the return-`this` shape and needs a shared 2D-array base. Same
  idiom as `CMenuItem::Construct` (batch 1). Kept.
- **Dtor-phase base-vtable RE-STAMPS** — `TileTriggerContainer.cpp`
  `g_tileGridCmdVtbl`/`g_tileTriggerSwitchVtbl` (inline element scalar-delete: restore
  base vtbl, clear field, RezFree — all `@early-stop` list-walk regalloc); `ReconBatch2.cpp`
  `Obj_11e8dc::StampVtbl` / `g_severusWorkerDtorVtbl` (0x5e8cb4 = MFC `??_7CObject`,
  a bare 7-byte base restamp); `WwdGameObjectEh.cpp` flat B-variant dtor (multi-level
  `g_wwd*`/`g_remus*` restamps, `@early-stop`; the A/C/F variants are ALREADY real
  polymorphic and cl auto-emits their restamps). A base restamp during teardown is
  not the class's own construction vtable → no single `??_7` expresses it.
- **Runtime vtable SWAPS (construction -> runtime)** — `TypeKeyColl.cpp`
  `DynInitButeTree`/`DynInitTypeColl` swap `g_buteTreeVtbl`/`g_buteTreeSubVtbl`/
  `g_typeCollRunVtbl` over a just-constructed global (virtual-base construction vtbl
  then runtime vtbl); `CButeTree::ScalarDtor` swaps the dtor-phase vtbls. A single
  compiler `??_7` cannot express a two-vtable construction->runtime swap. (`g_keyFinderVtbl`
  is NOT a vtable at all — 0x16e220 is a `.text` callback function stored in a struct
  field; making CKeyFinder polymorphic would fabricate a false vtable, so kept.)
- **Custom-allocator inline construction** — `UserBaseLink.cpp` EnsureWorker80/88/90
  `g_siriusWorkerVtbl` (RezOpNew + inline field block + mid-sequence vptr stamp, all
  three `@early-stop` on the zero-register-pin wall). A placement-new ctor would add
  the documented placement null-guard + vptr-first divergence; and the vtable is
  shared with the out-of-scope `CDDrawWorkerCache.cpp` ('C'), so the shared header
  extern must stay. The sibling `WorkerFull` ctor variant IS converted (above).
- **Multi-vtable status-bar tab HIERARCHY (needs the real CSBI_* ctors)** —
  `StatusBarGameMenu.cpp`, `SBI_RectOnly.cpp`, `SBI_TabzDialogEh.cpp`,
  `SBI_SideTabBuild.cpp` stamp ONE placeholder `CSbiTab`/`CSbiTabBase` with FIVE
  different vtables (`g_vtbl_rectBase`/`menuItem`/`t3`/`t4`/`sideTab`) by type tag. A
  class can carry only ONE compiler `??_7`; reproducing this needs a base + 5 derived
  classes each with its own placement-new ctor — beyond a mechanical stamp swap and
  high build-break risk on the already-`@early-stop` /GX EH-frame archetype. This IS
  the documented "re-attack once the CSBI_* item ctors land" final-sweep work.
---

## 0x24556c dual-view: `CGameRegistry` / `CGruntzMgr` (MFC/Win32 wall)

**Task:** unify the residual dual-canonical for the game-manager singleton at
RVA 0x24556c (VA 0x64556c) — `include/Gruntz/CGameRegistry.h` (`CGameRegistry`,
the plain-struct view; `g_gameReg`/`g_mgrSettings`/`g_64556c`/… ; ~60 TUs) and
`include/Gruntz/GruntzMgr.h` (`CGruntzMgr`, the MFC derived-class view; 6 direct
+ comment-referencing TUs).

**They ARE one object (verified):** the pointee at *0x24556c is the RTTI-true
`CGruntzMgr` (`??_7CGruntzMgr@@6B@` @0x5e9b64, `new`'d by
`CGruntzApp::InitializeGameManager` @0x080a20, `push 0xa30`). Proof of identity:
`CGameRegistry::Ack` and `CGruntzMgr::ReportError` are the **same function**
(both RVA 0x08dc60); slot meanings coincide (`m_48`==`m_sound`, `m_2c`==
`m_curState`, `m_13c..m_148`==`m_viewOrigin{L,T,R,B}`); and the "viewport X/Y"
guess in the old `CGameRegistry` comments was **wrong** — `RestoreVideoMode`
(0x08ddd0) does `cmp DWORD PTR [reg+0x8c],0x280` (0x280 = 640), i.e. `m_8c` is the
video-mode **width** (`m_modeW`), `m_90` the height (`m_modeH`).

**WALL — a single canonical header is build-impossible (not a codegen wall, a
toolchain/include wall):** `CGameRegistry.h` is included by ~60 TUs, and several
are **pure-Win32** (they `#include <Win32.h>` → `windows.h`, e.g.
`TileActionEvent.cpp`, `SaveScreenshot.cpp`). `CGruntzMgr` is an **MFC** class
(`: public WAP32::CGameMgr`, `CString`/`CByteArray`/`CState*` members) whose header
pulls `<Mfc.h>`/afx. afx hard-errors on a prior `windows.h`:

```
afxv_w32.h(14) : fatal error C1189: #error : WINDOWS.H already included.
                 MFC apps must not #include <windows.h>
```

Proven empirically: adding `#include <Wap32/Wap32.h>` to `CGameRegistry.h` (the
minimal step toward one class) breaks `tileactionevent.obj`/`savescreenshot.obj`
(and the rest of the pure-Win32 includers) with C1189. Therefore the plain,
MFC-free struct view MUST exist for the Win32 side, and the MFC class view MUST
stay in its own afx-pulling header. **The dual view is a NECESSARY split** — the
devs' `?g_gameReg@@3PAUWwdGameReg@@A` global is exactly this: an opaque
`WwdGameReg*` (MFC-free struct name) for engine/Win32 code, the full `CGruntzMgr`
for the game code.

**What was done (build stays GREEN, no regressions — 1858/3361 exact / 64.44%
fuzzy, unchanged):**
- Designated `CGameRegistry.h` the **field-offset source of truth** and reconciled
  its field comments with `CGruntzMgr`'s descriptive names (e.g. `m_8c`→`m_modeW`,
  `m_90`→`m_modeH`, `m_94/m_98`→`m_savedModeW/H`, `m_48`→`m_sound`, `m_2c`→
  `m_curState`, `m_100`→`m_isVoiceEnabled`, `m_13c..m_148`→`m_viewOrigin*`, and the
  0x150 region documented as `CGruntzMgr::m_options[4]`). Comment-only, so
  matching-neutral; the terse `m_<off>` names are kept because the ~60 (incl.
  untouchable grunt-family) TUs reference them.
- Cross-linked both headers so each names the other as the same singleton and
  points here for the wall.

**NOT done (blocked by the wall, deferred):** collapsing to one C++ class /
deleting `GruntzMgr.h` / re-pointing its includers — any of these requires the
MFC class in the Win32-shared header, which is the C1189 break above. Not a
final-sweep item (it is a genuine toolchain constraint, not a steerable codegen
idiom): the correct end-state is the two reconciled views, kept in sync.
## Batch 4 (ALL-VTABLES mandate, Gruntz [A-M] non-grunt) — 2026-07-02

User mandate override: "apply ALL vtables actually; ignore incorrect loads into
structs; implement them anyway." The vptr-first / class-private / matched-slots
gates from batches 1-3 are LIFTED — regressions are ACCEPTED, the only hard bar is a
green `gruntz build`. So the manual `*(void**)o = &g_*Vtbl` / raw-`operator new`
factory stamps are converted to REAL polymorphic classes built via `new` / placement
`new`, letting cl auto-emit `??_7` (with external slot refs, fine for per-TU COFF).
Net: **9 vtables realized** across 7 files; the factory functions that stamped them
regress from vptr-middle to vptr-first (accepted). Overall 55.3%->55.0% exact,
64.44%->64.37% fuzzy (`gruntz build` GREEN, no compile errors). Blessed the 11
factory regressions with `--accept-regressions`.

| Class / vtable | RVA | File | Action |
| :-- | :-- | :-- | :-- |
| `SiriusWorkerObj` / 0x5efb80 | 0x1efb80 | CDDrawWorkerCache.cpp | `new SiriusWorkerObj` (ctor added), stamp+extern removed. ??_7 orphan (0x1efb80 shared w/ CLogicRecord; left unpaired). |
| `HagridWorkerA` / 0x5efea0 | 0x1efea0 | CDDrawWorkerList.cpp | `new HagridWorkerA`; **paired** (vtable_names.csv ??_7HagridWorkerA@@6B@,0x1efea0,0x30). |
| `HagridWorkerB` / 0x5efed0 | 0x1efed0 | CDDrawWorkerList.cpp | `new HagridWorkerB`; **paired** (??_7HagridWorkerB@@6B@,0x1efed0,0x38). |
| `SeverusWorkerBase`+`SeverusWorkerObj` / 0x5efc30,0x5efbe8 | — | CDDrawWorkerRegistry.cpp | two-level `SeverusWorkerBase`->`SeverusWorkerObj`; `new` stamps base(0x5efc30)-then-derived(0x5efbe8) + auto CByteArray member ctor. ??_7 orphan (0x5efbe8 shared w/ CSeverusEntryList). |
| `AlbusWorkerObj` / 0x5f02d8 | 0x1f02d8 | CDDrawWorkerMapSmall.cpp | `new AlbusWorkerObj`; ??_7 orphan (shared w/ CAniRecord base-2). |
| `CAmbientSound`/`CAmbientPosSound`/`CRandomAmbientSound` / 0x5e710c,0x5e7124,0x5e713c | — | CWorldSoundSet.cpp | placement `new (raw) CXxx` (3 real RTTI classes); **PAIRED** — the names were already in vtable_names.csv but no TU emitted them; now the 3 factory vtables pair. |
| `CAniRecordInit` / 0x5f02c0 | 0x1f02c0 | CAniElement.cpp | already `new CAniRecordInit`; swapped the manual-stamp ctor for 5 real virtuals. ??_7 orphan (0x5f02c0 shared w/ CAniRecord). |
| `CRemusNode` (+`CRemusNodeBase`) / 0x5efbc0,0x5e8cb4 | 0x1efbc0 | CRemusNode.cpp | two-level real poly; ctors stamp ??_7CRemusNode vptr-first, ~ folds the ??_7CRemusNodeBase grand-base restamp (masks 0x5e8cb4). **paired** (??_7CRemusNode@@6B@,0x1efbc0,0x28). |

UnknownVTables.h entries removed (realized): ClassWithUnknownVTable24 (RemusNodeVtbl),
40 (HagridWorkerVtblA), 41 (HagridWorkerVtblB) — replaced with REALIZED comments.

### Already-real-polymorphic (no action, compliant): CSeverusEntryList, CRemusEntryList,
CSeverusWorkerHost, CSeverusWorkerEh, CDDrawSubMgrLeaf, CDDrawSubMgrLeafScan,
CDDrawWorkerMapSmall (manager), CDDrawSubMgrAni (CAniElementObj). These already stamp
via cl-emitted ??_7 (CSeverusBase/CRemusBase/AlbusMapBase real bases); their leftover
`g_*Vtbl` externs are foreign-base bindings (0x5e8cb4=??_7CObject etc.) kept in place.

### DEFERRED (documented, not done this batch) — real work, would break/exceed budget:
- **CMulti / CPlay / CState (0x1e9fe4/0x1ea0bc/0x1ea21c)**: big MFC state classes
  (0xac = 43 slots) derived through CGameMgr; converting emits a 43-slot divergent
  ??_7 and touches the MFC hierarchy — high build-break risk. In vtable_names.csv, so a
  future careful conversion could pair them.
- **CMenuItem / CMenuItem2 (0x5f08c0/0x5f08f8)**: 14-slot polymorphic leaf tightly
  coupled across MenuItem.cpp/MenuItem2.h/MenuPage.cpp (Construct helper + placement-new
  sites + CMenuItem2 derivation, many EH @early-stop walls). Converting could turn the
  reloc-artifact dtor plateaus (96.6%/92.3%) into 100% IF the vtable is added to csv, but
  the multi-file coupling is a large coordinated change — deferred to a focused pass.
- **CStatusBarMgr (CSBI_* config items)**: stamped via a non-ctor `Construct(vtbl,tag)`
  helper (vtbl passed as an argument) — cl cannot auto-emit an implicit stamp there.
- **wwd factories (CWwdObjMgrFactories.cpp / CDDrawSubMgr.cpp)**: inline raw-alloc +
  base-`Construct` + vtable re-stamp (vptr-not-at-ctor-entry); the game-object classes
  (0x5f0020/0x5f00a8/...) live in other TUs and are re-stamped mid-construction.
- **LogicRecord (CLogicRecord 0x5efb80)**: /GX EH dtor @early-stop on the base-subobject
  frame; vtable shared with the SiriusWorker realized above.
- **Boundary* / ApiHiCallers / CButeSectionCtor / DiscoveredSmall / GameText**: foreign
  base re-stamps (CUserBase/CState/CObject/containerErr) in non-ctor or shared-base sites;
  CAmbientSound.cpp's remaining stamp is the shared CUserBase base (own vtable already
  realized via CWorldSoundSet).
## Batch 4 (the GRUNT family — ALL-VTABLES mandate) — 2026-07-02

The ALL-VTABLES phase override: implement EVERY vtable in the grunt-entity scope as
a real C++ polymorphic class even where slots aren't all matched; accept layout/load
divergence + % regressions; only hard bar is `gruntz build` GREEN.

| Class / stamp | vtable RVA (VA) | Outcome | %-effect | Reason |
| :-- | :-- | :-- | :-- | :-- |
| `CGrunt` / `g_cgruntVtbl` | 0x1e8754 (0x5e8754) | **CONVERTED** | `~CGrunt` dtor 55.5%→94.9% | Modeled the full single-inheritance chain `CUserBase(3 slots) <- CUserLogic(16) <- CGrunt(17)` real-polymorphic (virtual dtor at slot 0), so `cl` auto-emits `??_7CGrunt/CUserLogic/CUserBase@@6B@`, the three vptr restamps, and the per-member /GX trylevel teardown. The six owned members (+0x1c0 CString, +0x31c/+0x338 CObList, +0x448/+0x44c CString, +0x468[9] cell array) are now value subobjects; +0x18 EngStr link is `CUserLogic::m_18`. Removed the 3 manual `*(void**)this=&g_*Vtbl` stamps + the `g_cgruntVtbl/g_userLogicVtbl/g_userBaseVtbl` externs + `GruntVecDtor`/`GruntCellDtor`. Residual is the EH-state-base-numbering wall (state COUNT matches at 8; retail numbers 1..8, MSVC gives 0..7) — `@early-stop` in Grunt.cpp. |
| `CUserLogic` / `g_userLogicVtbl` | 0x1e705c (0x5e705c) | **CONVERTED** | — | Emitted as the CGrunt base's `??_7CUserLogic@@6B@` (16 slots; slots 1/6/11 named SerializeMove/InitDirVectors/FreeNameList, the rest declared-only). CGrunt-local reconstruction of the class (the tile-logic game-object family keeps its own member view in include/Gruntz/UserLogic.h; the two never coexist in a TU — Grunt.h's 5 includers pull neither UserLogic.h). |
| `CUserBase` / `g_userBaseVtbl` | 0x1e70b4 (0x5e70b4) | **CONVERTED** | — | Emitted as the root `??_7CUserBase@@6B@` (3 slots: dtor + 2 declared-only). Same CGrunt-local reconstruction note as CUserLogic. |

### CGrunt dedup (the loose-end)

| TU | Local CGrunt view | Outcome | Reason |
| :-- | :-- | :-- | :-- |
| `FinishLevelSprite.cpp` | method-only (`ResolveDeathAnimation`) | **UNIFIED** → `#include <Gruntz/Grunt.h>` | Compiles clean; the `m_2a0->ResolveDeathAnimation()` call is a non-virtual direct call (codegen-neutral). |
| `TriggerMgr.cpp` | method-only (`ReadConfigFromButeMgr` PMF tag + 2 calls) | **KEPT (build-breaker)** | Including Grunt.h pulls CGameRegistry.h whose `g_gameReg` type clashes with this TU's own `g_gameReg` decl → `error C2371 redefinition`. Reverted; documented inline. |
| `GruntPhaseStep.cpp`, `GruntPathScan.cpp` | full raw-offset layouts (local type names: MapObj*/AnimRec*/TileMgr*, CScanSub10*/CScanNode*, m_31c/m_320/m_324/m_2ec…) | **DEFERRED** | Genuine layout duplicates, but the 0%-matched method bodies are written against local member vocabularies absent from Grunt.h; unifying needs a per-function rewrite (a separate pass), and these TUs hit the same g_gameReg-style include clashes. |
| `ApiCallers.cpp` (src/Stub) | method-only (backlog) | **DEFERRED** | src/Stub backlog file (holds real bodies; do not bulk-move per MEMORY). |

### Rules learned / reinforced (Batch 4)

- **A multi-level `/GX` dtor chain DOES convert cleanly to a real polymorphic
  hierarchy** (eh-dtor-multilevel-polymorphic-chain.md): modeling the whole base
  chain + the destructible members as value subobjects lets `cl` auto-emit the frame,
  the N vptr restamps, and the __ehvec_dtor/member teardowns — the manual stamps and
  `g_*Vtbl` externs all drop out. Big win where the residual was "no per-member
  trylevel" (55.5%→94.9%).
- **Beware stray CString value members that retail's dtor does NOT own.** CGrunt's
  +0x54 type-name CString would add a spurious auto-destruct + shift every EH state;
  modeled it as a raw `void* m_typeName` viewed via `TypeName()` (a `*(CString*)&`
  cast — codegen-neutral) so ~CGrunt tears down only the six members retail does.
- **The heavy shared header can clash on globals.** Unifying a per-TU CGrunt view by
  `#include <Gruntz/Grunt.h>` pulls CGameRegistry.h; TUs with their own `g_gameReg`
  decl (TriggerMgr) break — a real build-breaker, revert that TU.

## Batch 5 (Wwd + GameLevel cluster, vtable-walls + retrofit) — 2026-07-02

Scope: `src/Gruntz/` Wwd*/WwdGameObject*/CWwdObjMgr*/WwdSpatialMgr/GameLevel* +
headers. Mandate: drive the cluster's vtables to zero, irregardless of %,
regressions accepted, only hard bar `gruntz build` GREEN. Coordinator addendum:
`SIZE(Class,0xNN)` + `VTBL(Class,0x<rva>)` atop every realized/retrofit virtual
class; use the exact factory/ctor `RezAlloc(0xNN)`/`new(0xNN)` size where known.

Net: **1 fresh REALIZE (CWwdGridIter) + 1 paired retrofit (CWwdGrid) + 3 catalog
retrofits (already-realized orphan factory vtables) + 4 exact SIZEs**. Overall
1835/3361 exact / 64.38% fuzzy (was 64.39%); build GREEN, no dup-DATA. 3 accepted
regressions (CWwdGridIter realization, all on already-`@early-stop` walls).

### REALIZED / RETROFIT

| vtable (RVA) | class | action | VTBL? | SIZE |
| :-- | :-- | :-- | :-- | :-- |
| WwdGridVtbl 0x5f0328 | `CWwdGrid` (WwdGrid.h) | RETROFIT — was already real-polymorphic (abstract, `__purecall` @slot5), cl auto-emits ??_7CWwdGrid; placeholder removed | **VTBL(CWwdGrid,0x001f0328)** — 0x1f0328 was UNBOUND, so pairing is pure gain (emitted ??_7 now pairs the delinked datum) | **SIZE 0x44** (grid-setup RezAlloc(0x44) x3 @0x168094/af/cd) |
| WwdGridIterVtbl 0x5f02a8 | `CWwdGridIter` (WwdSpatialMgr.cpp) | REALIZE — converted the `void* m_vptr` member + manual `m_vptr=g_wwdGridIterVtbl` stamp to a 5-slot CObject-style polymorphic class (dtor @slot1, RemusV0/2/3/4 declared-only). cl auto-emits the implicit vptr-FIRST ctor stamp (== the old first-store). Extern + placeholder removed | **NO VTBL** — 0x5f02a8 is SHARED (already bound `g_planeRenderVtbl` in wwdfile); emitted ??_7CWwdGridIter is an orphan (unpaired, neutral) | **SIZE 0x44** (embedded m_iter spans CWwdSpatialMgr +0x70..+0xb4) |
| WwdGameObjectVtbl 0x5f0020 | `CWwdGameObjectE` (Mid, WwdGameObjectEh.cpp) | RETROFIT catalog — ALREADY realized in batch 4; cl emits orphan ??_7CWwdGameObjectE. Placeholder removed | **NO VTBL** — bound `g_wwdGameObjectVtbl` via the factory manual stamp (can't reference the compiler ??_7 without a `new`-ctor rewrite) | SIZE_UNKNOWN (Mid base subobject, not directly allocated) |
| Vtbl_1f0060 0x5f0060 | `CWwdGameObjectF` (159440 variant) | RETROFIT catalog — orphan ??_7CWwdGameObjectF. Placeholder removed | NO VTBL (bound g_wwd159440FinalVtbl) | **SIZE 0x18c** (factory RezAlloc(0x18c)) |
| WwdObjVtbl 0x5f00a8 | `CWwdGameObjectA` (166640 variant) | RETROFIT catalog — orphan ??_7CWwdGameObjectA. Placeholder removed | NO VTBL (bound g_wwdObjVtbl) | **SIZE 0x1dc** (factory RezAlloc(0x1dc)) |
| — 0x5effd0 | `CWwdGameObjectC` (159250 variant) | (not a scope catalog entry) orphan ??_7CWwdGameObjectC | — | **SIZE 0x190** (factory RezAlloc(0x190)) |

`CWwdGameObjectB` (the 0x1598d0/0x1fc flat variant) got **SIZE 0x1fc** (its flat
model IS complete to +0x1f8).

### DEFERRED (documented walls — NOT attempted; big rewrite, high revert risk)

- **GameLevelVtbl 0x5f0150 (`CGameLevel`) / g_severusWorkerBaseVtbl 0x5efc30
  (`CSeverusWorker`)** — `CGameLevel` is already declared polymorphic BUT the base
  obj emits NO ??_7 (verified `llvm-objdump -t gamelevel.obj`): the manual two-phase
  stamps (`CSeverusWorker` ctor stamps g_severusWorkerBaseVtbl, `StampLevelVtbl`
  stamps g_gameLevelVtbl) make cl DEAD-STORE-ELIMINATE its implicit vptr stores, so
  the ctor reproduces retail's exact 2-store two-phase sequence (89% — a documented
  faithful KEEP from batch 1). Realizing needs a CGrunt-style multi-level refactor:
  `CSeverusWorker` is modeled with 18 slots for CGameLevel's slot-numbering but the
  REAL SeverusWorker base vtable @0x5efc30 is only **9 slots**, and ~CSeverusWorker
  restamps 0x5e8cb4 (CObject grand-base) — so a correct realization = remodel
  CSeverusWorker to 9 slots + a CObject grand-base + CGameLevel adds 9. That emits a
  divergent 18-slot ??_7CSeverusWorker and destabilizes the 89% ctor + the whole
  CImageSet family. NO VTBL possible either (0x1f0150 bound g_gameLevelVtbl; a phantom
  ??_7CGameLevel binding would dup-DATA). Final-sweep candidate.
- **Vtbl_1f00e8 0x5f00e8 (`CWwdGameObjectB`, the 0x1598d0-final flat variant)** —
  `~CWwdGameObjectB` is a FLAT manual multi-vtable-restamp dtor (`@early-stop`
  eh-dtor trylevel wall). Not a cl-emitted ??_7. Realize when it converts to the
  CRemusNode-derived polymorphic chain (like A/C/F). Placeholder kept for slot
  tracking.
- **wwd game-object FACTORY realization (CWwdObjMgrFactories.cpp)** — the four
  factories build via `RezAlloc + placement + two-phase vtable stamp` (`@early-stop`
  rezalloc-placement-new-no-EH-frame wall). The dtor variants A/C/E/F are ALREADY
  real-polymorphic (their ??_7 emitted, orphan), but the CTOR side keeps the manual
  stamps because pairing the ??_7 to the delinked datum needs the factory to
  construct via `new WideObj`-with-throwing-ctor — the documented final-sweep upgrade.
- **Vtbl_1f0270 0x5f0270** — OUT OF SCOPE: stamped in `CSeverusWorkerHost.cpp` (not a
  Wwd*/GameLevel* file). Owner is a severus-worker-host class.
- **Vtbl_1f0310 0x5f0310** — NO owning class found in `src/` (unreferenced catalog
  orphan). Left as a placeholder.

### Rules reinforced (Batch 5)

- **Add VTBL only when the vtable RVA is UNBOUND.** CWwdGrid's 0x1f0328 was free →
  pair it (win). Every other scope vtable RVA is already bound to a `g_*Vtbl` DATA
  extern (the factory/manager manual stamps reference it), so a VTBL would dup-DATA
  and break the build — the emitted ??_7 must stay an ORPHAN (matching-neutral).
- **Realizing a class whose vtable is SHARED still removes the catalog placeholder**
  (correct devs' shape) but only emits an orphan ??_7 and can regress neighbors that
  embed/scope the object (CWwdGridIter: CountInRect/FlushGrid/ForEachGrid −2..−4% on
  the /GX scoped-local frame). Accepted per the irregardless-of-% mandate.
- **A polymorphic class whose ctor manual-stamps `*(void**)this=&extern` emits NO
  ??_7** — cl dead-store-eliminates its implicit vptr store (overwritten by the
  manual stamp), so the manual-stamp shape is a stable faithful reproduction that a
  naive "remove the stamp" would destabilize (GameLevel).
## Batch 5 (SIZE+VTBL retrofit — Dsndmgr / DinMgr2 / Wap32 / DDrawMgr) — 2026-07-02

VTBL()/SIZE() **retrofit** pass over the four device-manager modules. Every vtable
these modules own or stamp now carries a real `??_7<Class>@@6B@` catalog name (via a
VTBL() macro atop/beside the class) plus an exact SIZE() where `new(0xNN)`/`RezAlloc`
pins it. **All matching-neutral** (MSVC sees nothing; a vtable name is reloc-masked) —
build stayed GREEN, `no regressions vs baseline` (1835/3361 exact, 64.39% fuzzy).

**Realized (cl-emitted ??_7) — VTBL added atop the class:**

| Class | vtable RVA | SIZE | TU |
| :-- | :-- | :-- | :-- |
| `SoundDevice` | 0x1ef6c4 | 0x98 | include/Dsndmgr/SoundDevice.h |
| `SoundStream` | 0x1ef6ec | 0x9c | include/Dsndmgr/SoundStream.h |
| `StreamFeeder` | 0x1ef6f0 | 0x44 | include/Dsndmgr/StreamFeeder.h |
| `StreamVoiceFeeder` | 0x1ef6e0 | 0x44 | include/Dsndmgr/StreamFeeder.h |
| `CGruntzSoundInnerZ` | 0x1ef700 | 0x60 | include/Dsndmgr/CGruntzSoundZ.h |
| `DirectSoundMgr` | 0x1ef6b8 | (unk) | include/Dsndmgr/DirectSoundMgr.h |
| `DSoundBaseSub` | 0x1ef6c0 | (unk) | src/Dsndmgr/DirectSoundMgr.cpp |
| `DSoundCloneInst` | 0x1ef6bc | (unk) | src/Dsndmgr/DirectSoundMgr.cpp |
| `CShadeTableArray` | 0x1efb28 | 0x14 | src/DDrawMgr/ShadeTableCache.cpp |

**Non-polymorphic (manual stamp KEPT; VTBL is a target-side catalog name only):**

| Class | vtable RVA | SIZE | Why kept | TU |
| :-- | :-- | :-- | :-- | :-- |
| `CInputDevice` (=CDeviceConfigA) | 0x1ef628 | 0x338 | explicit m_vptr + /GX dtor restamps a 3-level base subobject chain (0x628/0x680/0x670) — needs the multilevel polymorphic model (eh-dtor-multilevel), dtor already `@early-stop` 42.7% | src/DinMgr2/DirectInputMgr2.cpp |
| `CDeviceConfigB` | 0x1ef640 | 0x2c8 | explicit m_vptr, foreign engine vtable ctor-stamped | src/DinMgr2/DirectInputMgr2.cpp |
| `DSoundVoice` | 0x1ef6d0 | 0x28 | explicit m_vtbl; ctor 0x136fe0 lives in another TU (cl cannot auto-emit) | src/Dsndmgr/DSoundVoice.cpp |
| `CContainerErr` | 0x1f04cc | (unk) | vptr-LAST ctor store (Batch 1 empirically regressed the real-virtual model) | src/Wap32/EngStr.cpp |
| `CShadeArrayBase` | (masks CObject 0x1e8cb4) | 0x14 | grand-base vtable is CObject's; already cataloged | src/DDrawMgr/ShadeTableCache.cpp (SIZE only) |

**Not attempted (remaining in scope — documented structural walls / out-of-module):**
- `PureVtbl` (0x1ef6c8): all-`__purecall` "poison" vtable stamped in a element-reaper
  (SoundDevice/SoundVoiceList), NOT a ctor — cl cannot auto-emit; no real class name.
- `g_StreamVoiceVtbl` (0x1ef6e0 embedded feeder override): vptr-override-after-base wall.
- `g_zDArrayVtbl`/`g_zDArrayDtorVtbl` (0x1e70fc/0x1f04d4): templated names (kept in
  config/vtable_names.csv, not VTBL-expressible); non-ctor `Construct` install site.
- `g_vtbl_5ec26c` (ComHelperThunks): already `??_7CNoTrackObject@@6B@` in csv (MFC base).
- DinMgr2 vtables 0x1ef658 (joystick) / 0x1ef670 / 0x1ef680: base-subobject / joystick
  vtables with no clean single owning-class name (would require inventing names).
- `Vtbl_1ef7a8`/`1ef7d0` (Rez), `PoolItemAVtbl` family `1efa88`/`1efab8`/`1efae8`
  (CDDrawPtrCollections, Gruntz TU): foreign — the item virtuals live in other TUs, the
  factory hand-stamps `g_poolItemVtbl*` DATA externs; not "driveable to zero" here and
  outside the four assigned modules.

### Rule learned (Batch 5)

- **`SIZE`/`VTBL` in a HEADER that pulls `<Win32.h>`/windows.h collides with the Win32
  `SIZE` type when the header is `#include`d BEFORE `<rva.h>` in the .cpp** (the macro
  isn't defined yet at header-parse, so `SIZE(Class,0xNN)` parses `SIZE` as the windef.h
  struct → `C2059 syntax error : constant` + `C2378 SIZE redefinition`). Fix: host the
  SIZE/VTBL in the .cpp (after its `#include <rva.h>`), like GameApp's EOF metadata —
  DirectInputMgr2.h / ShadeTableCache.h class metadata lives in their .cpp for this reason.
## Batch 5 (Net/Bute/Rez/Wwd/Io/Stub — retrofit + wall audit) — 2026-07-02

Findings: the scope's `g_*Vtbl` conversions were LARGELY done by Batches 1-4. This
batch (a) RETROFITs the two remaining realized-but-uncatalogued own-vtables, (b)
pins the exact SIZE for the realized Net/InterfaceObject nodes (from their
`operator new`/`RezAlloc` sizes), (c) marks the four now-cataloged catalog entries
REALIZED, and (d) audits every remaining scope stamp as a terminal/deferred wall.
`gruntz build` GREEN throughout; **no regressions** (54.6% exact / 64.39% fuzzy;
VTBL/SIZE are clang-only, matching-neutral).

| Class / stamp | vtable RVA | Outcome | Reason |
| :-- | :-- | :-- | :-- |
| `CRezDir` (Rez) | 0x1ef7a8 | **RETROFIT (VTBL added)** | Already real-polymorphic (`CRezDir : CRezItmBase`, ctor 0x13c940 cl-auto-stamps the derived vtable), but the retail datum was the anonymous `Vtbl_1ef7a8`. Added `VTBL(CRezDir, 0x001ef7a8)` (include/Rez/RezMgr.h) → datum now named `??_7CRezDir@@6B@`. |
| `InterfaceObject` (Net) | 0x5f0748 | **RETROFIT (VTBL + SIZE added)** | Already real-polymorphic (`InterfaceObject : InterfaceObjectBase`, dtor 0x179340 defines the key virtual so cl emits `??_7`), but the retail datum was the anonymous `Vtbl_1f0748` and the class carried NO size annotation. Added `VTBL(InterfaceObject, 0x005f0748)` + `SIZE(InterfaceObject, 0x10)` (AddGroupNode new-site). |
| `CNetPlayerListNode` (Net) | 0x5f0760 | **SIZE pinned** | VTBL was already present (Batch <5); `SIZE_UNKNOWN → SIZE(0x58)` from AddPlayerNode's `RezAlloc(0x58)`. Catalog entry marked REALIZED. |
| `CNetSessionNode` (Net) | 0x5f0778 | **SIZE pinned** | VTBL already present; `SIZE_UNKNOWN → SIZE(0x24)` from AddSessionNode's `RezAlloc(0x24)`. Catalog entry marked REALIZED. |

### DEFERRED walls (audited this batch — terminal or cross-TU, NOT ripped out)

- **Net factory stamps (`g_netPlayerNodeVtbl` 0x5f0760 / `g_netSessionNodeVtbl`
  0x5f0778 / `g_netSessionNodeDtorVtbl` 0x5e8cb4 in NetMgr.cpp; InterfaceObject own
  stamp in AddGroupNode)** — TERMINAL transitional. The node classes ARE realized
  (NetSessionNode.cpp / InterfaceObject.cpp, cl emits their `??_7`, now VTBL-cataloged),
  but the FACTORIES build via `RezAlloc(N)+manual stamp` (NOT a ctor), so cl has no
  auto-stamp site — the `*(void**)node=&g_*Vtbl` correctly reloc-masks against the
  realized `??_7`. Rewriting the factory as ctor-based `new` would change codegen on
  these `@early-stop` regalloc/EH-scheduling-wall functions (AddSessionNode ~56%,
  AddPlayerNode ~92%). No further vtable work possible here.
- **Bute `CButeMgrHelper` (`g_helperVtbl` 0x5f03bc + vbase tables
  `g_helperVbaseVtblA/B/C/D` 0x5f0374/0x5f0384/0x5f045c/0x5f047c + 0x5f0394)** —
  VIRTUAL-INHERITANCE class: the vbase tables are `??_8`/construction-vtables, NOT
  `??_7<Class>@@6B@`, so VTBL can't name them and cl can't reproduce the vbtable/vtordisp
  layout from a clean model. Terminal until the vbase machinery is modeled.
- **Bute `g_buteNodeSubVtbl` (0x5e949c)** — the PROMOTED second vtable of the embedded
  `m_entry` sub-object (raw `*(void**)&m_entry=&...`); a promoted 2nd vtable is not a
  C++-nameable `??_7` (ctor-handrolled-vptr-store-last.md). Terminal raw write.
- **Wwd `g_wwdObjVtbl` (0x5f00a8, CWwdGameObject family) / `g_planeRenderVtbl`
  (0x5f02a8, plane-render CSeverusWorker)** — inline raw-alloc factories
  (ReadPlaneObjects/RebuildPlanes) that `operator new` + base-`Construct` + re-stamp
  classes OWNED by src/Gruntz TUs (vptr-not-at-ctor-entry). Converting needs those
  cross-TU classes modeled; out of Wwd scope. (`g_wwdSubVtbl` 0x5f0128 already
  realized = CAniAdvanceCursor; `g_severusWorkerDtorVtbl` 0x5e8cb4 = shared CObject
  base, catalogued as `?g_severusBaseDtorVtbl`.)
- **Stub/Discovered `ClassUnknown_15/45/50/51/52/53/54`** (stamps 0x5ed36c / 0x5ea2a4 /
  0x5f04d8 / 0x5e8cb4×4) — tiny ctor / void-init vptr stamps for UNMODELED
  `ClassUnknown_N` whose slots point cross-TU / share the severus base 0x5e8cb4.
  Modeling would emit a divergent `??_7` and regress (comdat-inline-ctor-no-standalone.md,
  vptr-stamp-void-init-not-ctor.md). Correct transitional; per match-queue-priorities,
  lone `ClassUnknown_N` are skipped.
- **Stub/MallocConstructors `Node174d00`** (`m_vptr`=0x5f051c + `m_subVptr`=0x5f0518) —
  has an EXPLICIT `m_vptr` field + a second `m_subVptr`; adding `virtual` shifts every
  offset by 4 (explicit-mvptr-no-virtuals.md). Terminal manual stamp.

### Rule reinforced (Batch 5)

- **A realized (cl-emitting) class can still leave its retail vtable datum ANONYMOUS**
  (`Vtbl_<rva>` in UnknownVTables.h) when nobody added the `VTBL(Class, rva)` catalog
  line — the ctor/dtor auto-stamp reloc-masks either way, so it never shows in a %.
  The fix is a one-line `VTBL(...)` (matching-neutral); find them by reading the
  ctor/dtor's vtable-store reloc (dump_target) and cross-checking the datum is
  uncatalogued (not in symbol_names.csv). CRezDir (0x1ef7a8) + InterfaceObject
  (0x5f0748) were two such cases.
- **UnknownVTables.h REALIZED annotations are hand-maintained**, not auto-pruned by
  vtable_scan when a VTBL is added — update the entry to a `// REALIZED as ??_7…`
  comment in the same change (this batch cleaned up the two new + two stale Net ones).
## Batch 5 (menu/SBI/status + game-state — UNBLOCK the auto ??_7) — 2026-07-02

Net effect: **13 vtables realized** (manual `g_*Vtbl` DATA symbol → compiler `??_7`),
build GREEN, **0 regressions** (started-units 1835/3361, 64.39% fuzzy, unchanged; the
overall headline dips 54.55%→54.24% only because ~19 newly-named `??_7`/dtor-thunk
symbols joined the denominator — a realization side-effect, not a matched-code loss).

**KEY MECHANISM (new, generalizes the whole all-vtables mandate).** For an
RTTI-named vtable (`??_7X@@6B@` in config/vtable_names.csv) whose class is ALREADY
modeled real-polymorphic **with an out-of-line virtual dtor in a compiled base OBJ**
(so `cl` emits the `??_7` COMDAT), the ONLY thing blocking the auto-pair is a manual
`DATA(0x..) extern g_*Vtbl` at the SAME rva: the symbol_names generator prefers the
explicit DATA annotation over the vtable_names.csv `??_7` mapping. **Just delete the
`DATA(0x..)` pin (keep the bare `extern` so the vptr-middle manual stamps still
compile as reloc-masked refs) → the compiler `??_7` wins the rva, the vtable is
realized, and the giant `@early-stop` factory functions that stamp it are untouched.**
No `new Class` rewrite, no ctor regression, no dup-DATA (the pin is gone). This
sidesteps the vptr-middle wall entirely: the vtable realizes even though the factory
keeps its faithful manual stamp.

**Litmus test per candidate (cheap):** delete the pin, `gruntz build`, `grep <rva>
build/gen/symbol_names.csv`. If it shows `??_7X` → realized, keep. If it shows nothing
(UNNAMED) → the `??_7` is NOT emitted anywhere; restore the pin (removing it only
unnames the rva — a net loss).

| Family | Realized (pin removed) | KEPT (??_7 not emitted → pin restored) |
| :-- | :-- | :-- |
| CSBI_* hierarchy | `CStatusBarItem` 0x1eabcc, `CSBI_RectOnly` 0x1eab8c, `CSBI_MenuItem` 0x1eab4c, `CSBI_Image` 0x1eac0c, `CSBI_ImageSet` 0x1eac4c, `CSBI_WellGoo` 0x1eadfc (the 6 base/intermediate; the 6 leaves — StatzTabArrow/StatzTabGruntBar/WarlordHead/ImageSetAni/GruntMachine/SideTab — were already realized) | — |
| game-state (GruntzMgrTransition/CMulti) | `CState` 0x1ea21c, `CAttract` 0x1ea194, `CMenuState` 0x1e9e84, `CBootyState` 0x1e9cec, `CMultiBootyState` 0x1e9bdc | `CMulti` 0x1e9fe4, `CPlay` 0x1ea0bc, `CDemo` 0x1e9f0c, `CHelpState` 0x1e9dfc, `CSplashState` 0x1e9d74, `CCreditsState` 0x1e9c64 |
| app framework (BoundaryLowerThunks) | `CGameApp` 0x1e9b0c, `CGameWnd` 0x1ea344 | `CGameMgr` 0x1e9b8c |
| misc (in scope) | — | `CDoNothingNormal` 0x1e859c (CUserLogic-derived, empty out-of-line dtor does NOT force the vtable COMDAT the CSBI chains do), `g_vtbl_1396f0` 0x1ef740 (anonymous, no RTTI) |

**Menu item vtables — KEPT-foreign (unchanged).** `g_menuItemVtbl` 0x1f08c0 /
`g_menuItem2Vtbl` (`Vtbl_1f08f8`) 0x1f08f8 are ANONYMOUS (no RTTI COL → not in
vtable_names.csv), their slots point at the CMenuItem TU (0x184670+, unmatched), and
they are already un-pinned `extern`s (no `g_*Vtbl` DATA symbol to remove). Realizing
would require modeling CMenuItem/CMenuItem2 fully AND inventing config/vtable_names.csv
entries for a non-RTTI vtable — out of a matcher's remit; deferred to a dedicated pass.
`MenuItemVtbl` (MainMenuBuilder.cpp) is NOT a stamp at all — it is the recommended
typed-vtable-dispatch struct reading a FOREIGN item's slot; nothing to convert.

**Already-realized (no action):** `??_7CGruntzMapMgr` 0x1e9bb4 (RezSync's `g_vtbl5e9bb4`
is a bare extern), `??_7CSBI_SideTab` 0x1eae3c (SBI_SideTabBuild's `g_vtbl_sideTab` is a
bare extern).

### Rules learned / reinforced (Batch 5)

- **Pin-removal is the low-risk realization lever for the manual-stamp facet.** When a
  class's `??_7` already emits (its dtor chain / a sibling TU forces the COMDAT), the
  manual `g_*Vtbl` `DATA` pin is the ONLY blocker; removing just the pin realizes the
  vtable and leaves the vptr-middle `@early-stop` factory codegen byte-for-byte intact.
- **CUserLogic-family bases don't auto-emit like the CSBI chain.** An empty out-of-line
  leaf dtor (`~CDoNothingNormal(){}`) does NOT force the `??_7` COMDAT the way a
  multi-virtual `/GX` dtor chain (CSBI, with its 11-slot base + restamp walk) does — so
  CDoNothingNormal/CGameMgr/CMulti/CPlay stay pinned (removing the pin only unnames the
  rva). The determinant is whether `cl` actually emits the `??_7` in some base OBJ.
## Batch 5 (CDDraw worker / sub-mgr cluster — VTBL + slot-RVA retrofit) — 2026-07-02

The worker family (`CDDraw*` / `CSeverusWorker*` / `CAniElement` / `CAniRecord` /
Albus/Leaf/Catalog/`CDDrawSubMgr*`). These classes were ALREADY real-polymorphic
(prior batches removed the `m_vptr = &g_*Vtbl` stamps); the residual work was
(A) naming the compiler-emitted `??_7` so it pairs with the retail vtable datum,
via a `VTBL(Class, 0x<rva>)` catalog macro, and (B) renaming the anonymous
`SlotNN` / `vNN` / `Vslot*` placeholder virtuals to their real slot RVA
(`FUN_00<va>`, read from the retail `.rdata` vtable data). Slot RVAs dumped from
`$GRUNTZ_EXE` `.rdata` (VA = RVA + 0x400000).

### VTBL macros added (compiler-emitted `??_7` now named; UnknownVTables placeholders removed)

| Class | vtable RVA | slots | TU |
| --- | --- | --- | --- |
| `CDDrawSubMgrLeaf` | `0x001efc78` | 9 | CDDrawSubMgrLeaf.cpp |
| `CDDrawSubMgrLeafScan` | `0x001efca0` | 9 | CDDrawSubMgrLeafScan.cpp |
| `CAniRecordPrimary` | `0x001f02c0` | 5 | CAniRecord.cpp |
| `CAniRecordBase2` | `0x001f02d8` | 14 | CAniRecord.cpp |
| `SiriusWorkerObj` | `0x001efb80` | 10 | CDDrawWorkerCache.cpp |
| `CDDrawWorkerMapSmall` | `0x001efcc8` | 13 | CDDrawWorkerMapSmall.cpp |
| `HagridWorkerB` | `0x001efed0` | 14 | CDDrawWorkerList.cpp |
| `CAniElementObj` | `0x001efba8` | 5 | CDDrawSubMgrAni.cpp |
| `SeverusWorkerBase` | `0x001efc30` | 9 | CDDrawWorkerRegistry.cpp |
| `SeverusWorkerObj` | `0x001efbe8` | 17 | CDDrawWorkerRegistry.cpp |

- `SeverusWorkerObj` needed **2 extra declared-only slots** (`FUN_005522b0` [15],
  `FUN_005523b0` [16]) so its emitted vtable size (17 slots) matches retail; the
  model had truncated at 15.
- `HagridWorkerA` (0x1efea0, 12 slots) shares the 14-slot `HagridWorker` base, so its
  emitted vtable carries 2 spurious slots → **skipped VTBL** (size mismatch). Splitting
  A off a 12-slot base is a separate refactor.
- Two vtables are cross-modeled in two TUs (same retail address): `0x001efbe8`
  (`SeverusWorkerObj` in Registry ↔ `CSeverusEntryList`) and `0x001f02d8`
  (`CAniRecordBase2` in CAniRecord ↔ truncated `AlbusWorkerObj` in WorkerMapSmall).
  VTBL is placed on the **fuller** model of each; the other TU is left un-VTBL'd to
  avoid a keep-last conflict.

### SIZE(exact) set from the `operator new(0xNN)` / layout

`SiriusWorkerObj` 0x17c, `AlbusWorkerObj` 0x14, `HagridWorkerA/B` 0x7c,
`SeverusWorkerObj` 0x6c, `CAniElementObj` 0x28.

### Worklist slot stubs added (RVA-bound `@stub`; slot binds into the `??_7`)

- `CDDrawSubMgrLeaf::FUN_00552640` (0x152640, 6 B) — CatalogVtbl slot [6].
- `CDDrawWorkerMapSmall::FUN_00556db0` (0x156db0, 6 B) — slot [6].

### Dead `g_*Vtbl` externs removed (class already real-polymorphic)

- `CSeverusEntryList.cpp`: `g_severusEntryListVtbl` + `g_severusBaseDtorVtbl` (both
  unused after realization; 0x1efbe8 now named via `SeverusWorkerObj`'s VTBL, 0x5e8cb4
  still bound in CSeverusWorkerEh.cpp).

### KEPT-hand-rolled / DEFERRED (genuine walls — not converted this batch)

- **`LeafElementObj` (`CDDrawSubMgrLeafScan.cpp`)** — the factory (0x157d70, 99.81%)
  inline-constructs the 0x1c element with a SINGLE vptr write (raw `operator new` +
  manual field seed + `StampLeafElemVtbl`). A real `new LeafElementObj` would emit the
  base-then-derived double vptr write → regress the 99.81% factory. Kept `g_leafElemVtbl`
  + the base `g_remusBaseDtorVtbl` stamp.
- **`CSeverusWorkerEh.cpp` / `SeverusWorkerDtor.cpp`** — dtor-only / inline-embed
  models. The base is modeled with the C++ `~` as the *only/first* virtual, so `cl`
  emits a dtor-at-slot-0 vtable (retail is a CObject-style slot-0-thunk / slot-1-dtor
  layout) → a real VTBL would mis-size/mis-lay the emitted `??_7`. The `g_severusEmbedVtbl`
  / `g_severusWorkerVtbl` / `g_severusBaseDtorVtbl` externs stay as reloc-masked target
  naming; documented @early-stop.
- **`CDDrawSubMgr::CreateObject_159600` (wwd factory)** — `RezAlloc(0x1dc)` raw-alloc
  factory stamping `g_wwdGameObjectVtbl` / `g_wwdObjFinalVtbl` mid-construction; already
  @early-stop-deferred for the wide-object-ctor / class-`operator new` sweep.
- **`CDDrawPtrCollections` `CPoolItemAVtbl`** — a pointer-to-member vtable-as-struct
  (foreign vtable owned/dispatched dynamically); not a removable manual stamp.
- **Runtime dispatch views** (`CDDAttachedSurface` v00..v10 in CDDrawSurfacePair,
  `UnknownSeverusVtableView` in CDDrawWorkerRegistry) — the concrete class/vtable RVA
  is not statically known, so the `vNN`/`SlotNN` names have no RVA to bind to; left.

### Rule learned (Batch 5)

- **A `VTBL(Class, rva)` only helps when the emitted vtable is the RIGHT SIZE.** For a
  shared-base worker (`HagridWorkerA` vs `HagridWorkerB`) or a truncated model
  (`SeverusWorkerObj` at 15 vs 17), the base's slot count leaks into the derived
  `??_7`; add the missing trailing declared-only slots (size-match) or skip VTBL.
- **A dtor-only polymorphic base emits a dtor-at-slot-0 vtable**, which does NOT match a
  CObject-style slot-0-thunk / slot-1-dtor retail layout. To realize such a base
  correctly, model slot 0 as a regular `virtual void FUN_005bef01()` and slot 1 as a
  regular `virtual ScalarDtor(i32)` (the `CDDrawSubMgrLucius` pattern), not a bare C++ `~`.

## Batch 6 (iteration 2) — CDDraw Severus/Leaf walls FORCE-realized (irregardless of %)

USER MANDATE: "attack until ALL consumed; ignore incorrect loads; implement anyway."
The Batch-5 KEPT/DEFERRED walls below were force-realized, accepting the regressions.
Vtable NAMES are reloc-masked (matching-neutral), so a `VTBL(Class, rva)` never hurts
CODE matching even when the emitted `??_7` is mis-laid/mis-sized vs retail — it only
makes the cosmetic `vtables`-unit data symbol imperfect (tracking, not a match lever).

### REALIZED (placeholder + `g_*Vtbl` DATA-pin removed; `VTBL(Class, rva)` added)

- **`CSeverusWorkerX` @0x1f07d8** (`SeverusWorkerDtor.cpp`) — class was already real-
  polymorphic (dead `g_severusWorkerVtbl` DATA-pin). Swapped DATA-pin → `VTBL`. The
  emitted `??_7` is dtor-at-slot-0 (base's only virtual) vs retail's CObject slot-0-thunk
  layout — mis-laid but matching-neutral (accepted). Same datum is CFader17e940Sub.
- **`CSeverusWorkerHost` @0x1f0270** (`CSeverusWorkerHost.cpp`, Vtbl_1f0270) — already
  real-polymorphic (CSeverusBase-derived, virtual dtor 0x163af0). Dead
  `g_severusWorkerHostVtbl` DATA-pin → `VTBL`. Mis-laid emit, matching-neutral.
- **`CSeverusWorker3` @0x1effa0** (`CDDrawSubMgr.cpp`, Vtbl_1effa0) — the 3-arg leaf-
  worker ctor (0x158f30) manual-stamped `g_severusWorker3Vtbl`. Converted to a single
  `virtual v0()` so `cl` auto-stamps the vptr in the ctor prologue (vptr-middle→vptr-
  first regression: ctor 100%→96.3%, accepted). `VTBL(CSeverusWorker3, 0x001effa0)`.
- **`LeafElementObj` @0x1eff08** (`CDDrawSubMgrLeafScan.cpp`, LeafElemVtbl) — the 0x1c
  cache element, modeled real-polymorphic: `LeafElementBase` is now a CObject-style
  5-slot grand-base (slot-0 thunk / slot-1 virtual dtor), `LeafElementObj` adds 4 leaf
  virtuals (declared-only) + the virtual dtor. `MakeLeafElement` rewritten to `new
  LeafElementObj(count,handle)` (real ctor auto-stamps; the raw-alloc null-merge factory
  reshaped: CreateEntry/CreateEntry2 99.81%→63.3%, accepted). `~LeafElementObj` auto-
  stamps both vptrs; the manual `g_leafElemVtbl` + base `g_remusBaseDtorVtbl` stamps are
  gone from this TU. Emitted `??_7` matches retail's 9-slot CObject layout.
- **`CDDrawWorkerCache` @0x1efd00** (`CDDrawWorkerCache.cpp`, Vtbl_1efd00) — was a
  manual-vptr stub (explicit `void* m_vptr`). Converted to a full 10-slot CObject-style
  polymorphic class (slot-0 thunk / slot-1 dtor / … / slot-8 VM20 / slot-9 VM24). The
  matched `VirtualMethodUnknown20` (0x1576f0) STAYED 100% (trivial body, virtual-neutral);
  only the stub dtor/VirtualMethod_157720 regressed (were ~0%). Zero exact-fn cost.
- **`CDDrawSubMgrDraco` @0x1efe08** (`CDDrawSubMgrDraco.cpp`, Vtbl_1efe08) — manual-vptr
  stub → full 10-slot CObject polymorphic class; `Stub_1574b0` (0x1574b0) is now the
  virtual dtor. Both matched methods `VirtualMethodUnknown14` (0x157480) /
  `VirtualMethodUnknown1C` (0x158ac0) STAYED 100% (bodies don't dispatch on `this`);
  only the 3 backlog stubs regressed. Zero exact-fn cost.

### Catalog split-view artifacts removed (already-realized single vtables)

- **Vtbl_1efe3c** — the scanner cut CFileMem's one 13-slot CFile-style vtable at +0xc
  (0x5efe30→0x5efe3c). Already `??_7CFileMem` (VTBL 0x5efe30, `src/Io/FileMem.cpp`).
- **Vtbl_1efe74** — the +0xc cut of CFileMemBase's 13-slot vtable (0x5efe68→0x5efe74).
  Already `??_7CFileMemBase` (VTBL 0x5efe68). Neither is a separate class.

### REMAINING in scope (genuinely-hard — full large-class reconstruction / final sweep)

- **`g_severusEmbedVtbl` @0x1e971c** — the CSeverusEmbed subobject's OWN vtable is a
  **CArray template** (`??_7?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@6B@`, already the
  authoritative name in `config/vtable_names.csv`). It lives inside the >0x8694-byte
  CSeverusWorker modeled with the manual-vptr device to avoid divergent vtables; the
  worker dtor is @early-stop on the eh-dtor-inline-member wall. VTBL cannot emit a
  template vtable, and converting the worker to real-polymorphic would reshape its whole
  ctor/dtor. The g_severusEmbedVtbl DATA-pin is a redundant alias of the CArray name.
- **`g_severusBaseDtorVtbl` / `g_remusBaseDtorVtbl` @0x1e8cb4** — the shared CObject-like
  grand-base, ALREADY `??_7CObject@@6B@` in vtable_names.csv (authoritative). The
  remaining aliases are referenced by mid-dtor base-restamps in classes still modeled
  with the manual-vptr device (`CFaderMgr.h`, `CSeverusWorkerHost.h` CSeverusWorker::~,
  `CSeverusWorkerEh.cpp` CSeverusEmbed, `CDDrawSubMgr.cpp` CSeverusWorkerBase::Init).
  Removing them requires modeling each host as real CObject-derived (implicit base
  re-stamp) — reshapes several @early-stop eh-dtors. Datum is already correctly named.
- **Vtbl_1efd28 @0x5efd28** (23 slots) — CDDrawWorkerRegistry's OWN vtable (slot-1 =
  `Stub_156df0`). The registry is a large map-manager with many matched factory/scan
  methods; a 23-slot polymorphic conversion risks those matches (final sweep).
- **Vtbl_1efdc0 @0x5efdc0** (17 slots) — a SECOND vtable whose slot-1 dtor is
  `CDDrawWorkerMapSmall::Stub_157610` (its primary is already `??_7CDDrawWorkerMapSmall`
  @0x1efcc8). Needs multiple-inheritance / distinct-class modeling.
- **Vtbl_1efc58 @0x5efc58 (dtor 0x155890), Vtbl_1efd88 @0x5efd88 (0x156f30),
  Vtbl_1eff30 @0x5eff30 (0x1590d0), Vtbl_1eff70 @0x5eff70 (0x159190)** — the owning
  classes' dtors are NOT reconstructed anywhere in `src/` (no construction/dtor site to
  rewrite into a real `new`+ctor). Not realizable without first reconstructing the class
  body (final sweep).
- **Vtbl_1effd0 @0x5effd0 (= `g_wwd159250FinalVtbl`)** — a WWD factory ORPHAN: the
  `CWwdObjMgrFactories` RezAlloc factory manual-stamps it mid-construction and cannot
  reference a `cl`-emitted `??_7` without a `new`-based wide-object-ctor rewrite (the
  same documented VTable47-50 orphan case). Deferred to the wide-object-ctor sweep.
