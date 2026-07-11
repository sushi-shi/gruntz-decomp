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
| `0x03fc70-0x041db2` | wormhole+gruntpuddle+teleporter(+registrars) | **ONE TU** (Wormhole.cpp) | strong |
| `0x042d40-0x04d7c6` | warlord+grunt+fortressflag+particlez(+explosion) | **THREE TUs**: Warlord.cpp `0x42d40-0x45cc1` / FortressFlag.cpp `0x45d30-0x4763d` / Grunt.cpp `0x47a10-0x4d7c6` | strong |
| grunt 5-interval SPLIT | grunt cores 0x42d40/0x50ca0/0x56f80/0x616e0/0x67850 | **FIVE TUs** (Warlord-woven + Grunt + GruntSteps + GruntCombat + GruntEntranceArrival + GruntEntranceMove) | strong |
| `0x0d5960-0x0ddcc8` | play+channelslots+gruntzplayer+gamemodeobjlifecycle | **ONE TU** (Play.cpp) | strong |
| `0x08b8c0-0x093ce7` | gruntzmgr+playdtor+appdialogs | **ONE TU** (GruntzMgr.cpp) | anchored |
| `0x0e3690-0x0e579e` | savegame+levelinfodlg (+savegamemenu stray) | **ONE TU** (SaveGame.cpp) | strong |
| `0x0dec60-0x0e2213` | projectile+timebomb | **ONE TU** (Projectile.cpp) | strong |
| `0x095b10-0x099b46` | ingameicon+ingametext | **ONE TU** (InGameIcon.cpp) | strong |
| `0x0abfa0-0x0ad527` | frontcandyani+eyecandyani | **ONE TU** (FrontCandyAni.cpp) | strong |
| `0x041e90-0x042cd3` | secretteleportertrigger+secretleveltrigger | **ONE TU** (SecretTeleporterTrigger.cpp) | strong |
| `0x147390-0x148837` | ddpalette+dirpal+palettelerp (+PalLoad stray) | **ONE TU** (DirPal.cpp, DIRPAL.CPP anchored) | anchored |

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

## 12. the grunt region - 5-interval partition + wormhole trio (wave3-I)

**Verdicts** (all executed):

* `0x03fc70-0x041db2` wormhole trio -> **ONE TU** (Wormhole.cpp): text A-B-A weave
  (LoadColors/ReapplyConfig@CWormhole 0x411f0/0x412c0 INSIDE the CTeleporter
  block; SpawnPartners/LoadColors bracket the CGruntPuddle block); frags
  i297-i299 one run; private .data band 0x20d194-0x20d1d0 contiguous. The three
  in-interval registrar fns (0x406d0/0x408b0/0x41680) are text-contained ->
  folded in. GruntPuddle::SetBute @0x7d810 is NOT this TU (own interval at the
  TriggerMgr tail) - stays in GruntPuddle.cpp (@identity-TODO). WormholeActs.cpp
  (0x3f210, frag i296) left split: adjacent obj, no positive one-obj evidence.
* `0x042d40-0x04d7c6` -> **THREE TUs**:
  - **Warlord.cpp** `0x42d40-0x45cc1`: CWarlord + the five CGrunt::Resolve*Animation
    fns (0x45100/0x455f0/0x457b0/0x45960/0x45b60) - text A-B-A (0x45100 between
    warlord fns) + the ctor's private .data band interleaving the resolvers'
    cells (0x20d218-0x20d374). Frag i302.
  - **FortressFlag.cpp** `0x45d30-0x4763d`: CFortressFlag + CParticlez + CExplosion
    + LogicDispatchC (state-0 news a CPARTICLEZ; thunk 0x2a04 -> 0x46ad0) +
    Handler046990 (news a CEXPLOSION) + the explosion dispatch triple. DECISIVE:
    frag SANDWICH i303-i313 (ff x2 | explosion | ff x7 | particlez) + text
    containment. The new-site `push 0x54` binary-proved sizeof(CParticlez)=0x54
    (Particlez.h tail pad added).
  - **Grunt.cpp** `0x47a10-0x4d7c6` (grunt-main): the CGrunt ctor/anim-name loaders
    /movement/create-sprite family + Classify@MotionEntity (directionclassify
    frags i317/i323 WOVEN inside the grunt frag run i315-i323).
  - HandleFortConquered @0x3f5f0 -> **FortConquered.cpp** (@identity-TODO): its
    text + 3 private .data cells (0x20d154-16c) sit between WormholeActs and the
    trio, BEFORE the trio's band - it cannot be the ff obj at 0x45d30.
