#include <rva.h>

#include <Gruntz/GameObjectFactory.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/ObjTypeRegistrars.h>
#include <Gruntz/StaticHazard.h>

RVA(0x0000a3b0, 0x6e2)
void RegisterGameObjectTypes(CDDrawSurfaceMgr* ctx) {
    ctx->m_logicRegistry->RegisterLogicType(CreateAniCycle, "AniCycle", 2);
    CAniCycle::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateDoNothingNormal, "DoNothingNormal", 0);
    ctx->m_logicRegistry->RegisterLogicType(CreateDoNothing, "DoNothing", 2);
    ctx->m_logicRegistry->RegisterLogicType(CreateSimpleAnimation, "SimpleAnimation", 2);
    RegisterSimpleAnimLogic();
    ctx->m_logicRegistry->RegisterLogicType(CreateMenuSparkle, "MenuSparkle", 2);
    RegisterMenuSparkleActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateFrontCandy, "FrontCandy", 2);
    ctx->m_logicRegistry->RegisterLogicType(CreateBehindCandy, "BehindCandy", 2);
    ctx->m_logicRegistry->RegisterLogicType(CreateFrontCandyAni, "FrontCandyAni", 2);
    CFrontCandyAni::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateBehindCandyAni, "BehindCandyAni", 2);
    CBehindCandyAni::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateEyeCandy, "EyeCandy", 2);
    ctx->m_logicRegistry->RegisterLogicType(CreateEyeCandyAni, "EyeCandyAni", 2);
    CEyeCandyAni::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGrunt, "Grunt", 4);
    RegisterGruntActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateGlobalAmbientSound, "GlobalAmbientSound", 4);
    ctx->m_logicRegistry->RegisterLogicType(CreateAmbientSound, "AmbientSound", 1);
    ctx->m_logicRegistry->RegisterLogicType(CreateAmbientPosSound, "AmbientPosSound", 0);
    ctx->m_logicRegistry->RegisterLogicType(CreateSpotAmbientSound, "SpotAmbientSound", 0);
    ctx->m_logicRegistry->RegisterLogicType(CreateActionArea, "ActionArea", 4);
    CProjActObj::RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(CreateStatusBarSprite, "StatusBarSprite", 2);
    CStatusBarSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateParticlez, "Particlez", 4);
    CParticlez::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateExplosion, "Explosion", 4);
    RegisterExplosionActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntSelectedSprite, "GruntSelectedSprite", 2);
    CGruntSelectedSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntHealthSprite, "GruntHealthSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntStaminaSprite, "GruntStaminaSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntToySprite, "GruntToySprite", 2);
    CGruntToySprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntToyTimeSprite, "GruntToyTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntWingzTimeSprite, "GruntWingzTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntPowerupSprite, "GruntPowerupSprite", 2);
    CGruntPowerupSprite::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateToyPeek, "ToyPeek", 4);
    RegisterIconState();
    ctx->m_logicRegistry->RegisterLogicType(CreateTileTriggerSwitch, "TileTriggerSwitch", 4);
    CTileTriggerSwitch::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateTileTrigger, "TileTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateTileSecretTrigger, "TileSecretTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateBrickz, "Brickz", 4);
    CBrickz::RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(CreateTileTriggerTransition, "TileTriggerTransition", 4);
    CTileTriggerTransition::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntStartingPoint, "GruntStartingPoint", 4);
    ActReg4RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntCreationPoint, "GruntCreationPoint", 4);
    CGruntCreationPoint::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateFortressFlag, "FortressFlag", 4);
    CFortressFlag::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateExitTrigger, "ExitTrigger", 4);
    CExitTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGiantRock, "GiantRock", 4);
    CTileTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateCoveredPowerup, "CoveredPowerup", 4);
    CTileTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateInGameIcon, "InGameIcon", 4);
    RegisterIconActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateInGameText, "InGameText", 4);
    RegisterTextLogic();
    ctx->m_logicRegistry->RegisterLogicType(CreateWormhole, "Wormhole", 4);
    RegisterWormholeLogic();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntPuddle, "GruntPuddle", 4);
    RegisterLogic();
    ctx->m_logicRegistry->RegisterLogicType(CreateRollingBall, "RollingBall", 4);
    CRollingBall::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateObjectDropper, "ObjectDropper", 4);
    CObjectDropper::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateDroppedObject, "DroppedObject", 4);
    CDroppedObject::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateDroppedObjectShadow, "DroppedObjectShadow", 4);
    CDroppedObjectShadow::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateCheckpointTrigger, "CheckpointTrigger", 4);
    CCheckpointTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateTeleporter, "Teleporter", 4);
    CTeleporter_RegisterActs();
    ctx->m_logicRegistry
        ->RegisterLogicType(CreateSecretTeleporterTrigger, "SecretTeleporterTrigger", 4);
    CSecretTeleporterTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateSecretLevelTrigger, "SecretLevelTrigger", 4);
    CSecretLevelTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateProjectile, "Projectile", 4);
    CProjectile::RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(CreateBoomerang, "Boomerang", 4);
    CProjectile::RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(CreateStaticHazard, "StaticHazard", 4);
    CStaticHazard::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateToobSpikez, "ToobSpikez", 4);
    CToobSpikez::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateTimeBomb, "TimeBomb", 4);
    CTimeBomb::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateSpotLight, "SpotLight", 4);
    RegisterSpotLightActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateKitchenSlime, "KitchenSlime", 4);
    CKitchenSlime::RegisterType();
    ctx->m_logicRegistry->RegisterLogicType(CreateSingleAnimation, "SingleAnimation", 4);
    CSingleAnimation::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateWayPoint, "WayPoint", 4);
    ctx->m_logicRegistry->RegisterLogicType(CreateWarlord, "Warlord", 4);
    RegisterWarlordActions();
    ctx->m_logicRegistry->RegisterLogicType(CreatePathHazard, "PathHazard", 4);
    RegisterPathHazardActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateRainCloud, "RainCloud", 4);
    RegisterPathHazardActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateUFO, "UFO", 4);
    RegisterPathHazardActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateGruntVoice, "GruntVoice", 4);
    RegisterGruntVoiceActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateWarpStonePad, "WarpStonePad", 4);
    CWarpStonePad::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateGuardPoint, "GuardPoint", 4);
    ctx->m_logicRegistry->RegisterLogicType(CreateVoiceTrigger, "VoiceTrigger", 4);
    CVoiceTrigger::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateLevelTime, "LevelTime", 4);
    ctx->m_logicRegistry->RegisterLogicType(CreateCursorSnapSprite, "CursorSnapSprite", 1);
    RegisterCursorSnapActions();
    ctx->m_logicRegistry->RegisterLogicType(CreateLightFx, "LightFx", 4);
    CLightFx::RegisterActs();
    ctx->m_logicRegistry->RegisterLogicType(CreateDemoMover, "DemoMover", 0);
    ctx->m_logicRegistry->RegisterLogicType(CreateDemoSign, "DemoSign", 0);
}
