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
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPuddle.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GruntzPlayer.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapCellFlags.h>
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
#include <Lith/BDefs.h>
#include <Wap32/TileGeometry.h>
#include <Wap32/zBitVec.h>
#include <Wwd/WwdFile.h>

#include <limits.h>
#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>

static inline Coord ScreenTile(CGrunt* unit) {
    Coord out = unit->m_object->ScreenPos();
    ::ScreenTile(&out);
    return out;
}

RVA(0x000350d0, 0xfa)
i32 CBattlezMapConfig::RepathToFreeCell(CGrunt* unit) {
    if (static_cast<u32>(unit->m_dwell) > static_cast<u32>(m_repathBudget)) {
        POSITION pos = m_triggerMgr->m_baseList.GetHeadPosition();
        CGruntPuddle* best = NULL;
        i32 bestDist = INT_MAX;
        while (pos != NULL) {
            CGruntPuddle* cand = static_cast<CGruntPuddle*>(m_triggerMgr->m_baseList.GetAt(pos));
            m_triggerMgr->m_baseList.GetNext(pos);
            if (cand->m_pending == false) {
                Coord candidate = cand->m_tile;
                CGameObject* object = unit->m_object;
                Coord screen = object->ScreenPos();
                Coord current = ScreenTile(unit);
                if (candidate != current) {
                    ScreenTile(&screen);
                    i32 dist = candidate.DistSqr(screen);
                    if (dist < bestDist) {
                        bestDist = dist;
                        best = cand;
                    }
                }
            }
        }
        if (best != NULL) {
            RouteUnitTo(
                unit,
                best->m_tile.m_x,
                best->m_tile.m_y,
                IDX(CELL_FLAG_SOLID | CELL_FLAG_SPECIAL | CELL_FLAG_TRIGGER | CELL_FLAG_ARROW
                    | CELL_FLAG_WATER | CELL_FLAG_SPIKES | CELL_FLAG_SINK_HAZARD),
                0,
                0
            );
        }
        unit->m_dwell = 0;
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00035210, 0x4f)
i32 CBattlezMapConfig::ProbeUnoccupiedAt(i32 x, i32 y) {
    CPtrList& lst = m_ctx->m_triggerMgr->m_baseList;
    POSITION pos = lst.GetHeadPosition();
    while (pos != NULL) {
        CGruntPuddle* cand = static_cast<CGruntPuddle*>(lst.GetNext(pos));
        Coord tile(x, y);
        if (cand != NULL && cand->m_tile == tile && cand->m_pending == false) {
            return 1;
        }
    }
    return 0;
}
