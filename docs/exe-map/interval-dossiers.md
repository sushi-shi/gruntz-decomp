# Interval dossiers — glued-TU resolutions

Resolves the ambiguous seam-glued/mixed merge candidates from `TU_MIGRATION.md`
(method: unit-block sequence from `exe_map range`, seam-function content/xref
audit, `__FILE__` anchors, CRT-init-table fragment runs from `deep_layout.json`).
Calibration cases: netmgr+font = TWO files; ddpalette+dirpal = ONE (DIRPAL.CPP);
directinputmgr2 = TWO (DinMgr2.cpp + InputDevice.cpp).

Legend: **seam fn** rows are `rva — mangled — from-unit -> to-home — evidence`.
"COMDAT-at-usage" = inline/implicit member emitted in the using obj; not a re-home.
Confidence: **anchored** (a `__FILE__` string proves the file) / **strong** /
**weak**.

Verdict summary:

| interval | units | verdict | confidence |
|---|---|---|---|
| `0x0239d0-0x024ae0` | gruntzcmdmgr+gruntzcommand | **ONE TU** (GruntzCmd.cpp) | strong |
| `0x1350b0-0x13848b` | 5 sound units | **TWO TUs**, boundary `0x137330` | anchored |
| `0x0218e0-0x022a3a` | fontconfig+drawtext | **ONE TU** (FontConfig.cpp) | strong |
| `0x0c16b0-0x0c296b` | multistartdlgworld+multistartdlg | **ONE TU** (MultiStartDlg.cpp) | strong |
| `0x0b5380-0x0bd35d` | multi+netmgrgame (+9 stray units) | **ONE TU** (Multi.cpp) | strong |
| `0x104d60-0x10bc14` | sbi_rectonly+statusbarupdaters+warpstonefly | **ONE TU** | strong |
| `0x0e8a70-0x0ea3ea` | statusbartabbuilders+sbi_gruntmachine+sbi_sidetab | **ONE TU** | strong |
| `0x1832d0-0x185a0e` | menupage+menuitem+menuitem2+util pocket | **SPLIT ≥3 TUs**, boundaries `0x1848b0`, `0x185460` | strong |
| `0x174e90-0x177476` | imagepool+rezimage+scanlinesurface(+3) | **ONE TU** | strong |
| `0x15ccd0-0x163a00` | gamelevel+wwdfile+levelplane(+2) | **TWO TUs**, boundary `0x161350` | strong |
| `0x154aa0-0x15ccc8` | 13 ddraw-submgr units | partial: 4 probable boundaries | weak |
| `0x09e700-0x09fe39` | mapmgr+brickz | **ONE TU** (MapMgr.cpp) | strong |
| `0x077f80-0x07d7ca` | triggermgr+iconloaders(+7) | **ONE TU** (TriggerMgr.cpp) | strong |
| `0x0363a0-0x037900` | videoconfig+menustate(+2) | **ONE TU** (options dialogs) | strong |
| `0x00b5e0-0x00cc98` | worldsoundset+randomambientsound+ambientsound | **ONE TU** | strong |
| `0x0c5360-0x0c7e90` | roster tail (pumps+dropper) + actregsiblings+droppedobject | **ONE TU** (DroppedObject.cpp), boundary `0x0c5360` | strong |

---

## 1. `0x0239d0-0x024ae0` gruntzcmdmgr + gruntzcommand — ONE TU (strong)

**Verdict: merge** into one original file (GruntzCmd.cpp: `CGruntzCmdMgr` +
`CGruntzCommand`/`CGruntzSingleCommand`/`CGruntzMultiCommand`).

Evidence:
- Block order is a **sandwich**: cmdmgr (9 fns, `0x239d0-0x23d6a`) | command
  (22 fns, `0x23e20-0x2485b`) | cmdmgr again (`?Serialize@CGruntzCmdMgr` `0x24890`,
  `IsActive`/`IsActive2` to `0x24ae0`). A-B-A is impossible for two objs at
  first link.
- Init frags: gruntzcommand runs 18@`0x231d0` + 1@`0x24af0` + 7@`0x24b90`
  bracket the interval; **zero** gruntzcmdmgr frags → one obj owns the region.
- Semantics: the manager enqueues/serializes exactly these command objects —
  a natural single 1998 .cpp.

