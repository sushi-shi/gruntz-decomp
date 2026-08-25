# Inline-pattern inventory — 2026-08-22

Status: investigative snapshot and campaign input. This is not a source-model
claim, or a replacement for the derived `gruntz walls inventory`.

The audit inspected 3,719 source function anchors and normalized instruction
windows across 10,700 retail functions. Percentages were rechecked against the live
inventory at 2026-08-22 10:58 UTC. Concurrent work changed several unrelated scores
during the scan, but none of the candidate rows below changed.

No source files were edited and no build was run during the initial inventory pass.
The implementation campaign recorded below was performed afterward in an isolated
worktree.

## Main findings

The strongest nested-inline signal is coordinate cleanup:

- A `CGrunt::RecycleCoords`-shaped traversal occurs in 32 callers.
- Within it, the `FreeNodePool::Push` free-list splice is expanded in 21 functions.
- Another 17 functions retain the call to `Push`.
- Several functions contain both forms at different sites.

The initial mixed call/expansion population looked budget-dependent, but the campaign
disproved a one-entity explanation for `FreeNodePool::Push`: its body is below VC5's
free-inline threshold, so a canonical header-inline definition expands at every site,
removes the standalone body, and creates 23 broad regressions. The retained model is
an out-of-line `Push` plus an opt-in inline sibling for the proven expansions. This is
the two-entity mechanism documented in
`docs/patterns/two-shapes-need-two-entities.md`.

Other especially strong findings:

- `SoundCue::PlayIfElapsed`: 34 caller functions. Its body is currently out-of-line
  in `src/Gruntz/BootyStateActivate.cpp`, while retail repeatedly contains its
  expansion.
- `CAniElement::AtChecked(0)`: nine callers contain the guarded `GetAt(0)`
  expansion despite the current out-of-line definition in
  `src/Gruntz/GruntEntranceMove.cpp`.
- Animation-complete predicate: 17 callers repeat
  `m_finished != 0 && m_frameTicksLeft == 0`; there is no corresponding method in
  `include/Gruntz/AniAdvanceCursor.h`.
- Pixel packing: the same pack operation occurs across five TUs, while the current
  tree has separate TU-local copies in `src/DDrawMgr/DDSurface.cpp` and
  `src/DDrawMgr/LevelPlane.cpp`.
- The common Grunt AI routines repeatedly contain arrival, elapsed-timer,
  random-point, powered-state-reset, and reroll-timer bodies. `WanderStep` contains
  six detected candidates; `UpdateArrival`, `TryTeleportToCell`, and
  `ScanNearestTarget` each contain five.
- `StepArrivalDrop`, currently 32.303%, already exhibits three nested candidates:
  arrival predicate, coordinate-recycle body, and expanded pool push. Its low
  starting score should not exclude it from later inline accounting.

## Candidate legend

The names below are descriptive labels, not claims about the original spelling.

| Code | Candidate inline body |
|---|---|
| `FP` | `FreeNodePool::Push` free-list splice |
| `CR` | coordinate-list recycle loop, with `Push` retained as a call |
| `LC` | `SoundCue::PlayIfElapsed` |
| `AD` | Grunt has arrived at its stored pixel destination |
| `RX` | select random point inside object extent |
| `HE` | 64-bit hold timer has elapsed |
| `RT` | reset arrival reroll timer, including random delay |
| `RP` | reset powered/entrance state and animation |
| `HS` | hide a sprite and clear its member slot |
| `AC` | animation cursor is complete |
| `A0` | `CAniElement::AtChecked(0)` |
| `TC` | tile `Coord` to pixel-centre pair |
| `SP` | snap and compare a pixel pair to tile centre |
| `SO` | snap an object's `screenX/screenY` to tile centre |
| `SK` | set sort key if changed and mark dirty |
| `BA` | normalize big-animation flags |
| `PK` | pack RGB components into a 16-bit palette entry |
| `R5` | test whether the global pixel format is RGB555 |
| `CI` | common `CImage` clipping block |
| `AV` | scale and clamp ambient volume |

## Complete function matrix

`100.000` means the function was exact and therefore absent from the sub-100
inventory at the snapshot time.

