// GameObjectFactory.cpp - RegisterGameObjectTypes (RVA 0x000a3b0).
//
// The level/object-type factory registrar: walks one uniform list registering
// each named game-object type with the type registry (the world's +0x14
// m_workerCache, virtual slot 9 / +0x24) under a class-id flag, and (for most
// types) calls a per-type follow-up registration helper. Frameless __cdecl. Only
// offsets + code bytes are load-bearing; every create-fn / follow-up helper is a
// reloc-masked external (an unnamed ILT jmp-thunk to the real ctor/helper).
//
// IDENTITY (the ex "GameObjFactoryCtx"/"GameObjTypeRegistry" views, dissolved
// 2026-07-16): the ctx is the world CDDrawSurfaceMgr itself - the one caller
// (CGruntzMgr::Run, RezSync.cpp) passes the freshly-new'd world manager - and the
// "+0x14 registry" is its m_workerCache, the canonical CDDrawWorkerCache
// (<DDrawMgr/DDrawWorkerCache.h>). The "RegisterType" slot is CreateWorker: SAME
// vtable slot (9, +0x24), SAME 3-arg shape, and the body (0x1652c0,
// DDrawSurfacePair.cpp) consumes a1 as the factory fn-ptr (w->Init((GameObjNotifyFn)
// a1, a3)) keyed by the type name - exactly what the tile-logic registrars
// (LogicTypeTable.cpp / UserLogicCtorEmit.cpp) already dispatch on the same member.
#include <Gruntz/GameObjectFactory.h> // the shared RegisterGameObjectTypes decl
#include <rva.h>
#include <Gruntz/ObjTypeRegistrars.h>  // real per-type registrar entry points (reloc fidelity)
#include <DDrawMgr/DDrawSurfaceMgr.h>  // CDDrawSurfaceMgr - the ctx (m_workerCache @+0x14)
#include <DDrawMgr/DDrawWorkerCache.h> // CDDrawWorkerCache::CreateWorker (slot 9, 0x1652c0)

