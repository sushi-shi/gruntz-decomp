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
    Coord out;
    CGameObject* object = unit->m_object;
    out.m_x = object->m_screenX >> TILE_SHIFT_PX;
    out.m_y = object->m_screenY >> TILE_SHIFT_PX;
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
                i32 candX = cand->m_tileX;
                i32 candY = cand->m_tileY;
                CGameObject* object = unit->m_object;
                i32 screenX = object->m_screenX;
                i32 screenY = object->m_screenY;
                Coord current = ScreenTile(unit);
                if (candX != current.m_x || candY != current.m_y) {
                    i32 dx = candX - (screenX >> TILE_SHIFT_PX);
                    dx = abs(dx);
                    i32 dy = candY - (screenY >> TILE_SHIFT_PX);
                    dy = abs(dy);
                    i32 dist = SquaredDistance(dx, dy);
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00035210, 0x4f)
i32 CBattlezMapConfig::ProbeUnoccupiedAt(i32 x, i32 y) {
    CPtrList& lst = m_ctx->m_triggerMgr->m_baseList;
    POSITION pos = lst.GetHeadPosition();
    while (pos != NULL) {
        CGruntPuddle* cand = static_cast<CGruntPuddle*>(lst.GetNext(pos));
        if (cand != NULL && cand->m_tileX == x && cand->m_tileY == y && cand->m_pending == false) {
            return 1;
        }
    }
    return 0;
}
