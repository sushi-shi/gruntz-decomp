# src/Gruntz TU topology — audit + reorg plan (2026-07-04)

Evidence-driven recovery of the real TU structure. Method: per-TU retail RVA
bands (`build/gen/symbol_names.csv` × `config/units.toml`, 393 Gruntz units)
joined with retail `__FILE__`/RTTI strings, per-function class ownership, and
`docs/link-order-investigation.md` + `docs/tu-spatial-structure.md`.

**Two structural facts drive everything:**
1. Retail is banded low→high: game code `0x07000–0x130000`, engine library
   block `0x133000–0x18b440` (zlib intermixed at `0x185320+`), then a
   registry/COM helper cluster `0x1bf000–0x1d5000`. Engine-module medians sit
   ≥0x130000; the Gruntz-module median is 0x0b4d30. **A src/Gruntz TU living
   entirely ≥0x133000 is prima-facie mis-filed engine code** (76 such TUs).
2. Destructors are COMDAT-pooled — a lone far-RVA dtor does NOT make a TU an
   aggregate. The true aggregate signal is mass spread across many distinct
   classes (verified per-function, not by gap count).

## Categories

**A1 — healthy real game TUs (keep):** Grunt.cpp (89 fns), GruntzMgr.cpp (93,
string-confirmed `C:\Proj\Gruntz\GruntzMgr.cpp`), GameLevel (60), GameMode
(44), Play (71), TriggerMgr (51), Dialogs (42), SBI_RectOnly (70), Brickz,
BattlezData (30), ChatBox, Attract, GruntSpawnConfig, …

**A2 — pure aggregate buckets (NOT real dev TUs; dissolve per-fn):**
BoundaryLowerMethods (30 fns / 30 distinct placeholder classes),
BoundaryLowerThunks (26), BoundaryUpper (33, engine-band),
BoundaryUpper2 (27), BoundaryUpperEh/Upper2Eh, BoundaryMisc, BoundaryThunks,
BoundaryLowerDtors, BoundaryTail, BoundaryLeafLogic, BoundaryTailEh,
ReconBatch2 (19) + ReconBatch2O1 (5), DiscoveredSmall (10 unrelated fns
binary-wide), DiscoveredEh, OrphanLeaves, OrphanMethods, ApiWrappers,
ApiHiCallers, ApiMiscHelpers (+ src/Stub/ApiCallers.cpp).

**A3 — real single-class TUs under artifact names (rename/merge, not
dissolve):** UnknownClassArrays.cpp = **41 fns, all CBattlezMapConfig**;
UnknownClassInCGruntzMgr.cpp = CGruntSpawnLevel ctor+dtor;
Obj5f0890Dtor.cpp = ~CFader1816c0; StateLeaf8cf30.cpp = ~CHelpState + vtbls;
ArrayE40.cpp / DiscoveredArray.cpp = MFC CArray instantiations.

## Original-TU names recovered from retail strings

Assert `__FILE__` strings (sparse) + the ~230-entry RTTI roster:

| leaked dir | original file | our TU | status |
|---|---|---|---|
| DDrawMgr\ | DDRAWMGR.CPP | DDrawMgr/DirectDrawMgr.cpp | ✓ |
| DDrawMgr\ | DIRPAL.CPP | **src/Gruntz/DirPal.cpp** | ✗ mis-filed → DDrawMgr |
| DDrawMgr\ | DIRSURF.CPP | split across DDrawSurface*/Worker* | ~ (Gruntz/DDScreen, DDPageMgr, DDSurfaceDtor likely belong) |
| DinMgr2\ | DinMgr2.cpp | DinMgr2/DirectInputMgr2.cpp | ✓ |
| DinMgr2\ | InputDevice.cpp | — | lacking (≠ Gruntz/InputDeviceConfig.cpp, which is game code) |
| Dsndmgr\ | DSNDMGR.CPP | Dsndmgr/DirectSoundMgr.cpp | ✓ |
| Dsndmgr\ | DSndMgSR.cpp | — | lacking (snd save/restore; candidates: Gruntz/SoundStream{Free,Teardown}, SoundResMap) |
| Gruntz\ | GruntzMgr.cpp | Gruntz/GruntzMgr.cpp | ✓ |
| NetMgr\ | NetMgr.cpp | Net/NetMgr.cpp | ✓ |

