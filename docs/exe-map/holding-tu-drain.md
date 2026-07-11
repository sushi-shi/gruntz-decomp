# Package: Holding-TU drain — split by RVA gap AND chase the owner

**Doctrine (user, 2026-07-11): DO BOTH.**
1. **Split by RVA gap — always, never blocks.** Each original `.obj` is one
   contiguous strictly-ascending `.text` run (measured layout ground truth). A
   `.cpp` whose functions sit in ≥2 widely-separated RVA blocks conflates ≥2 objs.
   Split it so each contiguous block is its own `.cpp`. Name each split file by its
   RVA / registry-table address (e.g. `ConstructActRange_646188`) — RVA-address
   names are legitimate, they invent nothing.
2. **Chase the owner class where the xref cracks it — bonus, never required.**
   After (or alongside) the split, try to pin the real owning class via caller
   `this`, vtable DATA-ref, RTTI, field-usage. If it cracks: rename the split file +
   its functions to the real class, dissolve any fake view that was standing in for
   it ([[xref-identity-recovery-over-fake-views]]). If it does NOT crack: keep the
   RVA-address name and move on — the structural win (correct placement) already
   landed. Do not fabricate a placeholder class to "resolve" identity.

**Per-cluster classification (do this before splitting a cluster off).**
For each non-main cluster in a flagged file, decide which it is:
- **(a) pooled member → LEAVE in place.** COMDAT-pooled dtor (`??1`/`??_G`/`??_E`/
  ScalarDeletingDtor), GetTypeTag/Vslot/Serialize virtual, or a CRT dynamic-init
  fragment (`_$E`/`??__E`/`InitStr…`/small `Register…`/`Construct…` in the
  0x082000–0x08d000 init pool that inits THIS TU's own globals). These are linker-
  pooled away from the TU block by construction; they are NOT conflation. The
  gametext lesson: one `_$E<n>` thunk heading a contiguous init run == one obj.
- **(b) foreign obj → SPLIT out.** A contiguous run of real methods belonging to a
  different class than the file's main cluster (e.g. CState methods inside Play.cpp,
  CBrickzGrid inside Brickz.cpp). Carve into its own RVA-named `.cpp`, then chase
  its owner.

Gate on BUILD INTEGRITY only; take any %-hit (structure-recovery phase). Never
`git stash` in the shared worktree. Absolute paths only.

## Worklist — genuinely-conflated files (pooled fns already excluded)
Source: `docs/exe-map/flags.json` "conflated" set. Re-run
`python3 docs/exe-map/flag_outliers.py` after each landing to re-rank.

Pure holding TUs (every fragment is a foreign registrar pair → split each pair out):
- LogicActRegistrars.cpp  — Construct+Register pairs @0xb15b0 / 0xb3ae0 / 0x119fa0
- LogicActReg.cpp         — `_62bfa0` pair @0x3a530 + `_646010` pair @0xadde0
- AppHelpers.cpp          — 3 orphans: CHandlerB4::Handle, Unmatched_be030, CTitleApp::OnStart
- ResourceLoaders.cpp     — PalHost::Apply @0x1775f0 + PalCache::Snapshot @0x17cd90

Conflated real TUs (main cluster stays; classify + carve the foreign clusters):
- Attract.cpp (5 clusters)  GameApp.cpp  MenuState.cpp  Brickz.cpp
- LogicWorkerHandlers.cpp   TileLogicPump.cpp  GameWnd.cpp  CImage.cpp
- GruntzMgr.cpp  RezMgr.cpp  Play.cpp  WwdSpatialMgr.cpp

## Success metric
`frags` (RVA-contiguity) per file → 1 in the CLEAN view (`scatter_methods.json`,
which excludes pooled dtors + init fragments). Conflated count in flags.json → 0.
