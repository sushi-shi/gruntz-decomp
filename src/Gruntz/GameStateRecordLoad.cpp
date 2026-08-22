#include <rva.h>

#include <Gruntz/GameStateRecordLoad.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteRefTable.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <string.h>

#define SERIALREF(field)                                                                           \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(&id, 4);                                                                          \
        obj = NULL;                                                                                \
        CGameObject* r;                                                                            \
        if (MapLookupById(dir->m_childGroup->m_registeredGameObjectsById, id, obj) != 0            \
            && obj != NULL) {                                                                      \
            r = (obj->GetClassId() == CLASSID_SERIALREF) ? obj : NULL;                             \
        } else {                                                                                   \
            r = NULL;                                                                              \
        }                                                                                          \
        (field) = static_cast<CWwdGameObjectA*>(r);                                                \
        if (r == NULL && id != 0) {                                                                \
            return 0;                                                                              \
        }                                                                                          \
    } while (0)
#define READCSTR(field)                                                                            \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, SERIAL_NAME_LEN);                                                            \
        (field) = buf;                                                                             \
    } while (0)
#define NAMEREF(field)                                                                             \
    do {                                                                                           \
        ++g_serialCounter;                                                                         \
        ar->Read(buf, SERIAL_NAME_LEN);                                                            \
        if (strlen(buf) != 0) {                                                                    \
            CAniElement* value = NULL;                                                             \
            MapLookup(dir->m_animRegistry->m_animations, buf, value);                              \
            (field) = value;                                                                       \
        } else {                                                                                   \
            (field) = NULL;                                                                        \
        }                                                                                          \
    } while (0)

