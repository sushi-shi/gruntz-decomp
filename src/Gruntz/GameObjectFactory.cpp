#include <Gruntz/GameObjectFactory.h> // the shared RegisterGameObjectTypes decl
#include <rva.h>
#include <Gruntz/ObjTypeRegistrars.h>  // real per-type registrar entry points (reloc fidelity)
#include <DDrawMgr/DDrawSurfaceMgr.h>  // CDDrawSurfaceMgr - the ctx (m_workerCache @+0x14)
#include <DDrawMgr/DDrawWorkerCache.h> // CDDrawWorkerCache::CreateWorker (slot 9, 0x1652c0)

// Reloc-fidelity bindings for the factory-fn pointers the registrar pushes.
// Each `push offset _CreateXxx` (DIR32) relocates in retail to that create-fn's
// ILT jmp-thunk (a 5-byte `e9 <rel32>` in the <0x7c20 thunk band jumping to the
// real ctor body). The reference is reloc-masked, so the byte match already
// holds; these labels bind the undefined `_CreateXxx` external to the exact
// thunk RVA so reloc_fidelity scores them CORRECT (verified: every RVA is an E9
// thunk, and none collide with functions.csv / another symbol). The four thunks
// also used as placed-object type markers in Play.cpp (GiantRock/CoveredPowerup/
// InGameIcon/GruntStartingPoint) are bound here ONCE; Play.cpp references the
// same `_CreateXxx` symbol (its old `g_objIdThunk_<rva>` view was the same thunk).
DATA_SYMBOL(0x00002d2e, 0x0, _CreateGrunt)
DATA_SYMBOL(0x00002d15, 0x0, _CreateGlobalAmbientSound)
DATA_SYMBOL(0x00002158, 0x0, _CreateAmbientSound)
DATA_SYMBOL(0x00001217, 0x0, _CreateAmbientPosSound)
DATA_SYMBOL(0x00002851, 0x0, _CreateSpotAmbientSound)
DATA_SYMBOL(0x00002e1e, 0x0, _CreateParticlez)
DATA_SYMBOL(0x000019bf, 0x0, _CreateBrickz)
DATA_SYMBOL(0x00001d43, 0x0, _CreateTileTriggerTransition)
DATA_SYMBOL(0x000030f8, 0x0, _CreateProjectile)
DATA_SYMBOL(0x0000158c, 0x0, _CreateBoomerang)
DATA_SYMBOL(0x00003ca6, 0x0, _CreateStaticHazard)
DATA_SYMBOL(0x000016c7, 0x0, _CreateTimeBomb)
DATA_SYMBOL(0x00001b09, 0x0, _CreateLevelTime)
DATA_SYMBOL(0x00002bbc, 0x0, _CreateCursorSnapSprite)
DATA_SYMBOL(0x00002eb9, 0x0, _CreateDemoMover)
DATA_SYMBOL(0x0000448a, 0x0, _CreateDemoSign)
//
// The five former RegHelper CALL sites are now real static-member calls
// (CProjActObj::RegisterType / C{TileTriggerTransition,ToobSpikez,TimeBomb,
// VoiceTrigger}::RegisterActs). Each registrar's home TU declares it `static`
// (SAXXZ), so the retail this-less `call rel32` binds to its exact RVA with no
// `mov ecx` inserted (the bodies never touch `this`). Reloc-fidelity CORRECT.

