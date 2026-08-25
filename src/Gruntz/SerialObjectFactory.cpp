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
i32 RestoreGameFromFile(CGruntzMgr* mgr, char* path) {
    if (mgr == NULL) {
        return 0;
    }
    if (path == NULL) {
        return 0;
    }
    if (strlen(path) == 0) {
        return 0;
    }
    g_serialCounter = 0;
    memset(g_saveBuf, 0, 0x90);
    if (mgr->m_world == NULL) {
        return 0;
    }
    // Filter seeded with 0, NOT LOGIC_NONE(-1): retail pushes 0 here, the same
    // "no filter" idiom as CGameSave::Save's SnapshotChildren call.
    return mgr->m_world->RestoreChildren(&GameSerializationCallback, path, LOGIC_UNSET)
           != LOGIC_UNSET;
}

// @early-stop
RVA(0x0000d2a0, 0x1984)
i32 __cdecl GameSerializationCallback(
    CDDrawSurfaceMgr* ctx,
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    void* payload
) {
    if (ctx == NULL) {
        return 0;
    }
    if (archive == NULL) {
        return 0;
    }

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
                    *result = new CTileTrigger(CUserLogic::INLINE_BASE);
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
                    *result = new CGruntHealthSprite(CUserLogic::INLINE_BASE);
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
                    *result = new CProjectile(CUserLogic::INLINE_BASE);
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
                    *result = new CPathHazard(CUserLogic::INLINE_BASE);
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
    return g_gameReg->SerializeGameState(archive, mode, typeId, payloadWord.m_word) != 0;
}

// The pinned halves of the four ctor pairs declared in UserLogic.h,
// GruntHealthSprite.h, PathHazard.h and MovingLogic.h.  Each body EXPANDS its
// CUserLogic base - it stamps ??_7CUserBase and carries the single
// `call ??0zBitVec` - and the classes derived from it `call` the body, while
// the class's own `new` arm above expands the tagged inline sibling.
RVA_COMPGEN(0x00011160, 0x4b, ??0CTileTrigger@@QAE@XZ)
CTileTrigger::CTileTrigger() : CUserLogic(CUserLogic::INLINE_BASE) {}

RVA_COMPGEN(0x00011ef0, 0x4b, ??0CGruntHealthSprite@@QAE@XZ)
CGruntHealthSprite::CGruntHealthSprite() : CUserLogic(CUserLogic::INLINE_BASE) {}

// CMovingLogic/CProjectile realization group. Retail inlines CProjectile::CProjectile()
// into the LOGIC_PROJECTILE arm at depth 1: its base ??0CMovingLogic call stays
// out-of-line (the LOGIC_BOOMERANG arm likewise calls ??0CProjectile as its base).
RVA_COMPGEN(0x000126e0, 0x1fc, ??0CProjectile@@QAE@XZ)
CProjectile::CProjectile() : CMovingLogic(CMotionState::INLINE_BASE) {}

RVA_COMPGEN(0x00013170, 0x7b, ??0CPathHazard@@QAE@XZ)
CPathHazard::CPathHazard() : CUserLogic(CUserLogic::INLINE_BASE) {}

// The pinned half of CMotionState's two-entity split: CGrunt::CGrunt,
// CProjectile::CProjectile and GameSerializationCallback above all `call` it, while the
// CMovingLogic()/CProjectile() COMDATs beside it expand CMotionState(INLINE_BASE).
// The COMPGEN form is the accurate one and not a workaround: retail's 0x136d0 IS a
// compiler-emitted out-of-line copy of an inline ctor cl declined to expand, exactly
// like the six siblings pinned around it.  The definition below is only how we make
// our cl emit that same body.  It also keeps this unit's tu_order span honest - the
// span is measured from RVA() rows, and a real claim at 0x136d0 would stretch
// GameSerializationCallback across the carved SerializeSyncMarker (0x13610).
RVA_COMPGEN(0x000136d0, 0x184, ??0CMotionState@@QAE@XZ)
CMotionState::CMotionState() {
    InitBounds();
}

// The pinned half of CUserLogic's default-ctor pair: GameSerializationCallback above
// `call`s it at 45 of its 57 direct-derived sites and expands the CUserLogic(EInlineBase)
// sibling at the other 11.  Same COMPGEN reasoning as CMotionState below - a real RVA()
// claim here would stretch this unit's tu_order span across the carved 0x13610.
RVA_COMPGEN(0x000138d0, 0x4b, ??0CUserLogic@@QAE@XZ)
CUserLogic::CUserLogic() {}

RVA_COMPGEN(0x00013940, 0x1e1, ??0CMovingLogic@@QAE@XZ)
CMovingLogic::CMovingLogic()
    : CUserLogic(CUserLogic::INLINE_BASE), m_motion(CMotionState::INLINE_BASE) {}

RVA_COMPGEN(0x00013bd0, 0x44, ??1CMovingLogic@@UAE@XZ)
RVA_COMPGEN(0x00013c40, 0x1e, ??_GCMovingLogic@@UAEPAXI@Z)
