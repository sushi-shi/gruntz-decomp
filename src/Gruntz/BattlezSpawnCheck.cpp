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
#include <Gruntz/FreeNodePool.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/MapCellFlags.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordRecycleMacros.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
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
#include <Gruntz/VoiceManager.h>
#include <Io/FileMem.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

RVA(0x00034c70, 0x133)
i32 CBattlezMapConfig::CheckQueuedSpawnTile(CGrunt* unit) {
    if (unit->CoordCount() != 0) {
        return 1;
    }
    BrickzCell* tile = &(
        static_cast<BrickzCell*>((m_board)->m_rows[unit->ArrivalCell().m_y])
    )[unit->ArrivalCell().m_x];
    if (tile->m_flags & IDX(CELL_FLAG_DESTRUCTIBLE_ROCK)) {
        if (static_cast<u32>(unit->m_dwell) <= static_cast<u32>(m_reserveBudget)) {
            return 1;
        }
        if (unit->TileSwitch(
                unit->m_arrivalCell.m_x,
                unit->m_arrivalCell.m_y,
                0,
                IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER | CELL_FLAG_ARROW
                    | CELL_FLAG_WATER | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD),
                0,
                0
            )
            != 0) {
            unit->m_dwell = 0;
            return 1;
        }
        unit->m_battleState = BZTASK_ADVANCE;

        if (unit->CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS(unit)
        }
    } else {
        unit->m_battleState = BZTASK_ADVANCE;
        if (unit->CoordCount() != 0) {
            RECYCLE_GRUNT_COORDS_EXPANDED(unit)
        }
    }
    unit->m_arrivalCell.Set(-1, -1);
    unit->m_defenderState = AISTATE_SEEK;
    unit->m_dwell = 0;
    return 1;
}