```text
RVA     current  function                                         inlinees
00bf10  100.000  CAmbientSound::Recompute                         AV
00bfb0  100.000  CAmbientSound::Restart                           AV
00c200  100.000  CAmbientSound::SetLevel                          AV
00c2a0   89.257  CAmbientSound::Fade                              AV
00c5b0  100.000  CAmbientPosSound::Update                         AV
018d30   98.851  CBootyState::EnterState                          LC
01b690   95.483  CBootyState::UpdateBootyWalkingGruntz            LC
01c0f0  100.000  CBootyState::CheckPerfectBonus                   LC
01e570  100.000  CMultiBootyState::EnterState                     LC
025d90   95.329  CBattlezMapConfig::StepBoard                     AD,FP
0267c0   85.119  CBattlezMapConfig::StepRowUnits                  AD,HE,TC,FP
029b40   86.263  CBattlezMapConfig::ValidateUnitPath              FP,CR
02a570   72.969  CBattlezMapConfig::RepathAroundBlockedTiles      TC,CR
02c690   88.025  CBattlezMapConfig::ResolveArrival                CR
02edb0   85.515  CBattlezMapConfig::PathToNearestCandidate        CR
02f620   95.085  CBattlezMapConfig::ChooseIdleBehavior            FP
0300c0   85.148  CBattlezMapConfig::RouteUnitTo                   CR
0302c0   91.755  CBattlezMapConfig::RouteUnitToGoal               FP
030b20   87.820  CBattlezMapConfig::PathToNearestGoal             TC,FP
031610   87.230  CBattlezMapConfig::Step                          CR
031ca0   93.453  CBattlezMapConfig::TrackAssignedEnemy            FP
032060   83.043  CBattlezMapConfig::AdvanceToEnemyBase            FP,CR
032ce0   81.910  CBattlezMapConfig::ScanRegion                    CR
0343f0  100.000  CGrunt::RecycleCoords                            FP
0358a0   85.084  CBattlezMapConfig::RetargetIdleUnit              FP,CR
037260   94.257  ScrollDialog                                     LC
03e520   81.971  CGruntCreationPoint::CGruntCreationPoint         SO
03ecf0   97.764  CExitTrigger::CExitTrigger                       SO
0403b0  100.000  CWormhole::SpawnPartners                         AC
040490   56.165  CGruntPuddle::CGruntPuddle                       SO
040d20  100.000  CGruntPuddle::Remove                             AC
041020   96.029  CTeleporter::CTeleporter                         SO
041aa0  100.000  CTeleporter::Update                              AC
041e90   97.876  CSecretTeleporterTrigger::CSecretTeleporterTrigger SO
0424b0   97.780  CSecretLevelTrigger::CSecretLevelTrigger         SO
042d40   78.839  CWarlord::CWarlord                               SO
047090  100.000  CParticlez::Update                               AC
047a10   89.743  CGrunt::CGrunt                                   SK
048360  100.000  CGrunt::OnObjectRemoved                          FP
04ac10   95.397  CGrunt::SetFacing                                A0
04b240  100.000  CGrunt::ClearAllSprites                          HS
04b370   32.303  CGrunt::StepArrivalDrop                          AD,FP,CR
04d060  100.000  CGrunt::SetEntrancePos                           FP
04d3e0   99.722  CGrunt::CreateToyTimeSprite                      HS
04d520   99.706  CGrunt::CreateWingzTimeSprite                    HS
04dd50   95.005  CGrunt::LoadGruntTypeTable                       HS,RP,CR
050a50   98.825  CGrunt::SetupTubeAnim                            RP
051510   98.760  CGrunt::IsDropReady                              SK
0517b0  100.000  CGrunt::SnapToLastTile                           SK
052f40   98.867  CGrunt::ConsiderArrival                          SP
052fb0   93.230  CGrunt::TryTeleportToCell                        HS,SP,SK,RP,FP
0555e0   96.431  CGrunt::LoadStateRecord                          FP
057db0   89.192  CGrunt::PathScan                                 CR
05b050   95.351  CGrunt::CommitNeighbor                           SP
05b6f0  100.000  CGrunt::FindGridNeighbor                         AD
05d210   86.831  CGrunt::StepBehavior                             HS,RP
05ecd0   96.369  CGrunt::FinalizeStep                             SK
05f310   91.850  CGrunt::AdvanceMotion                            AD,TC,SK
060150   91.287  CGrunt::LoadGruntDeathAnimations                 HS,RP
0616e0  100.000  CGrunt::ResetGeometry                            A0
061940   88.742  CGrunt::RearmAttackAnim                          A0
061bc0   95.784  CGrunt::RearmAttackAnim2                         A0
061cb0   98.761  CGrunt::StepAttackFire                           SK
062110   95.475  CGrunt::UpdateArrival                            HS,RP
062840  100.000  CGrunt::StepEntranceRelatchA                     HS,AC,SK,A0
0633e0   92.520  CGrunt::ResolveEntranceArrival                   AD
0637a0   90.462  CGrunt::StepEntranceReinit                       RP
063db0  100.000  CGrunt::LoadVehicleGruntAnimations               AD,HS,AC,A0
0641b0  100.000  CGrunt::BuildGruntExitAnimation                  HS
064540   98.766  CGrunt::StepWarpExit                             AC
0646b0   94.024  CGrunt::StepCombatReaction                       HS,SP,RP,A0
065300  100.000  CGrunt::StepArrivalCommitA                       AC
0654b0  100.000  CGrunt::StepArrivalCommitB                       AC
065c20   98.219  CGrunt::StepEntranceRelatchB                     AC
065e80   80.775  CGrunt::LoadPickupSprites                        HS,RP
067850   94.859  CGrunt::RunEntranceMove                          RP
067f80   89.031  CGrunt::LoadEntranceConfig                       SK
068370  100.000  CGrunt::RearmEntranceDrop                        AC,A0
068520   99.719  CGrunt::StartBombGruntRun                        HS
068880   89.800  CGrunt::LoadWingzGruntSprites                    HS
0690a0  100.000  CGrunt::UpdateEntranceAnim                       AC,SK,A0
0692f0   90.991  CGrunt::StepArrivalCommit                        HS,SP,SK,RP
069fd0  100.000  CGrunt::FinishEntranceMove                       AC
06a6d0   89.264  CGrunt::FinishActiveAction                       HS,SP,SK,RP
06dae0   85.701  CTriggerMgr::ApplyTriggerA                       RP
075e90   79.390  CTriggerMgr::LoadTileArrivalFx                   LC
07b440   92.515  CTriggerMgr::BuildRockBreakParticles             LC
07cc60   89.224  CTriggerMgr::RebuildSelectionList                FP
07d0c0  100.000  CTriggerMgr::ClearSelections                     FP
07e3e0  100.000  CGruntSelectedSprite::CGruntSelectedSprite       SK
0862f0   98.541  CGruntzMgr::HandleCommand                        LC
091250   99.250  CGruntzMgr::CheatSkeletonToggle                  LC
091390  100.000  CGruntzMgr::CheatEclipseToggle                   LC
099110   97.014  CInGameText::CInGameText                         SO
0997c0   96.707  CInGameText::Update                              LC
09d7b0  100.000  CLightFx::AdvanceAnim                            AC
0a05a0  100.000  CMenuState::StartMusic                           LC
0ab940   98.720  CSimpleAnimation::CSimpleAnimation               BA
0abfa0   99.831  CFrontCandy::CFrontCandy                         BA
0ac1d0   99.829  CDoNothing::CDoNothing                           BA
0ac3f0   99.829  CBehindCandy::CBehindCandy                       BA
0ac620   97.849  CEyeCandy::CEyeCandy                             BA
0ac870   98.105  CEyeCandyAni::CEyeCandyAni                       BA
0acf40   91.186  CFrontCandyAni::CFrontCandyAni                   SK
0ad540   94.545  CBehindCandyAni::CBehindCandyAni                 BA
0aed80  100.000  CSingleAnimation::AdvanceAnim                    AC
0b0140   89.546  CRollingBall::Update                             SK
0b1af0   83.298  CSpotLight::Tick                                 LC
0b4640  100.000  CRainCloud::HitTest                              LC
0b86c0  100.000  CMulti::ShowMultiStartDlg                        LC
0ba620  100.000  CMulti::HandlePlayerCreated                      LC
0c7350  100.000  CDroppedObject::AdvanceAnimation                 AC
0c7490  100.000  CDroppedObjectShadow::CDroppedObjectShadow       SK
0de420  100.000  CPreviewState::LoadLevelPreviewScreen            LC
0def60  100.000  CProjectile::~CProjectile                        FP
0e2df0  100.000  CSpriteRef::Build                                PK
0e8310  100.000  CSBI_MenuItem::SetState                          LC
0ec670   95.291  CGrunt::ResolveArrivalReposition                 RX,HE,RT
0ecc90   83.830  CGrunt::StepBrickLayerBehavior                   AD,CR
0ed9f0   87.862  CGrunt::WanderStep                               AD,RX,HE,RT,FP,CR
0ee800   85.226  CGrunt::ArrivalReticleScan                       AD,FP,CR
0ef6b0   83.910  CGrunt::ChargeStep                               AD,RX,RP
0f0130   90.919  CGrunt::UpdateArrival                            AD,RX,HE,RT,RP
0f0e20   81.559  CGrunt::StepGooSuckerBehavior                    AD,CR
0f1c70   79.831  CGrunt::StepArrivalDefenseAlt                    AD,RP
0f26f0   86.544  CGrunt::ResolveArrivalNeighbor                   AD
0f2b20   85.678  CGrunt::StepArrivalDefense                       AD,RX,HE
0f36a0   83.869  CGrunt::StepDiggerBehavior                       AD
0f42f0   94.784  CGrunt::ScanNearestTarget                        AD,RX,HE,RT,RP
0f60f0   82.857  CGrunt::PhaseStep                                AD,FP
0f71c0   85.630  CGrunt::SeekTarget                               AD,RP,CR
0f7d90   97.992  CGrunt::StepPeerTracking                         AD
0f8240   88.973  CGrunt::StepArrivalDefenseLean                   AD,RX,HE
0fb7a0   91.685  CStaticHazard::CStaticHazard                     SO
0fe910   94.865  CStatusBarMgr::UpdateStatusBarTabHighlight       LC
0ff850   97.584  CStatusBarMgr::ClickHilite                       LC
104e60  100.000  CStatusBarMgr::LoadStatzTabToggleSprite          LC
1055b0  100.000  CStatusBarMgr::LoadGooCookingSprite              LC
105e40   99.154  CStatusBarMgr::LoadRezMachineConfig              LC
106bb0   94.149  CStatusBarMgr::LoadChipMachineConfig             LC
109bd0   91.329  CWarpStoneFly::Init                              LC
10b5d0  100.000  CStatusBarMgr::HlClickGroup0                     LC
10b6f0  100.000  CStatusBarMgr::HlClickGroup1                     LC
10b810  100.000  CStatusBarMgr::HlClickGroup2                     LC
10c230  100.000  CStatusBarSprite::CStatusBarSprite               SK
110110  100.000  CTileTriggerTransition::TransitionAct            AC
110570   93.598  CTileTriggerSwitchLogic::SwitchDown              LC
1106b0   93.598  CTileTriggerSwitchLogic::SwitchUp                LC
1122a0   99.459  CGiantRockLogic::BuildRockBreakInGameText        LC
114120  100.000  SoundCueRegistry::PlayCueIfElapsed               LC
1145c0   96.453  CToobSpikez::CToobSpikez                         SK
119b50   98.317  CVoiceTrigger::CVoiceTrigger                     SO
13f020   70.144  CDDSurface::ShadeBlt                             R5
13f460   76.991  CDDSurface::ShadeRect                            R5
13f740  100.000  BuildColorChannelTables                          R5
13fbb0   99.956  CDDSurface::Blit168                              PK
13fce0   95.148  CDDSurface::Blit1624                             PK
1495d0   91.007  CDDrawShadeBlit::EncodeRle16                     PK
1497f0  100.000  CDDrawShadeBlit::Blit                            R5
14eef0   92.864  CShadeTableCache::GreyTable                      R5
14f5b0   99.747  CShadeTableCache::AlphaTable                     PK
1538c0   98.759  CImage::BlitNorm                                 CI
153b20   97.464  CImage::BlitFlipV                                CI
153d90   97.990  CImage::BlitFlipH                                CI
153ff0   97.197  CImage::BlitShadeFlipHV                          CI
154270   99.852  CImage::BlitShadeNorm                            CI
1544d0   99.828  CImage::BlitShadeFlipV                           CI
154750   99.258  CImage::BlitShadeFlipH                           CI
163670  100.000  CDDrawWorkerHost::ResolveColorKey                PK
183030  100.000  CMenuTree::PlayFocusSound                        LC
1830b0  100.000  CMenuTree::PlayActivationSound                   LC
```

