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
| CGruntzMapMgr | game | C:\Proj\Gruntz | `.?AVCGruntzMapMgr@@` | name | rtti+strings | game/mapmgr.h |
| CMapMgr | game/wap32 | (engine) | `.?AVCMapMgr@@` | name | rtti | game/mapmgr.h |
| CNetMgr | game | C:\Proj\NetMgr\NetMgr.cpp | `.?AVCNetMgr@@` | layout(partial) | rtti+tomalla | src/Net/NetMgr.h (matched) |
| CGrunt | game | C:\Proj\Gruntz | `.?AVCGrunt@@` | fields-order | rtti+strings | game/grunt.h |
| CGruntVoice | game | C:\Proj\Gruntz | `.?AVCGruntVoice@@` | name | rtti | game/grunt.h |
| CGruntPuddle | game | C:\Proj\Gruntz | `.?AVCGruntPuddle@@` | name | rtti | game/grunt.h |
| CGruntCreationPoint | game | C:\Proj\Gruntz | `.?AVCGruntCreationPoint@@` | name | rtti | game/grunt.h |
| CGruntStartingPoint | game | C:\Proj\Gruntz | `.?AVCGruntStartingPoint@@` | name | rtti | game/grunt.h |
| CUserBase | game | C:\Proj\Gruntz | `.?AVCUserBase@@` | name | rtti | game/userlogic.h |
| CUserLogic | game | C:\Proj\Gruntz | `.?AVCUserLogic@@` | name (dispatch shape known) | rtti+strings | game/userlogic.h |
| CState | game | C:\Proj\Gruntz | `.?AVCState@@` | name | rtti | game/states.h |
| CSplashState | game | C:\Proj\Gruntz | `.?AVCSplashState@@` | name | rtti+strings | game/states.h |
| CMenuState | game | C:\Proj\Gruntz | `.?AVCMenuState@@` | name | rtti+strings | game/states.h |
| CHelpState | game | C:\Proj\Gruntz | `.?AVCHelpState@@` | name | rtti+strings | game/states.h |
| CCreditsState | game | C:\Proj\Gruntz | `.?AVCCreditsState@@` | name | rtti+strings | game/states.h |
| CAttract | game | C:\Proj\Gruntz | `.?AVCAttract@@` | name | rtti+strings | game/states.h |
| CDemo | game | C:\Proj\Gruntz | `.?AVCDemo@@` | name | rtti+strings | game/states.h |
| CPlay | game | C:\Proj\Gruntz | `.?AVCPlay@@` | name (+1 tomalla offset note) | rtti+tomalla | game/states.h |
| CMulti | game | C:\Proj\Gruntz | `.?AVCMulti@@` | name | rtti+strings | game/states.h |
| CBootyState | game | C:\Proj\Gruntz | `.?AVCBootyState@@` | name | rtti+strings | game/states.h |
| CMultiBootyState | game | C:\Proj\Gruntz | `.?AVCMultiBootyState@@` | name | rtti+strings | game/states.h |
| CTileTrigger | game | C:\Proj\Gruntz | `.?AVCTileTrigger@@` | name | rtti+strings | game/triggers.h |
| CTileTriggerLogic | game | C:\Proj\Gruntz | `.?AVCTileTriggerLogic@@` | layout | rtti+matched | graduated: src/Gruntz/TileTriggerLogic.h |
| CTileTriggerSwitch | game | C:\Proj\Gruntz | `.?AVCTileTriggerSwitch@@` | name | rtti+strings | game/triggers.h |
| CTileTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileTriggerSwitchLogic@@` | name | rtti+strings | game/triggers.h |
| CTileTriggerTransition | game | C:\Proj\Gruntz | `.?AVCTileTriggerTransition@@` | name | rtti | game/triggers.h |
| CTileMultiTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileMultiTriggerSwitchLogic@@` | name | rtti+strings | game/triggers.h |
| CTileExclusiveTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileExclusiveTriggerSwitchLogic@@` | name | rtti | game/triggers.h |
| CTileTimeTriggerLogic | game | C:\Proj\Gruntz | `.?AVCTileTimeTriggerLogic@@` | name | rtti+strings | game/triggers.h |
| CTileTimeTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileTimeTriggerSwitchLogic@@` | name | rtti+strings | game/triggers.h |
| CTileSecretTrigger | game | C:\Proj\Gruntz | `.?AVCTileSecretTrigger@@` | name | rtti+strings | game/triggers.h |
| CTileSecretTriggerLogic | game | C:\Proj\Gruntz | `.?AVCTileSecretTriggerLogic@@` | name | rtti+strings | game/triggers.h |
| CTileSecretTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCTileSecretTriggerSwitchLogic@@` | name | rtti+strings | game/triggers.h |
| CCheckpointTrigger | game | C:\Proj\Gruntz | `.?AVCCheckpointTrigger@@` | name | rtti | game/triggers.h |
| CCheckpointTriggerSwitchLogic | game | C:\Proj\Gruntz | `.?AVCCheckpointTriggerSwitchLogic@@` | name | rtti | game/triggers.h |
| CSecretLevelTrigger | game | C:\Proj\Gruntz | `.?AVCSecretLevelTrigger@@` | name | rtti | game/triggers.h |
| CSecretTeleporterTrigger | game | C:\Proj\Gruntz | `.?AVCSecretTeleporterTrigger@@` | name | rtti | game/triggers.h |
| CExitTrigger | game | C:\Proj\Gruntz | `.?AVCExitTrigger@@` | name | rtti | game/triggers.h |
| CVoiceTrigger | game | C:\Proj\Gruntz | `.?AVCVoiceTrigger@@` | name | rtti | game/triggers.h |
| CActionArea | game | C:\Proj\Gruntz | `.?AVCActionArea@@` | name | rtti | game/triggers.h |
| CGuardPoint | game | C:\Proj\Gruntz | `.?AVCGuardPoint@@` | name | rtti | game/triggers.h |
| CWayPoint | game | C:\Proj\Gruntz | `.?AVCWayPoint@@` | name | rtti | game/triggers.h |
| CCoveredPowerup | game | C:\Proj\Gruntz | `.?AVCCoveredPowerup@@` | name | rtti+strings | game/triggers.h |
| CCoveredPowerupLogic | game | C:\Proj\Gruntz | `.?AVCCoveredPowerupLogic@@` | name | rtti | game/triggers.h |
| CMovingLogic | game | C:\Proj\Gruntz | `.?AVCMovingLogic@@` | name | rtti | game/triggers.h |
| CGiantRock | game | C:\Proj\Gruntz | `.?AVCGiantRock@@` | name | rtti+strings | game/hazards.h |
| CGiantRockLogic | game | C:\Proj\Gruntz | `.?AVCGiantRockLogic@@` | name | rtti+strings | game/hazards.h |
| CRollingBall | game | C:\Proj\Gruntz | `.?AVCRollingBall@@` | name | rtti+strings | game/hazards.h |
| CRainCloud | game | C:\Proj\Gruntz | `.?AVCRainCloud@@` | name | rtti | game/hazards.h |
| CSpotLight | game | C:\Proj\Gruntz | `.?AVCSpotLight@@` | name | rtti | game/hazards.h |
| CLightFx | game | C:\Proj\Gruntz | `.?AVCLightFx@@` | name | rtti | game/hazards.h |
| CUFO | game | C:\Proj\Gruntz | `.?AVCUFO@@` | name | rtti+strings | game/hazards.h |
| CPathHazard | game | C:\Proj\Gruntz | `.?AVCPathHazard@@` | name | rtti | game/hazards.h |
| CStaticHazard | game | C:\Proj\Gruntz | `.?AVCStaticHazard@@` | name | rtti | game/hazards.h |
| CKitchenSlime | game | C:\Proj\Gruntz | `.?AVCKitchenSlime@@` | name | rtti | game/hazards.h |
| CToobSpikez | game | C:\Proj\Gruntz | `.?AVCToobSpikez@@` | name | rtti | game/hazards.h |
| CBrickz | game | C:\Proj\Gruntz | `.?AVCBrickz@@` | name | rtti+strings | game/hazards.h |
| CProjectile | game | C:\Proj\Gruntz | `.?AVCProjectile@@` | name | rtti | game/projectiles.h |
| CBoomerang | game | C:\Proj\Gruntz | `.?AVCBoomerang@@` | name | rtti | game/projectiles.h |
| CTimeBomb | game | C:\Proj\Gruntz | `.?AVCTimeBomb@@` | name | rtti+strings | game/projectiles.h |
| CExplosion | game | C:\Proj\Gruntz | `.?AVCExplosion@@` | name | rtti | game/projectiles.h |
| CParticlez | game | C:\Proj\Gruntz | `.?AVCParticlez@@` | name | rtti | game/projectiles.h |
| CEyeCandy | game | C:\Proj\Gruntz | `.?AVCEyeCandy@@` | name | rtti | game/eyecandy.h |
| CEyeCandyAni | game | C:\Proj\Gruntz | `.?AVCEyeCandyAni@@` | name | rtti | game/eyecandy.h |
| CFrontCandy | game | C:\Proj\Gruntz | `.?AVCFrontCandy@@` | name | rtti | game/eyecandy.h |
| CFrontCandyAni | game | C:\Proj\Gruntz | `.?AVCFrontCandyAni@@` | name | rtti | game/eyecandy.h |
| CBehindCandy | game | C:\Proj\Gruntz | `.?AVCBehindCandy@@` | name | rtti | game/eyecandy.h |
| CBehindCandyAni | game | C:\Proj\Gruntz | `.?AVCBehindCandyAni@@` | name | rtti | game/eyecandy.h |
| CMenuSparkle | game | C:\Proj\Gruntz | `.?AVCMenuSparkle@@` | name | rtti | game/eyecandy.h |
| CToyPeek | game | C:\Proj\Gruntz | `.?AVCToyPeek@@` | name | rtti | game/eyecandy.h |
| CAniCycle | game | C:\Proj\Gruntz | `.?AVCAniCycle@@` | name | rtti | game/eyecandy.h |
| CSimpleAnimation | game | C:\Proj\Gruntz | `.?AVCSimpleAnimation@@` | name | rtti | game/eyecandy.h |
| CSingleAnimation | game | C:\Proj\Gruntz | `.?AVCSingleAnimation@@` | name | rtti | game/eyecandy.h |
| CAmbientSound | game | C:\Proj\Gruntz | `.?AVCAmbientSound@@` | name | rtti+strings | game/ambient_sound.h |
| CAmbientPosSound | game | C:\Proj\Gruntz | `.?AVCAmbientPosSound@@` | name | rtti | game/ambient_sound.h |
| CRandomAmbientSound | game | C:\Proj\Gruntz | `.?AVCRandomAmbientSound@@` | name | rtti | game/ambient_sound.h |
| CStatusBarItem | game | C:\Proj\Gruntz | `.?AVCStatusBarItem@@` | layout | rtti+matched | graduated: src/Gruntz/StatusBarItem.h |
| CSBI_Image | game | C:\Proj\Gruntz | `.?AVCSBI_Image@@` | name | rtti | game/statusbar.h |
| CSBI_ImageSet | game | C:\Proj\Gruntz | `.?AVCSBI_ImageSet@@` | name | rtti | game/statusbar.h |
| CSBI_ImageSetAni | game | C:\Proj\Gruntz | `.?AVCSBI_ImageSetAni@@` | name | rtti | game/statusbar.h |
| CSBI_RectOnly | game | C:\Proj\Gruntz | `.?AVCSBI_RectOnly@@` | name | rtti | game/statusbar.h |
| CSBI_MenuItem | game | C:\Proj\Gruntz | `.?AVCSBI_MenuItem@@` | name | rtti | game/statusbar.h |
| CSBI_SideTab | game | C:\Proj\Gruntz | `.?AVCSBI_SideTab@@` | name | rtti | game/statusbar.h |
| CSBI_GruntMachine | game | C:\Proj\Gruntz | `.?AVCSBI_GruntMachine@@` | name | rtti | game/statusbar.h |
| CSBI_StatzTabArrow | game | C:\Proj\Gruntz | `.?AVCSBI_StatzTabArrow@@` | name | rtti | game/statusbar.h |
| CSBI_StatzTabGruntBar | game | C:\Proj\Gruntz | `.?AVCSBI_StatzTabGruntBar@@` | name | rtti | game/statusbar.h |
| CSBI_WarlordHead | game | C:\Proj\Gruntz | `.?AVCSBI_WarlordHead@@` | name | rtti | game/statusbar.h |
| CSBI_WellGoo | game | C:\Proj\Gruntz | `.?AVCSBI_WellGoo@@` | name | rtti | game/statusbar.h |
| CInGameIcon | game | C:\Proj\Gruntz | `.?AVCInGameIcon@@` | name | rtti | game/statusbar.h |
| CInGameText | game | C:\Proj\Gruntz | `.?AVCInGameText@@` | name | rtti | game/statusbar.h |
| CLevelTime | game | C:\Proj\Gruntz | `.?AVCLevelTime@@` | name | rtti | game/statusbar.h |
| CGruntHealthSprite | game | C:\Proj\Gruntz | `.?AVCGruntHealthSprite@@` | name | rtti | game/sprites.h |
| CGruntStaminaSprite | game | C:\Proj\Gruntz | `.?AVCGruntStaminaSprite@@` | name | rtti | game/sprites.h |
| CGruntPowerupSprite | game | C:\Proj\Gruntz | `.?AVCGruntPowerupSprite@@` | name | rtti | game/sprites.h |
| CGruntSelectedSprite | game | C:\Proj\Gruntz | `.?AVCGruntSelectedSprite@@` | name | rtti | game/sprites.h |
| CGruntToySprite | game | C:\Proj\Gruntz | `.?AVCGruntToySprite@@` | name | rtti | game/sprites.h |
| CGruntToyTimeSprite | game | C:\Proj\Gruntz | `.?AVCGruntToyTimeSprite@@` | name | rtti | game/sprites.h |
| CGruntWingzTimeSprite | game | C:\Proj\Gruntz | `.?AVCGruntWingzTimeSprite@@` | name | rtti | game/sprites.h |
| CCursorSnapSprite | game | C:\Proj\Gruntz | `.?AVCCursorSnapSprite@@` | name | rtti | game/sprites.h |
| CStatusBarSprite | game | C:\Proj\Gruntz | `.?AVCStatusBarSprite@@` | name | rtti | game/sprites.h |
| CGruntzCommand | game | C:\Proj\Gruntz | `.?AVCGruntzCommand@@` | name | rtti | game/commands.h |
| CGruntzSingleCommand | game | C:\Proj\Gruntz | `.?AVCGruntzSingleCommand@@` | name | rtti | game/commands.h |
| CGruntzMultiCommand | game | C:\Proj\Gruntz | `.?AVCGruntzMultiCommand@@` | name | rtti | game/commands.h |
| CTeleporter | game | C:\Proj\Gruntz | `.?AVCTeleporter@@` | name | rtti+strings | game/world_objects.h |
| CWormhole | game | C:\Proj\Gruntz | `.?AVCWormhole@@` | name | rtti | game/world_objects.h |
| CWarpStonePad | game | C:\Proj\Gruntz | `.?AVCWarpStonePad@@` | name | rtti | game/world_objects.h |
| CFortressFlag | game | C:\Proj\Gruntz | `.?AVCFortressFlag@@` | name | rtti+strings | game/world_objects.h |
| CWarlord | game | C:\Proj\Gruntz | `.?AVCWarlord@@` | name | rtti+strings | game/world_objects.h |
| CDroppedObject | game | C:\Proj\Gruntz | `.?AVCDroppedObject@@` | name | rtti | game/world_objects.h |
| CDroppedObjectShadow | game | C:\Proj\Gruntz | `.?AVCDroppedObjectShadow@@` | name | rtti | game/world_objects.h |
| CObjectDropper | game | C:\Proj\Gruntz | `.?AVCObjectDropper@@` | name | rtti | game/world_objects.h |
| CBattlezDlg | game | C:\Proj\Gruntz | `.?AVCBattlezDlg@@` | name | rtti+strings | game/dialogs.h |
| CBattlezDlgColors | game | C:\Proj\Gruntz | `.?AVCBattlezDlgColors@@` | name | rtti | game/dialogs.h |
| CBattlezDlgCustom | game | C:\Proj\Gruntz | `.?AVCBattlezDlgCustom@@` | name | rtti | game/dialogs.h |
| CMultiStartDlg | game | C:\Proj\Gruntz | `.?AVCMultiStartDlg@@` | name | rtti | game/dialogs.h |
| CMultiHelpDlg | game | C:\Proj\Gruntz | `.?AVCMultiHelpDlg@@` | name | rtti+strings | game/dialogs.h |
| CCheckpointDlg | game | C:\Proj\Gruntz | `.?AVCCheckpointDlg@@` | name | rtti | game/dialogs.h |
| CDoNothing | game | C:\Proj\Gruntz | `.?AVCDoNothing@@` | name | rtti | game/misc.h |
| CDoNothingNormal | game | C:\Proj\Gruntz | `.?AVCDoNothingNormal@@` | name | rtti | game/misc.h |
| CSingleFrameMessage | game | C:\Proj\Gruntz | `.?AVCSingleFrameMessage@@` | name | rtti | game/misc.h |

## Engine substrate (WAP32) classes

| Name | Kind | Source file | RTTI mangled | Known | From | Scaffold |
|---|---|---|---|---|---|---|
| CGameApp | wap32 | (engine, shared) | `.?AVCGameApp@@` | layout | rtti+tomalla | graduated: src/Wap32/Wap32.h |
| CGameMgr | wap32 | (engine, shared) | `.?AVCGameMgr@@` | layout | rtti+tomalla | src/Net/NetMgr.h (matched) |
| CGameWnd | wap32 | (engine, shared) | `.?AVCGameWnd@@` | layout | rtti+tomalla | graduated: src/Wap32/Wap32.h |
| CWapObj | wap32 | (engine) | `.?AVCWapObj@@` | name | rtti | wap32/cwapobj.h |
| CWapX | wap32 | (engine) | `.?AVCWapX@@` | name | rtti | wap32/cwapobj.h |
| zErrHandling | wap32 | (z-runtime) | `.?AVzErrHandling@@` | name | rtti | wap32/zruntime.h |
| zPtrColl | wap32 | (z-runtime) | `.?AVzPtrColl@@` | name | rtti | wap32/zruntime.h |
| zPTree | wap32 | (z-runtime) | `.?AVzPTree@@` | name | rtti | wap32/zruntime.h |
| _zvec | wap32 | (z-runtime) | `.?AV_zvec@@` | name | rtti | wap32/zruntime.h |
| _zdvec | wap32 | (z-runtime) | `.?AV_zdvec@@` | name | rtti | wap32/zruntime.h |

## Template instantiations

| Name | Kind | Source file | RTTI mangled | Known | From | Scaffold |
|---|---|---|---|---|---|---|
| zDArray< int(CUserLogic::*)() > | template/wap32 | (z-runtime) | `.?AV?$zDArray@P8CUserLogic@@AEHXZ@@` | element type | rtti | wap32/zruntime.h + game/userlogic.h |
| CArray<PLAYLISTINFOSTRUCT*> | template/mfc | C:\Proj\Dsndmgr | `.?AV?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@` | element type | rtti+strings | managers/directsoundmgr.h |

## Engine managers (named via strings/leaked paths, NOT in RTTI)

These primary manager classes are mined from strings + leaked source paths; they do
**not** have their own `.?AV…@@` RTTI entry (so no "RTTI mangled" cell). Their
companion classes (DirPal/DirSurf/InputDevice/DSndMgSR/SFManager/CRezDir/RezSync/
ButeMgr) are likewise name-only.

| Name | Kind | Source file | Known | From | Scaffold |
|---|---|---|---|---|---|
| CDirectDrawMgr | manager | C:\Proj\DDrawMgr\DDRAWMGR.CPP | name | strings | managers/cdirectdrawmgr.h |
| DirPal | manager | C:\Proj\DDrawMgr\DIRPAL.CPP | name | srcpath | managers/cdirectdrawmgr.h |
| DirSurf | manager | C:\Proj\DDrawMgr\DIRSURF.CPP | name | srcpath | managers/cdirectdrawmgr.h |
| DirectSoundMgr | manager | C:\Proj\Dsndmgr\DSNDMGR.CPP | name | strings | managers/directsoundmgr.h |
| DSndMgSR | manager | C:\Proj\Dsndmgr\DSndMgSR.cpp | name | srcpath | managers/directsoundmgr.h |
| SFManager | manager | C:\Proj\Dsndmgr | name | strings | managers/directsoundmgr.h |
| PLAYLISTINFOSTRUCT | struct | C:\Proj\Dsndmgr | name (held in CArray) | rtti(template)+strings | managers/directsoundmgr.h |
| DirectInputMgr2 | manager | C:\Proj\DinMgr2\DinMgr2.cpp | name | strings | managers/directinputmgr2.h |
| InputDevice | manager | C:\Proj\DinMgr2\InputDevice.cpp | name | srcpath | managers/directinputmgr2.h |
| RezSync | manager | (REZ/VRZ loader) | name | strings | graduated: src/Rez/RezMgr.h |
| CRezDir | manager | (REZ/VRZ loader) | name | strings | graduated: src/Rez/RezMgr.h |
| ButeMgr | manager | (attributez.txt/dwrects.txt) | name | strings | managers/butemgr.h |

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

All from RTTI; published library types; listed (as comments) in `mfc_runtime.h`.
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
