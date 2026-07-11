// ObjTypeRegistrars.h - external per-type registration entry points called by
// RegisterGameObjectTypes (GameObjectFactory.cpp). Each is the real reconstructed
// registrar in its own class's TU; declared here so the factory's reloc-masked
// CALLs bind to the correct retail RVAs (reloc_fidelity). Static/free registrars
// only - the __thiscall registrars stay in the .cpp (can't be plain-called here).
// The RTTI-known game-object classes carry their real base so the vtable audit
// (INHERIT) stays clean; the rest are non-RTTI helpers (no base).
#ifndef GRUNTZ_OBJTYPEREGISTRARS_H
#define GRUNTZ_OBJTYPEREGISTRARS_H

#include <Gruntz/MovingLogic.h> // CMovingLogic (CProjectile base) -> pulls CUserLogic

// RTTI-known logic classes: real base per vtable_hierarchy so INHERIT stays clean.
struct CObjectDropper : public CUserLogic {
    static void RegisterActs();
};
struct CParticlez : public CUserLogic {
    static void RegisterActs();
};
struct CProjectile : public CMovingLogic {
    static void RegisterType();
};
struct CRollingBall : public CUserLogic {
    static void RegisterActs();
};
struct CSecretLevelTrigger : public CUserLogic {
    static void RegisterActs();
};
struct CSecretTeleporterTrigger : public CUserLogic {
    static void RegisterActs();
};
struct CSingleAnimation : public CUserLogic {
    static void RegisterActs();
};
struct CStaticHazard : public CUserLogic {
    static void RegisterActs();
};
struct CStatusBarSprite : public CUserLogic {
    static void RegisterActs();
};
// CTileTrigger (+ its RegisterActs @0x10e600) comes from <Gruntz/UserLogic.h>.
struct CTileSecretTrigger : public CTileTrigger {
    static void RegisterActs();
};
struct CTileTriggerSwitch : public CUserLogic {
    static void RegisterActs();
};
struct CWarpStonePad : public CUserLogic {
    static void RegisterActs();
};
struct CWormhole : public CUserLogic {
    static void RegisterActs();
};

// non-RTTI helper registrars: CClass::RegisterActs()/RegisterType() -> ?..@@SAXXZ
struct CAniCycle {
    static void RegisterActs();
};
struct CFrontCandyAni {
    static void RegisterActs();
};
struct CBehindCandyAni {
    static void RegisterActs();
};
struct CEyeCandyAni {
    static void RegisterActs();
};
struct CGruntSelectedSprite {
    static void RegisterActs();
};
struct CGruntHealthSprite {
    static void RegisterActs();
};
struct CGruntToySprite {
    static void RegisterActs();
};
struct CGruntPowerupSprite {
    static void RegisterActs();
};
struct CCheckpointTrigger {
    static void RegisterActs();
};
struct CGruntCreationPoint {
    static void RegisterActs();
};
struct CFortressFlag {
    static void RegisterActs();
};
struct CDroppedObject {
    static void RegisterActs();
};
struct CDroppedObjectShadow {
    static void RegisterActs();
};
struct CKitchenSlime {
    static void RegisterType();
};
struct CLightFx {
    static void RegisterActs();
};

// free-function registrars: -> ?name@@YAXXZ
void RegisterSimpleAnimLogic();
void RegisterXLogic_646010();
void RegisterActs_644af0();
void RegisterXLogic_6447f8();
void RegisterIconState();
void ActReg4RegisterType();
void RegisterIconActions();
void RegisterTextLogic();
void RegisterWormholeLogic();
void RegisterLogic_6445e8();
void CTeleporter_RegisterActs();
void RegisterActs_646188();
void RegisterWarlordActions();
void RegisterActs_646250();
void RegisterActs_6514d8();
void RegisterXLogic_62bfa0();

#endif