Seam fns:
- `0x00023d90` — `?Blit@CObj23d90@@QAEXHHHHH@Z` — gamekeyhandler ->
  gruntzcmd TU — sits inside the sandwich; snaps a rect to the 32px tile grid
  and blits = the command-target tile marker; its only caller
  (`CGamePlayInput::DispatchKey` @`0x0cbcc0`) is a remote TU, so not
  COMDAT-at-usage. Identity still placeholder (`CObj23d90`) — @identity-TODO.

## 2. `0x1350b0-0x13848b` sound region — TWO TUs (anchored), boundary `0x137330`

**Verdict: split** into the two `__FILE__`-anchored Dsndmgr files:
- **`C:\Proj\Dsndmgr\DSNDMGR.CPP`** = `[0x1350b0 .. 0x137330)`. Asserts
  reference the DSNDMGR string from `0x1351d0` through `0x137260`
  (`CreatePrimaryBuffer`), spanning both the directsoundmgr AND sounddevice
  units (`0x1366f0`, `0x1371a0` are sounddevice fns with DSNDMGR asserts) →
  `SoundDevice` + `DirectSoundMgr` + `DSoundCloneInst`/`DSoundBaseSub` +
  `DSoundVoice`/`DSoundList` are ONE file.
- **`C:\Proj\Dsndmgr\DSndMgSR.cpp`** = `[0x137330 .. 0x13848b]`. Assert at
  `0x137859` inside `?CreateStreamBuffer@SoundStream` (fn @`0x137780`) →
  `SoundStream` + `StreamVoice` + `StreamFeeder`/`StreamVoiceFeeder` are the
  stream file.

Boundary: last mgr-side fn `?GetPrimary@SoundDevice` ends `0x137323`; the 7-B
abstract dtor `??1CAbstract137330` @`0x137330` is nominally the first fn of
DSndMgSR.cpp (its vtable `0x1ef6c8` fits the stream feeder base whose override
`?Feed@StreamVoiceFeeder` follows @`0x137380`); could also be the DSNDMGR tail
(±1 fn).

Unit partition: DSNDMGR.CPP ⇐ sounddevice, directsoundmgr (all fns <
`0x137330`), dsoundvoice, soundvoicelist. DSndMgSR.cpp ⇐ streamfeeder,
streamvoice, soundstream, soundstreamfree, soundstreamteardown, plus the
directsoundmgr-attributed fns ≥ `0x137340` (below).

Seam fns:
- `0x00135110` — `?ComputeCmdPercent@@YAHH@Z` — gruntcmdpercent -> DSNDMGR.CPP —
  a `pow()`-based volume↔percent curve sitting between
  `SoundDevice::VolumeToAttenuation` and `BuildVolumeTable`; the "GruntCmd" name
  is a misattribution (rename: volume-percent curve).
- `0x00135d70` — `?GetItem@CSoundCueMgr@@QAEPAVCStatusBarItem2@@XZ` —
  statusbarmgrgetitem -> DSNDMGR.CPP — body walks the DSound voice list calling
  `DirectSoundMgr::IsPlaying`; `CStatusBarItem2` is a wrong view.
- `0x001360d0` — `?ConfigureItem@CSoundCueMgr@@QAEHHHHH@Z` — spriteresource ->
  DSNDMGR.CPP — same placeholder `CSoundCueMgr` family, mid-DSNDMGR span.
- `0x00136a30` — `?LoadWave@WaveHost_136a30@ResLoaders@@QAEHPBDHH@Z` —
  resourceloaders -> DSNDMGR.CPP — wave loader sandwiched between
  `SoundDevice::Acquire*`/`Reload*`.
- `0x00136ce0` — `?LoadWave@WaveHost2_136ce0@ResLoaders@@QAEHPAVDirectSoundMgr@@PBDH@Z`
  — resourceloaders -> DSNDMGR.CPP — same.
- `0x00136fe0` — `SoundTick_Ctor` (unowned) -> DSNDMGR.CPP — position.
- `0x00137330` — `??1CAbstract137330@@UAE@XZ` — directsoundmgr -> DSndMgSR.cpp —
  stream-feeder abstract-base dtor (see boundary note).
- `0x00137ac0` — `?TickSubManagers@SoundDevice@@QAEHH@Z` — directsoundmgr ->
  DSndMgSR.cpp — body is pure stream machinery (walks `StreamVoice` list,
  pumps `StreamFeeder::Tick`/`TickPump`); a `SoundDevice` method the devs
  defined in the stream file (its RVA is mid-stream-span).