RTTI confirms the WAP32 engine framework classes as distinct types (CWapObj,
CWapX, CGameApp/Mgr/Wnd, CUserBase, CUserLogic, CMovingLogic, CDoNothing*),
several currently under src/Gruntz.

## Move-to-engine candidates (class @ band evidence)

| src/Gruntz TU | → module |
|---|---|
| DirPal.cpp (string-confirmed) | DDrawMgr |
| DDScreen.cpp, DDPageMgr.cpp, DDSurfaceDtor.cpp | DDrawMgr |
| LutShadeRect, PolyClipRaster, WarpTextureBlit, CircleShadeBlit, ColorHsv | DDrawMgr (some maybe Image) |
| Fader*.cpp (10 TUs, contiguous 0x17d8f0–0x182610) | DDrawMgr (verify game-vs-engine) |
| Wwd{GameObject,GameObjectEh,ObjMgr,ObjMgrFactories,SpatialMgr,Grid}.cpp | Wwd |
| SoundStreamFree, SoundStreamTeardown, SoundResMap | Dsndmgr |
| ButeSectionCtor.cpp | Bute |
| RezBufferObjectDtor.cpp, RezSync.cpp | Rez |
| WapUncompress.cpp, Misc18.cpp, BitStreamPack, BitStreamBlowfish | Wap32/Crypto (compression) |
| FecCrypt.cpp | Net or Crypto |
| UserBaseLink, UserLogic*, MovingLogic*, DoNothing* | Wap32 (engine logic framework; verify — derived logics are game) |
| SmackerVideoWindow.cpp, MoviePlayer.cpp | Io/video (engine) |

NOT moves: ConfigStore*/FilePathInfo/RegHelpers @0x1bf000–0x1d5000 are
double-claimed NAFXCW library bytes → P0 library reconciliation (externs, not
TUs).

## Merge candidates (one class, one band, over-split)

- UnknownClassArrays.cpp (41) + BattlezMapConfig.cpp → **BattlezMapConfig.cpp**
- MovingLogic{Ctor,Dtor,Update,Serial}.cpp → MovingLogic.cpp
- DoNothing.cpp + DoNothingNormal{Dtor,Logic}.cpp → DoNothing.cpp
- ConfigStore{,Read,Write}.cpp → ConfigStore.cpp (then library-externed per P0)
- Fader.cpp + Fader{Run,Radial,ElemSetup,ShapeSetup,TileRender,17e940Apply}.cpp
  + Obj5f0890Dtor.cpp → Fader.cpp (+ FaderMgr.cpp)
- StateLeaf8cf30.cpp + the CHelpState::LoadAssets fragment of
  BacklogStateLoaders.cpp → HelpState.cpp
- AssetNamespacePrefixes.cpp ↔ GameAssetNamespaces.cpp (verify first)
- **The `*Eh.cpp` flag-split family (22 TUs)**: each holds only a class's
  EH-frame members, split solely for the `/GX` profile; under `/GX` the plain
  methods are byte-identical → merge back into the base TU with `flags="eh"`.
  Pairs: every SBI_*+SBI_*Eh, WwdGameObject+Eh, AniElement+Eh, TriggerMgr+Eh,
  LightFx+Eh, MoviePlayer+Eh, BoundaryUpper*+Eh. Per-fn byte-verify each merge.

## Artifact-TU dissolution table

