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
| `0x1396f0-0x145e00` | engine-resource mega-region (15 units) | **SEVEN TUs** + 2 ambiguous pockets (§14): sym `0x1396f0` / hash `0x13c240` / rez `0x13c4e0` / GameWnd `0x13cf00` / GameApp `0x13d590` / DIRSURF `0x13e060` / DDRAWMGR `0x1413d0` / codec `0x143cf0` | anchored (G/H) / strong |

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
  (CTeleporter::LoadColors/ReapplyConfig 0x411f0/0x412c0 inside the CTeleporter
  block; CWormhole::SpawnPartners and the teleporter block bracket the
  CGruntPuddle block); frags i297-i299 one run; private .data band
  0x20d194-0x20d1d0 contiguous. The method ownership is not an ICF fold:
  CTeleporter's ctor and SerializeMove pass their unadjusted `this`, while
  CWormhole::SpawnPartners compares the candidate notify function with ILT
  0x4039b3, which jumps to CreateTeleporter, then calls 0x412c0 on that
  candidate's CTeleporter logic pointer. RTTI independently shows CTeleporter
  and CWormhole as sibling leaves with direct bases CUserLogic@0 and CWapX@0x34.
  The three in-interval registrar fns (0x406d0/0x408b0/0x41680) are
  text-contained -> folded in. GruntPuddle::SetBute @0x7d810 is NOT this TU
  (own interval at the TriggerMgr tail) - stays in GruntPuddle.cpp
  (@identity-TODO). WormholeActs.cpp (0x3f210, frag i296) left split: adjacent
  obj, no positive one-obj evidence.
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

## 14. wave4-K - the engine-resource mega-region `0x1396f0-0x145e00` (236 fns, 15 units)

**Verdict: SEVEN original TUs + two ambiguous pockets.** Derived boundary map
(evidence per boundary below; private-cell RVAs are initialized-.data only):

| # | original TU | .text span | our file (unit kept) | flags |
|---|---|---|---|---|
| A | ButeMgr sym file (name unknown) | `0x1396f0-0x13c23a` | src/Bute/SymTab.cpp (symtab) | eh |
| B | Hash pocket — AMBIGUOUS | `0x13c240-0x13c4ba` | src/Bute/Hash.cpp (hash) | eh |
| C | RezMgr archive file | `0x13c4e0-0x13ce8c` | src/Rez/RezMgr.cpp (rezmgr) | eh |
| D | GameWnd.cpp | `0x13cf00-0x13d58a` | src/Wap32/GameWnd.cpp (gamewnd) | base |
| E | GameApp.cpp | `0x13d590-0x13dfdf` | src/Wap32/GameApp.cpp (gameapp) | eh |
| F | DebugTiming pocket — AMBIGUOUS | `0x13dfe0-0x13e042` | src/Utils/DebugTiming.cpp (debugtiming) | eh |
| G | **DIRSURF.CPP** (anchored) | `0x13e060-0x1413cb` | src/DDrawMgr/DDSurface.cpp (ddsurface) | base->**eh** |
| H | **DDRAWMGR.CPP** (anchored) | `0x1413d0-0x143ca4` | src/DDrawMgr/DirectDrawMgr.cpp (directdrawmgr) | base->**eh** |
| I | surface file-codec file (name unknown; "DIRFILE"-like) | `0x143cf0-0x145dff` | src/Image/FileImage.cpp (fileimage) | eh |

Units dissolved: cremusreadstream+symrec+symparser (->A), rezfile (->C),
image+fileimageblit+fileimagerundecode+lutshaderect+fileimageloadbyext
(->G/H/I + the 0x148840 pocket), ddrawptrcollections' 34 in-band fns (->H).

* **A (sym file = ONE TU).** Multi-scale text A-B-A: the CParseSource run
  0x139800-0x139bbc sits INSIDE the SymTab head (Build@0x139710 .. ??0@0x139de0)
  with the CSymRec ctors between; the CSymTab Resolve* trio 0x13bae0-0x13bfec
  sits INSIDE the CSymParser run (SetDelims@0x13ba80 .. ResolveQualified@0x13bff0).
  Private-cell order: Load@CRezDirNode(0x13a0f0)'s cell 0x21a070 <
  ParseRecords@CSymParser's 0x21a0a0 < the rez obj's 0x21a0a4 - the two
  "rezmgr"-attributed strays (0x13a0f0, 0x13c080 - both text-contained) carry
  cells INSIDE the sym band -> they are this obj's fns wearing Rez names
  (@identity-TODO). EH sites throughout (SymRec/SymTab/SymParser ctors+dtors,
  ApplyRange, PopParseSlot) -> /GX; the cremusreadstream base profile was a
  seam artifact.
* **B (hash pocket).** One contiguous CHash/CHashB block directly after A; no
  frags, no private cells, no EH sites, no weave -> cannot bind to A or prove
  its own obj. Left split (conservative partial).