## Population qualifications

Most matrix entries are supported directly by repeated normalized retail
instructions. The few broader source-semantic additions are:

- `RX`: six exact retail clones plus `ScanNearestTarget`, whose scheduling differs.
- `HS`: six exact retail clones; eleven additional CGrunt functions contain the
  same hide-and-null body.
- `BA`: six exact clones plus `CDoNothing`.
- `CI`: five long exact clones, six shorter exact clones, seven source-equivalent
  functions.
- `AV`: four exact clones plus the same block inside `CAmbientPosSound::Update`.

The sort-key family is intentionally conservative: 18 exact retail-pattern
functions are in the matrix, although 67 source functions contain the broader
"compare sort key, store, set dirty" idiom.

## Confirmed existing inlines

These were also found, but are already explicitly modeled or are library/compiler
inlines rather than new candidates:

- `GetRandomNumber`: 38 expansion sites in 20 functions. Current nonexact examples
  include `CRandomAmbientSound::InitCycleTiming` 75.692%, `CSpotLight::Tick`
  83.298%, `CInGameIcon::PeekCycle` 88.678%, both
  `CVoiceManager::PlayVoice` overloads at 90.104% and 92.473%, and
  `CFaderSine::RenderFrame` 88.888%.
- `CDDrawWorker::GetAt`: `Refresh` 93.243%, `RenderCel` 75.636%,
  `CSBI_ImageSet::Render` 87.842%, `CSBI_GruntMachine::Render` 95.769%,
  `CSBI_StatzTabGruntBar::Update` 90.918%, `PlaceFrame` 100%, and
  `CDDrawWorkerHost::Draw` 80.615%.
