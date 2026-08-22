#include <rva.h>

#include <MfcWin.h>

#include <Bute/ButeMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/BattlezDifficulty.h>
#include <Gruntz/BattlezMapConfig.h>
#include <Gruntz/BattlezRouteMaskPreset.h>
#include <Gruntz/BattlezTask.h>
#include <Gruntz/BrickTileId.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/EnemyAiType.h>
#include <Gruntz/FreeNodePoolInline.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntSpawnConfig.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/Play.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/StaminaPct.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileCollisionKind.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h>
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

// @early-stop
RVA(0x000358a0, 0x2d6)
i32 CBattlezMapConfig::RetargetIdleUnit(CGrunt* unit) {
    GruntzPlayer* recA = 0;
    CBattlezMapConfig* cfgB = 0;
    i32 cell = unit->m_arrivalCell.m_x;
    if (cell >= 0 && cell < 4) {
        recA = &m_ctx->m_options[cell];
        cfgB = &recA->m_battlezConfig;
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
            // retail emits this guard (test edi,edi); SP3 folds it - era residue
            CBattlezMapConfig* b = &m_ctx->m_options[band].m_battlezConfig;
            if (b != NULL) {
                i32 cnt = b->m_attackWaypoints.GetSize();
                i32 x = b->m_marker.m_x;
                i32 y = b->m_marker.m_y;
                if (cnt != 0) {
                    Coord** arr = MfcPtrArrayData<Coord>(b->m_attackWaypoints);
                    Coord* pair = arr[rand() % cnt];
                    x = pair->m_x;
                    y = pair->m_y;
                }
                if (unit->TileSwitch(x, y, 0, 0x9cf, 0, 0x4020) != 0) {
                    unit->m_arrivalCell.m_x = band;
                    unit->m_arrivalCell.m_y = 0;
                    AcceptAlways(unit);
                }
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
        Coord none;
        unit->m_arrivalCell = *none.Set(-1, -1);
        return 1;
    }
    if (recA->m_humanControlled == 0 && cfgB->m_active == 0) {
        RECYCLE_GRUNT_COORDS(unit)
        Coord none;
        unit->m_arrivalCell = *none.Set(-1, -1);
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
        Coord** vec = MfcPtrArrayData<Coord>(cfgB->m_attackWaypoints);
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
    RECYCLE_GRUNT_COORDS_INLINE_PUSH(unit)
    return 1;
}
