#ifndef GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
#define GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H

#include <Ints.h>

class CDDrawSurfaceMgr;
struct CGameObject;

void RegisterGameObjectLogicTypes(CDDrawSurfaceMgr* ctx);

i32 DispatchAniCycleLogic(CGameObject* obj);
i32 DispatchDoNothingNormalLogic(CGameObject* obj);
i32 DispatchDoNothingLogic(CGameObject* obj);
i32 DispatchSimpleAnimationLogic(CGameObject* obj);
i32 DispatchMenuSparkleLogic(CGameObject* obj);
i32 DispatchFrontCandyLogic(CGameObject* obj);
i32 DispatchBehindCandyLogic(CGameObject* obj);
i32 DispatchFrontCandyAniLogic(CGameObject* obj);
i32 DispatchBehindCandyAniLogic(CGameObject* obj);
i32 DispatchEyeCandyLogic(CGameObject* obj);
i32 DispatchEyeCandyAniLogic(CGameObject* obj);
i32 DispatchGruntLogic(CGameObject* obj);
i32 DispatchGlobalAmbientSoundLogic(CGameObject* obj);
i32 DispatchAmbientSoundLogic(CGameObject* obj);
i32 DispatchAmbientPosSoundLogic(CGameObject* obj);
i32 DispatchSpotAmbientSoundLogic(CGameObject* obj);
i32 DispatchActionAreaLogic(CGameObject* obj);
i32 DispatchStatusBarSpriteLogic(CGameObject* obj);
i32 DispatchParticlezLogic(CGameObject* obj);
i32 DispatchExplosionLogic(CGameObject* obj);
i32 DispatchGruntSelectedSpriteLogic(CGameObject* obj);
i32 DispatchGruntHealthSpriteLogic(CGameObject* obj);
i32 DispatchGruntStaminaSpriteLogic(CGameObject* obj);
i32 DispatchGruntToySpriteLogic(CGameObject* obj);
i32 DispatchGruntToyTimeSpriteLogic(CGameObject* obj);
i32 DispatchGruntWingzTimeSpriteLogic(CGameObject* obj);
i32 DispatchGruntPowerupSpriteLogic(CGameObject* obj);
i32 DispatchToyPeekLogic(CGameObject* obj);
i32 DispatchTileTriggerSwitchLogic(CGameObject* obj);
i32 DispatchTileTriggerLogic(CGameObject* obj);
i32 DispatchTileSecretTriggerLogic(CGameObject* obj);
i32 DispatchBrickzLogic(CGameObject* obj);
i32 DispatchTileTriggerTransitionLogic(CGameObject* obj);
i32 DispatchGruntStartingPointLogic(CGameObject* obj);
i32 DispatchGruntCreationPointLogic(CGameObject* obj);
i32 DispatchFortressFlagLogic(CGameObject* obj);
i32 DispatchExitTriggerLogic(CGameObject* obj);
i32 DispatchGiantRockLogic(CGameObject* obj);
i32 DispatchCoveredPowerupLogic(CGameObject* obj);
i32 DispatchInGameIconLogic(CGameObject* obj);
i32 DispatchInGameTextLogic(CGameObject* obj);
i32 DispatchWormholeLogic(CGameObject* obj);
i32 DispatchGruntPuddleLogic(CGameObject* obj);
i32 DispatchRollingBallLogic(CGameObject* obj);
i32 DispatchObjectDropperLogic(CGameObject* obj);
i32 DispatchDroppedObjectLogic(CGameObject* obj);
i32 DispatchDroppedObjectShadowLogic(CGameObject* obj);
i32 DispatchCheckpointTriggerLogic(CGameObject* obj);
i32 DispatchTeleporterLogic(CGameObject* obj);
i32 DispatchSecretTeleporterTriggerLogic(CGameObject* obj);
i32 DispatchSecretLevelTriggerLogic(CGameObject* obj);
i32 DispatchProjectileLogic(CGameObject* obj);
i32 DispatchBoomerangLogic(CGameObject* obj);
i32 DispatchToobSpikezLogic(CGameObject* obj);
i32 DispatchTimeBombLogic(CGameObject* obj);
i32 DispatchSpotLightLogic(CGameObject* obj);
i32 DispatchKitchenSlimeLogic(CGameObject* obj);
i32 DispatchSingleAnimationLogic(CGameObject* obj);
i32 DispatchWayPointLogic(CGameObject* obj);
i32 DispatchWarlordLogic(CGameObject* obj);
i32 DispatchPathHazardLogic(CGameObject* obj);
i32 DispatchRainCloudLogic(CGameObject* obj);
i32 DispatchUFOLogic(CGameObject* obj);
i32 DispatchGruntVoiceLogic(CGameObject* obj);
i32 DispatchWarpStonePadLogic(CGameObject* obj);
i32 DispatchGuardPointLogic(CGameObject* obj);
i32 DispatchVoiceTriggerLogic(CGameObject* obj);
i32 DispatchLevelTimeLogic(CGameObject* obj);
i32 DispatchCursorSnapSpriteLogic(CGameObject* obj);
i32 DispatchLightFxLogic(CGameObject* obj);
i32 DispatchDemoMoverLogic(CGameObject* obj);
i32 DispatchDemoSignLogic(CGameObject* obj);

i32 DispatchLogicHit(CGameObject* obj);
i32 DispatchLogicAttack(CGameObject* obj);
i32 DispatchLogicBump(CGameObject* obj);

#endif // GRUNTZ_GRUNTZ_GAMEOBJECTFACTORY_H