- `CGameLevel::PointInBounds`/`PointInRect`: 28 functions across 18 TUs.
- `CMapMgr::CellFlagsAt` and related `m_rows[y][x]` accessors: numerous
  register/schedule variants across at least 23 functions and 12 TUs.
- `LOGIC_WORKER_PUMP`: 68 functions across 17 TUs.
- Act-name registrar/grow machinery: 65 functions across 43 TUs.
- `PROBE_TILE`: 26 functions.
- MFC `CArray::SetSize`, list traversal/GetNext, CString comparisons,
  destructors/EH, and CRT digit/pixel-copy loops were classified as
  library/compiler patterns and excluded from the candidate matrix.

## Scope boundary

The matrix remains the identification snapshot, not a hand-maintained worklist. The
campaign outcomes below qualify it; future work must still recheck the current source
hash, live percentage, retail call set, CFG, and ordered referents.

## Implementation campaign outcomes

Kept real inline abstractions, counted at call sites:

- `PushFreeNode`: 32 expanded free-list splices, with the standalone
  `FreeNodePool::Push` wrapper retained.
- `RecycleGruntCoords`: five coordinate-list cleanup bodies.
- `IsGruntAtSavedScreenPos`: two arrival predicates.
- `IsAniCursorComplete`: 17 animation-complete predicates.
- `GetAniElementAt`: nine guarded animation-element lookups.
- `PlaySoundCueIfElapsed`: eight elapsed-cue bodies in the green subset of callers.
- `ScaleAmbientVolume`: seven volume calculations.
- `ResetGruntArrivalReroll`: five full reroll resets.
- `IsGruntArrivalRerollPending`: six 64-bit reroll-window predicates.