// Reloc-masked externals: per-type object create-fns (ILT thunks to the real
// ctors) and the per-type follow-up registration helpers (by ILT-thunk RVA).
extern "C" {
    void* CreateAniCycle();
    void* CreateDoNothingNormal();
    void* CreateDoNothing();
    void* CreateSimpleAnimation();
    void* CreateMenuSparkle();
    void* CreateFrontCandy();
    void* CreateBehindCandy();
    void* CreateFrontCandyAni();
    void* CreateBehindCandyAni();
    void* CreateEyeCandy();
    void* CreateEyeCandyAni();
    void* CreateGrunt();
    void* CreateGlobalAmbientSound();
    void* CreateAmbientSound();
    void* CreateAmbientPosSound();
    void* CreateSpotAmbientSound();
    void* CreateActionArea();
    void* CreateStatusBarSprite();
    void* CreateParticlez();
    void* CreateExplosion();
    void* CreateGruntSelectedSprite();
    void* CreateGruntHealthSprite();
    void* CreateGruntStaminaSprite();
    void* CreateGruntToySprite();
    void* CreateGruntToyTimeSprite();
    void* CreateGruntWingzTimeSprite();
    void* CreateGruntPowerupSprite();
    void* CreateToyPeek();
    void* CreateTileTriggerSwitch();
    void* CreateTileTrigger();
    void* CreateTileSecretTrigger();
    void* CreateBrickz();
    void* CreateTileTriggerTransition();
    void* CreateGruntStartingPoint();
    void* CreateGruntCreationPoint();
    void* CreateFortressFlag();
    void* CreateExitTrigger();
    void* CreateGiantRock();
    void* CreateCoveredPowerup();
    void* CreateInGameIcon();
    void* CreateInGameText();
    void* CreateWormhole();
    void* CreateGruntPuddle();
    void* CreateRollingBall();
    void* CreateObjectDropper();
    void* CreateDroppedObject();
    void* CreateDroppedObjectShadow();
    void* CreateCheckpointTrigger();
    void* CreateTeleporter();
    void* CreateSecretTeleporterTrigger();
    void* CreateSecretLevelTrigger();
    void* CreateProjectile();
    void* CreateBoomerang();
    void* CreateStaticHazard();
    void* CreateToobSpikez();
    void* CreateTimeBomb();
    void* CreateSpotLight();
    void* CreateKitchenSlime();
    void* CreateSingleAnimation();
    void* CreateWayPoint();
    void* CreateWarlord();
    void* CreatePathHazard();
    void* CreateRainCloud();
    void* CreateUFO();
    void* CreateGruntVoice();
    void* CreateWarpStonePad();
    void* CreateGuardPoint();
    void* CreateVoiceTrigger();
    void* CreateLevelTime();
    void* CreateCursorSnapSprite();
    void* CreateLightFx();
    void* CreateDemoMover();
    void* CreateDemoSign();
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

// @source: string-xref
RVA(0x0000a3b0, 0x6e2)
void RegisterGameObjectTypes(CDDrawSurfaceMgr* ctx) {
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateAniCycle), "AniCycle", 2);
    CAniCycle::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateDoNothingNormal), "DoNothingNormal", 0);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateDoNothing), "DoNothing", 2);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateSimpleAnimation), "SimpleAnimation", 2);
    RegisterSimpleAnimLogic();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateMenuSparkle), "MenuSparkle", 2);
    RegisterXLogic_646010();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateFrontCandy), "FrontCandy", 2);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateBehindCandy), "BehindCandy", 2);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateFrontCandyAni), "FrontCandyAni", 2);
    CFrontCandyAni::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateBehindCandyAni), "BehindCandyAni", 2);
    CBehindCandyAni::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateEyeCandy), "EyeCandy", 2);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateEyeCandyAni), "EyeCandyAni", 2);
    CEyeCandyAni::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGrunt), "Grunt", 4);
    RegisterActs_644af0();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGlobalAmbientSound), "GlobalAmbientSound", 4);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateAmbientSound), "AmbientSound", 1);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateAmbientPosSound), "AmbientPosSound", 0);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateSpotAmbientSound), "SpotAmbientSound", 0);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateActionArea), "ActionArea", 4);
    CProjActObj::RegisterType();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateStatusBarSprite), "StatusBarSprite", 2);
    CStatusBarSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateParticlez), "Particlez", 4);
    CParticlez::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateExplosion), "Explosion", 4);
    RegisterXLogic_6447f8();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntSelectedSprite), "GruntSelectedSprite", 2);
    CGruntSelectedSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntHealthSprite), "GruntHealthSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntStaminaSprite), "GruntStaminaSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntToySprite), "GruntToySprite", 2);
    CGruntToySprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntToyTimeSprite), "GruntToyTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntWingzTimeSprite), "GruntWingzTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntPowerupSprite), "GruntPowerupSprite", 2);
    CGruntPowerupSprite::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateToyPeek), "ToyPeek", 4);
    RegisterIconState();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateTileTriggerSwitch), "TileTriggerSwitch", 4);
    CTileTriggerSwitch::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateTileTrigger), "TileTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateTileSecretTrigger), "TileSecretTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateBrickz), "Brickz", 4);
    CBrickz::RegisterActs(); // 0x10ebe0 (ex 'CCheckpointTrigger::' - the shift-by-one)
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateTileTriggerTransition), "TileTriggerTransition", 4);
    CTileTriggerTransition::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntStartingPoint), "GruntStartingPoint", 4);
    ActReg4RegisterType();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntCreationPoint), "GruntCreationPoint", 4);
    CGruntCreationPoint::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateFortressFlag), "FortressFlag", 4);
    CFortressFlag::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateExitTrigger), "ExitTrigger", 4);
    CExitTrigger::RegisterActs(); // 0x3f3f0 (ex 'CWormhole::' - CExitTrigger's act cluster)
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGiantRock), "GiantRock", 4);
    CTileTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateCoveredPowerup), "CoveredPowerup", 4);
    CTileTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateInGameIcon), "InGameIcon", 4);
    RegisterIconActions();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateInGameText), "InGameText", 4);
    RegisterTextLogic();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateWormhole), "Wormhole", 4);
    RegisterWormholeLogic();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntPuddle), "GruntPuddle", 4);
    RegisterLogic_6445e8();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateRollingBall), "RollingBall", 4);
    CRollingBall::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateObjectDropper), "ObjectDropper", 4);
    CObjectDropper::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateDroppedObject), "DroppedObject", 4);
    CDroppedObject::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateDroppedObjectShadow), "DroppedObjectShadow", 4);
    CDroppedObjectShadow::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateCheckpointTrigger), "CheckpointTrigger", 4);
    CCheckpointTrigger::RegisterActs(); // 0x10f340 (ex 'CTileSecretTrigger::')
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateTeleporter), "Teleporter", 4);
    CTeleporter_RegisterActs();
    ctx->m_workerCache
        ->CreateWorker(reinterpret_cast<i32>(CreateSecretTeleporterTrigger), "SecretTeleporterTrigger", 4);
    CSecretTeleporterTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateSecretLevelTrigger), "SecretLevelTrigger", 4);
    CSecretLevelTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateProjectile), "Projectile", 4);
    CProjectile::RegisterType();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateBoomerang), "Boomerang", 4);
    CProjectile::RegisterType();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateStaticHazard), "StaticHazard", 4);
    CStaticHazard::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateToobSpikez), "ToobSpikez", 4);
    CToobSpikez::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateTimeBomb), "TimeBomb", 4);
    CTimeBomb::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateSpotLight), "SpotLight", 4);
    RegisterActs_646188();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateKitchenSlime), "KitchenSlime", 4);
    CKitchenSlime::RegisterType();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateSingleAnimation), "SingleAnimation", 4);
    CSingleAnimation::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateWayPoint), "WayPoint", 4);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateWarlord), "Warlord", 4);
    RegisterWarlordActions();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreatePathHazard), "PathHazard", 4);
    RegisterActs_646250();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateRainCloud), "RainCloud", 4);
    RegisterActs_646250();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateUFO), "UFO", 4);
    RegisterActs_646250();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGruntVoice), "GruntVoice", 4);
    RegisterActs_6514d8();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateWarpStonePad), "WarpStonePad", 4);
    CWarpStonePad::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateGuardPoint), "GuardPoint", 4);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateVoiceTrigger), "VoiceTrigger", 4);
    CVoiceTrigger::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateLevelTime), "LevelTime", 4);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateCursorSnapSprite), "CursorSnapSprite", 1);
    RegisterXLogic_62bfa0();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateLightFx), "LightFx", 4);
    CLightFx::RegisterActs();
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateDemoMover), "DemoMover", 0);
    ctx->m_workerCache->CreateWorker(reinterpret_cast<i32>(CreateDemoSign), "DemoSign", 0);
}
