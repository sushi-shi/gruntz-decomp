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
