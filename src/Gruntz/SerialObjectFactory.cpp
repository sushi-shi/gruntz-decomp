#include <rva.h>

#include <Gruntz/SerialObjectFactory.h>

#include <AddrWord.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/ActionArea.h>
#include <Gruntz/AniCycle.h>
#include <Gruntz/BehindCandy.h>
#include <Gruntz/BehindCandyAni.h>
#include <Gruntz/Boomerang.h>
#include <Gruntz/CBrickz.h>
#include <Gruntz/CheckpointTrigger.h>
#include <Gruntz/CursorSnapSprite.h>
#include <Gruntz/DoNothing.h>
#include <Gruntz/DoNothingNormal.h>
#include <Gruntz/DroppedObject.h>
#include <Gruntz/DroppedObjectShadow.h>
#include <Gruntz/ExitTrigger.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/EyeCandy.h>
#include <Gruntz/EyeCandyAni.h>
#include <Gruntz/FortressFlag.h>
#include <Gruntz/FrontCandy.h>
#include <Gruntz/FrontCandyAni.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCreationPoint.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/GruntPowerupSprite.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GruntStaminaSprite.h>
#include <Gruntz/GruntStartingPoint.h>
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GruntToyTimeSprite.h>
#include <Gruntz/GruntWingzTimeSprite.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GuardPoint.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/KitchenSlime.h>
#include <Gruntz/LevelTime.h>
#include <Gruntz/LightFx.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MenuSparkle.h>
#include <Gruntz/ObjectDropper.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/PathHazard.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/RainCloud.h>
#include <Gruntz/RollingBall.h>
#include <Gruntz/SecretLevelTrigger.h>
#include <Gruntz/SecretTeleporterTrigger.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SimpleAnimation.h>
#include <Gruntz/SingleAnimation.h>
#include <Gruntz/SingleFrameMessage.h>
#include <Gruntz/SpotLight.h>
#include <Gruntz/StaticHazard.h>
#include <Gruntz/StatusBarSprite.h>
#include <Gruntz/Teleporter.h>
#include <Gruntz/TileTrigger.h>
#include <Gruntz/TileTriggerSwitch.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/ToobSpikez.h>
#include <Gruntz/ToyPeek.h>
#include <Gruntz/Ufo.h>
#include <Gruntz/VoiceTrigger.h>
#include <Gruntz/Warlord.h>
#include <Gruntz/WarpStonePad.h>
#include <Gruntz/WayPoint.h>
#include <Gruntz/Wormhole.h>
#include <Ints.h>
#include <Io/FileMem.h>
#include <Io/GameSave.h>

#include <string.h>

RVA(0x0000d210, 0x65)
i32 ParseSerial(CGruntzMgr* mgr, char* s) {
    if (mgr == NULL) {
        return 0;
    }
    if (s == NULL) {
        return 0;
    }
    if (strlen(s) == 0) {
        return 0;
    }
    g_serialCounter = 0;
    memset(g_saveBuf, 0, 0x90);
    if (mgr->m_world == NULL) {
        return 0;
    }
    return mgr->m_world->RestoreChildren(&SerialObjectFactory, s, LOGIC_NONE) != 0;
}