// @early-stop
RVA(0x000555e0, 0x12f8)
i32 CGrunt::LoadStateRecord(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* dir = g_gameReg->m_world;
    if (dir == NULL) {
        return 0;
    }

    i32 id;
    CGameObject* obj;
    char buf[SERIAL_NAME_LEN];

    m_struckSlotSound = NULL;
    m_struckVoiceSound = NULL;
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
    NAMEREF(AT(m_poseAttack, GRUNT_ATTACK1));
    NAMEREF(AT(m_poseAttack, GRUNT_ATTACK2));
    NAMEREF(m_poseAttackIdle);
    NAMEREF(AT(m_poseStruck, GRUNT_STRUCK1));
    NAMEREF(AT(m_poseStruck, GRUNT_STRUCK2));
    NAMEREF(AT(m_poseIdle, GRUNT_IDLE1));
    NAMEREF(AT(m_poseIdle, GRUNT_IDLE2));
    NAMEREF(AT(m_poseIdle, GRUNT_IDLE3));
    NAMEREF(AT(m_poseIdle, GRUNT_IDLE4));
    NAMEREF(AT(m_poseIdle, GRUNT_IDLE5));
    NAMEREF(m_poseDeath);
    NAMEREF(AT(m_poseToy, GRUNT_TOY1));
    NAMEREF(AT(m_poseToy, GRUNT_TOY2));
    NAMEREF(AT(m_poseToy, GRUNT_TOY_BREAK));
    NAMEREF(AT(m_poseItem, GRUNT_ITEM1));
    NAMEREF(AT(m_poseItem, GRUNT_ITEM2));
    NAMEREF(m_pickupGeoSrc);

    ar->Read(&m_reserved18c, sizeof(m_reserved18c));
    ar->Read(&m_toyBlendPct, sizeof(m_toyBlendPct));
    ar->Read(&m_brickPickupType, sizeof(m_brickPickupType));
    ar->Read(&m_entranceReason, sizeof(m_entranceReason));
    ar->Read(&m_vehiclePickupType, sizeof(m_vehiclePickupType));
    ar->Read(&m_toolId, sizeof(m_toolId));
    ar->Read(&m_entrancePickup, sizeof(m_entrancePickup));
    ar->Read(&m_helpCueId, sizeof(m_helpCueId));
    ar->Read(&m_reserved1a8, sizeof(m_reserved1a8));
    ar->Read(&m_reserved1ac, sizeof(m_reserved1ac));
    ar->Read(&m_reserved1b0, sizeof(m_reserved1b0));
    ar->Read(&m_reserved1b4, sizeof(m_reserved1b4));
    ar->Read(&m_arrived, sizeof(m_arrived));
    ar->Read(&m_entrancePx, sizeof(m_entrancePx));
    ar->Read(&m_lastTilePx, sizeof(m_lastTilePx));
    ar->Read(&m_commitPx, sizeof(m_commitPx));
    ar->Read(&m_reserved1dc, sizeof(m_reserved1dc));
    ar->Read(&m_entranceActive, sizeof(m_entranceActive));
    ar->Read(&m_arrivalPending, sizeof(m_arrivalPending));
    ar->Read(&m_tileOwnerHi, sizeof(m_tileOwnerHi));
    ar->Read(&m_tileOwnerLo, sizeof(m_tileOwnerLo));
    ar->Read(&m_moveIcon, sizeof(m_moveIcon));
    ar->Read(&m_savedMoveIcon, sizeof(m_savedMoveIcon));
    ar->Read(&m_entranceCommitted, sizeof(m_entranceCommitted));
    ar->Read(&m_neighborCell, sizeof(m_neighborCell));
    ar->Read(&m_attackTargetPx, sizeof(m_attackTargetPx));
    ar->Read(&m_reserved210, sizeof(m_reserved210));
    ar->Read(&m_struckPose, sizeof(m_struckPose));
    ar->Read(&m_combatActive, sizeof(m_combatActive));
    ar->Read(&m_neighborValid, sizeof(m_neighborValid));
    ar->Read(&m_poweredUp, sizeof(m_poweredUp));
    ar->Read(&m_daFlag, sizeof(m_daFlag));
    ar->Read(&m_entranceStamped, sizeof(m_entranceStamped));
    ar->Read(&m_bombRunActive, sizeof(m_bombRunActive));
    ar->Read(&m_arrivalActive, sizeof(m_arrivalActive));
    ar->Read(&m_reachRect, sizeof(m_reachRect));
    ar->Read(&m_reachExclusionRect, sizeof(m_reachExclusionRect));
    ar->Read(&m_toyRectA, sizeof(m_toyRectA));
    ar->Read(&m_toyRectB, sizeof(m_toyRectB));
    ar->Read(&m_health, sizeof(m_health));
    ar->Read(&m_stamina, sizeof(m_stamina));
    ar->Read(&m_toyTime, sizeof(m_toyTime));
    ar->Read(&m_wingzTime, sizeof(m_wingzTime));
    ar->Read(&m_moveSpeed, sizeof(m_moveSpeed));
    ar->Read(&m_reserved418, sizeof(m_reserved418));
    ar->Read(&m_reserved42c, sizeof(m_reserved42c));
    ar->Read(&m_reserved430, sizeof(m_reserved430));
    ar->Read(&m_startingItemId, sizeof(m_startingItemId));
    ar->Read(&m_recordedFrameTick, sizeof(m_recordedFrameTick));
    ar->Read(&m_arrivalState, sizeof(m_arrivalState));
    ar->Read(&m_defenderState, sizeof(m_defenderState));
    ar->Read(&m_battleState, sizeof(m_battleState));
    ar->Read(&m_defenderRadius, sizeof(m_defenderRadius));
    ar->Read(&m_defenderQueuePosition, sizeof(m_defenderQueuePosition));
    ar->Read(&m_defenderPickupType, sizeof(m_defenderPickupType));
    ar->Read(&m_dwell, sizeof(m_dwell));
    ar->Read(&m_arrivalCell, sizeof(m_arrivalCell));
    ar->Read(&m_defenderPx, sizeof(m_defenderPx));
    ar->Read(&m_toolConfigured, sizeof(m_toolConfigured));
    ar->Read(&m_neighborScanEnabled, sizeof(m_neighborScanEnabled));
    ar->Read(&m_tileMoveCommitted, sizeof(m_tileMoveCommitted));
    ar->Read(&m_reserved3dc, sizeof(m_reserved3dc));
    ar->Read(&m_moveTile, sizeof(m_moveTile));
    ar->Read(&m_arrivalPhase, sizeof(m_arrivalPhase));
    ar->Read(&m_timePerTile, sizeof(m_timePerTile));
    ar->Read(&m_movePosX, sizeof(m_movePosX));
    ar->Read(&m_movePosY, sizeof(m_movePosY));
    ar->Read(&m_reserved8d0, sizeof(m_reserved8d0));
    ar->Read(&m_coordToggle, sizeof(m_coordToggle));
    ar->Read(&m_wingzEnabled, sizeof(m_wingzEnabled));
    ar->Read(&m_freezeDelayDone, sizeof(m_freezeDelayDone));
    ar->Read(&m_freezeUnfrozen, sizeof(m_freezeUnfrozen));
    ar->Read(&m_resetApplied, sizeof(m_resetApplied));
    ar->Read(&m_arrivalFlags, sizeof(m_arrivalFlags));
    ar->Read(&m_passableMask, sizeof(m_passableMask));
    ar->Read(&m_gruntKind, sizeof(m_gruntKind));
    ar->Read(&m_entranceArmed, sizeof(m_entranceArmed));
    ar->Read(&m_deathType, sizeof(m_deathType));
    ar->Read(&m_entranceDropActive, sizeof(m_entranceDropActive));
    ar->Read(&m_hasExtent, sizeof(m_hasExtent));
    ar->Read(&m_unusedBattleCell, sizeof(m_unusedBattleCell));
    ar->Read(&m_cellRemovalNotified, sizeof(m_cellRemovalNotified));
    ar->Read(&m_pendingTrigger, sizeof(m_pendingTrigger));
    ar->Read(&m_killerSlot, sizeof(m_killerSlot));
    ar->Read(&m_tileClaimed, sizeof(m_tileClaimed));
    ar->Read(&m_deathAnimStarted, sizeof(m_deathAnimStarted));
    ar->Read(&m_pendingTriggerPx, sizeof(m_pendingTriggerPx));
    ar->Read(&m_routeMaskA, sizeof(m_routeMaskA));
    ar->Read(&m_routeMaskC, sizeof(m_routeMaskC));
    ar->Read(&m_moveVariantOverride, sizeof(m_moveVariantOverride));
    ar->Read(&m_moveKind, sizeof(m_moveKind));
    ar->Read(&m_moveVariant, sizeof(m_moveVariant));
    ar->Read(&m_coordRetryCount, sizeof(m_coordRetryCount));
    ar->Read(&m_toyTileIndex, sizeof(m_toyTileIndex));
    ar->Read(&m_blockedVoicePending, sizeof(m_blockedVoicePending));
    ar->Read(&m_powerupDuration, sizeof(m_powerupDuration));
    ar->Read(&m_warpstoneAnchorIndex, sizeof(m_warpstoneAnchorIndex));
    ar->Read(&m_lowStaminaCued, sizeof(m_lowStaminaCued));
    ar->Read(&m_targetTeam, sizeof(m_targetTeam));
    ar->Read(&m_arrivalTargetPx, sizeof(m_arrivalTargetPx));

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
        if (pos != NULL) {
            CoordPoolNode* fl = g_coordPool.m_freeHead;
            do {
                Coord* buf = static_cast<Coord*>(m_coordList.GetNext(pos));
                if (buf != NULL) {
                    CoordPoolNode* n2 = g_coordPool.NodeOf(buf);
                    n2->m_next = fl;
                    fl = n2;
                    g_coordPool.m_freeHead = n2;
                }
            } while (pos != NULL);
        }
        (&m_coordList)->RemoveAll();
    }

    i32 count;
    ar->Read(&count, sizeof(count));
    for (i32 a = 0; a < count; ++a) {
        CoordPoolNode* slot = g_coordPool.m_freeHead;
        CoordPoolNode* nf = slot->m_next;
        Coord* item = NULL;
        if (nf != NULL) {
            item = &slot->m_coord;
            g_coordPool.m_freeHead = nf;
        }
        ar->Read(item, 8);
        (&m_coordList)->AddTail(item);
    }

    // Retail's drain condition: a count-guarded head term tested for NULL, then
    // the count re-test (0x567c0..0x567dd) - the fused && form emits 2 blocks, not 6.
    while ((m_payloads.GetCount() != 0 ? m_payloads.GetHead() : NULL) != NULL
           && m_payloads.GetCount() != 0) {
        i32* rem = static_cast<i32*>((&m_payloads)->RemoveHead());
        delete[] rem;
    }

    ar->Read(&count, sizeof(count));
    for (i32 b = 0; b < count; ++b) {
        i32* mem = new i32[0xb];
        i32* item = 0;
        if (mem != NULL) {
            memset(mem, 0, 0xb * 4);
            item = mem;
        }
        ar->Read(item, 0x2c);
        (&m_payloads)->AddTail(item);
    }

    i32 flag = (m_entranceReason >= PICKUP_TOYZ_FIRST);
    CShadeTable* r = g_gameReg->m_spriteFactory->GetSel(IDX(m_moveIcon), flag);
    CWwdGameObjectA* cb = m_object;
    SET_DRAW_FILL(cb, SHADE_PAL_16, r);

    if (m_gruntKind == GRUNT_GHOST) {
        CWwdGameObjectA* cb2 = m_object;
        i32 v = g_buteMgr.GetIntDef("Powerupz", "GruntGhostTransparencyOn", 0xe0);
        SET_DRAW_FILL_FRACTION(cb2, SHADE_PAL_ALPHA_16, v);
    }
    return 1;
}