- `0x00138120` — `?SetDSoundReportModes@@YAXHHHH@Z` — directsoundmgr ->
  DSndMgSR.cpp tail — positioned AFTER the StreamFeeder block, so cannot be
  DSNDMGR.CPP; either DSndMgSR's tail or a small third reporting TU (weak).
- `0x00138150` — `?GetErrorString@DirectSoundMgr@@SAXPADHH@Z` — directsoundmgr
  -> DSndMgSR.cpp tail — same as above (827-B error-string table; weak).

## 3. `0x0218e0-0x022a3a` fontconfig + drawtext — ONE TU (strong)

**Verdict: merge** into one file (FontConfig.cpp — font handles + text input +
dialog text rendering).

Evidence:
- **Member-layout identity**: the drawtext hosts ARE `CFontConfig`.
  `m4::PwdHost` = `+0x1c` edit-text CString + `+0x38` control HFONT ≡
  `CFontConfig::m_inputText` (+0x1c) + `m_arialFont` (+0x38)
  (include/Gruntz/FontConfig.h vs src/Gruntz/DrawText.cpp). The renderers
  render the very string `TypeChar` accumulates.
- The single fontconfig fn inside the drawtext run (`0x22360`) is an
  unreconstructed `@stub` whose only signal is its winapi fingerprint
  (`DrawTextA/SelectObject/SetTextColor`) — the drawtext family itself; its
  `CFontConfig` attribution was an arbitrary parking, so the apparent
  interleave F|D|D|F|D|D is really F|D…D over one class.
- Init frags: single fontconfig 9-run @`0x21610` immediately before the
  interval; no drawtext frags.
- `~CGdiObject` @`0x220f0` is an MFC inline dtor COMDAT-at-usage (both halves
  use GDI fonts) — neutral.

Seam fns (identity folds, all stay in this TU):
- `0x00021f20` — `?MeasureLabel21f20@DrawHost@m4@@…` — drawtext ->
  fold host onto `CFontConfig` — +0x1c/+0x38 layout match.
- `0x00022160` — `?Render22160@PwdHost@m4@@…` — drawtext -> `CFontConfig` — same.
- `0x00022770` — `?DrawWithFont22770@PwdHost@m4@@…` — drawtext -> `CFontConfig` — same.
- `0x00022810` — `?Draw3DText22810@TextHost@m4@@…` — drawtext -> `CFontConfig` — same.
- `0x00022360` — `?winapi_022360_…@CFontConfig@@` — fontconfig (stub) — already
  home; reconstruct as another CFontConfig draw helper.

## 4a. `0x0c16b0-0x0c296b` multistartdlgworld + multistartdlg — ONE TU (strong)

**Verdict: merge** (MultiStartDlg.cpp).

Evidence:
- The "world" unit's fns are `CMultiStartDlg`'s own methods
  (`?SetupWorldCombo@CMultiStartDlg` @`0x0c1840`) + its free helpers
  (`BuildNamedGruntTable` @`0x0c16b0`, `_WndProc_c1a10`) — same class, no
  second identity exists.
- Init frags: multistartdlg 1@`0xc1410` + multistartdlgworld 1@`0xc1690`
  immediately precede the code — one fragment neighborhood, one obj.
- The adjacent WOVEN interval `0x0c2980-0x0c5f15` (roster/color/net dlg) has its
  own 8-frag run @`0xc5360` → a **separate** dialog obj; do not merge across
  `0x0c296b`.

Seam fns:
- `0x000c1aa0` — `?UpdateColorItems@MultiColorDlg@m4@@QAEHXZ` — multicolordlg ->
  this TU — 760-B fn compiled here by position; the color-dlg core lives in the
  next interval, so either the devs defined this method here or the identity is
  wrong (@identity-TODO, medium).

## 4b. `0x0b5380-0x0bd35d` multi + netmgrgame — ONE TU (strong)

**Verdict: merge** into one game-side multiplayer file (Multi.cpp: `CMulti` +
the game-side `CNetMgr` methods + lobby/net-dialog helpers).

Evidence:
- ~15 alternations between multi and netmgrgame blocks across the whole
  interval — a full weave, impossible for two first-link objs and far beyond
  seam-misattribution repair.
- `c:\proj\incs\netmgr.h` header-inline assert emitted at `?PollSession@CNetMgr`
  @`0x0b95f0` → this TU includes netmgr.h (game side of the NetMgr module).
- Semantics: `CMulti::StartSession/Tick/PumpA/PumpB` drive
  `CNetMgr::CreateSession/DispatchRecvMsg/...` — one cooperating session layer.