// @early-stop
RVA(0x0000d2a0, 0x1984)
i32 __cdecl
SerialObjectFactory(void* ctx, void* ar, SerialMode mode, LogicTypeId typeId, void* payload) {
    if (ctx == NULL) {
        return 0;
    }
    if (ar == NULL) {
        return 0;
    }

    CFileMemBase* archive = static_cast<CFileMemBase*>(ar);
    CUserLogic** result = static_cast<CUserLogic**>(payload);

    switch (mode) {
        case SERIAL_SNAPSHOT_BEGIN:
            archive->Write(g_saveBuf, 0x90);
            break;
        case SERIAL_RESTORE_BEGIN:
            archive->Read(g_saveBuf, 0x90);
            break;
        case SERIAL_CREATE:
            switch (typeId) {
                case LOGIC_GRUNT:
                    *result = new CGrunt();
                    break;
                case LOGIC_ROLLINGBALL:
                    *result = new CRollingBall();
                    break;
                case LOGIC_ANICYCLE:
                    *result = new CAniCycle();
                    break;
                case LOGIC_SINGLEFRAMEMESSAGE:
                    *result = new CSingleFrameMessage();
                    break;
                case LOGIC_DONOTHING:
                    *result = new CDoNothing();
                    break;
                case LOGIC_DONOTHINGNORMAL:
                    *result = new CDoNothingNormal();
                    break;
                case LOGIC_SIMPLEANIMATION:
                    *result = new CSimpleAnimation();
                    break;
                case LOGIC_FRONTCANDY:
                    *result = new CFrontCandy();
                    break;
                case LOGIC_BEHINDCANDY:
                    *result = new CBehindCandy();
                    break;
                case LOGIC_EYECANDY:
                    *result = new CEyeCandy();
                    break;
                case LOGIC_FRONTCANDYANI:
                    *result = new CFrontCandyAni();
                    break;
                case LOGIC_BEHINDCANDYANI:
                    *result = new CBehindCandyAni();
                    break;
                case LOGIC_EYECANDYANI:
                    *result = new CEyeCandyAni();
                    break;
                case LOGIC_MENUSPARKLE:
                    *result = new CMenuSparkle();
                    break;
                case LOGIC_GRUNTSTARTINGPOINT:
                    *result = new CGruntStartingPoint();
                    break;
                case LOGIC_EXITTRIGGER:
                    *result = new CExitTrigger();
                    break;
                case LOGIC_GRUNTCREATIONPOINT:
                    *result = new CGruntCreationPoint();
                    break;
                case LOGIC_WORMHOLE:
                    *result = new CWormhole();
                    break;
                case LOGIC_GRUNTPUDDLE:
                    *result = new CGruntPuddle();
                    break;
                case LOGIC_TELEPORTER:
                    *result = new CTeleporter();
                    break;
                case LOGIC_CURSORSNAPSPRITE:
                    *result = new CCursorSnapSprite();
                    break;
                case LOGIC_LEVELTIME:
                    *result = new CLevelTime();
                    break;
                case LOGIC_STATUSBARSPRITE:
                    *result = new CStatusBarSprite();
                    break;
                case LOGIC_TILETRIGGERSWITCH:
                    *result = new CTileTriggerSwitch();
                    break;
                case LOGIC_TILETRIGGER:
                    *result = new CTileTrigger();
                    break;
                case LOGIC_CHECKPOINTTRIGGER:
                    *result = new CCheckpointTrigger();
                    break;
                case LOGIC_TILESECRETTRIGGER:
                    *result = new CTileSecretTrigger();
                    break;
                case LOGIC_COVEREDPOWERUP:
                    *result = new CCoveredPowerup();
                    break;
                case LOGIC_TILETRIGGERTRANSITION:
                    *result = new CTileTriggerTransition();
                    break;
                case LOGIC_GIANTROCK:
                    *result = new CGiantRock();
                    break;
                case LOGIC_INGAMEICON:
                    *result = new CInGameIcon();
                    break;
                case LOGIC_INGAMETEXT:
                    *result = new CInGameText();
                    break;
                case LOGIC_BRICKZ:
                    *result = new CBrickz();
                    break;
                case LOGIC_GRUNTSELECTEDSPRITE:
                    *result = new CGruntSelectedSprite();
                    break;
                case LOGIC_GRUNTHEALTHSPRITE:
                    *result = new CGruntHealthSprite();
                    break;
                case LOGIC_GRUNTTOYSPRITE:
                    *result = new CGruntToySprite();
                    break;
                case LOGIC_LIGHTFX:
                    *result = new CLightFx();
                    break;
                case LOGIC_OBJECTDROPPER:
                    *result = new CObjectDropper();
                    break;
                case LOGIC_GRUNTSTAMINASPRITE:
                    *result = new CGruntStaminaSprite();
                    break;
                case LOGIC_GRUNTTOYTIMESPRITE:
                    *result = new CGruntToyTimeSprite();
                    break;
                case LOGIC_PROJECTILE:
                    *result = new CProjectile();
                    break;
                case LOGIC_BOOMERANG:
                    *result = new CBoomerang();
                    break;
                case LOGIC_DROPPEDOBJECT:
                    *result = new CDroppedObject();
                    break;
                case LOGIC_DROPPEDOBJECTSHADOW:
                    *result = new CDroppedObjectShadow();
                    break;
                case LOGIC_STATICHAZARD:
                    *result = new CStaticHazard();
                    break;
                case LOGIC_GRUNTWINGZTIMESPRITE:
                    *result = new CGruntWingzTimeSprite();
                    break;
                case LOGIC_TOOBSPIKEZ:
                    *result = new CToobSpikez();
                    break;
                case LOGIC_TIMEBOMB:
                    *result = new CTimeBomb();
                    break;
                case LOGIC_GRUNTPOWERUPSPRITE:
                    *result = new CGruntPowerupSprite();
                    break;
                case LOGIC_EXPLOSION:
                    *result = new CExplosion();
                    break;
                case LOGIC_PARTICLEZ:
                    *result = new CParticlez();
                    break;
                case LOGIC_SPOTLIGHT:
                    *result = new CSpotLight();
                    break;
                case LOGIC_SECRETTELEPORTERTRIGGER:
                    *result = new CSecretTeleporterTrigger();
                    break;
                case LOGIC_KITCHENSLIME:
                    *result = new CKitchenSlime();
                    break;
                case LOGIC_WAYPOINT:
                    *result = new CWayPoint();
                    break;
                case LOGIC_SINGLEANIMATION:
                    *result = new CSingleAnimation();
                    break;
                case LOGIC_WARLORD:
                    *result = new CWarlord();
                    break;
                case LOGIC_ACTIONAREA:
                    *result = new CActionArea();
                    break;
                case LOGIC_PATHHAZARD:
                    *result = new CPathHazard();
                    break;
                case LOGIC_RAINCLOUD:
                    *result = new CRainCloud();
                    break;
                case LOGIC_UFO:
                    *result = new CUFO();
                    break;
                case LOGIC_FORTRESSFLAG:
                    *result = new CFortressFlag();
                    break;
                case LOGIC_TOYPEEK:
                    *result = new CToyPeek();
                    break;
                case LOGIC_WARPSTONEPAD:
                    *result = new CWarpStonePad();
                    break;
                case LOGIC_GUARDPOINT:
                    *result = new CGuardPoint();
                    break;
                case LOGIC_VOICETRIGGER:
                    *result = new CVoiceTrigger();
                    break;
                case LOGIC_SECRETLEVELTRIGGER:
                    *result = new CSecretLevelTrigger();
                    break;
                default:
                    return 0;
            }
            return 1;
        case SERIAL_CREATE_BY_SERIAL_ID:
            return 0;
    }

    AddrWord<char> payloadWord;
    payloadWord.m_addr = static_cast<char*>(payload);
    return g_gameReg->BroadcastCmd(archive, mode, typeId, payloadWord.m_word) != 0;
}

