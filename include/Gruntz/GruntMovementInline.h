#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/CoordPool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirectionOffset.h>
#include <Gruntz/TriggerMgr.h>

inline i32 IsGruntAtSavedScreenPos(CGrunt* grunt) {
    CWwdSpriteObject* object = grunt->m_object;
    i32 x = grunt->m_lastTilePx.m_x;
    if (object->m_screenX == x && object->m_screenY == grunt->m_lastTilePx.m_y) {
        return 1;
    }
    return 0;
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

inline i32 CommitGruntNeighbor(CGrunt* grunt, CGrunt* target) {
    return grunt->CommitNeighbor(
        target->m_playerIndex,
        target->m_unitIndex,
        target->LastTilePx().m_x,
        target->LastTilePx().m_y
    );
}

inline void CopyLastTileToDefender(CGrunt* grunt) {
    grunt->m_defenderPx = grunt->m_lastTilePx;
}

inline void ResetGruntPoweredState(CGrunt* grunt) {
    grunt->m_entranceActive = false;
    grunt->m_combatActive = false;
    grunt->m_neighborValid = false;
    grunt->m_poweredUp = false;
    grunt->ResetEntranceAnimation(1, 0, 0);
}

inline void SetGruntArrivalTarget(CGrunt* grunt, CGrunt* target) {
    grunt->SetEntrancePos(1, 1);
    grunt->m_arrivalCell.Set(target->m_playerIndex, target->m_unitIndex);
}

inline void SetMovingDeathDirection(CGrunt* grunt, const GruntDirectionCell& direction) {
    grunt->m_entranceCell = direction;
    grunt->m_lastTilePx += GruntDirectionPixelOffset(direction) / 2;
}

inline void
SetEntranceDirection(CGrunt* grunt, const GruntDirectionCell& direction, Coord* newPosition) {
    grunt->m_entranceCell = direction;
    *newPosition = grunt->m_lastTilePx - GruntDirectionPixelOffset(direction);
}

#endif // GRUNTZ_GRUNTMOVEMENTINLINE_H
