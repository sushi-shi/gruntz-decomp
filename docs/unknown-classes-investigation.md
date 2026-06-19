# What the `Unknown*` classes are (RTTI investigation)

Reproduce: `nix develop --command python3 scripts/analysis/rtti_classify.py [--orphans|--vtable 0x..]`

## TL;DR

GRUNTZ.EXE ships **231 RTTI class names** (`.?AV<Class>@@`). For a polymorphic
class *with* RTTI the real name is recoverable through
`vtable[-1] → CompleteObjectLocator → TypeDescriptor → name`. The project already
uses this — almost every `src/Stub/` name (`CActionArea`, `CBoomerang`,
`CExplosion`, `CGiantRock`, `CRollingBall`, `CSpotLight`, `CUFO`, `CWayPoint`,
`CTile*TriggerSwitchLogic`, `CSBI_*`, …) is an RTTI-derived **real** name.

**The `Unknown*` "Harry-Potter-codename" classes are exactly the classes that
have NO RTTI** — every one of their vtables has `vtable[-1] == 0` (no
complete-object-locator), and none of their names appear among the 231. They come
from a module compiled **without `/GR`** (the leaked `C:\Proj\DDrawMgr` surface/
page-manager family). So **RTTI cannot name them** — that is *why* they are
"Unknown", and it is a dead end to look for their names in the 231.

What we *can* establish (below): they are one **CGruntzMgr DDraw sub-manager
hierarchy**, factory-constructed; the codenames are applied inconsistently across
the real vtable boundaries; and two codenames (`Salazar`, `Voldemort`) are
mis-grouped — they belong to the **Dsndmgr** (sound) module.

## Evidence

`rtti_classify.py` parses RTTI (231 type descriptors, 246 complete-object-locators,
**222 vtables with RTTI**) and cross-checks each `Unknown*` placeholder:

```
UnknownClassCGruntzMgrHarryPotter  vt@0x1efc58  RTTI=NONE  stamped-by: ??0...HarryPotter ctor
UnknownCGruntzMgrLucius            vt@0x1efc30  RTTI=NONE
UnknownAlbus                       vt@0x1efcc8  RTTI=NONE  stamped-by: HarryPotter::UnknownVirtualMethod18
UnknownMinerva                     vt@0x1efca0  RTTI=NONE  stamped-by: HarryPotter::UnknownVirtualMethod18
UnknownHagrid / UnknownHermiona    vt@0x1efd88  RTTI=NONE  stamped-by: HarryPotter::UnknownVirtualMethod18
UnknownSeverus / UnknownSirius     vt@0x1efd00  RTTI=NONE  stamped-by: HarryPotter::UnknownVirtualMethod18
UnknownDraco                       vt@0x1efe08  RTTI=NONE  stamped-by: HarryPotter::UnknownVirtualMethod18
UnknownPettigrew                   vt@0x1efc58  RTTI=NONE
UnknownRemus                       vt@0x1f0150  RTTI=NONE
WorkerA / WorkerB                  vt@0x1efea0  RTTI=NONE  stamped-by: UnknownHagrid::VirtualMethodUnknown24
UnknownSalazar                     vt@0x1ef6b8  RTTI=NONE  stamped-by: DirectSoundMgr::ErrorThunk_1351d0
UnknownVoldemort                   vt@0x1ef6e0  RTTI=NONE  stamped-by: FUN_1375b0 (Dsndmgr region)
```

### 1. It is one polymorphic hierarchy with a shared base
Every family vtable in `0x1efc30 … 0x1f0150` begins with the *same* 5-slot
prologue: `[vector-deleting-dtor FUN_1bef01, <class dtor>, FUN_0028ec, FUN_00106e,
FUN_004034]`. The three shared thunks are a common base subobject — i.e. all of
these classes derive from one base (the project already models this as
`UnknownCGruntzMgrHogwarts` → `UnknownCGruntzMgrLucius` → the leaf managers).

### 2. It is a factory-built sub-manager set of CGruntzMgr
`UnknownClassCGruntzMgrHarryPotter::UnknownVirtualMethod18` is the **factory** that
stamps the vtables of Albus / Minerva / Hagrid·Hermiona / Severus·Sirius / Draco.
The root class is literally codenamed `...CGruntzMgrHarryPotter` and the base
`...CGruntzMgrHogwarts` / `...CGruntzMgrLucius` — i.e. the project already reads
these as a `CGruntzMgr` (which *does* have RTTI: `.?AVCGruntzMgr@@`) sub-system,
the DDraw surface/page managers.

### 3. The codenames do NOT line up with the real classes
Walking the vtables (`--vtable 0x..`) shows single real vtables whose methods were
tagged under several different codenames, and adjacent real classes my earlier
crude walk had merged:

- `vt@0x1efd88` second sub-vtable: methods tagged **Hermiona + Albus + Lucius** — one class, three codenames.
- `vt@0x1efd00`: **Severus** and **Sirius** labels interleave across its two sub-vtables.
- `vt@0x1efc58`: `HarryPotter` (sub-vt 1) and `Pettigrew` (sub-vt 2) are **distinct adjacent** classes, not one.
- `vt@0x1efea0`: `WorkerA` and `WorkerB` are **two distinct adjacent** vtables, not one "Worker".

The real class boundary is each `FUN_1bef01`-headed prologue; the codenames should
be realigned to those boundaries (future cleanup).

### 4. Two codenames are mis-grouped (they are Dsndmgr, not the HP family)
`UnknownSalazar` (`vt@0x1ef6b8`) and `UnknownVoldemort` (`vt@0x1ef6e0`) sit in the
`0x1ef6xx` / `0x135xxx`–`0x137xxx` region and are stamped by **`DirectSoundMgr`**
code. Salazar's leaves are pure math curves (pow/acos/sqrt lookup tables) — sound
attenuation/scaling, not the DDraw family. They belong in the `dsndmgr` unit.

## RTTI orphans (real names still to reconstruct)
191 of the 231 RTTI names have no matched function in `src/` yet. Most are MFC/CRT
library classes we don't reconstruct (`CDC`, `CWnd`, `CDialog`, `ios`, `streambuf`,
`type_info`, …). The **game-specific** orphans are the real-name targets — and many
already exist as `src/Stub/` files named straight from RTTI. (`--orphans` lists all.)

## Recommendations (follow-ups, not done here)
1. Realign the HP-family codenames to the real vtable-prologue boundaries (merge the
   over-split ones; split the interleaved Severus/Sirius). Mechanical but touches
   matched code, so do it as its own verified pass.
2. Move `UnknownSalazar` / `UnknownVoldemort` into the `dsndmgr` unit.
3. Keep mining RTTI for the remaining game-specific orphan names (already the source
   of the stub backlog names) — but accept the HP family will stay codenamed: it has
   no RTTI to recover.