| TU | contents | verdict → destination |
|---|---|---|
| StateLeaf8cf30 | ~CHelpState + vtbls | rename/merge → HelpState.cpp |
| DiscoveredArray | CDwArray::SetSize (MFC CArray) | owner TU or MFC extern |
| DiscoveredEh | ~CU55 + ~CButeStore | split: Bute / owner |
| DiscoveredSmall | 10 unrelated fns | dissolve per-fn |
| FinalVtables | 6 anon vtables, data-only | each → its class; cosmetic host meanwhile |
| Misc18 | RecFree/CoderReset/BitFlush | → Wap32 compression |
| Obj5f0890Dtor | ~CFader1816c0 | → Fader.cpp |
| OrphanClassMeta | SIZE() macros only | redistribute to header owners |
| OrphanLeaves / OrphanMethods | unrelated leaves/methods | dissolve per-fn |
| UnknownClassArrays | 41× CBattlezMapConfig | rename+merge |
| UnknownClassInCGruntzMgr | CGruntSpawnLevel ctor+dtor | rename → GruntSpawnLevel.cpp |
| UnknownFileIOCtor | FileIOOwner ctor @0x8fea0 | → CFileIO (Io); verify |
| VtblForward | CVtblRecv::Accept | rename/home |
| ArrayE40 | CArrayE40::SetSize | owner TU |
| Api* (4 TUs) | Win32-API caller grab-bags | dissolve per-fn (docs/api-caller-name-plan.tsv) |
| ArraySerialize | TArray::Serialize | owner TU |
| AssetNamespacePrefixes | 1 fn, CNamespaceLoader | fragment → owner TU |
| BacklogStateLoaders | 3 unrelated state fns | dissolve per-fn |
| Boundary* (12 TUs) | proximity placeholders | Upper*→engine modules; Lower/Tail/Misc/LeafLogic/Thunks→game classes |

Same treatment: ClassUnknown9/30/43/48.cpp, Cluster0c.cpp, ChainForward.cpp,
Registry23.cpp, HelperHost.cpp, LeafElementObj.cpp, DataBuffer.cpp,
RangeSet.cpp, ScatterSamples.cpp, DebugPrintf.cpp, TypeKeyColl.cpp,
ReconBatch2*.cpp. Also fix: unit `minervainner` mis-bound to
src/Gruntz/SoundStreamTeardown.cpp (units.toml name mismatch).

## Target layout + execution order

src/Gruntz keeps only real game TUs (CGrunt, CGruntzMgr, CGameLevel, CGameMode,
CPlay, triggers, HUD/SBI, dialogs, states, brickz/battlez, grunt logic).
Engine framework → Wap32; world objects → Wwd; draw → DDrawMgr; sound →
Dsndmgr; rez → Rez; bute → Bute; compression → Wap32/Crypto; registry/profile
helpers → MFC library externs. The RTTI roster (~230 classes) is the
ground-truth reconciliation list.

Order (safe-mechanical → evidence-heavy), `gruntz build` green after each step:
1. **Same-class merges** (units.toml + RVA() moves; class stays one contiguous
   block, delinker packing undisturbed): *Eh merge-backs, MovingLogic*,
   DoNothing*, ConfigStore*, Fader*, UnknownClassArrays→BattlezMapConfig.
2. **Renames of mis-named real-class TUs** (UnknownClass*, Obj5f0890Dtor,
   StateLeaf8cf30) — RVA-owner renames interact with delinker section packing;
   build-check each.
3. **Whole-TU moves to engine modules** where class+band+string agree
   (DirPal→DDrawMgr, Wwd*→Wwd, SoundStream*→Dsndmgr, ButeSectionCtor→Bute,
   Rez*→Rez): units.toml `source=` edits.
4. **Per-fn dissolution of aggregates** (Boundary*, Discovered*, Orphan*, Api*,
   ReconBatch*): per-fn evidence (xref + vtable/RTTI; proximity is ~91%
   precise, not ground truth) before each fn graduates. Incremental, last.
5. P0 library reconciliation of ConfigStore*/FilePathInfo/RegHelpers runs as
   its own track (docs/cleanup-plan.md).
