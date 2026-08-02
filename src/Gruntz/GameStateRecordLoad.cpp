#include <Gruntz/GameStateRecordLoad.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Rez/RezAlloc.h>
#include <Gruntz/Grunt.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <Wwd/WwdGameObjectFamily.h>
#include <Io/FileMem.h>
#include <Gruntz/SpriteRefTable.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SerialArchive.h>
#include <Mfc.h>
#include <rva.h>
#include <string.h>

#include <Gruntz/FreeNodePool.h>
#include <Utils/MapTyped.h>

static const char s_Powerupz[] = "Powerupz";
static const char s_GruntGhostTransparencyOn[] = "GruntGhostTransparencyOn";

#define SERIALREF(field)                                                                           \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(&id, 4);                                                                          \
        obj = 0;                                                                                   \
        void* r;                                                                                   \
        if (MapLookupById(dir->m_childGroup->m_map48, id, obj) != 0 && obj != 0) {                 \
            r = ((static_cast<CGameObject*>(obj))->GetClassId() == CLASSID_SERIALREF) ? obj : 0;   \
        } else {                                                                                   \
            r = 0;                                                                                 \
        }                                                                                          \
        (field) = static_cast<CWwdGameObjectA*>(r);                                                \
        if (r == 0 && id != 0) {                                                                   \
            return 0;                                                                              \
        }                                                                                          \
    } while (0)
#define READCSTR(field)                                                                            \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, 0x80);                                                                       \
        (field) = buf;                                                                             \
    } while (0)
#define NAMEREF(field)                                                                             \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, 0x80);                                                                       \
        if (strlen(buf) != 0) {                                                                    \
            obj = 0;                                                                               \
            dir->m_animRegistry->m_animations.Lookup(buf, obj);                                    \
            (field) = static_cast<CAniElement*>(obj);                                              \
        } else {                                                                                   \
            (field) = 0;                                                                           \
        }                                                                                          \
    } while (0)

