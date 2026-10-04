#ifndef GRUNTZ_GRUNTMOVEMENTINLINE_H
#define GRUNTZ_GRUNTMOVEMENTINLINE_H

#include <Gruntz/CoordPool.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntAiState.h>
#include <Gruntz/GruntCoordInline.h>
#include <Gruntz/TriggerMgr.h>
#include <Wwd/WwdAniDrawValue.h>

inline i32 IsGruntAtSavedScreenPos(CGrunt* grunt) {
    return grunt->m_object->m_screenX == grunt->m_lastTilePx.m_x
           && grunt->m_object->m_screenY == grunt->m_lastTilePx.m_y;
}

inline void ClearMoveTileFx(CGrunt* grunt) {
    grunt->m_triggerMgr->LoadTileArrivalFx(
        grunt->GetPlayerIndex(),
        grunt->GetUnitIndex(),
        grunt->m_moveTile.m_x,
        grunt->m_moveTile.m_y,
        grunt->m_activePickupType,
        WWDDRAW_NO_ANIMATION
    );
}

inline void UnregisterFromBoard(CGrunt* grunt, i32 exitedLevel) {
    if (grunt->m_cellRemovalNotified == false) {
        grunt->m_triggerMgr
            ->UnregisterUnit(grunt->GetPlayerIndex(), grunt->GetUnitIndex(), exitedLevel);
    }
}

inline void CGrunt::SetNeighbor(i32 playerIndex, i32 unitIndex) {
    m_neighborPlayerIndex = playerIndex;
    m_neighborUnitIndex = unitIndex;
}

inline void ResetToSeek(CGrunt* grunt) {
    UNSET_COORD(grunt->m_arrivalCell);
    grunt->SetDefenderState(AISTATE_SEEK);
}

inline void RepathToward(CGrunt* grunt, CGrunt* target) {
    if (static_cast<u32>(grunt->GetDwell()) > DWELL_REPATH_MS) {
        grunt->StepArrivalDrop(
            target->m_lastTilePx.m_x,
            target->m_lastTilePx.m_y,
            0,
            grunt->m_arrivalFlags,
            1,
            0
        );
        grunt->ResetDwell();
    }
}

#define MIRROR_GRUNT_ACROSS_ARRIVAL()                                                              \
    do {                                                                                           \
        i32 gx = ScanCell().m_x - m_arrivalCell.m_x + ScanCell().m_x;                              \
        i32 gy = ScanCell().m_y - m_arrivalCell.m_y + ScanCell().m_y;                              \
        TileSwitch(gx, gy, 0, m_arrivalFlags, 1, 0);                                               \
    } while (0)

inline void ScreenTile(Coord* pos) {
    pos->m_x >>= TILE_SHIFT_PX;
    pos->m_y >>= TILE_SHIFT_PX;
}

inline Coord ScreenTile(Coord pos) {
    ScreenTile(&pos);
    return pos;
}

inline Coord ScreenTile(CGrunt* unit) {
    Coord out;
    CGameObject* object = unit->m_object;
    out.Set(object->m_screenX, object->m_screenY);
    ScreenTile(&out);
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
    grunt->m_triggerMgr->RemoveCellRecord(grunt->GetPlayerIndex(), grunt->GetUnitIndex(), 1);
}

#endif // GRUNTZ_GRUNTMOVEMENTINLINE_H
