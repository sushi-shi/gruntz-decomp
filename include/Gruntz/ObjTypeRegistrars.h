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
// The REAL grunt-HUD sprite leaves (they already declare the static RegisterActs the factory
// calls). The old base-less shells for these four are gone - see the note below.
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntToySprite.h>

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
struct CTimeBomb : public CUserLogic {
    static void RegisterActs(); // 0x0e1990 (home: Projectile.cpp)
};
struct CToobSpikez : public CUserLogic {
    static void RegisterActs(); // 0x1149c0 (home: ToobSpikez.cpp)
};
struct CVoiceTrigger : public CUserLogic {
    static void RegisterActs(); // 0x11a500 (home: GruntVoice.cpp)
};
struct CTileTriggerTransition : public CUserLogic {
    static void RegisterActs(); // 0x10fe70 (home: TileLogicPump.cpp)
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
struct CProjActObj {
    static void RegisterType(); // 0x8240 (home: ActionArea.cpp; the ActionArea class registrar)
};
SIZE_UNKNOWN(CProjActObj); // static-only registrar shell - never instantiated
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
// The four grunt-HUD sprite leaves are NOT redeclared here. They used to be, as base-less
// empty `struct C... { static void RegisterActs(); };` shells - a THIRD definition of each
// (canonical header + the now-deleted AnimWorkerSpriteLeaves.h size-view + this one), each
// disagreeing about the base. The canonical classes ALREADY declare the same static
// RegisterActs() at the same RVAs, so the shells bought nothing and only cost a divergent
// shape. Including the real headers is layout-neutral (a static member changes no offset and
// no vtable) and the mangled names the factory CALLs bind to are identical.
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
