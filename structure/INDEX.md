# INDEX — all 231 RTTI classes

Complete accounting of every `.?A[VU]…@@` RTTI name in `GRUNTZ.EXE` (retail v1.0.0.76,
MD5 `81c7f648…`). Total distinct: **231** (229 plain classes/structs + 2 template
instantiations).

Columns:
- **Name** — demangled class/struct name.
- **Kind** — `game` (Gruntz-specific decomp target), `wap32` (engine substrate),
  `mfc`/`crt`/`com`/`afx` (statically-linked library link-artifact — not a target),
  `template`.
- **Source file** — leaked `C:\Proj\…` TU if known, else where mined.
- **RTTI mangled** — verbatim from the binary.
- **Known** — `layout` (fields ± vtable, ported/derived), `fields-order` (field
  *order* only), `name` (name-only stub).
- **From** — source-of-knowledge: `rtti`, `tomalla`, `strings`, `srcpath`.

Scaffold file column: where the stub lives under `structure/`.

## Game-specific classes (the decomp targets)

| Name | Kind | Source file | RTTI mangled | Known | From | Scaffold |
|---|---|---|---|---|---|---|
| CGruntzApp | game | C:\Proj\Gruntz | `.?AVCGruntzApp@@` | layout | rtti+tomalla | graduated: src/Gruntz/GruntzApp.cpp |
| CGruntzWnd | game | C:\Proj\Gruntz | `.?AVCGruntzWnd@@` | layout | rtti+tomalla | src/Gruntz/GruntzWnd.cpp (matched ctor) |
| CGruntzMgr | game | C:\Proj\Gruntz\GruntzMgr.cpp | `.?AVCGruntzMgr@@` | layout | rtti+tomalla | game/cgruntzmgr.h |
| CGruntzMapMgr | game | C:\Proj\Gruntz | `.?AVCGruntzMapMgr@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CMapMgr | game/wap32 | (engine) | `.?AVCMapMgr@@` | name | rtti | src/ (header removed; layout in src/) |
| CNetMgr | game | C:\Proj\NetMgr\NetMgr.cpp | `.?AVCNetMgr@@` | layout(partial) | rtti+tomalla | src/Net/NetMgr.h (matched) |
| CGrunt | game | C:\Proj\Gruntz | `.?AVCGrunt@@` | fields-order | rtti+strings | src/ (header removed; layout in src/) |
| CGruntVoice | game | C:\Proj\Gruntz | `.?AVCGruntVoice@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntPuddle | game | C:\Proj\Gruntz | `.?AVCGruntPuddle@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntCreationPoint | game | C:\Proj\Gruntz | `.?AVCGruntCreationPoint@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntStartingPoint | game | C:\Proj\Gruntz | `.?AVCGruntStartingPoint@@` | name | rtti | src/ (header removed; layout in src/) |
| CUserBase | game | C:\Proj\Gruntz | `.?AVCUserBase@@` | name | rtti | src/ (header removed; layout in src/) |
| CUserLogic | game | C:\Proj\Gruntz | `.?AVCUserLogic@@` | name (dispatch shape known) | rtti+strings | src/ (header removed; layout in src/) |
| CState | game | C:\Proj\Gruntz | `.?AVCState@@` | name | rtti | src/ (header removed; layout in src/) |
| CSplashState | game | C:\Proj\Gruntz | `.?AVCSplashState@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CMenuState | game | C:\Proj\Gruntz | `.?AVCMenuState@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CHelpState | game | C:\Proj\Gruntz | `.?AVCHelpState@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CCreditsState | game | C:\Proj\Gruntz | `.?AVCCreditsState@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CAttract | game | C:\Proj\Gruntz | `.?AVCAttract@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CDemo | game | C:\Proj\Gruntz | `.?AVCDemo@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CPlay | game | C:\Proj\Gruntz | `.?AVCPlay@@` | name (+1 tomalla offset note) | rtti+tomalla | src/ (header removed; layout in src/) |
| CMulti | game | C:\Proj\Gruntz | `.?AVCMulti@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CBootyState | game | C:\Proj\Gruntz | `.?AVCBootyState@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CMultiBootyState | game | C:\Proj\Gruntz | `.?AVCMultiBootyState@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileTrigger | game | C:\Proj\Gruntz | `.?AVCTileTrigger@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileTriggerLogic | game | C:\Proj\Gruntz | `.?AVCTileTriggerLogic@@` | layout | rtti+matched | graduated: src/Gruntz/TileTriggerLogic.h |
| CTileTriggerSwitch | game | C:\Proj\Gruntz | `.?AVCTileTriggerSwitch@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileTriggerSwitchLogic@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileTriggerTransition | game | C:\Proj\Gruntz | `.?AVCTileTriggerTransition@@` | name | rtti | src/ (header removed; layout in src/) |
| CTileMultiTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileMultiTriggerSwitchLogic@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileExclusiveTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileExclusiveTriggerSwitchLogic@@` | name | rtti | src/ (header removed; layout in src/) |
| CTileTimeTriggerLogic | game | C:\Proj\Gruntz | `.?AVCTileTimeTriggerLogic@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileTimeTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileTimeTriggerSwitchLogic@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileSecretTrigger | game | C:\Proj\Gruntz | `.?AVCTileSecretTrigger@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileSecretTriggerLogic | game | C:\Proj\Gruntz | `.?AVCTileSecretTriggerLogic@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CTileSecretTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileSecretTriggerSwitchLogic@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CCheckpointTrigger | game | C:\Proj\Gruntz | `.?AVCCheckpointTrigger@@` | name | rtti | src/ (header removed; layout in src/) |
| CCheckpointTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCCheckpointTriggerSwitchLogic@@` | name | rtti | src/ (header removed; layout in src/) |
| CSecretLevelTrigger | game | C:\Proj\Gruntz | `.?AVCSecretLevelTrigger@@` | name | rtti | src/ (header removed; layout in src/) |
| CSecretTeleporterTrigger | game | C:\Proj\Gruntz | `.?AVCSecretTeleporterTrigger@@` | name | rtti | src/ (header removed; layout in src/) |
| CExitTrigger | game | C:\Proj\Gruntz | `.?AVCExitTrigger@@` | name | rtti | src/ (header removed; layout in src/) |
| CVoiceTrigger | game | C:\Proj\Gruntz | `.?AVCVoiceTrigger@@` | name | rtti | src/ (header removed; layout in src/) |
| CActionArea | game | C:\Proj\Gruntz | `.?AVCActionArea@@` | name | rtti | src/ (header removed; layout in src/) |
| CGuardPoint | game | C:\Proj\Gruntz | `.?AVCGuardPoint@@` | name | rtti | src/ (header removed; layout in src/) |
| CWayPoint | game | C:\Proj\Gruntz | `.?AVCWayPoint@@` | name | rtti | src/ (header removed; layout in src/) |
| CCoveredPowerup | game | C:\Proj\Gruntz | `.?AVCCoveredPowerup@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CCoveredPowerupLogic | game | C:\Proj\Gruntz | `.?AVCCoveredPowerupLogic@@` | name | rtti | src/ (header removed; layout in src/) |
| CMovingLogic | game | C:\Proj\Gruntz | `.?AVCMovingLogic@@` | name | rtti | src/ (header removed; layout in src/) |
| CGiantRock | game | C:\Proj\Gruntz | `.?AVCGiantRock@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CGiantRockLogic | game | C:\Proj\Gruntz | `.?AVCGiantRockLogic@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CRollingBall | game | C:\Proj\Gruntz | `.?AVCRollingBall@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CRainCloud | game | C:\Proj\Gruntz | `.?AVCRainCloud@@` | name | rtti | src/ (header removed; layout in src/) |
| CSpotLight | game | C:\Proj\Gruntz | `.?AVCSpotLight@@` | name | rtti | src/ (header removed; layout in src/) |
| CLightFx | game | C:\Proj\Gruntz | `.?AVCLightFx@@` | name | rtti | src/ (header removed; layout in src/) |
| CUFO | game | C:\Proj\Gruntz | `.?AVCUFO@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CPathHazard | game | C:\Proj\Gruntz | `.?AVCPathHazard@@` | name | rtti | src/ (header removed; layout in src/) |
| CStaticHazard | game | C:\Proj\Gruntz | `.?AVCStaticHazard@@` | name | rtti | src/ (header removed; layout in src/) |
| CKitchenSlime | game | C:\Proj\Gruntz | `.?AVCKitchenSlime@@` | name | rtti | src/ (header removed; layout in src/) |
| CToobSpikez | game | C:\Proj\Gruntz | `.?AVCToobSpikez@@` | name | rtti | src/ (header removed; layout in src/) |
| CBrickz | game | C:\Proj\Gruntz | `.?AVCBrickz@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CProjectile | game | C:\Proj\Gruntz | `.?AVCProjectile@@` | name | rtti | src/ (header removed; layout in src/) |
| CBoomerang | game | C:\Proj\Gruntz | `.?AVCBoomerang@@` | name | rtti | src/ (header removed; layout in src/) |
| CTimeBomb | game | C:\Proj\Gruntz | `.?AVCTimeBomb@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CExplosion | game | C:\Proj\Gruntz | `.?AVCExplosion@@` | name | rtti | src/ (header removed; layout in src/) |
| CParticlez | game | C:\Proj\Gruntz | `.?AVCParticlez@@` | name | rtti | src/ (header removed; layout in src/) |
| CEyeCandy | game | C:\Proj\Gruntz | `.?AVCEyeCandy@@` | name | rtti | src/ (header removed; layout in src/) |
| CEyeCandyAni | game | C:\Proj\Gruntz | `.?AVCEyeCandyAni@@` | name | rtti | src/ (header removed; layout in src/) |
| CFrontCandy | game | C:\Proj\Gruntz | `.?AVCFrontCandy@@` | name | rtti | src/ (header removed; layout in src/) |
| CFrontCandyAni | game | C:\Proj\Gruntz | `.?AVCFrontCandyAni@@` | name | rtti | src/ (header removed; layout in src/) |
| CBehindCandy | game | C:\Proj\Gruntz | `.?AVCBehindCandy@@` | name | rtti | src/ (header removed; layout in src/) |
| CBehindCandyAni | game | C:\Proj\Gruntz | `.?AVCBehindCandyAni@@` | name | rtti | src/ (header removed; layout in src/) |
| CMenuSparkle | game | C:\Proj\Gruntz | `.?AVCMenuSparkle@@` | name | rtti | src/ (header removed; layout in src/) |
| CToyPeek | game | C:\Proj\Gruntz | `.?AVCToyPeek@@` | name | rtti | src/ (header removed; layout in src/) |
| CAniCycle | game | C:\Proj\Gruntz | `.?AVCAniCycle@@` | name | rtti | src/ (header removed; layout in src/) |
| CSimpleAnimation | game | C:\Proj\Gruntz | `.?AVCSimpleAnimation@@` | name | rtti | src/ (header removed; layout in src/) |
| CSingleAnimation | game | C:\Proj\Gruntz | `.?AVCSingleAnimation@@` | name | rtti | src/ (header removed; layout in src/) |
| CAmbientSound | game | C:\Proj\Gruntz | `.?AVCAmbientSound@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CAmbientPosSound | game | C:\Proj\Gruntz | `.?AVCAmbientPosSound@@` | name | rtti | src/ (header removed; layout in src/) |
| CRandomAmbientSound | game | C:\Proj\Gruntz | `.?AVCRandomAmbientSound@@` | name | rtti | src/ (header removed; layout in src/) |
| CStatusBarItem | game | C:\Proj\Gruntz | `.?AVCStatusBarItem@@` | layout | rtti+matched | graduated: src/Gruntz/StatusBarItem.h |
| CSBI_Image | game | C:\Proj\Gruntz | `.?AVCSBI_Image@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_ImageSet | game | C:\Proj\Gruntz | `.?AVCSBI_ImageSet@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_ImageSetAni | game | C:\Proj\Gruntz | `.?AVCSBI_ImageSetAni@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_RectOnly | game | C:\Proj\Gruntz | `.?AVCSBI_RectOnly@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_MenuItem | game | C:\Proj\Gruntz | `.?AVCSBI_MenuItem@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_SideTab | game | C:\Proj\Gruntz | `.?AVCSBI_SideTab@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_GruntMachine | game | C:\Proj\Gruntz | `.?AVCSBI_GruntMachine@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_StatzTabArrow | game | C:\Proj\Gruntz | `.?AVCSBI_StatzTabArrow@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_StatzTabGruntBar | game | C:\Proj\Gruntz | `.?AVCSBI_StatzTabGruntBar@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_WarlordHead | game | C:\Proj\Gruntz | `.?AVCSBI_WarlordHead@@` | name | rtti | src/ (header removed; layout in src/) |
| CSBI_WellGoo | game | C:\Proj\Gruntz | `.?AVCSBI_WellGoo@@` | name | rtti | src/ (header removed; layout in src/) |
| CInGameIcon | game | C:\Proj\Gruntz | `.?AVCInGameIcon@@` | name | rtti | src/ (header removed; layout in src/) |
| CInGameText | game | C:\Proj\Gruntz | `.?AVCInGameText@@` | name | rtti | src/ (header removed; layout in src/) |
| CLevelTime | game | C:\Proj\Gruntz | `.?AVCLevelTime@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntHealthSprite | game | C:\Proj\Gruntz | `.?AVCGruntHealthSprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntStaminaSprite | game | C:\Proj\Gruntz | `.?AVCGruntStaminaSprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntPowerupSprite | game | C:\Proj\Gruntz | `.?AVCGruntPowerupSprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntSelectedSprite | game | C:\Proj\Gruntz | `.?AVCGruntSelectedSprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntToySprite | game | C:\Proj\Gruntz | `.?AVCGruntToySprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntToyTimeSprite | game | C:\Proj\Gruntz | `.?AVCGruntToyTimeSprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntWingzTimeSprite | game | C:\Proj\Gruntz | `.?AVCGruntWingzTimeSprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CCursorSnapSprite | game | C:\Proj\Gruntz | `.?AVCCursorSnapSprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CStatusBarSprite | game | C:\Proj\Gruntz | `.?AVCStatusBarSprite@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntzCommand | game | C:\Proj\Gruntz | `.?AVCGruntzCommand@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntzSingleCommand | game | C:\Proj\Gruntz | `.?AVCGruntzSingleCommand@@` | name | rtti | src/ (header removed; layout in src/) |
| CGruntzMultiCommand | game | C:\Proj\Gruntz | `.?AVCGruntzMultiCommand@@` | name | rtti | src/ (header removed; layout in src/) |
| CTeleporter | game | C:\Proj\Gruntz | `.?AVCTeleporter@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CWormhole | game | C:\Proj\Gruntz | `.?AVCWormhole@@` | name | rtti | src/ (header removed; layout in src/) |
| CWarpStonePad | game | C:\Proj\Gruntz | `.?AVCWarpStonePad@@` | name | rtti | src/ (header removed; layout in src/) |
| CFortressFlag | game | C:\Proj\Gruntz | `.?AVCFortressFlag@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CWarlord | game | C:\Proj\Gruntz | `.?AVCWarlord@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CDroppedObject | game | C:\Proj\Gruntz | `.?AVCDroppedObject@@` | name | rtti | src/ (header removed; layout in src/) |
| CDroppedObjectShadow | game | C:\Proj\Gruntz | `.?AVCDroppedObjectShadow@@` | name | rtti | src/ (header removed; layout in src/) |
| CObjectDropper | game | C:\Proj\Gruntz | `.?AVCObjectDropper@@` | name | rtti | src/ (header removed; layout in src/) |
| CBattlezDlg | game | C:\Proj\Gruntz | `.?AVCBattlezDlg@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CBattlezDlgColors | game | C:\Proj\Gruntz | `.?AVCBattlezDlgColors@@` | name | rtti | src/ (header removed; layout in src/) |
| CBattlezDlgCustom | game | C:\Proj\Gruntz | `.?AVCBattlezDlgCustom@@` | name | rtti | src/ (header removed; layout in src/) |
| CMultiStartDlg | game | C:\Proj\Gruntz | `.?AVCMultiStartDlg@@` | name | rtti | src/ (header removed; layout in src/) |
| CMultiHelpDlg | game | C:\Proj\Gruntz | `.?AVCMultiHelpDlg@@` | name | rtti+strings | src/ (header removed; layout in src/) |
| CCheckpointDlg | game | C:\Proj\Gruntz | `.?AVCCheckpointDlg@@` | name | rtti | src/ (header removed; layout in src/) |
| CDoNothing | game | C:\Proj\Gruntz | `.?AVCDoNothing@@` | name | rtti | src/ (header removed; layout in src/) |
| CDoNothingNormal | game | C:\Proj\Gruntz | `.?AVCDoNothingNormal@@` | name | rtti | src/ (header removed; layout in src/) |
| CSingleFrameMessage | game | C:\Proj\Gruntz | `.?AVCSingleFrameMessage@@` | name | rtti | src/ (header removed; layout in src/) |

