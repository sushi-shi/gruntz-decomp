#ifndef GRUNTZ_GRUNTZ_MAPCELLINLINE_H
#define GRUNTZ_GRUNTZ_MAPCELLINLINE_H

#include <DDrawMgr/DDrawWorkerHost.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MapCellFlags.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdGameObjectFamily.h>

static inline void ClearTileBit(CGruntzMgr* reg, CGameObject* owner) {
    reg->m_tileGrid->SetObjectIdAt(
        owner->m_screenPosition.m_x >> TILE_SHIFT_PX,
        owner->m_screenPosition.m_y >> TILE_SHIFT_PX,
        0
    );
}

#define SET_MAIN_PLANE_TILE(reg, tileX, tileY, tile)                                               \
    {                                                                                              \
        CDDrawWorkerHost* plane = (reg)->m_world->m_level->m_mainPlane;                            \
        SET_WORKER_HOST_CELL(plane, tileX, tileY, tile);                                           \
        (reg)->m_tileGrid->ComputeCellFlags(tileX, tileY, tile);                                   \
    }

inline SIZE
CGruntzMapMgr::GetGridSize() const {
    SIZE
    size;
    size.cx = m_width;
    size.cy = m_height;
    return size;
}

inline i32 CGruntzMapMgr::OccupantAt(u32 x, u32 y) const {
    if (x < m_width && y < m_height) {
        return m_rows[y][x].m_occupantId;
    }
    return -1;
}

inline i32 CGruntzMapMgr::TileIdAt(u32 x, u32 y) const {
    if (x < m_width && y < m_height) {
        return m_rows[y][x].m_tileId;
    }
    return 0;
}

inline void CGruntzMapMgr::ReleaseCellOccupancy(i32 tileX, i32 tileY) {
    CellFlagsAtUnchecked(tileX, tileY) &= BRICKZ_CELL_UNOCCUPIED_MASK;
    m_rows[tileY][tileX].m_occupantId = -1;
}

inline void
CGruntzMapMgr::AcquireCellOccupancy(i32 tileX, i32 tileY, i32 playerIndex, i32 unitIndex) {
    CellFlagsAtUnchecked(tileX, tileY) |= BRICKZ_CELL_OCCUPIED;
    m_rows[tileY][tileX].m_occupantId = (playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | unitIndex;
}

static inline void TBombGridClear(CGameObject* obj) {
    CMapMgr* g = g_gameReg->m_tileGrid;
    i32 cy = obj->m_screenPosition.m_y >> TILE_SHIFT_PX;
    i32 cx = obj->m_screenPosition.m_x >> TILE_SHIFT_PX;
    if (static_cast<u32>(cx) < static_cast<u32>(g->GetWidth())
        && static_cast<u32>(cy) < static_cast<u32>(g->GetHeight())) {
        g->m_rows[cy][cx].m_flags &= ~IDX(CELL_FLAG_TIME_BOMB);
    }
}

#endif // GRUNTZ_GRUNTZ_MAPCELLINLINE_H
