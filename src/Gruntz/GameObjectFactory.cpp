// GameObjectFactory.cpp - RegisterGameObjectTypes (RVA 0x000a3b0).
//
// The level/object-type factory registrar: walks one uniform list registering
// each named game-object type with the type registry (ctx->m_14, virtual slot
// +0x24) under a class-id flag, and (for most types) calls a per-type follow-up
// registration helper. Frameless __cdecl. Only offsets + code bytes are
// load-bearing; every create-fn / follow-up helper is a reloc-masked external
// (an unnamed ILT jmp-thunk to the real ctor/helper).
#include <rva.h>
#include <Globals.h>                  // g_dat6295d8 (the 0xaf50 reset thunk's target global)
#include <Gruntz/ObjTypeRegistrars.h> // real per-type registrar entry points (reloc fidelity)

typedef void* (*ObjCreateFn)();

// The object-type registry reached through ctx->m_14. RegisterType is virtual
// slot 9 (+0x24): record one named type + its create-fn under a class-id flag.
struct GameObjTypeRegistry {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void RegisterType(ObjCreateFn create, const char* name, i32 flags); // +0x24
};
struct GameObjFactoryCtx {
    char m_pad00[0x14];
    GameObjTypeRegistry* m_14; // +0x14
};

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
    // The remaining follow-up registrars are __thiscall members (QAE) in their home
    // TUs, but the retail factory calls them as plain `call rel32` with no `this`
    // set up (they ignore ecx). Binding them to the real ??@..@@QAEXXZ symbols would
    // force a `mov ecx,..` here and change the code bytes, so they stay reloc-masked
    // externs. Real identities (retail RVA -> symbol):
    //   RegHelper_1e65 0x8240   ?RegisterType@CProjActObj@@QAEXXZ
    //   RegHelper_272a 0x10fe70 ?RegisterActs@CTileTriggerTransition@@QAEXXZ
    //   RegHelper_10a5 0x1149c0 ?RegisterActs@CToobSpikez@@QAEXXZ
    //   RegHelper_146f 0xe1990  ?RegisterActs@CTimeBomb@@QAEXXZ
    //   RegHelper_2400 0x11a500 ?RegisterActs@CVoiceTrigger@@QAEXXZ
    void RegHelper_10a5();
    void RegHelper_146f();
    void RegHelper_1e65();
    void RegHelper_2400();
    void RegHelper_272a();
}