## Engine substrate (WAP32) classes

| Name | Kind | Source file | RTTI mangled | Known | From | Scaffold |
|---|---|---|---|---|---|---|
| CGameApp | wap32 | (engine, shared) | `.?AVCGameApp@@` | layout | rtti+tomalla | graduated: src/Wap32/Wap32.h |
| CGameMgr | wap32 | (engine, shared) | `.?AVCGameMgr@@` | layout | rtti+tomalla | src/Net/NetMgr.h (matched) |
| CGameWnd | wap32 | (engine, shared) | `.?AVCGameWnd@@` | layout | rtti+tomalla | graduated: src/Wap32/Wap32.h |
| CWapObj | wap32 | (engine) | `.?AVCWapObj@@` | name | rtti | src/ (header removed; layout in src/) |
| CWapX | wap32 | (engine) | `.?AVCWapX@@` | name | rtti | src/ (header removed; layout in src/) |
| zErrHandling | wap32 | (z-runtime) | `.?AVzErrHandling@@` | name | rtti | src/ (header removed; layout in src/) |
| zPtrColl | wap32 | (z-runtime) | `.?AVzPtrColl@@` | name | rtti | src/ (header removed; layout in src/) |
| zPTree | wap32 | (z-runtime) | `.?AVzPTree@@` | name | rtti | src/ (header removed; layout in src/) |
| _zvec | wap32 | (z-runtime) | `.?AV_zvec@@` | name | rtti | src/ (header removed; layout in src/) |
| _zdvec | wap32 | (z-runtime) | `.?AV_zdvec@@` | name | rtti | src/ (header removed; layout in src/) |