Kept exact-expansion macro fallbacks:

- `HIDE_AND_CLEAR_GRUNT_SPRITE`: 49 sprite-retirement bodies across seven TUs.
  The six bodies in `ClearAllSprites` remain textual because macro-origin metadata
  before `StepArrivalDrop` changes that later caller's inline set even though
  `ClearAllSprites` itself remains exact.
- `RESET_GRUNT_ARRIVAL_REROLL_COMPACT`: the shorter `DefenseLean` spelling. The
  real full helper moved that function 88.9729% to 88.7946%; the macro restores it.
- `SNAP_OBJECT_TO_TILE_CENTER`: 13 complete two-axis tile-centre expansions. All
  owner-member, object-member, free-helper, and by-reference function forms had
  lowered the ten initially measured constructor callers; the token-preserving
  macro is green across those callers and three additional in-function sites.
- `NORMALIZE_BIG_ANIMATION_WITH_AUX` / `NORMALIZE_BIG_ANIMATION_DIRECT`: all seven
  big-animation blocks, preserving the two observed receiver spellings. The real
  helpers lowered every caller by roughly four to five points; both macro forms
  preserve every measured caller's pre-macro fuzzy score.

Measured score improvements retained by the campaign:

| Function | Snapshot | Retained | Delta |
|---|---:|---:|---:|
| `CAmbientSound::Fade` | 89.25694% | 91.80556% | +2.54862 |
| `CGrunt::ArrivalReticleScan` | 85.22585% | 85.99320% | +0.76735 |
| `CBootyState::UpdateBootyWalkingGruntz` | 95.48321% | 95.99254% | +0.50933 |
| `CGruntzMgr::HandleCommand` | 98.54093% | 98.56809% | +0.02716 |

`StepArrivalDrop` is the local-minimum control. Making the coordinate cleanup and
arrival predicate visible initially sent it to 0.000%; composing the cleanup helper,
the selected arrival site, and the expanded pool-splice sibling recovered its banked
32.302708%. This validates the instruction to finish the caller rather than reverting
the first score drop in isolation.

Rejected or bounded candidates:

- `PK` pixel packing: a definition header placed at the top of `DDSurface.cpp`
  trades one exact closure in `LevelPlane` for a fresh 0.027-point regression in
  `CDDSurface::ShadeRect`. Scoping the header below `ShadeRect` retains stable macro
  uses in `BuildColorChannelTables` and `Blit1624`; the earlier TU-local helper and
  the exact `CSpriteRef::Build` / `CDDrawShadeBlit::EncodeRle16` spellings stay
  textual.
- `HS` sprite retirement as a pointer-reference function: `ClearAllSprites` stays
  exact, but exact `BuildGruntExitAnimation` moves to 98.4167% through register
  scheduling. The macro fallback above preserves the expansion.
- Broad `SoundCue` conversion: several exact callers regress despite local include
  scoping. Only the eight green sites are retained.

## Exhaustion pass

The follow-up pass reconciled every code in the matrix and then searched the source
again for the raw bodies. No inventory family remains unmodeled: every one now has a
real inline helper or a used token-preserving macro fallback. This is exhaustion of
the audited candidate inventory, not a claim that no future retail comparison can
reveal another inline.

