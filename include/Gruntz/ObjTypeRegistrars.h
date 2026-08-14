#ifndef GRUNTZ_OBJTYPEREGISTRARS_H
#define GRUNTZ_OBJTYPEREGISTRARS_H

#include <rva.h>

#include <Gruntz/AniCycle.h>
#include <Gruntz/BehindCandyAni.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/CheckpointTrigger.h>
#include <Gruntz/DroppedObject.h>
#include <Gruntz/DroppedObjectShadow.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/FortressFlag.h>
#include <Gruntz/FrontCandyAni.h>
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/KitchenSlime.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/MovingLogic.h>
#include <Gruntz/ObjectDropper.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/Projectile.h>
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

// CProjActObj has no state and no vtable: it exists only to own the
// `A`-action registrar that installs CActionArea::Tick. Its body lives in
// ActionArea.cpp beside the method it registers.
struct CProjActObj {
    static void RegisterType();
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