// @early-stop
RVA(0x000555e0, 0x12f8)
i32 CGrunt::LoadStateRecord(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* dir = g_gameReg->m_world;
    if (dir == 0) {
        return 0;
    }

    i32 id;
    void* obj;
    char buf[0x80];

    m_struckSlotSound = 0;
    m_struckVoiceSound = 0;
    m_struckCount = 0;
    m_struckClockLo = 0;
    m_struckTimerLo = 0;
    m_struckClockHi = 0;
    m_struckTimerHi = 0;

    SERIALREF(m_selectedSprite);
    SERIALREF(m_toySprite);
    SERIALREF(m_healthSprite);
    SERIALREF(m_staminaSprite);
    SERIALREF(m_toyTimeSprite);
    SERIALREF(m_wingzTimeSprite);
    SERIALREF(m_powerupSprite);

    READCSTR(m_animSetName);
    READCSTR(m_frameSetName);
    READCSTR(m_deathFrameSetName);

    NAMEREF(m_poseWalk);
    NAMEREF(m_poseAttack[GRUNT_ATTACK1]);
    NAMEREF(m_poseAttack[GRUNT_ATTACK2]);
    NAMEREF(m_poseAttackIdle);
    NAMEREF(m_poseStruck[GRUNT_STRUCK1]);
    NAMEREF(m_poseStruck[GRUNT_STRUCK2]);
    NAMEREF(m_poseIdle[GRUNT_IDLE1]);
    NAMEREF(m_poseIdle[GRUNT_IDLE2]);
    NAMEREF(m_poseIdle[GRUNT_IDLE3]);
    NAMEREF(m_poseIdle[GRUNT_IDLE4]);
    NAMEREF(m_poseIdle[GRUNT_IDLE5]);
    NAMEREF(m_poseDeath);
    NAMEREF(m_poseToy[GRUNT_TOY1]);
    NAMEREF(m_poseToy[GRUNT_TOY2]);
    NAMEREF(m_poseToy[GRUNT_TOY_BREAK]);
    NAMEREF(m_poseItem[GRUNT_ITEM1]);
    NAMEREF(m_poseItem[GRUNT_ITEM2]);
    NAMEREF(m_pickupGeoSrc);

    ar->Read(&m_reserved18c, 4);
    ar->Read(&m_toyBlendPct, 4);
    ar->Read(&m_brickPickupType, 4);
    ar->Read(&m_entranceReason, 4);
    ar->Read(&m_vehiclePickupType, 4);
    ar->Read(&m_toolId, 4);
    ar->Read(&m_moveMode, 4);
    ar->Read(&m_helpCueId, 4);
    ar->Read(&m_reserved1a8, 4);
    ar->Read(&m_reserved1ac, 4);
    ar->Read(&m_reserved1b0, 4);
    ar->Read(&m_reserved1b4, 4);
    ar->Read(&m_arrived, 4);
    ar->Read(&m_entrancePx, 8);
    ar->Read(&m_lastTilePx, 8);
    ar->Read(&m_commitPx, 8);
    ar->Read(&m_reserved1dc, 8);
    ar->Read(&m_entranceActive, 4);
    ar->Read(&m_arrivalPending, 4);
    ar->Read(&m_tileOwnerHi, 4);
    ar->Read(&m_tileOwnerLo, 4);
    ar->Read(&m_moveIcon, 4);
    ar->Read(&m_savedMoveIcon, 4);
    ar->Read(&m_entranceCommitted, 4);
    ar->Read(&m_neighborCell, 8);
    ar->Read(&m_attackTargetPx, 8);
    ar->Read(&m_reserved210, 4);
    ar->Read(&m_struckPose, 4);
    ar->Read(&m_combatActive, 4);
    ar->Read(&m_neighborValid, 4);
    ar->Read(&m_poweredUp, 4);
    ar->Read(&m_daFlag, 4);
    ar->Read(&m_entranceStamped, 4);
    ar->Read(&m_bombRunActive, 4);
    ar->Read(&m_arrivalActive, 4);
    ar->Read(&m_reachRect, 0x10);
    ar->Read(&m_reachExclusionRect, 0x10);
    ar->Read(&m_toyRectA, 0x10);
    ar->Read(&m_toyRectB, 0x10);
    ar->Read(&m_health, 4);
    ar->Read(&m_stamina, 4);
    ar->Read(&m_toyTime, 4);
    ar->Read(&m_wingzTime, 4);
    ar->Read(&m_moveSpeed, 8);
    ar->Read(&m_reserved418, 4);
    ar->Read(&m_reserved42c, 4);
    ar->Read(&m_reserved430, 4);
    ar->Read(&m_startingItemId, 4);
    ar->Read(&m_recordedFrameTick, 4);
    ar->Read(&m_arrivalState, 4);
    ar->Read(&m_defenderState, 4);
    ar->Read(&m_battleState, 4);
    ar->Read(&m_defenderRadius, 4);
    ar->Read(&m_defenderQueuePosition, 4);
    ar->Read(&m_defenderPickupType, 4);
    ar->Read(&m_dwell, 4);
    ar->Read(&m_arrivalCell, 8);
    ar->Read(&m_defenderPx, 8);
    ar->Read(&m_toolConfigured, 4);
    ar->Read(&m_neighborScanEnabled, 4);
    ar->Read(&m_tileMoveCommitted, 4);
    ar->Read(&m_reserved3dc, 8);
    ar->Read(&m_moveTile, 8);
    ar->Read(&m_arrivalPhase, 4);
    ar->Read(&m_timePerTile, 4);
    ar->Read(&m_movePosX, 8);
    ar->Read(&m_movePosY, 8);
    ar->Read(&m_reserved8d0, 4);
    ar->Read(&m_coordToggle, 4);
    ar->Read(&m_wingzEnabled, 4);
    ar->Read(&m_freezeDelayDone, 4);
    ar->Read(&m_freezeUnfrozen, 4);
    ar->Read(&m_resetApplied, 4);
    ar->Read(&m_arrivalFlags, 4);
    ar->Read(&m_passableMask, 4);
    ar->Read(&m_gruntKind, 4);
    ar->Read(&m_entranceArmed, 4);
    ar->Read(&m_deathType, 4);
    ar->Read(&m_entranceDropActive, 4);
    ar->Read(&m_hasExtent, 4);
    ar->Read(&m_unusedBattleCell, 8);
    ar->Read(&m_cellRemovalNotified, 4);
    ar->Read(&m_pendingTrigger, 4);
    ar->Read(&m_killerSlot, 4);
    ar->Read(&m_tileClaimed, 4);
    ar->Read(&m_deathAnimStarted, 4);
    ar->Read(&m_pendingTriggerPx, 8);
    ar->Read(&m_routeMaskA, 4);
    ar->Read(&m_routeMaskC, 4);
    ar->Read(&m_moveVariantOverride, 4);
    ar->Read(&m_moveKind, 4);
    ar->Read(&m_moveVariant, 4);
    ar->Read(&m_coordRetryCount, 4);
    ar->Read(&m_toyTileIndex, 4);
    ar->Read(&m_blockedVoicePending, 4);
    ar->Read(&m_powerupDuration, 4);
    ar->Read(&m_warpstoneAnchorIndex, 4);
    ar->Read(&m_lowStaminaCued, 4);
    ar->Read(&m_targetTeam, 4);
    ar->Read(&m_arrivalTargetPx, 8);

    CGruntCellRec* row = m_cells;
    for (i32 gi = 0; gi < 3; ++gi) {
        CGruntCellRec* cell = row;
        for (i32 gj = 0; gj < 3; ++gj) {
            if (cell->DeserializeStrings(ar) == 0) {
                return 0;
            }
            cell += 1;
        }
        row += 3;
    }

    if (m_coordList.GetCount() != 0) {
        POSITION pos = m_coordList.GetHeadPosition();
        if (pos != 0) {
            CoordPoolNode* fl = g_coordPool.m_freeHead;
            do {
                void* buf = m_coordList.GetNext(pos);
                if (buf != 0) {
                    CoordPoolNode* n2 = g_coordPool.NodeOf(buf);
                    n2->m_next = fl;
                    fl = n2;
                    g_coordPool.m_freeHead = n2;
                }
            } while (pos != 0);
        }
        (&m_coordList)->RemoveAll();
    }

    i32 count;
    ar->Read(&count, 4);
    for (i32 a = 0; a < count; ++a) {
        CoordPoolNode* slot = g_coordPool.m_freeHead;
        CoordPoolNode* nf = slot->m_next;
        void* item = 0;
        if (nf != 0) {
            item = &slot->m_coord;
            g_coordPool.m_freeHead = nf;
        }
        ar->Read(item, 8);
        (&m_coordList)->AddTail(item);
    }

    while (m_payloads.GetCount() != 0 && m_payloads.GetHead() != 0) {
        void* rem = (&m_payloads)->RemoveHead();
        RezFree(rem);
    }

    ar->Read(&count, 4);
    for (i32 b = 0; b < count; ++b) {
        void* mem = operator new(0x2c);
        void* item = 0;
        if (mem != 0) {
            memset(mem, 0, 0xb * 4);
            item = mem;
        }
        ar->Read(item, 0x2c);
        (&m_payloads)->AddTail(item);
    }

    i32 flag = (m_entranceReason >= 0x17);
    CShadeTable* r = g_gameReg->m_spriteFactory->GetSel(m_moveIcon, flag);
    CWwdGameObjectA* cb = m_object;
    cb->m_drawActive = 1;
    cb->m_drawFillCmd = 0xa;
    cb->m_drawFillArg = r;

    if (m_gruntKind == 0x36) {
        CWwdGameObjectA* cb2 = m_object;
        i32 v = g_buteMgr.GetIntDef(s_Powerupz, s_GruntGhostTransparencyOn, 0xe0);
        cb2->m_drawActive = 1;
        cb2->m_drawFillCmd = 0xb;
        cb2->m_fillFraction = v;
    }
    return 1;
}