Seam fns (all -> this TU, position + lobby/net semantics):
- `0x000b6330` — `?Vslot09@CMulti@@UAEHH@Z` — multiresumeslots -> Multi TU — a
  `CMulti` virtual, trivially same class.
- `0x000b77a0` — `?Open@NetSessionOpener@@QAEHXZ` — netsessionopen -> Multi TU.
- `0x000b7ec0` — `?ReportStatusId@CMulti@@QAEXIH@Z` — netlobbydialogs -> Multi TU
  — `CMulti` method.
- `0x000b86c0` — `?ShowMultiStartDlg@CNetMgrLite@@QAEHXZ` — showmultidlg ->
  Multi TU — launches the dialog from the session layer.
- `0x000b89e0` — `?FillPlayerList@@YAXPAUHWND__@@PAUSession@@@Z` — netsession ->
  Multi TU — lobby list-box helper.
- `0x000ba620` — `?LoadMenuSelectSprite@CNetMgr@@QAEHPAX@Z` — netmgrmenuselect ->
  Multi TU — `CNetMgr` method.
- `0x000bb3e0` — `?AppendEditLine@NetLobby@@YGXPAUHWND__@@PAD@Z` —
  netlobbydialogs -> Multi TU.
- `0x000bb700` / `0x000bba10` — `?WaitForOtherPlayers@CNetMgr` / `?Poll@CNetMgr`
  — netmgrwait -> Multi TU — `CNetMgr` methods.
- `0x000bbf80` — `?ResetAll@CNetSession@@QAEXXZ` — netcmdsession -> Multi TU —
  position (netcmdsession's core is the earlier `0x0bef80` interval; this one
  fn was compiled here).
- `0x000b6220`/`0x000b62a0`/`0x000bc3f0` — `??1CLobbyObjB`/`??1CLobbySlot`/
  `?BuildHostName@CLobbySlot` — lobbyobjb -> Multi TU or COMDAT-at-usage of
  inline lobby-class members (weak; dtors may be implicit).
- `0x000b8960` — `??1CMultiStartDlg@@UAE@XZ` — showmultidlg — **leave**:
  implicit/inline dtor COMDAT-at-usage (the dialog's file is `0x0c16b0`;
  destroyed here by `ShowMultiStartDlg`).

## 5a. `0x104d60-0x10bc14` sbi_rectonly + statusbarupdaters + warpstonefly — ONE TU (strong)

**Verdict: merge** (the status-bar machines/items file, SBI_RectOnly.cpp).

Evidence:
- The five statusbarupdaters fns (`Update*StatusBar@EngineLabelBacklog`) and
  `CSBI_RectOnly`'s own `Update*StatusBar` methods are one naming/semantic
  family, interleaved fn-by-fn — one file, our attribution split it.
- `??0CWarpStoneFly` @`0x109bb0` directly abuts `UpdateWarpStoneStatusBar`
  @`0x109bd0`: the warpstone fly-to-statusbar animation lives with its bar.
- `CLevelSync::Sync` @`0x1084d0` sits between CSBI_RectOnly members and its
  `Serialize`/`Deserialize` pair, and its op-8 resets `g_mgrSettings` — whose
  `Serialize` (`0x109e00`, mgrsettings) is also inside this interval. The whole
  serialize cluster is one TU.

Seam fns (all -> this TU by position/family):
- `0x00104e60`, `0x00105310`, `0x001076a0`, `0x00109bd0`, `0x0010b320` — the 5
  `…StatusBar@EngineLabelBacklog` updaters — statusbarupdaters -> this TU —
  same family as CSBI_RectOnly's updaters.
- `0x00105070` — `?Build@CStatzTabBuilder@@QAEHXZ` — sbi_sidetab_build -> this
  TU — position (a statz-tab builder among the rect-only members).
- `0x001084d0` — `?Sync@CLevelSync@@QAEHPAUCSerialArchive@@HHH@Z` — levelsync ->
  this TU — see above; @identity-TODO (likely the status-bar/level-state
  container itself).
- `0x00109e00` — `?Serialize@CMgrSettings@@QAEHPAUCSerialArchive@@HHH@Z` —
  mgrsettings -> this TU — g_mgrSettings tie-in to `CLevelSync::Sync`.
- `0x0010a340` — `?BuildTabzDialog@CTabzBuilder@@QAEHXZ` — sbi_tabzdialog_eh ->
  this TU — position.