## Template instantiations

| Name | Kind | Source file | RTTI mangled | Known | From | Scaffold |
|---|---|---|---|---|---|---|
| zDArray< int(CUserLogic::*)() > | template/wap32 | (z-runtime) | `.?AV?$zDArray@P8CUserLogic@@AEHXZ@@` | element type | rtti | src/ (header removed; layout in src/) + src/ (header removed; layout in src/) |
| CArray<PLAYLISTINFOSTRUCT*> | template/mfc | C:\Proj\Dsndmgr | `.?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@` | element type | rtti+strings | src/ (header removed; layout in src/) |

## Engine managers (named via strings/leaked paths, NOT in RTTI)

These primary manager classes are mined from strings + leaked source paths; they do
**not** have their own `.?AV…@@` RTTI entry (so no "RTTI mangled" cell). Their
companion classes (DirPal/DirSurf/InputDevice/DSndMgSR/SFManager/CRezDir/RezSync/
ButeMgr) are likewise name-only.

| Name | Kind | Source file | Known | From | Scaffold |
|---|---|---|---|---|---|
| CDirectDrawMgr | manager | C:\Proj\DDrawMgr\DDRAWMGR.CPP | name | strings | src/ (header removed; layout in src/) |
| DirPal | manager | C:\Proj\DDrawMgr\DIRPAL.CPP | name | srcpath | src/ (header removed; layout in src/) |
| DirSurf | manager | C:\Proj\DDrawMgr\DIRSURF.CPP | name | srcpath | src/ (header removed; layout in src/) |
| DirectSoundMgr | manager | C:\Proj\Dsndmgr\DSNDMGR.CPP | name | strings | src/ (header removed; layout in src/) |
| DSndMgSR | manager | C:\Proj\Dsndmgr\DSndMgSR.cpp | name | srcpath | src/ (header removed; layout in src/) |
| SFManager | manager | C:\Proj\Dsndmgr | name | strings | src/ (header removed; layout in src/) |
| PLAYLISTINFOSTRUCT | struct | C:\Proj\Dsndmgr | name (held in CArray) | rtti(template)+strings | src/ (header removed; layout in src/) |
| DirectInputMgr2 | manager | C:\Proj\DinMgr2\DinMgr2.cpp | name | strings | src/ (header removed; layout in src/) |
| InputDevice | manager | C:\Proj\DinMgr2\InputDevice.cpp | name | srcpath | src/ (header removed; layout in src/) |
| RezSync | manager | (REZ/VRZ loader) | name | strings | graduated: src/Rez/RezMgr.h |
| CRezDir | manager | (REZ/VRZ loader) | name | strings | graduated: src/Rez/RezMgr.h |
| ButeMgr | manager | (attributez.txt/dwrects.txt) | name | strings | src/ (header removed; layout in src/) |