* **C (rez file = ONE TU).** DECISIVE shared private cells: the "r+b"/"w+b"
  fopen-mode literals 0x21a0a4/0x21a0a8 are referenced ONLY by Open@CRezItm
  (0x13c760, rezmgr) + Open@CRezFile (0x13cdc0, rezfile). Text A-B-A:
  CloseAllOpen@CRezFileMgr (0x13ca80) inside the rezmgr run, ??1CRezDir13cb80
  (0x13cb80) inside the rezfile neighborhood. EH sites (CRezItm/CRezDir dtors,
  CRezParseNode ctor) -> /GX; rezfile's base profile flips.
* **D/E (GameWnd vs GameApp = TWO TUs).** Two clean class blocks, no weave
  across 0x13d590; GameWindowProc@CGameApp (0x13cff0) is the static WNDPROC
  compiled in the window file (stays D). EH sites only in E
  (InitializeGameWindow/Manager) - D base, E eh, exactly the current profiles.
  The five "rezmgr" timing fns are E's: text A-B-A (UpdateClock@0x13ddc0
  between Close@CGameMgr@0x13ddb0 and InitTimeFields@CGameMgr@0x13de70;
  SpinWaitUntil/SetFrameRate/TrySetFrameRate/WaitKeyEdge 0x13dec0-0x13df30
  after InitializeTimeGlobal@0x13dea0). Identity: the "RezMgr" receiver view
  == WAP32::CGameMgr (m_fps@0x18/m_pauseFlag@0x1c/m_elapsedMs@0x20/
  m_startTick@0x24 == the view's m_smoothedFrameCount/m_pacingGate/
  m_frameCounter/m_windowStartTick; UpdateClock calls InitTimeFields ==
  0x13de70), and its duplicate frame-clock statics are the canonical
  g_wap32Now/FrameDelta/ClockReset/Run7c/Run80 cells (0x253c70-0x253c80,
  Globals.cpp) - the statics dissolve onto the canonicals with the move.
  Full RezMgr->CGameMgr method fold deferred to the gruntzmgr package (its
  other methods live at 0x8b740/0x8e470/0x91670, outside this band).
* **F (DebugTiming pocket).** ActiveWait/DebugTrace sit between E's tail and
  G's init frag. ActiveWait's callers are engine-wide (multi/attract/winmain),
  DebugTrace has zero callers -> no discriminating evidence for E-tail vs
  G-head. Left in the 2-fn holding unit.
* **G (DIRSURF.CPP, anchored).** The `C:\Proj\DDrawMgr\DIRSURF.CPP` assert
  string is referenced from 16 sites 0x13e140-0x13f460 spanning ddsurface AND
  image (Fill@0x13e760) AND lutshaderect (ShadeRect@0x13f460) fns. The single
  image-unit init frag @0x13e060 + atexit companion ClearImageCache_13e070 =
  the file's leading static. Every fn in the span is a CDDSurface method or
  its factory/helper; fn-granularity interleave of ddsurface|image|fileimage|
  fileimageblit|fileimagerundecode -> one obj. Private cells: DumpSurfaceInfo's
  table 0x21a0ec-0x21a33c sits between E's cell (0x21a0ac) and H's band
  (0x21a378+) - monotone. EH sites at Build_13e9a0 + ??1CDDSurface -> the obj
  was /GX -> ddsurface's base profile flips to eh. MakeImageKey@RezMgr
  (0x13e5d0, text-contained, no cells) moves in by position.
  The fileimagerundecode "od" per-file override is structurally IMPOSSIBLE
  (DecodeRun8/24 are text-contained in this /GX /O2 obj; one obj = one flag
  set) - the 99.5%-under-/Od shape means retail compiled exactly these fns
  unoptimized, i.e. `#pragma optimize("", off)` islands in the source (VC5
  supports it). Modeled that way; craters accepted if the pragma is refused.