- `0x00109ad0` — `?EnsureSub@CSBI_RectOnly@@QAEHHHH@Z` — sbi_rectonly_eh -> this
  TU — same class.
- `0x00105200` — `??1CSBI_SideTab@@UAE@XZ` — sbi_sidetab_eh — **leave**:
  COMDAT-at-usage implicit dtor; the class's file is interval `0x0e8a70` (5b).

## 5b. `0x0e8a70-0x0ea3ea` statusbartabbuilders + sbi_gruntmachine + sbi_sidetab — ONE TU (strong)

**Verdict: merge** (the status-bar tab file: the `CSbTab` builders + the tab
item classes they build).

Evidence:
- Alternation builders|gruntmachine|builders|sidetab|configitem|builders — a
  weave; the `Build*TabStatusBar` builders construct exactly the item classes
  defined between them.
- Single 9-frag sbi_sidetab init run @`0xe9e20` inside the interval → one obj.

Seam fns:
- `0x000ea0f0` / `0x000ea170` — `?SetDirection(Alt)@CSbConfigItem@@QAEXHH@Z` —
  statusbarmgr -> this TU — positioned between the sidetab block and
  `BuildMultiplayerTabStatusBar`; the config-item setters used by the builders.

## 6. `0x1832d0-0x185a0e` menu + engine-util pocket — SPLIT ≥3 TUs (strong boundaries)

**Verdict: split** at `0x1848b0` and `0x185460`:

1. **MenuPage.cpp** `[0x1832d0 .. 0x1848b0)` — `CMenuPage` (34 fns) + a
   trailing cluster of `CMenuItem`/`CMenuItem2` 32-B CString accessors, small
   virtuals and dtors (`0x184610-0x184886`) = inline-in-header COMDAT-at-usage
   emissions of MenuItem classes inside MenuPage's obj (MenuPage news/deletes
   items). Not re-homes.
2. **Engine-lib utility pocket** `[0x1848b0 .. 0x185460)` — foreign to any menu
   TU (contains zlib `_uncompress`!). Probable sub-objs (weak, resolution not
   required here):
   - `0x1848b0-0x184b5d` rezcoll+symtab+hash woven (RezNode/CSymList/CHashBase)
     — one small rez/sym/hash utility obj;
   - `0x184b70-0x1851d3` debugprintf+rangeset woven (`RezDebugInit`,
     `RezDebugPrintf*`, `CDebugConfig::InitFromEnv` + `CRangeSet` — the debug
     config parses range strings, same file); 1-frag debugprintf init run
     @`0x184b60` at its head;
   - `0x1851e0-0x1852d8` rezlist (`CRezList`);
   - `0x1852e0` symparser (`?Remove@CObjList` — 1 fn, possibly COMDAT);
   - `0x185320` zlib `uncompr.c` (library obj, vendor zlib-1.0.4);
   - `0x1853b0` `WapUncompress` (the engine wrapper — own obj or a rez-file tail).
3. **MenuItem.cpp** `[0x185460 .. 0x185a0e]` — `CMenuItem` out-of-line
   `Init`/virtuals + `CMenuItem2`.

Seam fns:
- `0x00185510` — `?Dispatch0c@CMenuItem@@UAEXXZ` — menupage -> menuitem TU —
  5-B virtual between `Init@CMenuItem` and the CMenuItem vfunc block.

## 7. `0x174e90-0x177476` imagepool + rezimage + scanlinesurface — ONE TU (strong)

**Verdict: merge** all six units (imagepool, rezimage, scanlinesurface,
imagevflip, scanlinesurfacesave, imagerectfill) into one image file
(ImagePool.cpp: `CImagePool` + `CRezImage` + `CImagePaletteNode` +
`CImageExtLoader`).

Evidence: every unit holds methods of the SAME classes — `CRezImage` methods
live in rezimage AND scanlinesurface AND imagepool (`?Free@CRezImage`
@`0x175c90`, `?SetPalette@CRezImage` @`0x176ad0`) AND the three singleton
units; the interleave is total (10+ alternations). No second identity exists.

Seam fns (unit-dissolves, all stay):
- `0x00176840` — `?FlipVertical@CRezImage@@QAEXXZ` — imagevflip -> this TU.
- `0x00176b00`/`0x00176b30` — `?Save`/`?SaveBmp@CRezImage` — scanlinesurfacesave
  -> this TU.
- `0x00176d20`/`0x00176da0` — `?FillRect(At)@CRezImage` — imagerectfill -> this TU.