## Reconstructed util classes (tomalla-derived, NOT in RTTI)

tomalla-invented names for real binary classes with matched layouts.
`@approx tomalla 1.0.1.77` — offsets version-independent; function addresses
deferred to the re-anchor.

| Name | Kind | Source file | Known | From | Scaffold |
|---|---|---|---|---|---|
| Utils::RegistryHelper | util | C:\Proj\Gruntz (util TU?) | layout (0x21C) | tomalla | graduated: src/Utils/RegistryHelper.h |
| Utils::MemoryPool<T> | util/template | C:\Proj\Gruntz (util TU?) | layout (0x10) | tomalla | utils/memory_pool.h |
| Font | util | C:\Proj\Gruntz\font.cpp | layout (0x14) + .fnt format | tomalla | utils/font.h |
| Pair | struct | C:\Proj\Gruntz | layout {int;int} | tomalla | game/cgruntzmgr.h |
| UnknownClassArrays | nested | C:\Proj\Gruntz\GruntzMgr.cpp | layout (~0x144) | tomalla | game/cgruntzmgr.h |
| UnknownClassInCGruntzMgr | nested | C:\Proj\Gruntz\GruntzMgr.cpp | layout (0x238) | tomalla | game/cgruntzmgr.h |

## HYPOTHESIZED CDirectDrawMgr surface/page-manager family (tomalla "harry_potter")