* `0x50ca0` -> **GruntSteps.cpp** (movement-step/move-sound/serialize family +
  LoadVehicleGruntSprites @0x50ce0 text-contained). The TU_MIGRATION
  extent-overlap claim "0x50ca0+0x56f80 one obj" is **REFUTED**: it came from the
  .bss act-registry singleton band (0x2445xx), which is provably NOT obj-ordered
  (wormholeacts/warlord/secrettrigs interleave there); the initialized-.data band
  is perfectly monotone with zero overlaps across the whole region.
* `0x56f80` -> **GruntCombat.cpp** (combat/struck/attack/tuning/spawn family;
  frags i324-i342 one run; absorbs 0x57100 LoadGruntAbilityTuning + 0x57db0
  PathScan + 0x597a0 LoadGruntCombatAnimations + 0x5baf0 GruntSpawnPump + the
  644af0 registrar pair - each pinned by private-.data cells inside this TU's
  band). The mid-block foreign leaves (0x58b60/0x58bc0/0x58ca0/0x58cd0/0x58ee0/
  0x5b7e0) are COMDAT-at-usage emissions - NOT re-homed (legend).
* `0x616e0` (WOVEN grunt+gruntentrancearrival, weave 0.38) -> **ONE TU**, hosted
  in **GruntEntranceArrival.cpp** (flags base->eh per the interval's 2 EH sites);
  absorbs Update@CGruntFireView @0x61cb0 (its private cell 0x20e180 HEADS the
  band), winapi_064540 (text-contained) and LoadWandGruntItemConfig @0x65a60
  (its cells 0x20e27c-8c sit inside the band, NOT the 0x612a0 behaviorleaf
  extent). RunPositionInterpStep @0x5ecd0 kept with an @identity-TODO note (own
  interval outside the obj).
* `0x67850` -> **GruntEntranceMove.cpp** (entrance-move/asset/arrival-commit
  family; absorbs LoadWingzGruntSprites @0x68880 - 31 private cells inside the
  band).

**Method note (the .bss oracle trap):** per-interval private extents must be
split by band. Initialized .data (0x208000-0x229400 raw) is 98%-monotone and
decisive; the .bss tail (0x2445xx act-registry singletons etc.) is NOT
obj-ordered in this EXE and produced all four false extent-overlap merge rows
for this region (0x3fc70+0x42d40, 0x42d40+0x4dd50, 0x50ca0+0x56f80,
0x56f80+0x5d210).

**Not executed (noted in-tree):** GruntTubeAnim @0x50a50 (probable GruntSteps
head, unproven); Update@RbEffect @0x476b0 (ff/grunt boundary, unpinned);
CGrunt::Load @0xd8060 (play-TU move blocked by Play.cpp/Grunt.h header
conflicts - deferred to the play package); the far arrival-defense family
(0xec670/0xf26f0/0xf2b20/0xf8240 + MovingSlot16 @0x5f310) parked at Grunt.cpp's
tail pending the 0xea990-0xf8800 partition package.

## 13. wave3-J - the play/gruntzmgr packages + the small one-TU pairs

**Verdicts** (all executed):

* `0x0d5960-0x0ddcc8` play group -> **ONE TU** (Play.cpp): one 20-frag init run
  (play x1 @0xda4c0 | channelslots x1 @0xda510 | play x16 @0xda560 - the
  channelslots frag INSIDE the play run is the frag-level sandwich); the ex
  channelslots + gruntzplayer + gamemodeobjlifecycle units folded in, /GX
  unified. The "CGameModeObj" view was offset-proven to be CPlay ITSELF
  (+0x1cc m_savedClock / +0x2dc m_guts / +0x3a4 array block / +0x4fc
  m_overlayDrag) and its four lifecycle methods are now CPlay members; the
  retail `lea ecx,[this+idx*0x14+0x3a4]` RemoveAt base proves CPlay::m_3a4 is
  **CPtrArray[4]** (not CByteArray[4] - same 0x14 size, dtor fold unchanged,
  ~CPlay/~CDemo held 100%). The "Cdb2f0" orphan folded to
  GruntzPlayer::Deactivate (m_014/m_020/+0x38 CBattlezMapConfig::Clear_02ade0
  pin); Cdb200 @0xdb200 stays an @identity-TODO (single +0x08 cell, the
  GruntzPlayer::m_008-index conflict blocks the pin). CGrunt::Load @0xd8060
  moved in from Grunt.cpp (the wave3-I "Play.cpp cannot include Grunt.h" wall is
  GONE - the g_gameReg extern moved out of Grunt.h; byte-preserved at 87.01%,
  and the Grunt.h include RIPPLED StepScroll 63.5->81.6).
* `0x08b8c0-0x093ce7` -> **ONE TU** (GruntzMgr.cpp, __FILE__-anchored): the ex
  playdtor + appdialogs units folded in (FLAGS group unified /GX; the base-
  profile appdialogs procs held 100% under /GX). The ex-appdialogs CGameRegLevel
  view folded onto GruntzMgr.cpp's existing ScoreSub2c (same
  g_gameReg->m_curState +0x1c slot; the dialogs read it as the LEVEL NUMBER -
  name reconciliation flagged). The 5 CState/CPlay inline-virtual COMDATs the
  playdtor obj used to emit re-bound to multi/menustate/gruntzmgrtransition at
  100%.