## 8. `0x15ccd0-0x163a00` gamelevel + wwdfile + planes — TWO TUs, boundary `0x161350` (strong)

**Verdict: split**:
- **GameLevel.cpp** `[0x15ccd0 .. 0x161322]` — pure `CGameLevel` (64 fns incl.
  dtor @`0x1611e0`) once the two wwdfile pockets are recognized as its own
  content (below).
- **Plane/render TU** (LevelPlane.cpp) `[0x161350 .. 0x163a00]` — `CLevelPlane`
  + `CPlaneRender` (incl. the 2237-B `Draw`) + `WwdFile::RebuildPlanes`/
  `ReadPlaneObjects` + the CImageSet3 helpers — heavily woven, one obj; zero
  `CGameLevel` fns after `0x161322`.

Boundary: between `0x161322` (end `?AxisProbe@CGameLevel`) and `0x1615a0`
(`??0CDDrawWorkerHost`); the `0x161350-0x161558` pocket of CImageSet1/2/3
scalar/vector dtors is COMDAT-at-usage emission (class homes elsewhere,
cf. imageset cores in `0x1504d0`) so the nominal boundary is `0x161350`.

Seam fns:
- `0x0015d8d0` / `0x0015d9a0` — `?ReadPlane`/`?ReadObjectPlane@CGameLevelPlanes`
  — wwdfile -> GameLevel TU — sandwiched inside CGameLevel runs; LoadWwd
  helpers (@identity: likely `CGameLevel` private or free helpers).
- `0x00160530` — `?WwdFile_IsValidWwd@@YGHPBDPAX@Z` — wwdfile -> GameLevel TU —
  free `__stdcall` helper inside the CGameLevel span; reads
  `g_gameReg->m_world` wwdPath.
- `0x00160660` — `?WwdFile_CheckHeader@@YGHPBDPAX@Z` — wwdfile -> GameLevel TU — same.
- `0x00160790` — `_WwdFile_InflateMainBlock@12` — wwdfile -> GameLevel TU — same.
- `0x00160870` — `?WwdFile_CompressMainBlock@@YGHPAEK0K@Z` — wwdfile -> GameLevel TU — same.
- `0x001615a0`/`0x00161640`/`0x00161c50` — CDDrawWorkerHost ctor/Gap/
  `RegisterNamed` — ddrawworkerhost -> Plane TU — woven into the plane block
  (medium: identity placeholder).
- `0x00161bf0`/`0x001628d0`/`0x001633e0` — `Cleanup_161bf0`/`Prune_1628d0`/
  `GetSize_1633e0@CImageSet3` — imageset3 -> Plane TU — RVA-suffixed
  placeholders compiled in the plane obj.

## 9. `0x154aa0-0x15ccc8` ddraw-submgr region — partial (weak; boundaries only)

Not fully resolved (per brief). Four probable file boundaries from the
class-family transitions:

1. **`0x155840`** — WorkerRegistry file ends (`CDDrawWorkerRegistry` symtab-keyed
   dispatch/scan block `0x154aa0-0x155833`, + `??1CDDrawWorker`) →
   `??0CDDrawSurfaceMgr` begins.
2. **`0x156cb0`** — SurfaceMgr file ends (`CDDrawSurfaceMgr` ctor/dtor/Init/
   Snapshot/RestoreChildren, `0x155840-0x156ca2`) → the SubMgr worker-family
   begins (`??0CDDrawSubMgr` + the IsReady/GetStateId/ScalarDtor/dtor quartets
   of WorkerMapSmall/WorkerList/WorkerA/B/Cache/SubMgrPages/SubMgrLeaf/
   LeafScan, then LeafScan/LeafElementObj resource maps `0x157a80-0x158b04`,
   then SubMgrPages `Method_*` + `CDDrawSurfacePair` `0x158b10-0x1591c9`).
   This middle zone may itself split (~`0x157a80` before the leaf-scan block,
   ~`0x1588f0` before the pages block) — unresolved.
3. **`0x1591e0`** (alt `0x159250`) — → the `CWwdObjMgr` file
   (CreateObject/CreateNamed factories, find/foreach/serialize family,
   `0x159250-0x15b2b0`; `CDDrawChildGroup` walk dispatchers woven in).
4. **`0x15b2c0`** — → the `CWwdGameObject`/factory-object file
   (`CWwdGameObjectA-F` dtors + `CWwdFactoryObject` Release/Reset +
   `CDDrawBlitParam` + `CAniAdvanceCursor::Advance`, to `0x15ccc8`).