| Code | Retained representation and source-wide result |
|---|---|
| `FP` | `PushFreeNode` plus the inline-pool recycle macro variants preserve both observed free-list spellings. |
| `CR` | Five real `RecycleGruntCoords` call sites and 50 recycle-macro invocations cover the call, inline-push, direct-pool, and MFC `POSITION` traversals. The explicit `StepArrivalDrop` traversal is the load-bearing nested-inline control. |
| `LC` | Seven caller expansions plus the standalone wrapper use `PlaySoundCueIfElapsed`; the wrapper also uses the call-preserving `PLAY_LEAF_CUE_INLINE_HELPER` macro. The remaining direct callers are bounded by measured exact-caller regressions. |
| `AD` | Two real-helper sites and 67 movement-predicate macro sites cover the detected and broader source population. `StepArrivalDrop` and `StepDiggerBehavior` retain their direct predicate spelling because placing the definition boundary above them breaks MAX. |
| `RX` | All seven source-visible random-extent variants use one of the six exact-expansion macros. |
| `HE` | All seven detected hold/reroll predicates use `IsGruntHoldPending` or `IsGruntArrivalRerollPending`. |
| `RT` | Five full helper sites and the compact macro variant cover all six reset bodies. |
| `RP` | 33 self/receiver reset bodies use the powered-state macros. The two extra `StepDiggerBehavior` bodies remain the direct boundary control. |
| `HS` | 49 retirement bodies use `HIDE_AND_CLEAR_GRUNT_SPRITE`; the six pre-`StepArrivalDrop` bodies in `ClearAllSprites` remain direct. |
| `AC` | The real helper now covers all 34 safe source-visible predicates. The sole raw predicate is the pre-helper-boundary compound in `RunEntranceMove`. |
| `A0` | The real helper now covers 23 safe guarded lookups. Two pre-`StepArrivalDrop` lookups stay direct, and `DEATH_FRAME` was already a macro. |
| `TC` | All 13 audited adjacent tile-centre pairs use the set/declare macros. |
| `SP` | All six audited functions use the snapped-pair declaration and comparison macros (12 sites). |
| `SO` | All 13 audited object snap bodies use `SNAP_OBJECT_TO_TILE_CENTER`. |
| `SK` | 72 exact compare/store/dirty bodies use the macros. `CGrunt`'s constructor stays direct because a definition/use before `StepArrivalDrop` changes that later inline set; the remaining flag-local variants are distinct schedules. |
| `BA` | All seven audited big-animation normalizations use the two receiver-shape macros. |
| `PK` | Four stable pack macro sites remain in three TUs; the earlier `DDSurface` helper and the two definition-boundary-sensitive functions retain direct spellings. |
| `R5` | Two stable RGB555 macros remain below sensitive functions; the three earlier checks retain direct spelling. |
| `CI` | All seven `CImage` functions use `DECLARE_CLIPPED_IMAGE_RECT`. |
| `AV` | All seven volume blocks use `ScaleAmbientVolume`. |

The full build after this pass scored 3,704 exact functions at 95.01% overall fuzzy,
with 82 carried below-bank rows and zero fresh regressions. The load-bearing control
scores were restored to `StepArrivalDrop` 32.302708%, `CSpriteRef::Build` 100.0000%,
`StepDiggerBehavior` 83.86855%, `ConvertRowDoubleFwd` 71.84295%, and
`CDDSurface::ShadeRect` 76.990906%.

## Second follow-up: semantic source pass

A second source-wide semantic scan found six families that the initial normalized
retail-window inventory did not group. Each was first tested as a real inline on
representative callers. The local instruction streams matched, but definitions in
the hot shared headers perturbed unrelated C1 state. The retained representation is
therefore token-preserving macros, including separate retail-observed source shapes:

| Family | Retained coverage |
|---|---:|
| animation-act transition (`m_prevAnimSetNode` then `ActFindId`) | 110 sites |
| draw-fill table setup | 57 sites |
| draw-fill fraction setup | 7 sites |
| scaled/raw plane scroll plus `RecomputePlaneCoords` | 9 sites |
| arrival-target reset/copy | 5 sites |
| same-target `CommitNeighbor` argument expansion | 29 sites |

The neighbor family needs two macro shapes: eleven callers originally read the
target fields directly, while eighteen first copied `m_lastTilePx` into a `Coord`.
Collapsing both populations to direct reads regressed seven functions by 0.79 to
5.57 points; restoring the copy inside the second macro restored all seven current
fingerprints. See `docs/patterns/inline-macro-must-preserve-caller-local.md`.

The one animation transition with an intervening `m_priority` store remains
textual because combining it would reorder observable stores. Partial draw-fill
updates likewise remain textual; the macros cover only complete adjacent field
sets.

The authoritative full build on current `main` after this follow-up scored 3,738
exact functions at 95.18% overall fuzzy, with 75 carried below-bank rows and zero
fresh regressions.

## Third follow-up: residual whole-source pass

A third pass rescanned every `RVA`-anchored function after the first two campaigns,
then audited the remaining repeated statement sequences against the current header
inlines. The normalized source-clone population fell from 166 to 156 sequences. The
ten removed motifs were cohesive operations; the surviving high-frequency motifs are
null guards, `CFile::Open`, `GetDC`/`Lock` prologues, serialization scaffolding,
separate out-of-line `IsLoaded` bodies, and generic tile/rectangle arithmetic. Those
survivors are not evidence of another missing inline boundary.

New real inline abstractions retained by this pass:

- `FlipFrontAndRestoreOverlay`: three identical page flip plus overlay-restore bodies
  in both booty renderers and `CMenuTree::PresentFrame`.
- `TickSoundVolumeRamps`: thirteen guarded sound-stream purge bodies. A member declaration in
  the widely included `SoundCueRegistry` header produced eleven fresh regressions;
  the free inline in a narrow definition header retained the caller bodies and restored
  the full gate.
