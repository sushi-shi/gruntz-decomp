#include <Gruntz/EnemyAiType.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/Play.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Io/FileMem.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/GameLevel.h>
#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/MapMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Wap32/zBitVec.h>
#include <Gruntz/ActReg.h>
#include <Bute/ButeMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h>

#include <stdlib.h>
#include <math.h>
#pragma intrinsic(sqrt)
#include <string.h>
#include <new>
#include <MfcWin.h>
#include <Gruntz/TileTriggerContainer.h>

#include <Gruntz/FreeNodePool.h>
#include <Wap32/TileGeometry.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/StaminaPct.h>
#include <limits.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/GruntDirStatics.h>

static inline Coord** CoordArrayData(CPtrArray& a) {
    union {
        void** m_untyped;
        Coord** m_typed;
    } band;
    band.m_untyped = a.GetData();
    return band.m_typed;
}

// @early-stop
RVA(0x000358a0, 0x2d6)
i32 CBattlezMapConfig::RetargetIdleUnit(CGrunt* unit) {
    GruntzPlayer* recA = 0;
    CBattlezMapConfig* cfgB = 0;
    i32 cell = unit->m_arrivalCell.m_x;
    if (cell >= 0 && cell < 4) {
        recA = &m_ctx->m_options[cell];
        cfgB = &m_ctx->m_options[cell].m_battlezConfig;
    }
    if (unit->CoordCount() == 0) {
        if (cell == -1) {
            if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_moveBudget)) {
                return 1;
            }
            i32 r = rand() % 4;
            if (r == m_ownerId) {
                r++;
            }
            i32 band = r % 4;
            CBattlezMapConfig* b = &m_ctx->m_options[band].m_battlezConfig;
            i32 cnt = b->m_attackWaypoints.GetSize();
            i32 x = b->m_marker.m_x;
            i32 y = b->m_marker.m_y;
            if (cnt != 0) {
                Coord** arr = CoordArrayData(b->m_attackWaypoints);
                Coord* pair = arr[rand() % cnt];
                x = pair->m_x;
                y = pair->m_y;
            }
            if (unit->TileSwitch(x, y, 0, 0x9cf, 0, 0x4020) != 0) {
                unit->m_arrivalCell.m_x = band;
                unit->m_arrivalCell.m_y = 0;
                AcceptAlways(unit);
            }
            unit->m_dwell = 0;
            return 1;
        }
        CBattlezMapConfig* recB = &m_ctx->m_options[cell].m_battlezConfig;
        if (recB == NULL) {
            return 1;
        }
        if (static_cast<u32>(unit->m_dwell) <= 0x7d0) {
            return 1;
        }

        i32 y = recB->m_marker.m_y;
        i32 x = recB->m_marker.m_x;
        unit->TileSwitch(x, y, 0, 0x987, 0, 0x4068);
        unit->m_dwell = 0;
        return 1;
    }
    if (recA == NULL || cfgB == NULL) {
        unit->m_arrivalCell.m_x = -1;
        unit->m_arrivalCell.m_y = -1;
        return 1;
    }
    if (recA->m_humanControlled == 0 && cfgB->m_active == 0) {
        CoordNode* n = unit->CoordHead();
        while (n != NULL) {
            CoordNode* cur = n;
            n = n->m_next;
            if (cur->m_coord != NULL) {
                g_coordPool.Push(cur->m_coord);
            }
        }
        unit->m_coordList.RemoveAll();
        unit->m_arrivalCell.m_x = -1;
        unit->m_arrivalCell.m_y = -1;
        return 1;
    }
    i32 saved = unit->m_arrivalCell.m_x;
    static_cast<void>(saved);
    if (unit->m_arrivalCell.m_y == 1) {
        return 1;
    }
    CGameObject* lvl = unit->m_object;
    i32 px = lvl->m_screenX >> TILE_SHIFT_PX;
    i32 py = lvl->m_screenY >> TILE_SHIFT_PX;
    i32 nearBand = 0;

    i32 cnt2 = cfgB->m_attackWaypoints.GetSize();
    if (cnt2 > 0) {
        Coord** vec = CoordArrayData(cfgB->m_attackWaypoints);
        for (i32 j = cnt2; j > 0; j--) {
            Coord* pair = *vec;
            i32 dy = abs(pair->m_y - py);
            i32 dx = abs(pair->m_x - px);
            if (dx + dy <= 6) {
                nearBand = 1;
            }
            vec++;
        }
    }
    if (nearBand == 0) {
        return 1;
    }
    unit->m_arrivalCell.m_x = unit->m_arrivalCell.m_x;
    unit->m_arrivalCell.m_y = 1;
    if (unit->CoordCount() == 0) {
        return 1;
    }
    CoordNode* n = unit->CoordHead();
    while (n != NULL) {
        CoordNode* cur = n;
        n = n->m_next;
        if (cur->m_coord != NULL) {
            CoordPoolNode* slot = g_coordPool.NodeOf(cur->m_coord);
            slot->m_next = g_coordPool.m_freeHead;
            g_coordPool.m_freeHead = slot;
        }
    }
    unit->m_coordList.RemoveAll();
    return 1;
}