Note: the CFileMem pocket `0x157850-0x157a66` is COMDAT-at-usage emission
(filemem's core is `0x165e30`), NOT a file boundary. Strays `CSoundResMap`
(`0x157b00`), `CSpriteFactory` (`0x1597b0`), `Rng::Next2` (`0x15cbe0`),
`CSprite::GetFrame` (`0x15cc30`) sit inside these zones by position.

## 10a. `0x09e700-0x09fe39` mapmgr + brickz — ONE TU (strong)

**Verdict: merge** (MapMgr.cpp: `CMapMgr` + `CMapArrayA/B` + `CBrickzGrid`).

Evidence:
- Weave A-B-A-B-A: `?Reset@CMapMgr` @`0x9ec30` between the two CBrickzGrid
  blocks; `?Save`/`?Load@CMapMgr` @`0x9f840` after them.
- Init frags: brickz 6@`0x9de10` + mapmgr 10@`0x9fb20` are table-adjacent, and
  mapmgr's HEAD code (`0x9e700-0x9ea3d`) lies between brickz's frags and
  brickz's code — impossible for two contiguous objs; the 16 frags are one
  obj's run with the attribution flipping at the internal class boundary.

Seam fns:
- `0x0009f7f0` — `?Visit@CMapVisitTarget@@QAEHPAXHHH@Z` — maplogic -> this TU —
  single fn between the brickz block and CMapMgr Save/Load; the map-visit
  callback target.

## 10b. `0x077f80-0x07d7ca` triggermgr + iconloaders — ONE TU (strong)

**Verdict: merge** (TriggerMgr.cpp) — the 9 foreign singletons are all
spawn/FX/icon/selection helpers embedded fn-by-fn between `CTriggerMgr` runs;
none forms a contiguous second obj.

Seam fns (all -> triggermgr TU by position; identities are placeholders,
@identity-TODO):
- `0x00078060` — `?HudRect@WorldTimeline@CWorld@@QAEXUtagRECT@@H@Z` — play.
- `0x000788d0` — `?PositionUpdate@CSnd788d0@@QAEHXZ` — multi.
- `0x00078960` — `?LoadCameraSprite@EngineLabelBacklog@@QAEHXZ` — iconloaders.
- `0x0007a3f0` — `?LoadToyBoxIcon@EngineLabelBacklog@@QAEHHHHHH@Z` — iconloaders
  — icons for trigger-spawned toyz.
- `0x0007b330` — `?LoadExplosionSprites@EngineLabelBacklog@@QAEHHHHH@Z` —
  iconloaders — trigger-spawned FX.
- `0x0007b440` — `?BuildRockBreakParticles@CRockBreakMgr@@QAEHHHHH@Z` —
  rockbreakparticles — rock-break FX spawned by triggers.
- `0x0007b930` — `?CombatCue@CGruntTileMgr@@QAEHHHHHH@Z` — grunttilemgr.
- `0x0007be60` — `?LoadGruntResurrectTuning@CGruntResurrector@@QAEHHHH@Z` —
  gruntresurrectradius.
- `0x0007c3d0` — `?LoadFinishLevelSprite@CFinishLevelState@@QAEXH@Z` —
  finishlevelsprite.
- `0x0007c620` — `?LoadPowerupIconSprites@EngineLabelBacklog@@QAEHHHHHHH@Z` —
  iconloaders.
- `0x0007cf40` — `?CenterOnGroup@CGroupSel@@QAEHH@Z` — groupops — directly
  abuts `?CenterSelectionGroup@CTriggerMgr` @`0x7cd40`, same selection feature.

## 10c. `0x0363a0-0x037900` videoconfig + menustate — ONE TU (strong)

**Verdict: merge** (the options-dialogs file: `GameOptionsDlgProc` +
`VideoOptionsDlgProc` + their load/save/scroll helpers). The menustate fns here
(Load/Read options, toggles, scroll winapi helpers) are options-dialog code,
NOT the real menustate core (`0x0a02c0`, which keeps its own frag runs
@`0xa0ea0`/`0xa0f90`).

Evidence:
- Full weave: videoconfig | gameoptionsdialog | menustate | play | menustate |
  videoconfig | menustate | gameoptionsdialog | videoconfig.
- menustate 27-frag init run @`0x35c40` directly precedes the interval — one
  obj's fragment block.

Seam fns:
- `0x00036be0` — `?ApplyGameOptions@CPlay@@QAEXXZ` — play -> this TU — compiled
  here by position; applies the dialog's settings (@identity-TODO whether it is
  really a CPlay member or a free/dialog helper).