NOT in RTTI; ALL names are tomalla placeholders. The class IDENTITY (= CDirectDrawMgr
family, C:\Proj\DDrawMgr) is a HYPOTHESIS; offsets/sizes/inheritance are high
confidence. `@approx tomalla 1.0.1.77`. Held in CGruntzMgr @0x30. Method addresses
(~120 stubs) deferred to the re-anchor.

| Name (placeholder) | Size | Base | Notes | Scaffold |
|---|---|---|---|---|
| UnknownClassCGruntzMgrHarryPotter | 0x40 | CObject | family manager; UnknownVirtualMethod18 = 640x480x16 init | managers/ddrawmgr_surface_family.h |
| UnknownCGruntzMgrHogwarts | 0x8 | CObject | common base | managers/ddrawmgr_surface_family.h |
| UnknownCGruntzMgrLucius | 0x10 | …Hogwarts | shared sub-manager base | managers/ddrawmgr_surface_family.h |
| UnknownDraco | 0x1c | …Lucius | | managers/ddrawmgr_surface_family.h |
| UnknownHermiona | 0x6c | …Lucius | CObList + 2×CMapPtrToPtr | managers/ddrawmgr_surface_family.h |
| UnknownHagrid | 0x2c | …Lucius | CObList | managers/ddrawmgr_surface_family.h |
| UnknownSeverus | 0x2c | …Lucius | static DDSURFACEDESC-shaped struct | managers/ddrawmgr_surface_family.h |
| UnknownSirius | 0x2c | …Lucius | CMapStringToOb | managers/ddrawmgr_surface_family.h |
| UnknownAlbus | 0x68 | …Lucius | 3×CMapStringToOb | managers/ddrawmgr_surface_family.h |
| UnknownRemus | 0x6d4 | …Lucius | resolution ladder; 3×CObArray + buffer | managers/ddrawmgr_surface_family.h |
| UnknownMinerva | 0x38 | …Lucius | CMapStringToPtr | managers/ddrawmgr_surface_family.h |
| UnknownPettigrew | 0x2c | …Lucius | CMapStringToPtr | managers/ddrawmgr_surface_family.h |
| UnknownFilch | 0x948 | (none) | 2×CPtrList + CPtrArray | managers/ddrawmgr_surface_family.h |
| UnknownSalazar | 0x94 | (vtable) | 101-entry volume→attenuation table | managers/ddrawmgr_surface_family.h |
| UnknownVoldemort | 0x9c | UnknownSalazar | | managers/ddrawmgr_surface_family.h |

