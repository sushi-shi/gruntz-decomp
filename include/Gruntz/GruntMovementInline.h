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

inline void CGrunt::CancelToolAnimationEffects() {
    m_triggerMgr->HandleToolAnimationCue(
        GetPlayerIndex(),
        GetUnitIndex(),
        m_toolTargetTile.m_x,
        m_toolTargetTile.m_y,
        GetActivePickupType(),
        WWDDRAW_NO_ANIMATION
    );
}

inline void CGrunt::UnregisterFromBoard(i32 exitedLevel) {
    if (IsUnregisteredFromBoard() == false) {
        m_triggerMgr->UnregisterUnit(GetPlayerIndex(), GetUnitIndex(), exitedLevel);
    }
}

inline void CGrunt::SetNeighbor(i32 playerIndex, i32 unitIndex) {
    m_neighborPlayerIndex = playerIndex;
    m_neighborUnitIndex = unitIndex;
}

inline void CGrunt::ResetToSeek() {
    UNSET_COORD(m_arrivalCell);
    SetAiState(AISTATE_SEEK);
}

inline void RepathToward(CGrunt* grunt, CGrunt* target) {
    if (static_cast<u32>(grunt->GetDwell()) > DWELL_REPATH_MS) {
        grunt->MoveTo(
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
        MoveToTile(gx, gy, 0, m_arrivalFlags, 1, 0);                                               \
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

inline void CGrunt::SetBusyAndDeselect() {
    m_busy = true;
    m_triggerMgr->RemoveUnitFromSelection(GetPlayerIndex(), GetUnitIndex(), 1);
}

#endif // GRUNTZ_GRUNTMOVEMENTINLINE_H