// CMovingLogic/CProjectile realization group. Retail inlines CProjectile::CProjectile()
// into the LOGIC_PROJECTILE arm at depth 1: its base ??0CMovingLogic call stays
// out-of-line (the LOGIC_BOOMERANG arm likewise calls ??0CProjectile as its base).
// Our cl inlines that base ctor fully, so ??0CMovingLogic is emitted nowhere and its
// pin dangles - an inliner-heuristic residue for the permute campaign, not a shape bug.
RVA_COMPGEN(0x000126e0, 0x1fc, ??0CProjectile@@QAE@XZ)
RVA_COMPGEN(0x000136d0, 0x184, ??0CMotionState@@QAE@XZ)
RVA_COMPGEN(0x000138d0, 0x4b, ??0CUserLogic@@QAE@XZ)
RVA_COMPGEN(0x00013940, 0x1e1, ??0CMovingLogic@@QAE@XZ)
RVA_COMPGEN(0x00013bb0, 0x4, ?GetTypeTag@CMovingLogic@@UAE?AW4LogicTypeId@@XZ)
RVA_COMPGEN(0x00013bd0, 0x44, ??1CMovingLogic@@UAE@XZ)
RVA_COMPGEN(0x00013c40, 0x1e, ??_GCMovingLogic@@UAEPAXI@Z)
