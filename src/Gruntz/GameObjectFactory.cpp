#include <rva.h>

#include <Gruntz/GameObjectFactory.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/ObjTypeRegistrars.h>
#include <Gruntz/StaticHazard.h>

RVA(0x0000a3b0, 0x6e2)
void RegisterGameObjectLogicTypes(CDDrawSurfaceMgr* ctx) {
    ctx->m_logicRegistry->RegisterLogicType(DispatchAniCycleLogic, "AniCycle", 2);
    CAniCycle::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchDoNothingNormalLogic, "DoNothingNormal", 0);
    ctx->m_logicRegistry->RegisterLogicType(DispatchDoNothingLogic, "DoNothing", 2);
    ctx->m_logicRegistry->RegisterLogicType(DispatchSimpleAnimationLogic, "SimpleAnimation", 2);
    RegisterSimpleAnimLogic();
    ctx->m_logicRegistry->RegisterLogicType(DispatchMenuSparkleLogic, "MenuSparkle", 2);
    RegisterMenuSparkleActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchFrontCandyLogic, "FrontCandy", 2);
    ctx->m_logicRegistry->RegisterLogicType(DispatchBehindCandyLogic, "BehindCandy", 2);
    ctx->m_logicRegistry->RegisterLogicType(DispatchFrontCandyAniLogic, "FrontCandyAni", 2);
    CFrontCandyAni::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchBehindCandyAniLogic, "BehindCandyAni", 2);
    CBehindCandyAni::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchEyeCandyLogic, "EyeCandy", 2);
    ctx->m_logicRegistry->RegisterLogicType(DispatchEyeCandyAniLogic, "EyeCandyAni", 2);
    CEyeCandyAni::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchGruntLogic, "Grunt", 4);
    RegisterGruntActions();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchGlobalAmbientSoundLogic, "GlobalAmbientSound", 4);
    ctx->m_logicRegistry->RegisterLogicType(DispatchAmbientSoundLogic, "AmbientSound", 1);
    ctx->m_logicRegistry->RegisterLogicType(DispatchAmbientPosSoundLogic, "AmbientPosSound", 0);
    ctx->m_logicRegistry->RegisterLogicType(DispatchSpotAmbientSoundLogic, "SpotAmbientSound", 0);
    ctx->m_logicRegistry->RegisterLogicType(DispatchActionAreaLogic, "ActionArea", 4);
    CProjActObj::RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(DispatchStatusBarSpriteLogic, "StatusBarSprite", 2);
    CStatusBarSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchParticlezLogic, "Particlez", 4);
    CParticlez::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchExplosionLogic, "Explosion", 4);
    RegisterExplosionActions();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchGruntSelectedSpriteLogic, "GruntSelectedSprite", 2);
    CGruntSelectedSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchGruntHealthSpriteLogic, "GruntHealthSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchGruntStaminaSpriteLogic, "GruntStaminaSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchGruntToySpriteLogic, "GruntToySprite", 2);
    CGruntToySprite::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchGruntToyTimeSpriteLogic, "GruntToyTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchGruntWingzTimeSpriteLogic, "GruntWingzTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchGruntPowerupSpriteLogic, "GruntPowerupSprite", 2);
    CGruntPowerupSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchToyPeekLogic, "ToyPeek", 4);
    RegisterIconState();
    ctx->m_logicRegistry->RegisterLogicType(DispatchTileTriggerSwitchLogic, "TileTriggerSwitch", 4);
    CTileTriggerSwitch::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchTileTriggerLogic, "TileTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchTileSecretTriggerLogic, "TileSecretTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchBrickzLogic, "Brickz", 4);
    CBrickz::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchTileTriggerTransitionLogic, "TileTriggerTransition", 4);
    CTileTriggerTransition::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchGruntStartingPointLogic, "GruntStartingPoint", 4);
    ActReg4RegisterType();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchGruntCreationPointLogic, "GruntCreationPoint", 4);
    CGruntCreationPoint::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchFortressFlagLogic, "FortressFlag", 4);
    CFortressFlag::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchExitTriggerLogic, "ExitTrigger", 4);
    CExitTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchGiantRockLogic, "GiantRock", 4);
    CTileTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchCoveredPowerupLogic, "CoveredPowerup", 4);
    CTileTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchInGameIconLogic, "InGameIcon", 4);
    RegisterIconActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchInGameTextLogic, "InGameText", 4);
    RegisterTextLogic();
    ctx->m_logicRegistry->RegisterLogicType(DispatchWormholeLogic, "Wormhole", 4);
    RegisterWormholeLogic();
    ctx->m_logicRegistry->RegisterLogicType(DispatchGruntPuddleLogic, "GruntPuddle", 4);
    RegisterLogic();
    ctx->m_logicRegistry->RegisterLogicType(DispatchRollingBallLogic, "RollingBall", 4);
    CRollingBall::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchObjectDropperLogic, "ObjectDropper", 4);
    CObjectDropper::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchDroppedObjectLogic, "DroppedObject", 4);
    CDroppedObject::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchDroppedObjectShadowLogic, "DroppedObjectShadow", 4);
    CDroppedObjectShadow::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchCheckpointTriggerLogic, "CheckpointTrigger", 4);
    CCheckpointTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchTeleporterLogic, "Teleporter", 4);
    CTeleporter_RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchSecretTeleporterTriggerLogic, "SecretTeleporterTrigger", 4);
    CSecretTeleporterTrigger::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(DispatchSecretLevelTriggerLogic, "SecretLevelTrigger", 4);
    CSecretLevelTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchProjectileLogic, "Projectile", 4);
    CProjectile::RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(DispatchBoomerangLogic, "Boomerang", 4);
    CProjectile::RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(DispatchStaticHazardLogic, "StaticHazard", 4);
    CStaticHazard::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchToobSpikezLogic, "ToobSpikez", 4);
    CToobSpikez::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchTimeBombLogic, "TimeBomb", 4);
    CTimeBomb::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchSpotLightLogic, "SpotLight", 4);
    RegisterSpotLightActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchKitchenSlimeLogic, "KitchenSlime", 4);
    CKitchenSlime::RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(DispatchSingleAnimationLogic, "SingleAnimation", 4);
    CSingleAnimation::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchWayPointLogic, "WayPoint", 4);
    ctx->m_logicRegistry->RegisterLogicType(DispatchWarlordLogic, "Warlord", 4);
    RegisterWarlordActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchPathHazardLogic, "PathHazard", 4);
    RegisterPathHazardActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchRainCloudLogic, "RainCloud", 4);
    RegisterPathHazardActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchUFOLogic, "UFO", 4);
    RegisterPathHazardActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchGruntVoiceLogic, "GruntVoice", 4);
    RegisterGruntVoiceActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchWarpStonePadLogic, "WarpStonePad", 4);
    CWarpStonePad::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchGuardPointLogic, "GuardPoint", 4);
    ctx->m_logicRegistry->RegisterLogicType(DispatchVoiceTriggerLogic, "VoiceTrigger", 4);
    CVoiceTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchLevelTimeLogic, "LevelTime", 4);
    ctx->m_logicRegistry->RegisterLogicType(DispatchCursorSnapSpriteLogic, "CursorSnapSprite", 1);
    RegisterCursorSnapActions();
    ctx->m_logicRegistry->RegisterLogicType(DispatchLightFxLogic, "LightFx", 4);
    CLightFx::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(DispatchDemoMoverLogic, "DemoMover", 0);
    ctx->m_logicRegistry->RegisterLogicType(DispatchDemoSignLogic, "DemoSign", 0);
}
