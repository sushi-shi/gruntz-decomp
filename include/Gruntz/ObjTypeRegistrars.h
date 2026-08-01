#ifndef GRUNTZ_OBJTYPEREGISTRARS_H
#define GRUNTZ_OBJTYPEREGISTRARS_H

#include <Gruntz/MovingLogic.h>
#include <rva.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/ObjectDropper.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/RollingBall.h>
#include <Gruntz/SecretLevelTrigger.h>
#include <Gruntz/SecretTeleporterTrigger.h>
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/StaticHazard.h>
#include <Gruntz/StatusBarSprite.h>
#include <Gruntz/TileTriggerSwitch.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/ToobSpikez.h>
#include <Gruntz/VoiceTrigger.h>
#include <Gruntz/WarpStonePad.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntToySprite.h>

struct CProjectile : public CMovingLogic {
    static void RegisterType();
};
SIZE(0x228);

struct CProjActObj {
    static void RegisterType();
};
SIZE_UNKNOWN();
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

void RegisterSimpleAnimLogic();
void RegisterMenuSparkleActions();
void RegisterGruntActions();
void RegisterExplosionActions();
void RegisterIconState();
void ActReg4RegisterType();
void RegisterIconActions();
void RegisterTextLogic();
void RegisterWormholeLogic();
void RegisterLogic();
void CTeleporter_RegisterActs();
void RegisterSpotLightActions();
void RegisterWarlordActions();
void RegisterPathHazardActions();
void RegisterGruntVoiceActions();
void RegisterCursorSnapActions();

#endif