// @source: string-xref
RVA(0x0000a3b0, 0x6e2)
void RegisterGameObjectTypes(GameObjFactoryCtx* ctx) {
    ctx->m_14->RegisterType(CreateAniCycle, "AniCycle", 2);
    CAniCycle::RegisterActs();
    ctx->m_14->RegisterType(CreateDoNothingNormal, "DoNothingNormal", 0);
    ctx->m_14->RegisterType(CreateDoNothing, "DoNothing", 2);
    ctx->m_14->RegisterType(CreateSimpleAnimation, "SimpleAnimation", 2);
    RegisterSimpleAnimLogic();
    ctx->m_14->RegisterType(CreateMenuSparkle, "MenuSparkle", 2);
    RegisterXLogic_646010();
    ctx->m_14->RegisterType(CreateFrontCandy, "FrontCandy", 2);
    ctx->m_14->RegisterType(CreateBehindCandy, "BehindCandy", 2);
    ctx->m_14->RegisterType(CreateFrontCandyAni, "FrontCandyAni", 2);
    CFrontCandyAni::RegisterActs();
    ctx->m_14->RegisterType(CreateBehindCandyAni, "BehindCandyAni", 2);
    CBehindCandyAni::RegisterActs();
    ctx->m_14->RegisterType(CreateEyeCandy, "EyeCandy", 2);
    ctx->m_14->RegisterType(CreateEyeCandyAni, "EyeCandyAni", 2);
    CEyeCandyAni::RegisterActs();
    ctx->m_14->RegisterType(CreateGrunt, "Grunt", 4);
    RegisterActs_644af0();
    ctx->m_14->RegisterType(CreateGlobalAmbientSound, "GlobalAmbientSound", 4);
    ctx->m_14->RegisterType(CreateAmbientSound, "AmbientSound", 1);
    ctx->m_14->RegisterType(CreateAmbientPosSound, "AmbientPosSound", 0);
    ctx->m_14->RegisterType(CreateSpotAmbientSound, "SpotAmbientSound", 0);
    ctx->m_14->RegisterType(CreateActionArea, "ActionArea", 4);
    RegHelper_1e65();
    ctx->m_14->RegisterType(CreateStatusBarSprite, "StatusBarSprite", 2);
    CStatusBarSprite::RegisterActs();
    ctx->m_14->RegisterType(CreateParticlez, "Particlez", 4);
    CParticlez::RegisterActs();
    ctx->m_14->RegisterType(CreateExplosion, "Explosion", 4);
    RegisterXLogic_6447f8();
    ctx->m_14->RegisterType(CreateGruntSelectedSprite, "GruntSelectedSprite", 2);
    CGruntSelectedSprite::RegisterActs();
    ctx->m_14->RegisterType(CreateGruntHealthSprite, "GruntHealthSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_14->RegisterType(CreateGruntStaminaSprite, "GruntStaminaSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_14->RegisterType(CreateGruntToySprite, "GruntToySprite", 2);
    CGruntToySprite::RegisterActs();
    ctx->m_14->RegisterType(CreateGruntToyTimeSprite, "GruntToyTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_14->RegisterType(CreateGruntWingzTimeSprite, "GruntWingzTimeSprite", 2);
    CGruntHealthSprite::RegisterActs();
    ctx->m_14->RegisterType(CreateGruntPowerupSprite, "GruntPowerupSprite", 2);
    CGruntPowerupSprite::RegisterActs();
    ctx->m_14->RegisterType(CreateToyPeek, "ToyPeek", 4);
    RegisterIconState();
    ctx->m_14->RegisterType(CreateTileTriggerSwitch, "TileTriggerSwitch", 4);
    CTileTriggerSwitch::RegisterActs();
    ctx->m_14->RegisterType(CreateTileTrigger, "TileTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_14->RegisterType(CreateTileSecretTrigger, "TileSecretTrigger", 4);
    CTileTrigger::RegisterActs();
    ctx->m_14->RegisterType(CreateBrickz, "Brickz", 4);
    CCheckpointTrigger::RegisterActs();
    ctx->m_14->RegisterType(CreateTileTriggerTransition, "TileTriggerTransition", 4);
    RegHelper_272a();
    ctx->m_14->RegisterType(CreateGruntStartingPoint, "GruntStartingPoint", 4);
    ActReg4RegisterType();
    ctx->m_14->RegisterType(CreateGruntCreationPoint, "GruntCreationPoint", 4);
    CGruntCreationPoint::RegisterActs();
    ctx->m_14->RegisterType(CreateFortressFlag, "FortressFlag", 4);
    CFortressFlag::RegisterActs();
    ctx->m_14->RegisterType(CreateExitTrigger, "ExitTrigger", 4);
    CWormhole::RegisterActs();
    ctx->m_14->RegisterType(CreateGiantRock, "GiantRock", 4);
    CTileTrigger::RegisterActs();
    ctx->m_14->RegisterType(CreateCoveredPowerup, "CoveredPowerup", 4);
    CTileTrigger::RegisterActs();
    ctx->m_14->RegisterType(CreateInGameIcon, "InGameIcon", 4);
    RegisterIconActions();
    ctx->m_14->RegisterType(CreateInGameText, "InGameText", 4);
    RegisterTextLogic();
    ctx->m_14->RegisterType(CreateWormhole, "Wormhole", 4);
    RegisterWormholeLogic();
    ctx->m_14->RegisterType(CreateGruntPuddle, "GruntPuddle", 4);
    RegisterLogic_6445e8();
    ctx->m_14->RegisterType(CreateRollingBall, "RollingBall", 4);
    CRollingBall::RegisterActs();
    ctx->m_14->RegisterType(CreateObjectDropper, "ObjectDropper", 4);
    CObjectDropper::RegisterActs();
    ctx->m_14->RegisterType(CreateDroppedObject, "DroppedObject", 4);
    CDroppedObject::RegisterActs();
    ctx->m_14->RegisterType(CreateDroppedObjectShadow, "DroppedObjectShadow", 4);
    CDroppedObjectShadow::RegisterActs();
    ctx->m_14->RegisterType(CreateCheckpointTrigger, "CheckpointTrigger", 4);
    CTileSecretTrigger::RegisterActs();
    ctx->m_14->RegisterType(CreateTeleporter, "Teleporter", 4);
    CTeleporter_RegisterActs();
    ctx->m_14->RegisterType(CreateSecretTeleporterTrigger, "SecretTeleporterTrigger", 4);
    CSecretTeleporterTrigger::RegisterActs();
    ctx->m_14->RegisterType(CreateSecretLevelTrigger, "SecretLevelTrigger", 4);
    CSecretLevelTrigger::RegisterActs();
    ctx->m_14->RegisterType(CreateProjectile, "Projectile", 4);
    CProjectile::RegisterType();
    ctx->m_14->RegisterType(CreateBoomerang, "Boomerang", 4);
    CProjectile::RegisterType();
    ctx->m_14->RegisterType(CreateStaticHazard, "StaticHazard", 4);
    CStaticHazard::RegisterActs();
    ctx->m_14->RegisterType(CreateToobSpikez, "ToobSpikez", 4);
    RegHelper_10a5();
    ctx->m_14->RegisterType(CreateTimeBomb, "TimeBomb", 4);
    RegHelper_146f();
    ctx->m_14->RegisterType(CreateSpotLight, "SpotLight", 4);
    RegisterActs_646188();
    ctx->m_14->RegisterType(CreateKitchenSlime, "KitchenSlime", 4);
    CKitchenSlime::RegisterType();
    ctx->m_14->RegisterType(CreateSingleAnimation, "SingleAnimation", 4);
    CSingleAnimation::RegisterActs();
    ctx->m_14->RegisterType(CreateWayPoint, "WayPoint", 4);
    ctx->m_14->RegisterType(CreateWarlord, "Warlord", 4);
    RegisterWarlordActions();
    ctx->m_14->RegisterType(CreatePathHazard, "PathHazard", 4);
    RegisterActs_646250();
    ctx->m_14->RegisterType(CreateRainCloud, "RainCloud", 4);
    RegisterActs_646250();
    ctx->m_14->RegisterType(CreateUFO, "UFO", 4);
    RegisterActs_646250();
    ctx->m_14->RegisterType(CreateGruntVoice, "GruntVoice", 4);
    RegisterActs_6514d8();
    ctx->m_14->RegisterType(CreateWarpStonePad, "WarpStonePad", 4);
    CWarpStonePad::RegisterActs();
    ctx->m_14->RegisterType(CreateGuardPoint, "GuardPoint", 4);
    ctx->m_14->RegisterType(CreateVoiceTrigger, "VoiceTrigger", 4);
    RegHelper_2400();
    ctx->m_14->RegisterType(CreateLevelTime, "LevelTime", 4);
    ctx->m_14->RegisterType(CreateCursorSnapSprite, "CursorSnapSprite", 1);
    RegisterXLogic_62bfa0();
    ctx->m_14->RegisterType(CreateLightFx, "LightFx", 4);
    CLightFx::RegisterActs();
    ctx->m_14->RegisterType(CreateDemoMover, "DemoMover", 0);
    ctx->m_14->RegisterType(CreateDemoSign, "DemoSign", 0);
}

// ---------------------------------------------------------------------------
// 0x00af50 - reset a global DWORD to 0 (the global at VA 0x6295d8 / RVA 0x2295d8).
// __cdecl free function. RVA-homed here (RVA-contiguous with the factory registrar).
// @orphan: only caller is an unrecovered fn (~0xaa92); free reset with no owner.
// ---------------------------------------------------------------------------
RVA(0x0000af50, 0xb)
void ResetDat6295d8() {
    g_dat6295d8 = 0;
}

SIZE_UNKNOWN(GameObjFactoryCtx);
SIZE_UNKNOWN(GameObjTypeRegistry);

// --- vtable catalog ---