- `CMapMgr::CellFlagsAt`: four final guarded flag reads in
  `CBattlezMapConfig::{StepRowUnits,ValidateUnitPath}` plus the complete
  `CMapMgr::IsCellClear` wrapper. `IsCellClear` remains exact, and both large callers
  retain their pre-conversion scores.
- `CDDrawChildGroup::NextChild`: the final eight typed `m_list.GetNext` expansions in
  `WwdSpatialMgr`, `BrickzLoad`, `BattlezMapConfig`, and `TriggerMgr` now use the real
  inline. No typed raw child-group traversal remains.
- `CDDrawWorker::GetAt`: every remaining complete bounds-check/frame-access body was
  folded. Real calls are retained where VC5 reproduces the retail expansion, including
  `CWwdGameObjectA::SwitchGeometry` (which became exact), serializers, glyph loaders,
  status widgets, and the worker's own `GetFrame`/`ReloadFrame` wrappers.

New exact-expansion macro fallbacks:

- the four `DDRAW_WORKER_*` range-test shapes plus
  `DDRAW_WORKER_FRAME_AT_UNCHECKED` preserve guarded `GetAt` callers whose real inline
  changed CFG or register scheduling;
- `GET_SCREEN_TILE_Y_FIRST` preserves the two reversed-axis expansions in
  `CBattlezMapConfig::StepRowUnits` (real `GetScreenTile` changed 85.4017% to
  85.1993%);
- `INITIALIZE_STATUS_BAR_ITEM` covers four five-statement widget initialization
  bodies; the real inline moved an exact later `CSBI_ImageSetAni::Render` to 99.8667%;
- `READ_TILE_IMAGE_DIMENSIONS` covers the three tile-image parsers. The shared
  `m_height` field was moved from all three derived classes to `CTileImageSet`, keeping
  every object layout unchanged. A real helper left `CImageSet1/2::Parse` unchanged
  and moved exact `CImageSet3::Parse` to 70.3051%; the token macro restores exact;
- the existing caller-shape macros cover the last `NextChild`, pickup, lookup,
  object-flag, and frame-selection sites where a real helper changed a caller or later
  TU state.

The complete guarded `CDDrawWorker::GetAt`, `CMapMgr::CellFlagsAt`, reversed
`GetScreenTile`, and typed `CDDrawChildGroup::NextChild` raw-body searches are now
empty apart from the helper definitions and deliberate unchecked first/last-frame
accesses. The final clone audit therefore has no remaining actionable inline-shaped
motif.

The authoritative full build after this pass scored 3,741 exact functions at 95.20%
overall fuzzy, with 73 carried below-bank rows, 328 strict-below rows (255 sub-EPS),
and zero fresh regressions.

## Fourth follow-up: definition-against-every-function pass

The final pass changed the question from repeated local motifs to whether the complete
body of every reconstructed function occurs as a statement subsequence inside any other
function. Receiver names, local names, and harmless wrapper syntax were normalized. A
second scanner compared optimized retail instruction fragments so that an expansion
whose prologue, registers, or surrounding schedule differs from its standalone body was
still considered.

- 3,709 `RVA`-bound source definitions were compared against every other function.
- The final nested source scan contains 51 candidate records. The survivors are the
  retained macro invocations below, generic one-statement operations, separate virtual
  overrides, or the bounded cases listed after the table.
- The optimized-retail scan produced 418 broad fragments. Requiring high body coverage
  and useful instruction mass reduced that to 91; every one was reconciled against
  source ownership, retail calls, branches, and referents.

The following standalone functions are exact in the final experimental build. The
`expanded inside` column names the functions in which their complete useful body was
open-coded. A semicolon-separated list means multiple expansion sites or source shapes.