* **H (DDRAWMGR.CPP, anchored).** The `C:\Proj\DDrawMgr\DDRAWMGR.CPP` string is
  referenced from directdrawmgr fns (CreateDevice/Init/SetupCaps/CreatePoolItem/
  GetDisplayMode) AND ddrawptrcollections fns (ComputeColorMasks@0x143b20,
  ConfigureSurface@0x143c20); `C:\Proj\DDrawMgr\ddrawmgr.h` from CreateDevice.
  The directdrawmgr init frag @0x141c70 + ClearModeArray atexit companion
  @0x141c80 ($E at the static's source position - the mode array is defined
  mid-file). ONE-OBJ cell proof: 0x21a9f8 is shared by CreateDevice/Init/
  SetupCaps/CreatePoolItem/GetDisplayMode (directdrawmgr) AND ComputeColorMasks
  (ddrawptrcollections). Head attribution: GetErrorString's private string
  table 0x21a378-0x21a9e4 directly precedes the mode-array band and follows
  G's - so the SetDDrawReportModes/GetErrorString pair @0x1413d0/0x141400 is
  H's head (the DSNDMGR sibling has the same reporting head/tail pair). Text
  A-B-A: collections 0x141cc0-0x143150 | mgr Find*/caps block 0x143240-0x1438c0
  | collections 0x143900-0x143c20. EH sites throughout -> /GX flip for
  directdrawmgr. The CFileImageSurface dtor pair (0x142340/0x142360, image
  unit) is text-contained -> homed here (likely COMDAT-at-usage of the pool
  surface class's inline dtor).
* **I (codec file = ONE TU, third DDrawMgr image file).** The 0x143cf0-0x145dff
  block is ALL CDDSurface file-format code (Load/Save/Decode BMP/PCX/PID/TGA/
  RLE16 + run-decode) but CANNOT be DIRSURF.CPP: the whole DDRAWMGR obj sits
  between (contiguity at first link). No __FILE__ string (no asserts) -> the
  original name is unknown (DIRFILE.CPP-like); hosted in FileImage.cpp. Total
  image|fileimage interleave at fn granularity -> one obj. EH sites (LoadFile2/
  LoadBmp/SaveBmp/SaveRle16/SaveTga/LoadFile/LoadPcx/DecodePcxEx/LoadPid) ->
  /GX (fileimage already eh). ResLoad_144270 (resourceloaders, 0x144270,
  text-contained) moves in by position; RunDecode1/3 are the codec file's
  `#pragma optimize` islands (see G).
* **The `0x148840-0x148cd8` pocket** (outside this band; TU_MIGRATION row
  "WOVEN 0.29" = one obj): the leftover second cores of image (LoadKeyed/
  ResolveEx/UpdateOverlay) + ddrawptrcollections (the CPoolItem* virtuals) +
  fileimageloadbyext (LoadByExt). Consolidated into DDrawPtrCollections.cpp as
  the pocket's holding unit so no unit spans two blocks; its original file is
  a fourth DDrawMgr surface file (unnamed - no anchor), unresolved here.

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

## 15. wave4-L - the ddraw-submgr region `0x1504d0-0x15ccc8` + tail `0x163a40-0x166100`

**Verdict: 4 STRONG one-obj blocks (S1/S2/C/T) + a 5-block E/F/G/H/I complex held
at the dossier-#9 boundaries (documented-ambiguous, leaning fewer objs).**
Evidence per boundary below. Negative oracles first: ZERO $E init frags anywhere
in `0x1504d0-0x166100` (no static ctors in any of these TUs); no __FILE__ anchors
(asserts compiled out); initialized-.data references are THIN (the region runs on
.bss + heap), so the privates oracle is weak here.

| # | block | span | verdict | our file (host) | flags |
|---|---|---|---|---|---|
| S1 | wwd game-object core + worker frames | `0x1504d0-0x152636` | ONE obj (WOVEN, strong) | src/Wwd/WwdGameObject.cpp | eh |
| S2 | submgr leaf/ani catalog | `0x152640-0x152e83` | ONE obj (A-B-A, strong) | src/DDrawMgr/DDrawSubMgrLeaf.cpp | eh |
| C | CImage impl (Create/Render/Blit*) | `0x152e90-0x1549c5` | ONE obj (single class, strong) | src/Image/CImage.cpp | eh |
| D | CResolveNode/base-slot COMDAT pocket | `0x1549d0-0x154a90` | E's leading COMDATs (high) | src/Gruntz/ResolveNode.cpp (held) | eh |
| E | CDDrawWorkerRegistry + ~CDDrawWorker | `0x154aa0-0x155833` | block held (#9 boundary 1 @0x155840) | src/DDrawMgr/DDrawWorkerRegistry.cpp | eh |
| F | CDDrawSurfaceMgr (+Snapshot/Restore) | `0x155840-0x156ca2` | block held (#9 boundary 2 @0x156cb0); LEANS F==G | src/DDrawMgr/DDrawSurfaceMgr.cpp | eh |
| G | submgr worker-family (quartets+meat) | `0x156cb0-0x1591c9` | ONE block (WOVEN internally); #9's ~0x157a80/~0x1588f0 sub-splits REFUTED | src/DDrawMgr/DDrawSubMgr.cpp | eh |
| H | CWwdObjMgr finds/foreach/factories + childgroup walks | `0x1591e0-0x15b2b0` | block held (#9 boundary 3 @0x1591e0) | src/Wwd/WwdObjMgr.cpp | eh |
| I | CWwdFactoryObject/CWwdGameObject dtors + CDDrawBlitParam + cursor | `0x15b2c0-0x15ccc8` | block held (#9 boundary 4 @0x15b2c0) | src/Wwd/WwdFactoryObject.cpp (new) | eh |
| T | family "meat" file (pair draw + workers + mapsmall + filemem core + logicrecord IO) | `0x163a40-0x1660e6` | ONE obj (WOVEN, strong) | src/DDrawMgr/DDrawSurfacePair.cpp | eh |
| R | wwd render + imageset1/2/3 Parse/Query | `0x1660f0-0x1670d0+` | UNMAPPED (beyond brief); render fns held | src/Wwd/WwdGameObjectRender.cpp (holding) | eh |

* **S1 (ONE obj).** Fn-granularity weave of five of our units:
  sprite(0x1504d0) | wwd(0x150660) | userbaselink(0x150eb0) | wwd(0x151150) |
  logicrecord(0x151da0) | workercache(0x151e70) | worker(0x151eb0) |
  sprite(0x151f00) | imageset(0x151fb0) | worker(0x1521c0) | imageset(0x1523f0).
  Class proof: ??_7CDDrawWorker (0x1efbe8) slots [10]-[16] = our "CImageSet::
  CreateFrame24/28/30" + "CSprite::InsertFrame" + the worker Build/Validate fns -
  ONE class's virtuals spanning the whole weave (the imageset/spriteresource
  names were views of CDDrawWorker slots). ??_7AnimWorkerObj (0x1efb80) is
  first-stamped by the userbaselink EnsureWorker fns and its slot bodies are the
  0x151d60-0x151e70 run; ??1CLogicRecord (0x151da0) re-stamps it => our
  "CLogicRecord" IS AnimWorkerObj (identity note, not renamed this wave).
* **S2 (ONE obj).** A-B-A: leaf(0x152640-0x1527d0) | ani(0x1528d0-0x152ad0) |
  leaf(0x152c50-0x152d30). ??1CAniElement@0x152e30 directly after = the element
  class the ani CreateAniEntry fns stamp (??_7CAniElementObj first-stamp
  0x1528d0) -> S2's tail.
* **C (ONE obj).** 0x152e90-0x1549c5 is ALL CImage: the RTTI vtable 0x1eaa2c
  slots [7]-[15] point 0x152e90-0x153470; Blit*@0x1538c0-0x154750 are CImage
  methods. The 0xd5c10-0xd5e80 quartet (Gap/dtor/Slot17) are COMDAT-at-usage
  exiles kept at the play-region obj (which first-references the vtable ->
  carries the /GR RTTI). Whether C itself is a Gruntz(/GR) or DDrawMgr file is
  OPEN (its own vtable emission was discarded); one obj regardless.
* **D (pocket).** ??0/??1CResolveNode + four tiny unnamed slot bodies
  (0x154a00-0x154a80). 0x154a00 is the SHARED slot-[8] body of ??_7WwdBResolve/
  ??_7CDDrawSubMgr/??_7LeafElementObj/??_7CWwdGameObjectE - a grand-base inline
  virtual; the pocket is the COMDAT cluster kept at the first obj emitting those
  vtables. ??_7WwdBResolve first-stamp = 0x1549d0 itself; next obj (E) starts
  0x154aa0 -> pocket rides E's contribution head. Held in ResolveNode.cpp.
* **E/F/G (held at 0x155840 + 0x156cb0, LEANING ONE-TO-TWO objs).** The
  keeper-argument LEANS F==G: ALL eight family vtables (??_7CDDrawSubMgrLeaf/
  LeafScan/MapSmall/Cache/List/ChildGroup/Pages/CWorkerVtableView 0x1efc78-
  0x1efe08) are FIRST-stamped by CDDrawSurfaceMgr::Init@0x155900 (F), yet their
  tiny-slot bodies (IsReady/GetStateId/ScalarDtor quartets) sit at
  0x156cd0-0x1577e0 (G) - if F and G were different objs, F (the keeper) would
  hold those inline-slot COMDATs inside its own span. Same for ??_7CFileMem
  (first-stamp SnapshotChildren@0x156020=F; the inline ctor/dtor pocket
  0x157850-0x157a66 sits in G). ESCAPE (why held, not merged): the quartet
  bodies may be real out-of-line definitions in a second file (1997 code often
  wrote 6-byte getters out-of-line), which no oracle here can exclude. The
  .data band 0x21ab14-0x21ab30 (g_wwdObjIdCounter / a shared symtab cell /
  g_sndEnabled / g_sndCueTag / three Gap_15a210 cells) binds S2+E+G+H+I if its
  cells are file-statics - but every one is plausibly extern, so not decisive.
* **G internal sub-splits REFUTED.** #9 suggested ~0x157a80 and ~0x1588f0 as
  possible boundaries; the weave crosses both: leaf fns at 0x1577a0-0x1577e0 AND
  0x157ae0/0x157bc0 (filemem pocket between); submgr's TriggerBlit@0x1587f0
  between the leafscan block and the pages block; pages fns run 0x1588f0-0x158ee0
  continuously then subworker|pair|subworker|pair alternates 0x158f30-0x1591b0.
  G is internally woven -> one block.
* **H/I (held at 0x15b2c0).** H = the CWwdObjMgr collection file (finds/foreach/
  serialize + the CreateObject factories + CDDrawChildGroup walk dispatchers
  woven in, exactly #9's picture). I = the object-lifecycle tail: the five
  ??1CWwdGameObject[A-F] dtors + CWwdFactoryObject Release/Reset + CDDrawBlitParam
  + ??0CAniAdvanceCursor + Advance. Weave across 0x15b2c0 is limited to
  factory-ctor 0x15b390 (EH ctor, COMDAT-able) - held as two blocks.
* **T (ONE obj, strong).** 0x163a40-0x1660e6 is a TOTAL fn-granularity weave of
  twelve units: spatial-dtor | workerhost-dtor | list | pair(13 fns) | pages |
  helper | resolvenode | logicrecord(Load/Save 1KB each) | registry | cache |
  anielement | entrylist | anirecord | mapsmall(8) | anirecord | filemem(core:
  Open/Read/Write) | workers | helper | workers. Sizes up to 1KB with EH => a
  real TU (not a COMDAT pool): the family's "runtime meat" file, sitting after
  levelplane (its own obj per dossier #8). Vtable corroboration: ??_7CAniRecordView
  (0x1f02c0, first-stamp Build_165460) and ??_7CAniRecordBase2 (0x1f02d8,
  first-stamp 0x1658c0) are kept HERE, in .rdata order right after levelplane's
  vtables - monotone with T as the next obj.
* **Vtable .rdata run (order oracle).** 0x1efb80..0x1f02d8 is one unbroken
  adjacency run whose first-stamper .text positions are PERFECTLY monotone
  (0x150eb0 -> 0x1528d0 -> 0x1549d0 -> 0x154ae0 -> 0x155840 -> 0x155900 ->
  0x156020 -> ... -> 0x15ccd0(gamelevel) -> 0x15d820 -> 0x1615a0 -> 0x1628f0
  (levelplane) -> 0x165460 -> 0x1658c0 (T)) - corroborates .text order == obj
  link order across the whole region, and places gamelevel/levelplane/T's
  vtables in their own contributions.
* **Strays (position-homed or left as exiles).** CSoundResMap::RemoveByValue
  @0x157b00 -> G (the leaf/ani catalog IS the sound-res map neighborhood);
  CSpriteFactory pair @0x1597b0/0x159830 -> H; CSprite::GetFrame@0x15cc30 -> I;
  Rng::Next2@0x15cbe0 LEFT in Random.cpp (foreign inline-COMDAT exile hole in
  I's span); leafscan's far exiles 0x1f940/0x5b7e0/0x114120 ride G's file head;
  CAniElement::AtChecked@0x6b270 rides T's file head; leaf's 0x6b2a0 rides S2's
  file head; SpriteResource's 0x58b60 exile rides the S1 file head.

### #15 execution notes (wave4-L, as landed)

* **Executed:** S1 -> `src/Wwd/WwdGameObject.cpp`; S2 -> `src/DDrawMgr/DDrawSubMgrLeaf.cpp`;
  C -> `src/Image/CImage.cpp` (Blit family merged in); D pocket -> `src/Gruntz/ResolveNode.cpp`
  (ctor/dtor only); E -> `src/DDrawMgr/DDrawWorkerRegistry.cpp`; F -> `src/DDrawMgr/DDrawSurfaceMgr.cpp`
  (reordered ascending); G -> `src/DDrawMgr/DDrawSubMgr.cpp` (~90 fns, woven); H -> `src/Wwd/WwdObjMgr.cpp`;
  I -> `src/Wwd/WwdFactoryObject.cpp` (NEW unit); T -> `src/DDrawMgr/DDrawSurfacePair.cpp`;
  R holding file -> `src/Wwd/WwdGameObjectRender.cpp` (NEW unit). 22 units dissolved, 2 added.
  Shared class hierarchies hoisted to headers so split method sets can live in their objs:
  WwdGameObjectFamily.h, WwdGameObjCtor.h, WwdFactoryObject.h, ResolveNode.h, AnimWorkerObj.h,
  DDrawWorkerCache.h, DDrawWorkerList.h, DDrawWorkerMapSmall.h, DDrawSubMgrLeaf.h, AniAdvance.h.
* **Held correct-partials (documented, NOT merged):** DDrawSurfaceMgrSerialize.cpp stays a
  separate F-block file (its CFileMem/CDDrawSurfaceMgr/CDDrawSubMgrPages local views clash with
  F's; fold deferred); WwdSpatialMgr.cpp keeps the 0x163a40 dtor (co-located with FreeGrids);
  DDrawWorkerHost.cpp keeps 0x163af0; AniRecord.cpp keeps the 0x1657a0/0x165dd0 dtor pair;
  Random.cpp keeps Rng::Next2@0x15cbe0 (foreign inline-COMDAT exile hole in I's span).
* **0x155720 (`??_GCDDrawSubMgr`, E span):** the RVA_COMPGEN row must stay in DDrawSubMgr.cpp -
  labels.py's authority check requires the base obj that actually emits the COMDAT ??_G (only G's
  local CDDrawSubMgr does). Retail-span home in E is a deferred identity-pass item.
* **Recovered in verification:** four previously-100% fns dropped by the shuffle were restored
  (CResolveNode::Init@0x1647e0, CDDrawWorkerRegistry::DestroyAll@0x165210 +
  FindKeyOfValue@0x165360 -> ddrawsurfacepair; CDDrawChildGroup::IsReady@0x1575e0 -> ddrawsubmgr);
  all back at 100%. CImageSet::CreateFrame30's stale size annotation fixed (0xdc -> 0xa4, retail 164 B).

## 16. waveM-judgment - the final three merge groups (customworld / sbi-items / menu-tail)

### 16a. `0x03ac30-0x03e135` customworld group - TWO objs, boundary `0x3bc78/0x3bfa0` (strong)

**Verdict: TWO original objs** (the "WOVEN 0.30" metric was intra-half weave plus the
orphanleaves aggregation artifact spanning both halves; no unit truly crosses the
boundary once orphanleaves' members are re-attributed):

1. **CustomWorldDialog.cpp** `[0x3ac10(frag i513) .. 0x3bc78]` - the custom-world
   picker feature file: launcher + DlgProc + level-list filler + info-pane filler +
   CUSTOM_WORLDINFO popup + path builder + the two WwdFile statics. /GX (EH
   prologues at 0x3b470/0x3b940/0x3bb50).
2. **Demo.cpp** `[0x3bfa0 .. 0x3dee1]` - the demo/attract feature file: CDemo
   slot bodies + CDemoSetup + BuildWorldLevelKey + the auto-scroll camera + Orient3
   + the bute debug editors + CButeMgr::Parse + CTriRecord::Serialize + the
   COwnerWithSubs teardown pair + the ten anim-worker Handlers. /GX (0x3c0e0/0x3cc20).

Method notes (the conflicting-evidence reconciliation the brief asked for):
* **Init-frag slot HOLES are NOT obj boundaries.** The confirmed one-obj
  DroppedObject TU (dossier #11) has a 3-null hole (i923-925) INSIDE its own frag
  run, so the original "i514-515 vs i519-550 = two runs = two objs" reading was
  unsound as stated. The frag evidence that DOES hold: the region has FIVE
  positional frag clusters (i513-515 head singletons; the 9-frag {0,1,1}-triple
  groups i519-527 @0x3bcd0, i530-538 @0x3c520, i541-549 @0x3cfe0; i550 @0x3e100),
  and the i539-540 hole separates two clusters both INSIDE the demo obj's text -
  intra-obj holes proven in-region.
* **The 9-frag {0,1,1} triple groups** ($E jmp -> `mov eax,1; mov cell,0; mov
  cell+4,eax; mov cell+8,eax; ret`, cells .bss, referenced by nothing else) are
  positional-only markers. i530-538 + i541-549 BRACKET democameratools fns inside
  the demo block (A-B-A -> one obj); i519-527 sits between the two objs (0-gap
  adjacent to the demo block's first fn, 88-B pad after the cw block) - lean
  demo-head, execution-neutral (frags are not carved).
* **Initialized-.data privates** (the decisive band): cw extent 0x20cf80..0x20cfe4
  with cross-unit private cells (0x20cfa4 FillLevelInfoDialog+CustomWorldInfoDlgProc;
  0x20cfbc LoadCustomWorldSelection+LoadCustomWorldInfo+BuildCustomWwdPath;
  0x20cfc4) binding customworlddialog+customworldinfodlg+customwwdpath into ONE
  obj; demo extent 0x20d008..0x20d148 (StepA/StepB/Gap_03c990/Gap_03cdd0/
  ??0CGruntStartingPoint) monotone; the two bands DISJOINT at the text boundary.
  The .bss statics 0x22c25c-0x22c274 are read intermixed by cw-dialog AND
  cw-info-dlg fns (same-obj corroboration). Zero cross-half shared private cells.
* **Demo-oracle**: all probed fns sit at IDENTICAL relative offsets in GruntDem.exe
  (uniform -6560 shift; the ten Handlers differ in bytes - class-size immediates -
  but their space is preserved). No boundary discrimination, attributions corroborated.
* **The tail `[0x3df30..0x3e135]` is NOT the demo obj**: g_actReg4 @0x6446d8 is
  PRIVATE to {0x3e12b, 0x3e185, 0x3e1cd/20a/242/284, 0x3e42b, 0x3e46e} - the
  GruntStartingPoint activation cluster {ctor 0x3df30, Register6446d8Range 0x3e120,
  FireActivation 0x3e1a0, ActReg4RegisterType 0x3e300 (+uncarved 0x3e185/0x3e42x)},
  the same {handler, ctor, registrars, triples} shape as the CCursorSnapSprite
  cluster at 0x3a200-0x3ac10. Executed: ActReg4.cpp + Register6446d8Range folded
  into GruntStartingPoint.cpp (the class file; wormhole-#12 registrar precedent).
* **GameKeyStr dissolved**: its methods were the anchored MFC CString entry points
  (Set=??4 0x1b9e74, Append=?+= 0x1ba0c8, Reset=?Empty 0x1b9c69, Free1b9b93=??0
  0x1b9b93 per config/library_labels.csv) - g_pathStr/g_levelStr are plain CString
  globals; the CustomWorldDialog/CustomWorldInfoDlg/OrphanLeaves triple-decl of
  0x22c25c unified. The 0x22c26c/0x22c270 cells are the popup's parent HWND /
  HINSTANCE (semantic names won over g_dat62c26c/70).
* Executed moves: customworldinfodlg+customlevellist+customwwdpath+wwdfile(2 fns)
  +FreeGlobal62c25c -> CustomWorldDialog.cpp; gruntzmgrtransition's Vslot15 +
  demosetup+worldlevelkey+democameratools+animworkerhandlers(11)+butemgrparse+
  trirecordserialize(0x3c8f0)+orphanleaves(DtorSubC/8) -> Demo.cpp; Handler03a200
  -> CursorSnapSprite.cpp (its own cluster). Nine units dissolved. Every moved fn
  held its % (RunCustomWorldDialog ROSE 72.7->95.7 under the obj's real /GX profile;
  FreeGlobal62c25c 0->100 via the explicit-ctor-call spelling).

### 16b. `0x0e5ad0-0x0e8733` sbi-items region - PER-CLASS objs (strong); the "3-interval sbi_rectonly conflation" resolved here

**Verdict: NOT one obj - a SEQUENCE of one-file-per-class SBI item objs**, each
headed by its own 9-frag {0,1,1} static group, exactly the structure the *Eh.cpp
collapse already posited ("the original one-file-per-class SBI TUs"):

| frag group (9x{0,1,1}) | class block | our file |
|---|---|---|
| i1005-1013 @0xe5800 | `[0xe5ad0..0xe5d17]` timed-play leaf (CAniPlayer, @identity-TODO) | src/Gruntz/AniPlayer.cpp |
| i1016-1024 @0xe5d50 | `[0xe6020..0xe68a7]` CSBI_WellGoo | src/Gruntz/SBI_WellGoo.cpp |
| i1027-1035 @0xe69b0 | `[0xe6c80..0xe6fbc]` CSBI_Image | src/Gruntz/SBI_Image.cpp |
| i1038-1046 @0xe7020 | `[0xe72f0..0xe7642]` CSBI_ImageSet | src/Gruntz/SBI_ImageSet.cpp |
| i1049-1057 @0xe76b0 | `[0xe7980..0xe7dc8]` CSBI_ImageSetAni | src/Gruntz/SBI_ImageSetAni.cpp |
| i1060-1068 @0xe7e10 | `[0xe80e0..0xe8672]` CSBI_MenuItem | src/Gruntz/SBI_MenuItem.cpp |
| (headless tail) | `[0xe86e0..0xe8733+]` thin CSBI_RectOnly | src/Gruntz/SBI_RectOnlyBase.cpp (NEW) |

* **The "suspicious-width 1-frag run i1016-1068"** decomposes into FIVE 9-groups
  with 2-3-slot holes between them; each group sits in the inter-class gap at its
  class's head. The savegame region before (i984-1013) and statusbartabbuilders
  after (i1071-1101) continue the same per-file 9-group pattern.
* **The apparent weave was ATTRIBUTION NOISE** (the brief's "frag-run boundary
  inside woven text means wrong attributions" case). Vtable-slot proof (gruntz
  sema class, thunks resolved): 0xe6d90/0xe6dd0/0xe6e40 are CSBI_Image slots
  3/5/1 (were CSBI_MenuItem::ClearFrame / CAniPlayer::TickRenderCurrent /
  CSBI_MenuItem::SerializeChain); 0xe72f0/0xe7400/0xe7440 are CSBI_ImageSet slots
  11/3/5 (were CSBI_RectOnly::ConfigureRect / CSBI_RectOnly::ResetCounters /
  CAniPlayer::TickRenderFrame); 0xe7980/0xe7b00/0xe7c30 are CSBI_ImageSetAni
  slots 13/5/14 (were CAniPlayer::Init/Tick/SetRange); 0xe6020 is CSBI_WellGoo
  slot 2 (was the AniPlayer-TU StubOwner_e6020); 0xe86e0 is the thin RTTI
  CSBI_RectOnly's slot 2 (vtbl 0x1eab8c); 0x10bfc0 is CStatusBarItem's slot-1
  base leg (was CSBI_MenuItem::SerializeFields). With those identities the region
  is CLEANLY class-blocked - zero cross-class weave.
* **CAniPlayer identity fold**: the old standalone CAniPlayer view WAS the SBI item
  chain (fields +0x04..+0x38 = CStatusBarItem/CSBI_Image/CSBI_ImageSet's,
  +0x3c..+0x50 = CSBI_ImageSetAni's m_3c..m_50; its "AniSeq" = CSbiConfigHost, its
  "AniCelTable" = CSbiConfigRecord/CSprite). CAniPlayer is now the
  CSBI_ImageSetAni-derived timed-play leaf (adds the i64 window at +0x58/+0x60)
  holding only the four non-slot methods 0xe5ad0/0xe5b90/0xe5c10/0xe5c90
  (@identity-TODO: no vtable slot / no surviving direct caller; likely the
  statz-tab/warlord-head family).
* **The sbi_rectonly 3-interval conflation** dissolves for THIS interval: its three
  fns here were the thin chain's (2 -> CSBI_ImageSet, 1 -> the thin CSBI_RectOnly's
  own headless obj, hosted in the NEW SBI_RectOnlyBase.cpp). The 0xfdc00 and
  0x104d60 cores (the 0x570 HOST wearing the same class name - the known
  conflation) were left alone per the brief.
* Executed at held %s (TickRenderCurrent_0e6dd0 went 74 -> 100 EXACT once defined
  on its real class); discovered-but-uncarved slot bodies for a later pass:
  0xe6db0 (CSBI_Image slot 4), 0xe7420 (CSBI_ImageSet slot 4), 0xe74c0
  (CSBI_ImageSet slot 12), 0xe7ae0 (CSBI_ImageSetAni slot 4).

### 16c. `0x184610-0x185a0e` menu-tail pocket - boundaries CONFIRMED; pocket = FIVE objs (strong)

**Verdict: dossier #6's boundaries hold under the full oracle kit**; the pocket
sub-structure is now resolved (was "weak, resolution not required"):

1. **`0x1848b0` (MenuPage | pocket) CONFIRMED.** The 0x184610-0x1848a6
   CMenuItem/CMenuItem2 accessor/dtor cluster is MenuPage-obj COMDAT-at-usage
   content: ??_7CMenuItem@@6B@ (0x1f08c0) is first-stamped at 0x183510 (a
   CMenuPage::AddItem new-site) with all four ctor-side stamps in MenuPage's text,
   and the CRT ??_G__non_rtti_object COMDAT sits interleaved at 0x1847c0 - a
   COMDAT pool zone, not re-homes. The definitions STAY in MenuItem.cpp (the class
   file) per the legend + the inline-member-crater constraint (retail's inline-in-
   header members must be defined out-of-line in our tree or MSVC5 /O2 inlines
   them into MenuPage's callers); the MenuItem-vs-pocket rows in tu_order_check
   are this documented COMDAT-pool artifact, not a fixable misplacement.
2. **Pocket `[0x1848b0..0x185460)` = FIVE objs**, current tree layout already
   correct - no moves:
   - RezColl.cpp `[0x1848b0..0x184b5d]` (RezNode/CSymList/CHashBase - the rez
     hash/collection utility; no frags, no privates, clean block);
   - DebugPrintf.cpp `[0x184b70..0x1851d3]` - its OWN obj, positively proven: own
     init frag i1493 @0x184b60 (ctor-attributed -> RezDebugInit) + own PRIVATE
     initialized-.data band 0x224eb4..0x224f0c (AddFromString + InitFromEnv);
   - RezList.cpp `[0x1851e0..0x185315]` (CRezList + CObjList::Remove);
   - vendor zlib uncompr.obj @0x185320 (references zlib's shared 0x224f14 with the
     deflate/inflate band);
   - WapUncompress.cpp @0x1853b0 (the engine wrapper; its 0x224f14 ref is use of
     zlib's public cell, not a merge proof).
   No cross-binding between the three engine objs (zero weave, zero shared
   privates) -> held split per the conservative precedent (WormholeActs, #12).
3. **`0x185460` (pocket | MenuItem out-of-line block) CONFIRMED** (Init/virtuals
   run 0x185460-0x185a0e, zero foreign fns).
* EH scan: all 11 EH prologues in [0x1832d0..0x185a10] are MenuPage-block fns +
  the two dtors in the COMDAT cluster - consistent with menupage/menuitem 'eh'
  and the pocket units 'base' (current flags correct; no flips).