## Version-independent on-disk data formats (shared editor<->game)

Not classes — on-disk record formats. Field SET high confidence (editor labels /
loader asserts); byte layout @todo. See docs/editor-notes.md.

| Name | Kind | Source | Known | From | Scaffold |
|---|---|---|---|---|---|
| WwdObject | format | .WWD world file | field set + flag enums | editor strings | formats/wwd_object.h |
| RezDirEntry | format | .REZ/.VRZ "RezMgr" archive | field set {Type,Name,Size,ID} + sorted invariant | editor strings + game assert | formats/rez.h |

## Library link-artifacts (statically-linked MFC / CRT / COM / AFX — NOT targets)

All from RTTI; published library types; listed (as comments) in `src/ (header removed; layout in src/)`.
Not stubbed. **96** names (the lists below enumerate every one):

MFC app/cmd: `CWinApp` `.?AVCWinApp@@`, `CWinThread` `.?AVCWinThread@@`,
`CCmdTarget` `.?AVCCmdTarget@@`, `CCmdUI` `.?AVCCmdUI@@`,
`CTestCmdUI` `.?AVCTestCmdUI@@`, `CCommandLineInfo` `.?AVCCommandLineInfo@@`,
`CRecentFileList` `.?AVCRecentFileList@@`, `CNoTrackObject` `.?AVCNoTrackObject@@`,
`CObject` `.?AVCObject@@`.