| Standalone function | Current | Expanded inside |
|---|---:|---|
| `CDDrawWorkerHost::Build` | 100.000% | `CDDrawWorkerHost::Read`, `CDDrawWorkerHost::InitGeometry` |
| `CTimer::Reset` | 100.000% | `CTimer::CTimer` |
| `CVoiceManager::ClearVoiceIndicatorSlots` | 100.000% | `CVoiceManager::Init` |
| `CDDrawChildGroup::RemoveByPosition` | 100.000% | `CDDrawChildGroup::RemoveAll` |
| `AnimationRegistry::AddAnimation` | 100.000% | `LoadAnimationFromSource`, `LoadAnimationFromFile` |
| `SoundCueRegistry::AddCue` | 100.000% | `LoadCueFromSource`, `LoadCueFromFile` |
| `CGameApp::FreeGameManager` | 100.000% | `CGameApp::CloseResources` |
| `CGrunt::DestroyAnims` | 100.000% | `LoadGruntDeathAnimations`, `BuildGruntExitAnimation` |
| `CGameLevel::SyncToMainIndex` | 100.000% | first half of `VisitVisible` |
| `CGameLevel::SyncAfterMainIndex` | 100.000% | second half of `VisitVisible` |
| `CGameLevel::ResetMainPlane` | 100.000% | `CGameLevel::RemovePlane` |
| `CMenuTree::InitializeMembers` | 100.000% | `CMenuTree::Reset` |
| `GruntzPlayer::Clear` | 100.000% | `GruntzPlayer::Reset` |
| `CDDrawWorker::AddFrameAt` | 100.000% | `InsertFrame`, `LoadFrame`, `CreateDescriptorFrame`, `CreateBlankFrame` |
| `CGameLevel::ReleaseChildren` | 100.000% | `CGameLevel::Unload` |
| `CWwdGameObject::Clear` | 100.000% | `CWwdGameObject::Setup` |
| `CMenuPage::ResolveSubPage` | 100.000% | `CMenuPage::Configure` |
| `CGruntzMgr::IsStandardMode` | 100.000% | `CGruntzMgr::RestoreVideoMode` |
| `CAniElement::DeleteAll` | 100.000% | failure cleanup in `CAniElement::Build` |
| `CDDrawWorkerHost::SetCell` | 100.000% | 24 complete tile-grid stores in loaders, triggers, and map setup |
| `CDDrawWorkerHost::SetTileSizeFromImage` | 100.000% | `CDDrawWorkerHost::Read`, `SetTileSizeFromImageSet` |
| `CDDrawChildGroup::RegisterObjectId` | 100.000% | `CDDrawChildGroup::InsertSorted` |
| `Font::SetGlyph` | 100.000% | `Font::AllocateMemory` |
| `CDDrawSurfacePair::BltSelf` | 100.000% | `CreateOverlay`, `TransTitle`, `TransExit`, `CCreditsState::StepVideo` |
| `CDDrawWorkerBase::SetPosition` | 100.000% | `CDDrawWorkerA::PlaceFrameValue`; three `CDDrawWorkerB::Place*` functions |
| `CGameMgr::Close` | 100.000% | `CGameMgr::CGameMgr` |
| `CWwdGameObjectA::ApplyGeometryDirect` | 100.000% | `CWwdGameObjectA::ApplyLookupGeometry` |
| `CMapArrayA/B` zero-state bodies | 100.000% | both constructors and both `Free` tails |

Every family is represented once by a named macro because a real helper either changed
the retail source shape or could not retain both the expansion and the pinned standalone
COMDAT. `ApplyGeometryDirect` is a controlled example: a narrow real inline preserved the
two target functions but moved the later exact `CDDrawWorker::GetMemoryUsage` to 99.9623%.
The macro fallback restored all three to 100.000%.

`CDDrawWorkerHost::Build` proves why a canonical header-inline member is not always the
answer. Making that member visible to both TUs expanded every use, removed its standalone
body, and changed the two retail call sites in `CGameLevel`. The retained macro preserves
the exact pinned body, the two expansions, and the two real calls. Its definition boundary
does move the later `CDDrawWorkerHost::Draw` from its banked 85.6551% to 81.5202% while
`Build` remains exact, `Read` remains 97.36%, and `InitGeometry` remains 97.71%. This is a
measured C1 state effect, not evidence against the recovered inline relationship.

`CGameLevel::ReleaseChildren` supplies the second retained descent. Its complete body is
expanded in `Unload`; consolidating it leaves the standalone exact and moves `Unload`
from 100.0000% to 99.8824% through an independent-store schedule. Both cases are humane,
source-structural macro fallbacks with their historical MAX preserved, and are adjudicated
as documented keeps under the exploratory-descent ruling.

Bounded and rejected final candidates:

- `CDDrawWorkerHost::SetTileSize` shares substantial optimized arithmetic with
  `InitGeometry`, but it is not expanded there as one body. A whole-body macro moved
  `InitGeometry` from 97.7143% to 65.08%. `gruntz walls diagnose 0x1619f0` found the same
  five calls, eight branches, one return, and seven relocations, but retail stores grid
  size, tile size, bounds, and movement before the wrap products and loops. No legal
  single insertion point reproduces that interleaving, so the experiment was reverted.
- `CParticlez::Update` is a suffix of `CProjectile::DetachRenderObj`, but the handlers
  have incompatible owners and different semantics. Their real shared predicate is
  already `IsAniCursorComplete`; inventing a cross-class call would be a model defect.
- `CImage::RenderFrame` and `RenderFrameClipped` have similar clipping blocks but distinct
  function-local statics and dynamic-initializer identities. Combining them would merge
  retail storage identities.
- `StreamVoice::~StreamVoice` is not manually expanded in `SoundStream::DestroyVoice`:
  `delete voice` already invokes the destructor, while the explicit feeder reset is a
  separate pre-destruction operation.
- the repeated `IsLoaded` bodies are independent overrides; state `ReleaseResources`
  bodies are base calls; serialization bodies already use their common macro; music
  mute/restore and UI `PostMessage` hits are only generic guard/return subsequences.

After these classifications, no remaining source or optimized-retail candidate supports
another missing inline boundary.
