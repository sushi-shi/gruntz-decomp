# DDraw surface-family class names (was: `Unknown*` placeholder codenames)

This family was originally reconstructed under throwaway placeholder class names
(the "Harry Potter" set); that left column below is kept only as the historical
old→new mapping record. The classes have **no RTTI** (their vtables carry no
complete-object-locator), so these names are **role inferences
from the matched code**, not recovered RTTI names — hence the deliberately
generic `CDDraw*` prefix (they are CGruntzMgr's DirectDraw surface/page
sub-manager subsystem). This was a mechanical, match-transparent rename: the
class change flows into both the recompiled base obj and the delinked target
symbol, so objdiff still pairs them — `213/742` byte-exact, **unchanged, no
regressions**.

| old codename | new name | role (from behaviour) |
|---|---|---|
| `UnknownClassCGruntzMgrHarryPotter` | `CDDrawSurfaceMgr` | root; owns one child-manager per slot (at `CGruntzMgr+0x30`); `…Method18` factory |
| `UnknownCGruntzMgrHogwarts` | `CDDrawSubMgrBase` | polymorphic base of the family |
| `UnknownCGruntzMgrLucius` | `CDDrawSubMgr` | shared base for the leaf managers |
| `UnknownRemus` | **`CGameLevel`** (merged into the `gamelevel` unit) | WWD/level-geometry data — its vtable carries `CGameLevel::LoadWwd` (slot 0x38), so it *is* `CGameLevel` (not the guessed `CDDrawLevelData`) |
| `UnknownHermiona` | `CDDrawChildGroup` | broadcasts virtual calls across an intrusive child list |
| `UnknownHagrid` | `CDDrawWorkerList` | factory; appends 0x7c-byte workers to a `CObList` |
| `UnknownAlbus` | `CDDrawWorkerMapSmall` | factory; `CMapStringToOb` of 0x14-byte workers keyed by string |
| `UnknownSeverus` | `CDDrawWorkerRegistry` | `CMapStringToOb` registry; keyed remove/destroy + worker ops |
| `UnknownSirius` | `CDDrawWorkerCache` | factory; `CMapStringToOb` of 0x17c-byte workers keyed by string |
| `UnknownPettigrew` | `CDDrawSubMgrLeaf` | leaf readiness-predicate manager |
| `UnknownFilch` | `CDDrawPtrCollections` | standalone holder of two `CPtrList` + a `CPtrArray` |
| `UnknownMinerva` | `CDDrawMapHolder` | map holder (`ClearUnknownMap`) |
| `UnknownDraco` | `CDDrawSubMgrPages` | slot `m_04`; builds+owns a fixed trio of geometry-init'd surface children (2 are `CDDrawSurfacePair`, 3rd optional via flag&1) sized to the display (w,h,bpp) |
| `WorkerA` / `WorkerB` (`Hagrid*`) | `CDDrawWorkerA` / `CDDrawWorkerB` | 0x7c-byte worker elements created by `CDDrawWorkerList` (vtables `??_7CDDrawWorkerA/B`) |
| `HagridChild` | `CDDrawWorkerItem` | ref-counted worker view walked by `CDDrawWorkerList`'s tick/teardown |
| `DracoWorkerB` (vtable 0x5eff30) | `CDDrawSurfacePair` | front/back surface-pair child (its own vtable = `CDDrawSurfacePair`) |
| `DracoWorkerA` (vtable 0x5eff70) | `CDDrawSurfaceChildA` | 0x30-byte type-A child of `CDDrawSubMgrPages` (identity uncertain) |
| `HarrySurface` | `CDDrawSurfaceSource` | lockable named source surface (Lock/Unlock/format-probe) cached by `CDDrawWorkerMapSmall` |

Files and units were renamed to match (e.g. `UnknownHagrid.cpp`/`unknownhagrid` →
`CDDrawWorkerList.cpp`/`cddrawworkerlist`; and `CDDrawSubMgrDraco.cpp`/`cddrawsubmgrdraco`
→ `CDDrawSubMgrPages.cpp`/`cddrawsubmgrpages`), and `UnknownRemus` was merged into the
existing `gamelevel` unit as `CGameLevel` (its methods now live in `GameLevel.cpp`,
using raw `this`-offset casts so the bytes are identical).

`UnknownSalazar` / `UnknownVoldemort` are **Dsndmgr** (DirectSoundMgr) helpers,
not this family: the matched Salazar unit was moved to the new `src/Dsndmgr/`
module dir; the Salazar/Voldemort backlog stubs stay in `src/Stub/` (the backlog,
next to the `DirectSoundMgr` stub — Voldemort has no matched code yet). Renaming
them off the HP codenames is an optional follow-up (Salazar builds a sound
volume/attenuation lookup table; Voldemort's role is not yet identified).

Still a follow-up: the method-level mislabeling
(some real vtables — e.g. the `0x1efd00` MI vtable — carry methods tagged under
several codenames; see the investigation doc).