MFC wnd/view/doc: `CWnd` `.?AVCWnd@@`, `CFrameWnd` `.?AVCFrameWnd@@`,
`CDialog` `.?AVCDialog@@`, `CView` `.?AVCView@@`, `CCtrlView` `.?AVCCtrlView@@`,
`CScrollView` `.?AVCScrollView@@`, `CDocument` `.?AVCDocument@@`.

MFC GDI/DC: `CDC` `.?AVCDC@@`, `CClientDC` `.?AVCClientDC@@`,
`CPaintDC` `.?AVCPaintDC@@`, `CWindowDC` `.?AVCWindowDC@@`,
`CGdiObject` `.?AVCGdiObject@@`, `CBrush` `.?AVCBrush@@`, `CPen` `.?AVCPen@@`,
`CRgn` `.?AVCRgn@@`, `CMenu` `.?AVCMenu@@`, `CImageList` `.?AVCImageList@@`,
`CImage` `.?AVCImage@@`.

MFC controls: `CButton` `.?AVCButton@@`, `CComboBox` `.?AVCComboBox@@`,
`CEdit` `.?AVCEdit@@`, `CStatic` `.?AVCStatic@@`, `CListBox` `.?AVCListBox@@`,
`CDragListBox` `.?AVCDragListBox@@`, `CScrollBar` `.?AVCScrollBar@@`,
`CSliderCtrl` `.?AVCSliderCtrl@@`, `CSpinButtonCtrl` `.?AVCSpinButtonCtrl@@`,
`CProgressCtrl` `.?AVCProgressCtrl@@`, `CHeaderCtrl` `.?AVCHeaderCtrl@@`,
`CHotKeyCtrl` `.?AVCHotKeyCtrl@@`, `CListCtrl` `.?AVCListCtrl@@`,
`CTabCtrl` `.?AVCTabCtrl@@`, `CTreeCtrl` `.?AVCTreeCtrl@@`,
`CRichEditCtrl` `.?AVCRichEditCtrl@@`, `CAnimateCtrl` `.?AVCAnimateCtrl@@`,
`CStatusBarCtrl` `.?AVCStatusBarCtrl@@`, `CToolBarCtrl` `.?AVCToolBarCtrl@@`.

