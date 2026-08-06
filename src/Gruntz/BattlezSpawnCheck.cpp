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

// @early-stop
RVA(0x00034c70, 0x133)
i32 CBattlezMapConfig::CheckQueuedSpawnTile(CGrunt* unit) {
    if (unit->CoordCount() != 0) {
        return 1;
    }
    i32 x = unit->m_arrivalCell.m_x;
    i32 y = unit->m_arrivalCell.m_y;
    BrickzCell* tile = &(static_cast<BrickzCell*>((m_board)->m_rows[y]))[x];
    if (tile->m_flags & 0x20) {
        if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_reserveBudget)) {
            return 1;
        }
        if (unit->TileSwitch(unit->m_arrivalCell.m_x, unit->m_arrivalCell.m_y, 0, 0xd87, 0, 0)
            != 0) {
            unit->m_dwell = 0;
            return 1;
        }
        unit->m_battleState = BZTASK_ADVANCE;

        if (unit->CoordCount() != 0) {
            CoordNode* n = unit->CoordHead();
            while (n != NULL) {
                CoordNode* cur = n;
                n = n->m_next;
                if (cur->m_coord != NULL) {
                    g_coordPool.Push(cur->m_coord);
                }
            }
            unit->m_coordList.RemoveAll();
        }
    } else {
        unit->m_battleState = BZTASK_ADVANCE;
        if (unit->CoordCount() != 0) {
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
        }
    }
    unit->m_arrivalCell.m_x = -1;
    unit->m_arrivalCell.m_y = -1;
    unit->m_defenderState = AISTATE_SEEK;
    unit->m_dwell = 0;
    return 1;
}
