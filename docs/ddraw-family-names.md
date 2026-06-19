# DDraw surface-family class names (was: `Unknown*` Harry-Potter codenames)

The `Unknown*` "Harry Potter family" has **no RTTI** (see
`docs/unknown-classes-investigation.md`), so these names are **role inferences
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
| `UnknownRemus` | `CDDrawLevelData` | WWD/level-geometry data (loads `.wwd`, coord/extent, child arrays) |
| `UnknownHermiona` | `CDDrawChildGroup` | broadcasts virtual calls across an intrusive child list |
| `UnknownHagrid` | `CDDrawWorkerList` | factory; appends 0x7c-byte workers to a `CObList` |
| `UnknownAlbus` | `CDDrawWorkerMapSmall` | factory; `CMapStringToOb` of 0x14-byte workers keyed by string |
| `UnknownSeverus` | `CDDrawWorkerRegistry` | `CMapStringToOb` registry; keyed remove/destroy + worker ops |
| `UnknownSirius` | `CDDrawWorkerCache` | factory; `CMapStringToOb` of 0x17c-byte workers keyed by string |
| `UnknownPettigrew` | `CDDrawSubMgrLeaf` | leaf readiness-predicate manager |
| `UnknownFilch` | `CDDrawPtrCollections` | standalone holder of two `CPtrList` + a `CPtrArray` |
| `UnknownMinerva` | `CDDrawMapHolder` | map holder (`ClearUnknownMap`) |
| `UnknownDraco` | `CDDrawSubMgrDraco` | leaf, role not yet identified (keeps the codename hint) |
| `WorkerA` / `WorkerB` | `CDDrawWorkerA` / `CDDrawWorkerB` | 0x7c-byte worker elements created by `CDDrawWorkerList` |

Not renamed here: `UnknownSalazar` / `UnknownVoldemort` — they are **Dsndmgr**
helpers, not this family (moved in a follow-up). File and unit names still use the
old codenames; realigning files/units AND fixing the method-level mislabeling
(some real vtables carry methods tagged under several of these codenames — see the
investigation doc) is the bigger follow-up.