MFC file/archive: `CFile` `.?AVCFile@@`, `CMemFile` `.?AVCMemFile@@`,
`CMirrorFile` `.?AVCMirrorFile@@`, `CArchiveStream` `.?AVCArchiveStream@@`.

MFC collections: `CByteArray` `.?AVCByteArray@@`, `CDWordArray` `.?AVCDWordArray@@`,
`CStringArray` `.?AVCStringArray@@`, `CPtrArray` `.?AVCPtrArray@@`,
`CObArray` `.?AVCObArray@@`, `CStringList` `.?AVCStringList@@`,
`CPtrList` `.?AVCPtrList@@`, `CObList` `.?AVCObList@@`,
`CMapStringToOb` `.?AVCMapStringToOb@@`, `CMapStringToPtr` `.?AVCMapStringToPtr@@`,
`CMapPtrToPtr` `.?AVCMapPtrToPtr@@`.

MFC exceptions: `CException` `.?AVCException@@`,
`CArchiveException` `.?AVCArchiveException@@`,
`CFileException` `.?AVCFileException@@`, `CMemoryException` `.?AVCMemoryException@@`,
`CNotSupportedException` `.?AVCNotSupportedException@@`,
`CResourceException` `.?AVCResourceException@@`,
`CSimpleException` `.?AVCSimpleException@@`, `CUserException` `.?AVCUserException@@`.

AFX internal: `_AFX_BASE_MODULE_STATE` `.?AV_AFX_BASE_MODULE_STATE@@`,
`_AFX_CTL3D_STATE` `.?AV_AFX_CTL3D_STATE@@`, `_AFX_CTL3D_THREAD` `.?AV_AFX_CTL3D_THREAD@@`,
`AFX_MODULE_STATE` `.?AVAFX_MODULE_STATE@@`,
`AFX_MODULE_THREAD_STATE` `.?AVAFX_MODULE_THREAD_STATE@@`,
`_AFX_THREAD_STATE` `.?AV_AFX_THREAD_STATE@@`, `_AFX_WIN_STATE` `.?AV_AFX_WIN_STATE@@`,
`CThreadData` (struct) `.?AUCThreadData@@`.

CRT iostream: `type_info` `.?AVtype_info@@`, `ios` `.?AVios@@`,
`iostream` `.?AViostream@@`, `istream` `.?AVistream@@`, `ostream` `.?AVostream@@`,
`istream_withassign` `.?AVistream_withassign@@`,
`ostream_withassign` `.?AVostream_withassign@@`, `istrstream` `.?AVistrstream@@`,
`ostrstream` `.?AVostrstream@@`, `strstream` `.?AVstrstream@@`,
`filebuf` `.?AVfilebuf@@`, `fstream` `.?AVfstream@@`, `ifstream` `.?AVifstream@@`,
`ofstream` `.?AVofstream@@`, `streambuf` `.?AVstreambuf@@`,
`strstreambuf` `.?AVstrstreambuf@@`.

COM interface structs: `IUnknown` `.?AUIUnknown@@`, `IStream` `.?AUIStream@@`,
`ISequentialStream` `.?AUISequentialStream@@`.

## Tally

- Game-specific (decomp targets, incl. CMapMgr and CNetMgr): **123**.
- WAP32 engine substrate (incl. CGameApp/CGameMgr/CGameWnd): **10**.
- Template instantiations: **2** (one wap32 `zDArray<…>`, one mfc `CArray<…>`).
- Library link-artifacts (MFC/CRT/COM/AFX, not targets): **96**.
- **Total distinct RTTI names: 231** (123 + 10 + 2 + 96 = 231).

> The named engine MANAGERS (CDirectDrawMgr, DirectSoundMgr, DirectInputMgr2,
> RezSync, CRezDir, ButeMgr, DirPal, DirSurf, InputDevice, DSndMgSR, SFManager,
> PLAYLISTINFOSTRUCT) are scaffolded under `managers/` but are NOT counted in the
> 231 RTTI total because most have no `.?AV…@@` entry (they are mined from strings/
> paths). The two that DO have RTTI presence (CNetMgr; PLAYLISTINFOSTRUCT via the
> CArray template) are counted in the game/template rows above.
