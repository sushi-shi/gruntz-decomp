#include <Gruntz/GameObjectFactory.h> // the shared RegisterGameObjectTypes decl
#include <rva.h>
#include <Gruntz/ObjTypeRegistrars.h>  // real per-type registrar entry points (reloc fidelity)
#include <DDrawMgr/DDrawSurfaceMgr.h>  // CDDrawSurfaceMgr - the ctx (m_workerCache @+0x14)
#include <DDrawMgr/DDrawWorkerCache.h> // CDDrawWorkerCache::CreateWorker (slot 9, 0x1652c0)

extern "C" {
    i32 CreateAniCycle(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateDoNothingNormal(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateDoNothing(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateSimpleAnimation(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateMenuSparkle(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateFrontCandy(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateBehindCandy(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateFrontCandyAni(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateBehindCandyAni(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateEyeCandy(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateEyeCandyAni(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGrunt(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGlobalAmbientSound(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateAmbientSound(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateAmbientPosSound(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateSpotAmbientSound(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateActionArea(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateStatusBarSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateParticlez(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateExplosion(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntSelectedSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntHealthSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntStaminaSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntToySprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntToyTimeSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntWingzTimeSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntPowerupSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateToyPeek(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateTileTriggerSwitch(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateTileTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateTileSecretTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateBrickz(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateTileTriggerTransition(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntStartingPoint(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntCreationPoint(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateFortressFlag(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateExitTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGiantRock(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateCoveredPowerup(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateInGameIcon(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateInGameText(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateWormhole(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntPuddle(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateRollingBall(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateObjectDropper(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateDroppedObject(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateDroppedObjectShadow(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateCheckpointTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateTeleporter(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateSecretTeleporterTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateSecretLevelTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateProjectile(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateBoomerang(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateStaticHazard(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateToobSpikez(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateTimeBomb(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateSpotLight(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateKitchenSlime(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateSingleAnimation(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateWayPoint(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateWarlord(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreatePathHazard(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateRainCloud(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateUFO(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGruntVoice(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateWarpStonePad(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateGuardPoint(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateVoiceTrigger(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateLevelTime(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateCursorSnapSprite(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateLightFx(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateDemoMover(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    i32 CreateDemoSign(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)
    // (The five remaining follow-up registrars are now bound as real static-member
    // calls via <Gruntz/ObjTypeRegistrars.h>: their home TUs declare them `static`
    // (SAXXZ), so the retail this-less `call rel32` binds to the exact RVA without a
    // `mov ecx`. See the call sites below - CProjActObj::RegisterType (0x8240),
    // CTileTriggerTransition::RegisterActs (0x10fe70), CToobSpikez::RegisterActs
    // (0x1149c0), CTimeBomb::RegisterActs (0xe1990), CVoiceTrigger::RegisterActs
    // (0x11a500).)
}

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
// @data-symbol: _CreateAniCycle 0x00001f19
// @data-symbol: _CreateDoNothingNormal 0x00003dc8
// @data-symbol: _CreateDoNothing 0x000019d3
// @data-symbol: _CreateSimpleAnimation 0x00001a37
// @data-symbol: _CreateMenuSparkle 0x00003f3a
// @data-symbol: _CreateFrontCandy 0x000013c0
// @data-symbol: _CreateBehindCandy 0x000018bb
// @data-symbol: _CreateFrontCandyAni 0x00001f91
// @data-symbol: _CreateBehindCandyAni 0x00003c65
// @data-symbol: _CreateEyeCandy 0x00001aaa
// @data-symbol: _CreateEyeCandyAni 0x00002f27
// @data-symbol: _CreateGrunt 0x00002d2e
// @data-symbol: _CreateGlobalAmbientSound 0x00002d15
// @data-symbol: _CreateAmbientSound 0x00002158
// @data-symbol: _CreateAmbientPosSound 0x00001217
// @data-symbol: _CreateSpotAmbientSound 0x00002851
// @data-symbol: _CreateActionArea 0x0000349a
// @data-symbol: _CreateStatusBarSprite 0x000027c0
// @data-symbol: _CreateParticlez 0x00002e1e
// @data-symbol: _CreateExplosion 0x00003468
// @data-symbol: _CreateGruntSelectedSprite 0x00003da0
// @data-symbol: _CreateGruntHealthSprite 0x000037b5
// @data-symbol: _CreateGruntStaminaSprite 0x000025d1
// @data-symbol: _CreateGruntToySprite 0x00001910
// @data-symbol: _CreateGruntToyTimeSprite 0x00001bae
// @data-symbol: _CreateGruntWingzTimeSprite 0x00002d24
// @data-symbol: _CreateGruntPowerupSprite 0x000027cf
// @data-symbol: _CreateToyPeek 0x00001e1f
// @data-symbol: _CreateTileTriggerSwitch 0x00001799
// @data-symbol: _CreateTileTrigger 0x00003bfc
// @data-symbol: _CreateTileSecretTrigger 0x000037b0
// @data-symbol: _CreateBrickz 0x000019bf
// @data-symbol: _CreateTileTriggerTransition 0x00001d43
// @data-symbol: _CreateGruntStartingPoint 0x000024a5
// @data-symbol: _CreateGruntCreationPoint 0x000017e4
// @data-symbol: _CreateFortressFlag 0x00003148
// @data-symbol: _CreateExitTrigger 0x0000192e
// @data-symbol: _CreateGiantRock 0x0000137a
// @data-symbol: _CreateCoveredPowerup 0x00003d0f
// @data-symbol: _CreateInGameIcon 0x0000288d
// @data-symbol: _CreateInGameText 0x00002bad
// @data-symbol: _CreateWormhole 0x0000191a
// @data-symbol: _CreateGruntPuddle 0x00002a68
// @data-symbol: _CreateRollingBall 0x0000191f
// @data-symbol: _CreateObjectDropper 0x00002d79
// @data-symbol: _CreateDroppedObject 0x00001e24
// @data-symbol: _CreateDroppedObjectShadow 0x000011b8
// @data-symbol: _CreateCheckpointTrigger 0x00001794
// @data-symbol: _CreateTeleporter 0x000039b3
// @data-symbol: _CreateSecretTeleporterTrigger 0x000015af
// @data-symbol: _CreateSecretLevelTrigger 0x00001bf4
// @data-symbol: _CreateProjectile 0x000030f8
// @data-symbol: _CreateBoomerang 0x0000158c
// @data-symbol: _CreateStaticHazard 0x00003ca6
// @data-symbol: _CreateToobSpikez 0x0000182a
// @data-symbol: _CreateTimeBomb 0x000016c7
// @data-symbol: _CreateSpotLight 0x00001a8c
// @data-symbol: _CreateKitchenSlime 0x000015f0
// @data-symbol: _CreateSingleAnimation 0x00002702
// @data-symbol: _CreateWayPoint 0x00001087
// @data-symbol: _CreateWarlord 0x00004354
// @data-symbol: _CreatePathHazard 0x00003814
// @data-symbol: _CreateRainCloud 0x00001e5b
// @data-symbol: _CreateUFO 0x00002d7e
// @data-symbol: _CreateGruntVoice 0x00001f32
// @data-symbol: _CreateWarpStonePad 0x00001f0a
// @data-symbol: _CreateGuardPoint 0x0000164f
// @data-symbol: _CreateVoiceTrigger 0x00002cf2
// @data-symbol: _CreateLevelTime 0x00001b09
// @data-symbol: _CreateCursorSnapSprite 0x00002bbc
// @data-symbol: _CreateLightFx 0x00002bdf
// @data-symbol: _CreateDemoMover 0x00002eb9
// @data-symbol: _CreateDemoSign 0x0000448a
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
    RegisterLogic_6445e8();
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
