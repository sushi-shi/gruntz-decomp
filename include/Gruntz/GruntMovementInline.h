#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/CoordPool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/TriggerMgr.h>
#include <Wwd/WwdAniDrawValue.h>

inline i32 IsGruntAtSavedScreenPos(CGrunt* grunt) {
    CWwdSpriteObject* object = grunt->m_object;
    i32 x = grunt->m_lastTilePx.m_x;
    if (object->m_screenX == x && object->m_screenY == grunt->m_lastTilePx.m_y) {
        return 1;
    }
    return 0;
}

inline void ClearMoveTileFx(CGrunt* grunt) {
    grunt->m_triggerMgr->LoadTileArrivalFx(
        grunt->m_playerIndex,
        grunt->m_unitIndex,
        grunt->m_moveTile.m_x,
        grunt->m_moveTile.m_y,
        grunt->m_entranceReason,
        WWDDRAW_NO_ANIMATION
    );
}

inline void UnregisterFromBoard(CGrunt* grunt, i32 exitedLevel) {
    if (grunt->m_cellRemovalNotified == false) {
        grunt->m_triggerMgr->UnregisterUnit(grunt->m_playerIndex, grunt->m_unitIndex, exitedLevel);
    }
}

inline void SetGruntNeighbor(CGrunt* grunt, i32 playerIndex, i32 unitIndex) {
    grunt->m_neighborPlayerIndex = playerIndex;
    grunt->m_neighborUnitIndex = unitIndex;
}

inline void ResetToSeek(CGrunt* grunt) {
    UNSET_COORD(grunt->m_arrivalCell);
    grunt->m_defenderState = AISTATE_SEEK;
}

inline void RepathToward(CGrunt* grunt, CGrunt* target) {
    if (static_cast<u32>(grunt->m_dwell) > DWELL_REPATH_MS) {
        grunt->StepArrivalDrop(
            target->m_lastTilePx.m_x,
            target->m_lastTilePx.m_y,
            0,
            grunt->m_arrivalFlags,
            1,
            0
        );
        grunt->m_dwell = 0;
    }
}

inline void CGrunt::MirrorAcrossArrival() {
    Coord pa;
    GetScreenTile(&pa);
    Coord pb;
    pb.m_y = pa.m_y;
    GetScreenPos(&pb);
    i32 gx = (pb.m_x >> TILE_SHIFT_PX) - m_arrivalCell.m_x + pa.m_x;
    GetScreenTile(&pa);
    pb.m_x = pa.m_x;
    GetScreenPos(&pb);
    i32 gy = (pb.m_y >> TILE_SHIFT_PX) - m_arrivalCell.m_y + pa.m_y;
    TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);
}

inline void RecycleGruntCoords(CGrunt* grunt) {
    if (grunt->CoordCount() == 0) {
        return;
    }
    POSITION node = grunt->CoordHead();
    if (node != NULL) {
        do {
            POSITION current = node;
            grunt->m_coordList.GetNext(node);
            Coord* coord = static_cast<Coord*>(grunt->m_coordList.GetAt(current));
            if (coord != NULL) {
                g_coordPool.Push(coord);
            }
        } while (node != NULL);
    }
    grunt->m_coordList.RemoveAll();
}

inline void ScreenTile(Coord* pos) {
    pos->m_x >>= TILE_SHIFT_PX;
    pos->m_y >>= TILE_SHIFT_PX;
}

inline Coord ScreenTile(CGrunt* unit) {
    Coord out;
    CGameObject* object = unit->m_object;
    out.m_x = object->m_screenX >> TILE_SHIFT_PX;
    out.m_y = object->m_screenY >> TILE_SHIFT_PX;
    return out;
}

inline i32 CGrunt::GetScreenTileX() const {
    return m_object->m_screenX >> TILE_SHIFT_PX;
}

inline i32 CGrunt::GetScreenTileY() const {
    return m_object->m_screenY >> TILE_SHIFT_PX;
}

inline Coord CGrunt::ScanCell() {
    Coord t;
    GetScreenTile(&t);
    return t;
}

inline void BeginGruntEntranceAndReleaseCell(CGrunt* grunt) {
    grunt->m_entranceActive = true;
    grunt->m_triggerMgr->RemoveCellRecord(grunt->m_playerIndex, grunt->m_unitIndex, 1);
}

#endif // GRUNTZ_GRUNTMOVEMENTINLINE_H