- `0x00036410`/`0x00037260` — `GameOptionsDlgProc`/`ScrollDialog` —
  gameoptionsdialog unit merges wholesale into this TU.

## 10d. `0x00b5e0-0x00cc98` worldsoundset + randomambientsound + ambientsound — ONE TU (strong)

**Verdict: merge** (the world-sound file: `CWorldSoundSet` + `CSoundChannel` +
`CAmbientSound` + `CRandomAmbientSound` + the free PosSound helpers).

Evidence:
- `CWorldSoundSet`'s `CreateAmbient*/CreatePos*/CreateRandom*` factories
  manufacture exactly the channel classes defined around them; the head is
  woven (`??1CAmbientSound` @`0xb790` and `??0CRandomAmbientSound` @`0xbb40`
  inside worldsoundset runs), the tail is class-grouped — the classic
  single-file class-grouped layout.
- Init frags: worldsoundset 9@`0xac60` + 18@`0xb310` (single foreign
  `gameobjectfactory` attribution between) = one contiguous fragment region
  before the code; randomambientsound/ambientsound have no separate runs.

Seam fns: none to move (unit-merge only; `?Recompute@CSoundChannel` @`0xbf10`
and the PosSound free fns `0xc840-0xca00` are already in-place).

## 11. `0x0c5360-0x0c7e90` roster tail + actregsiblings + droppedobject — ONE TU (strong; wave2-H)

**Verdict: one obj** (DroppedObject.cpp: CObjectDropper + CDroppedObject +
CDroppedObjectShadow, their pumps, act registries and serializes), and the
MultiStartDlgRoster obj ends at `0xc5333` + its 8-frag run. This REFUTES the
"pumps belong to the roster" contiguity hypothesis (which rested on the
`NetConfigureBe90` 0xc5f00 attribution).

Evidence (private-globals oracle, computed from `.reloc` sites):
- **.data extent weave**: the roster-interval-attributed 9 leading frag statics
  (0x64be20..88 + 0x64bec8..d0, the {0,1,1} triples) INTERLEAVE with the dropped
  TU's registry singletons (0x64be90 / 0x64bed8 / 0x64bf00 and the 0x64bed8+
  cells) in ONE contribution band 0x64be10..0x64c268 — impossible for two objs.
  MultiStartDlgRoster's private extent ends cleanly at 0x64bdcc; the 0x212xxx
  string band is likewise ordered rosterA < tail < dropped with no weave.
- **g_dropperActReg (ex "g_netBe90") @0x64be90 is private to the tail+interval**:
  referenced ONLY by 0xc5f00 (its Construct) + FireAct 0xc5f80 + RegisterActs
  0xc60e0. Zero roster references — 0xc5f00 is CObjectDropper::InitActReg, not a
  roster net helper.
- **The static-registry triple pattern**: each of the three classes carries an
  identical {$E frag, Construct(0x15), atexit thunk(0xe)} triple —
  0xc5ee0/0xc5f00/0xc5f30 (dropper), 0xc6b30/0xc6b50/0xc6b80 (dropped),
  0xc76b0/0xc76d0/0xc7700 (shadow) — the middle two INSIDE the 0xc5f80 interval;
  the first one sits in the disputed tail, structurally parallel.
- **A-B-A sandwich**: actregsiblings fns (0xc5f80/0xc60e0 ... 0xc7750/0xc78b0/
  0xc7ab0) bracket the droppedobject block — one obj regardless.
- The `$E` frags are emitted at the SOURCE POSITION of each file-scope static
  (verified across this TU and areamgr) — so the 9-frag block @0xc5360 is the
  file's leading statics, not a roster tail.

Identity folds executed with the merge: "CSiblingActorA" == CObjectDropper
(registry entry fires &CObjectDropper::Update); "CSiblingActorB" ==
CDroppedObjectShadow (its Advance spawns the "DroppedObject" sprite on the
anim's drop frame); "NetConfigureBe90"/"Unmatched_c76d0" == the
CObjectDropper/CDroppedObjectShadow InitActReg constructors. The
CCheckpointTrigger pair (0x10ea80/0x10ebe0) that shared ActRegSiblings.cpp moved
to CheckpointTrigger.cpp (its class TU, 0x10cb10 interval).
