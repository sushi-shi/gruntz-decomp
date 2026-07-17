// ObjTypeRegistrars.h - external per-type registration entry points called by
// RegisterGameObjectTypes (GameObjectFactory.cpp). Each is the real reconstructed
// registrar in its own class's TU; declared here so the factory's reloc-masked
// CALLs bind to the correct retail RVAs (reloc_fidelity). Static/free registrars
// only - the __thiscall registrars stay in the .cpp (can't be plain-called here).
// The RTTI-known game-object classes carry their real base so the vtable audit
// (INHERIT) stays clean; the rest are non-RTTI helpers (no base).
// NOTE: a base-less / single-base `struct C... { static void RegisterActs(); };`
// shell here is a SECOND, DIVERGENT definition of a real class (these leaves are MI:
// CUserLogic + CWapX). It also MASKS the size gate: class_sizes keeps one size per
// NAME, so a multiply-defined class reports UNVERIFIABLE instead of being checked -
// which is exactly how CBrickz's and CSingleAnimation's stale CWapX-as-padding
// members stayed hidden. Include the real header instead; it is layout-neutral
// (a static member changes no offset and no vtable) and the mangled names the
// factory CALLs bind to are identical.
#ifndef GRUNTZ_OBJTYPEREGISTRARS_H
#define GRUNTZ_OBJTYPEREGISTRARS_H

#include <Gruntz/MovingLogic.h> // CMovingLogic (CProjectile base) -> pulls CUserLogic
// The REAL grunt-HUD sprite leaves (they already declare the static RegisterActs the factory
// calls). The old base-less shells for these four are gone - see the note below.
#include <Gruntz/CBrickz.h>     // the REAL CBrickz (its static RegisterActs @0x10ebe0)
#include <Gruntz/ExitTrigger.h> // the REAL CExitTrigger (its static RegisterActs @0x3f3f0)
#include <Gruntz/ObjectDropper.h> // the REAL CObjectDropper (MI: CUserLogic + CWapX)
#include <Gruntz/Particlez.h> // the REAL CParticlez (MI: CUserLogic + CWapX)
#include <Gruntz/RollingBall.h> // the REAL CRollingBall (MI: CUserLogic + CWapX)
#include <Gruntz/SecretLevelTrigger.h> // the REAL CSecretLevelTrigger (MI: CUserLogic + CWapX)
#include <Gruntz/SecretTeleporterTrigger.h> // the REAL CSecretTeleporterTrigger (MI: CUserLogic + CWapX)
#include <Gruntz/SingleAnimation.h> // the REAL CSingleAnimation (MI: CUserLogic + CWapX)
#include <Gruntz/StaticHazard.h> // the REAL CStaticHazard (MI: CUserLogic + CWapX)
#include <Gruntz/StatusBarSprite.h> // the REAL CStatusBarSprite (MI: CUserLogic + CWapX)
#include <Gruntz/TileTriggerSwitch.h> // the REAL CTileTriggerSwitch (MI: CUserLogic + CWapX)
#include <Gruntz/TileTriggerTransition.h> // the REAL CTileTriggerTransition (MI: CUserLogic + CWapX)
#include <Gruntz/TimeBomb.h> // the REAL CTimeBomb (MI: CUserLogic + CWapX)
#include <Gruntz/ToobSpikez.h> // the REAL CToobSpikez (MI: CUserLogic + CWapX)
#include <Gruntz/VoiceTrigger.h> // the REAL CVoiceTrigger (MI: CUserLogic + CWapX)
#include <Gruntz/WarpStonePad.h> // the REAL CWarpStonePad (MI: CUserLogic + CWapX)
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntToySprite.h>

// RTTI-known logic classes: real base per vtable_hierarchy so INHERIT stays clean.
struct CProjectile : public CMovingLogic {
    static void RegisterType();
};
// CTileTrigger (+ its RegisterActs @0x10e600) comes from <Gruntz/UserLogic.h>.
// (The CTileSecretTrigger shell is GONE: its "RegisterActs @0x10f340" is
// CCheckpointTrigger's - the TileLogicPump act clusters were shifted one class.
// CBrickz's registrar @0x10ebe0 comes from the REAL <Gruntz/CBrickz.h> included
// above - no base-less shell: this class is MI (CUserLogic + CWapX) and a shell
// would be a second, divergent definition.)
// (CExitTrigger's registrar @0x3f3f0 comes from the REAL <Gruntz/ExitTrigger.h>
// included above - the ex 'CWormhole' shell here declared a base-less/single-base
// shape while the real class is MI (CUserLogic + CWapX): a divergent redefinition.)

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
    static void RegisterActs(); // 0x10f340 (home: TileLogicPump.cpp; ex 'CTileSecretTrigger's')
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