* `0x0e3690-0x0e579e` -> **ONE TU** (SaveGame.cpp): text L-S-L-S sandwich
  (levelinfodlg x3 | savegame x3 | [savegamemenu] | levelinfodlg x2 | savegame
  x23) + contiguous private initialized-.data extents (levelinfodlg
  0x213ac8..0x213b60 directly before savegame 0x213b6c..0x213c10). Absorbs the
  ex levelinfodlg + savegamemenu units; DrawSaveGameMenu's `SlotHasSave` decl
  unified onto IsSlotOccupied (the same 0x2694->0xe5700 probe, killing the
  (i32) cast); the ex-LevelInfoDlg `DATA(0x0064c864)` row was a VA-as-RVA bug,
  fixed to 0x0024c864.
* `0x0dec60-0x0e2213` -> **ONE TU** (Projectile.cpp): text P-T-P sandwich
  (projectile x10 | timebomb x6 | projectile LaunchSound @0xe2190, a real 0x83-B
  method) + contiguous private extents (projectile 0x213624..0x213838, timebomb
  0x213860..0x2138b4). NOTE: the shared type-name registry cells
  (0x2bf650-0x2bf670/0x21aea8/0x6bf464) temporarily carry BOTH view-name sets
  (g_projType* / g_nameReg*) inside the merged TU - unifying that family
  tree-wide (it spans kitchenslime/particlez/statichazard/... too) is deferred.
* `0x095b10-0x099b46` -> **ONE TU** (InGameIcon.cpp): text I-T-I sandwich
  (icon x14 | text 0x99110..0x99a30 | icon SetField54 @0x99b10) + contiguous
  private extents. The duplicated static-inline ResolveNameSlot/ResolveSlot
  helpers deduped; g_textRegCounter/s_textLogicKey unified onto
  g_iconRegCounter/s_iconKeyA (same 0x61aea8/0x60a454 cells).
