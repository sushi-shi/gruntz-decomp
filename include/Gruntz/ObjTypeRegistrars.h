#ifndef GRUNTZ_OBJTYPEREGISTRARS_H
#define GRUNTZ_OBJTYPEREGISTRARS_H

#include <Gruntz/MovingLogic.h> // CMovingLogic (CProjectile base) -> pulls CUserLogic
#include <rva.h>
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

struct CProjectile : public CMovingLogic {
    static void RegisterType();
};
SIZE(0x228);

struct CProjActObj {
    static void RegisterType(); // 0x8240 (home: ActionArea.cpp; the ActionArea class registrar)
};
SIZE_UNKNOWN(); // static-only registrar shell - never instantiated
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

void RegisterSimpleAnimLogic();
void RegisterXLogic_646010();
void RegisterActs_644af0();
void RegisterXLogic_6447f8();
void RegisterIconState();
void ActReg4RegisterType();
void RegisterIconActions();
void RegisterTextLogic();
void RegisterWormholeLogic();
void RegisterLogic();
void CTeleporter_RegisterActs();
void RegisterActs_646188();
void RegisterWarlordActions();
void RegisterActs_646250();
void RegisterActs_6514d8();
void RegisterXLogic_62bfa0();

#endif
