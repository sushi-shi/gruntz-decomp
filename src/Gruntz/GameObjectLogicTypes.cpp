#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/GameObjectLogicTypes.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/ObjTypeRegistrars.h>
#include <Gruntz/StaticHazard.h>

RVA(0x0000a3b0, 0x6e2)
void RegisterGameObjectLogicTypes(CGameWorld* ctx) {
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchAniCycleLogic, "AniCycle", 2);
    CAniCycle::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchDoNothingNormalLogic, "DoNothingNormal", 0);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchDoNothingLogic, "DoNothing", 2);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchSimpleAnimationLogic, "SimpleAnimation", 2);
    RegisterSimpleAnimLogic();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchMenuSparkleLogic, "MenuSparkle", 2);
    RegisterMenuSparkleActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchFrontCandyLogic, "FrontCandy", 2);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchBehindCandyLogic, "BehindCandy", 2);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchFrontCandyAniLogic, "FrontCandyAni", 2);
    CFrontCandyAni::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchBehindCandyAniLogic, "BehindCandyAni", 2);
    CBehindCandyAni::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchEyeCandyLogic, "EyeCandy", 2);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchEyeCandyAniLogic, "EyeCandyAni", 2);
    CEyeCandyAni::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchGruntLogic, "Grunt", 4);
    RegisterGruntActions();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGlobalAmbientSoundLogic, "GlobalAmbientSound", 4);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchAmbientSoundLogic, "AmbientSound", 1);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchAmbientPosSoundLogic, "AmbientPosSound", 0);
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchSpotAmbientSoundLogic, "SpotAmbientSound", 0);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchActionAreaLogic, "ActionArea", 4);
    CProjActObj::RegisterType();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchStatusBarSpriteLogic, "StatusBarSprite", 2);
    CStatusBarSprite::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchParticlezLogic, "Particlez", 4);
    CParticlez::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchExplosionLogic, "Explosion", 4);
    RegisterExplosionActions();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGruntSelectedSpriteLogic, "GruntSelectedSprite", 2);
    CGruntSelectedSprite::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGruntHealthSpriteLogic, "GruntHealthSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGruntStaminaSpriteLogic, "GruntStaminaSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchGruntToySpriteLogic, "GruntToySprite", 2);
    CGruntToySprite::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGruntToyTimeSpriteLogic, "GruntToyTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGruntWingzTimeSpriteLogic, "GruntWingzTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGruntPowerupSpriteLogic, "GruntPowerupSprite", 2);
    CGruntPowerupSprite::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchToyPeekLogic, "ToyPeek", 4);
    RegisterIconState();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchTileTriggerSwitchLogic, "TileTriggerSwitch", 4);
    CTileTriggerSwitch::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchTileTriggerLogic, "TileTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchTileSecretTriggerLogic, "TileSecretTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchBrickzLogic, "Brickz", 4);
    CBrickz::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchTileTriggerTransitionLogic, "TileTriggerTransition", 4);
    CTileTriggerTransition::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGruntStartingPointLogic, "GruntStartingPoint", 4);
    RegisterGruntStartingPointActions();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchGruntCreationPointLogic, "GruntCreationPoint", 4);
    CGruntCreationPoint::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchFortressFlagLogic, "FortressFlag", 4);
    CFortressFlag::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchExitTriggerLogic, "ExitTrigger", 4);
    CExitTrigger::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchGiantRockLogic, "GiantRock", 4);
    CTileTrigger::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchCoveredPowerupLogic, "CoveredPowerup", 4);
    CTileTrigger::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchInGameIconLogic, "InGameIcon", 4);
    RegisterIconActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchInGameTextLogic, "InGameText", 4);
    RegisterTextLogic();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchWormholeLogic, "Wormhole", 4);
    RegisterWormholeLogic();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchGruntPuddleLogic, "GruntPuddle", 4);
    RegisterLogic();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchRollingBallLogic, "RollingBall", 4);
    CRollingBall::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchObjectDropperLogic, "ObjectDropper", 4);
    CObjectDropper::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchDroppedObjectLogic, "DroppedObject", 4);
    CDroppedObject::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchDroppedObjectShadowLogic, "DroppedObjectShadow", 4);
    CDroppedObjectShadow::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchCheckpointTriggerLogic, "CheckpointTrigger", 4);
    CCheckpointTrigger::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchTeleporterLogic, "Teleporter", 4);
    CTeleporter_RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchSecretTeleporterTriggerLogic, "SecretTeleporterTrigger", 4);
    CSecretTeleporterTrigger::RegisterActs();
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchSecretLevelTriggerLogic, "SecretLevelTrigger", 4);
    CSecretLevelTrigger::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchProjectileLogic, "Projectile", 4);
    CProjectile::RegisterType();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchBoomerangLogic, "Boomerang", 4);
    CProjectile::RegisterType();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchStaticHazardLogic, "StaticHazard", 4);
    CStaticHazard::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchToobSpikezLogic, "ToobSpikez", 4);
    CToobSpikez::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchTimeBombLogic, "TimeBomb", 4);
    CTimeBomb::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchSpotLightLogic, "SpotLight", 4);
    RegisterSpotLightActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchKitchenSlimeLogic, "KitchenSlime", 4);
    CKitchenSlime::RegisterType();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchSingleAnimationLogic, "SingleAnimation", 4);
    CSingleAnimation::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchWayPointLogic, "WayPoint", 4);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchWarlordLogic, "Warlord", 4);
    RegisterWarlordActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchPathHazardLogic, "PathHazard", 4);
    RegisterPathHazardActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchRainCloudLogic, "RainCloud", 4);
    RegisterPathHazardActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchUFOLogic, "UFO", 4);
    RegisterPathHazardActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchGruntVoiceLogic, "GruntVoice", 4);
    RegisterGruntVoiceActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchWarpStonePadLogic, "WarpStonePad", 4);
    CWarpStonePad::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchGuardPointLogic, "GuardPoint", 4);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchVoiceTriggerLogic, "VoiceTrigger", 4);
    CVoiceTrigger::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchLevelTimeLogic, "LevelTime", 4);
    ctx->GetLogicRegistry()
        ->RegisterLogicType(DispatchCursorSnapSpriteLogic, "CursorSnapSprite", 1);
    RegisterCursorSnapActions();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchLightFxLogic, "LightFx", 4);
    CLightFx::RegisterActs();
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchDemoMoverLogic, "DemoMover", 0);
    ctx->GetLogicRegistry()->RegisterLogicType(DispatchDemoSignLogic, "DemoSign", 0);
}
