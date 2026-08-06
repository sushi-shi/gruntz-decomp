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
RVA(0x000350d0, 0xfa)
i32 CBattlezMapConfig::RepathToFreeCell(CGrunt* unit) {
    if (static_cast<u32>(unit->m_dwell) > static_cast<u32>(m_repathBudget)) {
        CGruntPuddle* best = 0;
        i32 bestDist = INT_MAX;
        POSITION pos = m_triggerMgr->m_baseList.GetHeadPosition();
        while (pos != NULL) {
            CGruntPuddle* cand = static_cast<CGruntPuddle*>(m_triggerMgr->m_baseList.GetNext(pos));
            if (cand->m_pending == 0) {
                CGameObject* lvl = unit->m_object;
                i32 lx = lvl->m_screenX >> TILE_SHIFT_PX;
                i32 ly = lvl->m_screenY >> TILE_SHIFT_PX;
                if (cand->m_tileX != lx || cand->m_tileY != ly) {
                    i32 dx = cand->m_tileX - lx;
                    dx = abs(dx);
                    i32 dy = cand->m_tileY - ly;
                    dy = abs(dy);
                    i32 dist = dx * dx + dy * dy;
                    if (dist < bestDist) {
                        bestDist = dist;
                        best = cand;
                    }
                }
            }
        }
        if (best != NULL) {
            RouteUnitTo(unit, best->m_tileX, best->m_tileY, 0xd87, 0, 0);
        }
        unit->m_dwell = 0;
    }
    return 1;
}

RVA(0x00035210, 0x4f)
i32 CBattlezMapConfig::ProbeUnoccupiedAt(i32 x, i32 y) {
    CPtrList& lst = m_ctx->m_cmdGrid->m_baseList;
    POSITION pos = lst.GetHeadPosition();
    while (pos != NULL) {
        CGruntPuddle* cand = static_cast<CGruntPuddle*>(lst.GetNext(pos));
        if (cand != NULL && cand->m_tileX == x && cand->m_tileY == y && cand->m_pending == 0) {
            return 1;
        }
    }
    return 0;
}