* `0x0abfa0-0x0ad527` -> **ONE TU** (FrontCandyAni.cpp): text F-E-F sandwich
  (front @0xabfa0 | eye 0xac870..0xacf10 | front 0xacf40..0xad510); the
  frontcandyani init frag @0xad110 sits in the front tail; eyecandyani has no
  frags/private cells. Both sides already used the shared
  ActNameRegistry/ActReg headers - clean fold.
* `0x041e90-0x042cd3` -> **ONE TU** (SecretTeleporterTrigger.cpp): text T-L-T
  sandwich (teleporter x4 | level x5 | SpawnTeleporter @0x42b80, a real 0x153-B
  method); the two init frags (i40 @0x420b0 / i41 @0x426c0) are one adjacent
  run. The teleporter side's hand-rolled name-registry decl block dissolved onto
  the shared <Gruntz/ActNameRegistry.h>; its WwdGameReg extern unified onto the
  canonical CGameRegistry (killing the (CTriggerMgr*)m_68 cast and the
  CTeleResHolder/CTeleSpriteFactory views - CreateSprite now reloc-names the
  REAL ?CreateSprite@CSpriteFactory).
* `0x147390-0x148837` -> **ONE TU** (DirPal.cpp, `C:\Proj\DDrawMgr\DIRPAL.CPP`
  anchored; unit name kept `ddpalette`): the calibration case executed. All 23
  interval fns in one file: the ex ddpalette+dirpal+palettelerp units + the
  PalLoad_1479e0 stray (its Apply@0x147390 IS CDDPalette::Create ->
  CDDPalette::LoadDefault). The PalCtx/DirPal/PaletteLerp views were RVA-proven
  slices of CDDPalette (Install@0x147aa0==SetAndNotify, Teardown/Finish@0x148250
  ==Flush, Finalize@0x1480a0==Tick) and folded onto the canonical
  (include/DDrawMgr/DirectDrawMgr.h), migrating PaletteLerp's semantic field
  names (m_targetPalette/m_sourcePalette/m_fixedR..B/m_durationMs/m_startTimeMs/
  m_lastElapsedMs/m_firstColorIndex/m_colorCount/m_active) onto the +0x14..+0x34
  layout. Every fn byte-preserved (Tick/Flush/Destroy 100% held).

**Extent-overlap rows audited (job 9, initialized-.data oracle):**

* `0xc8700 (play) + 0x99b80 (areamgr)` - **REFUTED** at unit level: play's
  private extent 0x212620..0x2135c8 and areamgr's 0x2113a0..0x2113c4 are
  disjoint (no window). Stale interval-level artifact; do not merge.
* `0xb5380 (multi) + 0x8b8c0 (gruntzmgr)` - **FALSE POSITIVE**: the extents
  interleave heavily (multi 0x20fa74..0x2120e0 inside gruntzmgr
  0x20fac8..0x212614) but the two .text blocks (0xb5380-0xbd35d vs
  0x83030-0x93ce7) are far apart with many other TUs between - impossible for
  one contiguous first-link obj. The extent method needs a text-contiguity
  sanity check; likely extent-stretching outlier cells.
* `0x8b8c0 (gruntzmgr) + 0x82990 (gametext)` - **WEAK seam candidate**: exactly
  ONE gruntzmgr cell (g_pendingFrame @0x20fac8, referenced only by PumpIdleFrame
  @0x8b8c0 - the interval head fn) precedes gametext's $E1 static block
  (0x20fae0..0x20fb84, frags 0x82991..0x829fa) which precedes the rest of
  gruntzmgr's cells. Either PumpIdleFrame/g_pendingFrame belong to the gametext
  obj (a seam fn), or gametext+gruntzmgr are one obj (consistent with
  gruntzmgr's 0x855e0 split-row). Needs a dedicated audit - not executed.
