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
DATA_SYMBOL(0x00001f19, 0x0, _CreateAniCycle)
DATA_SYMBOL(0x00003dc8, 0x0, _CreateDoNothingNormal)
DATA_SYMBOL(0x000019d3, 0x0, _CreateDoNothing)
DATA_SYMBOL(0x00001a37, 0x0, _CreateSimpleAnimation)
DATA_SYMBOL(0x00003f3a, 0x0, _CreateMenuSparkle)
DATA_SYMBOL(0x000013c0, 0x0, _CreateFrontCandy)
DATA_SYMBOL(0x000018bb, 0x0, _CreateBehindCandy)
DATA_SYMBOL(0x00001f91, 0x0, _CreateFrontCandyAni)
DATA_SYMBOL(0x00003c65, 0x0, _CreateBehindCandyAni)
DATA_SYMBOL(0x00001aaa, 0x0, _CreateEyeCandy)
DATA_SYMBOL(0x00002f27, 0x0, _CreateEyeCandyAni)
DATA_SYMBOL(0x00002d2e, 0x0, _CreateGrunt)
DATA_SYMBOL(0x00002d15, 0x0, _CreateGlobalAmbientSound)
DATA_SYMBOL(0x00002158, 0x0, _CreateAmbientSound)
DATA_SYMBOL(0x00001217, 0x0, _CreateAmbientPosSound)
DATA_SYMBOL(0x00002851, 0x0, _CreateSpotAmbientSound)
DATA_SYMBOL(0x0000349a, 0x0, _CreateActionArea)
DATA_SYMBOL(0x000027c0, 0x0, _CreateStatusBarSprite)
DATA_SYMBOL(0x00002e1e, 0x0, _CreateParticlez)
DATA_SYMBOL(0x00003468, 0x0, _CreateExplosion)
DATA_SYMBOL(0x00003da0, 0x0, _CreateGruntSelectedSprite)
DATA_SYMBOL(0x000037b5, 0x0, _CreateGruntHealthSprite)
DATA_SYMBOL(0x000025d1, 0x0, _CreateGruntStaminaSprite)
DATA_SYMBOL(0x00001910, 0x0, _CreateGruntToySprite)
DATA_SYMBOL(0x00001bae, 0x0, _CreateGruntToyTimeSprite)
DATA_SYMBOL(0x00002d24, 0x0, _CreateGruntWingzTimeSprite)
DATA_SYMBOL(0x000027cf, 0x0, _CreateGruntPowerupSprite)
DATA_SYMBOL(0x00001e1f, 0x0, _CreateToyPeek)
DATA_SYMBOL(0x00001799, 0x0, _CreateTileTriggerSwitch)
DATA_SYMBOL(0x00003bfc, 0x0, _CreateTileTrigger)
DATA_SYMBOL(0x000037b0, 0x0, _CreateTileSecretTrigger)
DATA_SYMBOL(0x000019bf, 0x0, _CreateBrickz)
DATA_SYMBOL(0x00001d43, 0x0, _CreateTileTriggerTransition)
DATA_SYMBOL(0x000024a5, 0x0, _CreateGruntStartingPoint)
DATA_SYMBOL(0x000017e4, 0x0, _CreateGruntCreationPoint)
DATA_SYMBOL(0x00003148, 0x0, _CreateFortressFlag)
DATA_SYMBOL(0x0000192e, 0x0, _CreateExitTrigger)
DATA_SYMBOL(0x0000137a, 0x0, _CreateGiantRock)
DATA_SYMBOL(0x00003d0f, 0x0, _CreateCoveredPowerup)
DATA_SYMBOL(0x0000288d, 0x0, _CreateInGameIcon)
DATA_SYMBOL(0x00002bad, 0x0, _CreateInGameText)
DATA_SYMBOL(0x0000191a, 0x0, _CreateWormhole)
DATA_SYMBOL(0x00002a68, 0x0, _CreateGruntPuddle)
DATA_SYMBOL(0x0000191f, 0x0, _CreateRollingBall)
DATA_SYMBOL(0x00002d79, 0x0, _CreateObjectDropper)
DATA_SYMBOL(0x00001e24, 0x0, _CreateDroppedObject)
DATA_SYMBOL(0x000011b8, 0x0, _CreateDroppedObjectShadow)
DATA_SYMBOL(0x00001794, 0x0, _CreateCheckpointTrigger)
DATA_SYMBOL(0x000039b3, 0x0, _CreateTeleporter)
DATA_SYMBOL(0x000015af, 0x0, _CreateSecretTeleporterTrigger)
DATA_SYMBOL(0x00001bf4, 0x0, _CreateSecretLevelTrigger)
DATA_SYMBOL(0x000030f8, 0x0, _CreateProjectile)
DATA_SYMBOL(0x0000158c, 0x0, _CreateBoomerang)
DATA_SYMBOL(0x00003ca6, 0x0, _CreateStaticHazard)
DATA_SYMBOL(0x0000182a, 0x0, _CreateToobSpikez)
DATA_SYMBOL(0x000016c7, 0x0, _CreateTimeBomb)
DATA_SYMBOL(0x00001a8c, 0x0, _CreateSpotLight)
DATA_SYMBOL(0x000015f0, 0x0, _CreateKitchenSlime)
DATA_SYMBOL(0x00002702, 0x0, _CreateSingleAnimation)
DATA_SYMBOL(0x00001087, 0x0, _CreateWayPoint)
DATA_SYMBOL(0x00004354, 0x0, _CreateWarlord)
DATA_SYMBOL(0x00003814, 0x0, _CreatePathHazard)
DATA_SYMBOL(0x00001e5b, 0x0, _CreateRainCloud)
DATA_SYMBOL(0x00002d7e, 0x0, _CreateUFO)
DATA_SYMBOL(0x00001f32, 0x0, _CreateGruntVoice)
DATA_SYMBOL(0x00001f0a, 0x0, _CreateWarpStonePad)
DATA_SYMBOL(0x0000164f, 0x0, _CreateGuardPoint)
DATA_SYMBOL(0x00002cf2, 0x0, _CreateVoiceTrigger)
DATA_SYMBOL(0x00001b09, 0x0, _CreateLevelTime)
DATA_SYMBOL(0x00002bbc, 0x0, _CreateCursorSnapSprite)
DATA_SYMBOL(0x00002bdf, 0x0, _CreateLightFx)
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
    ctx->m_workerCache
        ->CreateWorker(CreateSecretTeleporterTrigger, "SecretTeleporterTrigger", 4);
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