RVA(0x0000a3b0, 0x6e2)
void RegisterGameObjectTypes(CDDrawSurfaceMgr* ctx) {
    ctx->m_workerCache->CreateWorker(CreateAniCycle, "AniCycle", 2);
    CAniCycle::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateDoNothingNormal, "DoNothingNormal", 0);
    ctx->m_workerCache->CreateWorker(CreateDoNothing, "DoNothing", 2);
    ctx->m_workerCache->CreateWorker(CreateSimpleAnimation, "SimpleAnimation", 2);
    RegisterSimpleAnimLogic();
    ctx->m_workerCache->CreateWorker(CreateMenuSparkle, "MenuSparkle", 2);
    RegisterXLogic_646010();
    ctx->m_workerCache->CreateWorker(CreateFrontCandy, "FrontCandy", 2);
    ctx->m_workerCache->CreateWorker(CreateBehindCandy, "BehindCandy", 2);
    ctx->m_workerCache->CreateWorker(CreateFrontCandyAni, "FrontCandyAni", 2);
    CFrontCandyAni::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateBehindCandyAni, "BehindCandyAni", 2);
    CBehindCandyAni::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateEyeCandy, "EyeCandy", 2);
    ctx->m_workerCache->CreateWorker(CreateEyeCandyAni, "EyeCandyAni", 2);
    CEyeCandyAni::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGrunt, "Grunt", 4);
    RegisterActs_644af0();
    ctx->m_workerCache->CreateWorker(CreateGlobalAmbientSound, "GlobalAmbientSound", 4);
    ctx->m_workerCache->CreateWorker(CreateAmbientSound, "AmbientSound", 1);
    ctx->m_workerCache->CreateWorker(CreateAmbientPosSound, "AmbientPosSound", 0);
    ctx->m_workerCache->CreateWorker(CreateSpotAmbientSound, "SpotAmbientSound", 0);
    ctx->m_workerCache->CreateWorker(CreateActionArea, "ActionArea", 4);
    CProjActObj::RegisterType();
    ctx->m_workerCache->CreateWorker(CreateStatusBarSprite, "StatusBarSprite", 2);
    CStatusBarSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateParticlez, "Particlez", 4);
    CParticlez::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateExplosion, "Explosion", 4);
    RegisterXLogic_6447f8();
    ctx->m_workerCache->CreateWorker(CreateGruntSelectedSprite, "GruntSelectedSprite", 2);
    CGruntSelectedSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGruntHealthSprite, "GruntHealthSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGruntStaminaSprite, "GruntStaminaSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGruntToySprite, "GruntToySprite", 2);
    CGruntToySprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGruntToyTimeSprite, "GruntToyTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGruntWingzTimeSprite, "GruntWingzTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGruntPowerupSprite, "GruntPowerupSprite", 2);
    CGruntPowerupSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateToyPeek, "ToyPeek", 4);
    RegisterIconState();
    ctx->m_workerCache->CreateWorker(CreateTileTriggerSwitch, "TileTriggerSwitch", 4);
    CTileTriggerSwitch::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateTileTrigger, "TileTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateTileSecretTrigger, "TileSecretTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateBrickz, "Brickz", 4);
    CBrickz::RegisterActs(); // 0x10ebe0 (ex 'CCheckpointTrigger::' - the shift-by-one)
    ctx->m_workerCache->CreateWorker(CreateTileTriggerTransition, "TileTriggerTransition", 4);
    CTileTriggerTransition::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGruntStartingPoint, "GruntStartingPoint", 4);
    ActReg4RegisterType();
    ctx->m_workerCache->CreateWorker(CreateGruntCreationPoint, "GruntCreationPoint", 4);
    CGruntCreationPoint::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateFortressFlag, "FortressFlag", 4);
    CFortressFlag::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateExitTrigger, "ExitTrigger", 4);
    CExitTrigger::RegisterActs(); // 0x3f3f0 (ex 'CWormhole::' - CExitTrigger's act cluster)
    ctx->m_workerCache->CreateWorker(CreateGiantRock, "GiantRock", 4);
    CTileTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateCoveredPowerup, "CoveredPowerup", 4);
    CTileTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateInGameIcon, "InGameIcon", 4);
    RegisterIconActions();
    ctx->m_workerCache->CreateWorker(CreateInGameText, "InGameText", 4);
    RegisterTextLogic();
    ctx->m_workerCache->CreateWorker(CreateWormhole, "Wormhole", 4);
    RegisterWormholeLogic();
    ctx->m_workerCache->CreateWorker(CreateGruntPuddle, "GruntPuddle", 4);
    RegisterLogic();
    ctx->m_workerCache->CreateWorker(CreateRollingBall, "RollingBall", 4);
    CRollingBall::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateObjectDropper, "ObjectDropper", 4);
    CObjectDropper::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateDroppedObject, "DroppedObject", 4);
    CDroppedObject::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateDroppedObjectShadow, "DroppedObjectShadow", 4);
    CDroppedObjectShadow::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateCheckpointTrigger, "CheckpointTrigger", 4);
    CCheckpointTrigger::RegisterActs(); // 0x10f340 (ex 'CTileSecretTrigger::')
    ctx->m_workerCache->CreateWorker(CreateTeleporter, "Teleporter", 4);
    CTeleporter_RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateSecretTeleporterTrigger, "SecretTeleporterTrigger", 4);
    CSecretTeleporterTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateSecretLevelTrigger, "SecretLevelTrigger", 4);
    CSecretLevelTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateProjectile, "Projectile", 4);
    CProjectile::RegisterType();
    ctx->m_workerCache->CreateWorker(CreateBoomerang, "Boomerang", 4);
    CProjectile::RegisterType();
    ctx->m_workerCache->CreateWorker(CreateStaticHazard, "StaticHazard", 4);
    CStaticHazard::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateToobSpikez, "ToobSpikez", 4);
    CToobSpikez::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateTimeBomb, "TimeBomb", 4);
    CTimeBomb::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateSpotLight, "SpotLight", 4);
    RegisterActs_646188();
    ctx->m_workerCache->CreateWorker(CreateKitchenSlime, "KitchenSlime", 4);
    CKitchenSlime::RegisterType();
    ctx->m_workerCache->CreateWorker(CreateSingleAnimation, "SingleAnimation", 4);
    CSingleAnimation::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateWayPoint, "WayPoint", 4);
    ctx->m_workerCache->CreateWorker(CreateWarlord, "Warlord", 4);
    RegisterWarlordActions();
    ctx->m_workerCache->CreateWorker(CreatePathHazard, "PathHazard", 4);
    RegisterActs_646250();
    ctx->m_workerCache->CreateWorker(CreateRainCloud, "RainCloud", 4);
    RegisterActs_646250();
    ctx->m_workerCache->CreateWorker(CreateUFO, "UFO", 4);
    RegisterActs_646250();
    ctx->m_workerCache->CreateWorker(CreateGruntVoice, "GruntVoice", 4);
    RegisterActs_6514d8();
    ctx->m_workerCache->CreateWorker(CreateWarpStonePad, "WarpStonePad", 4);
    CWarpStonePad::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateGuardPoint, "GuardPoint", 4);
    ctx->m_workerCache->CreateWorker(CreateVoiceTrigger, "VoiceTrigger", 4);
    CVoiceTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateLevelTime, "LevelTime", 4);
    ctx->m_workerCache->CreateWorker(CreateCursorSnapSprite, "CursorSnapSprite", 1);
    RegisterXLogic_62bfa0();
    ctx->m_workerCache->CreateWorker(CreateLightFx, "LightFx", 4);
    CLightFx::RegisterActs();
    ctx->m_workerCache->CreateWorker(CreateDemoMover, "DemoMover", 0);
    ctx->m_workerCache->CreateWorker(CreateDemoSign, "DemoSign", 0);
}
