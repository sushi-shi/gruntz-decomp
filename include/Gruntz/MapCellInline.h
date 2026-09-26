#ifndef GRUNTZ_GRUNTZ_MAPCELLINLINE_H
#define GRUNTZ_GRUNTZ_MAPCELLINLINE_H

#include <DDrawMgr/DDrawWorkerHost.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntzMapMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Wap32/TileGeometry.h>
#include <Wwd/WwdGameObjectFamily.h>

static inline void ClearTileBit(CGruntzMgr* reg, CGameObject* owner) {
    SetCellObject(
        reg->m_tileGrid,
        owner->m_screenX >> TILE_SHIFT_PX,
        owner->m_screenY >> TILE_SHIFT_PX,
        0
    );
}

#define SET_MAIN_PLANE_TILE(reg, tileX, tileY, tile)                                               \
    {                                                                                              \
        CDDrawWorkerHost* plane = (reg)->m_world->m_level->m_mainPlane;                            \
        SET_WORKER_HOST_CELL(plane, tileX, tileY, tile);                                           \
        (reg)->m_tileGrid->ComputeCellFlags(tileX, tileY, tile);                                   \
    }

static inline BrickzCellNode* PopFreeCellNode(BrickzCellNode*& freeList) {
    BrickzCellNode* node = freeList;
    BrickzCellNode* next = node->m_cellNext;
    if (next == NULL) {
        return NULL;
    }
    freeList = next;
    next->m_cellPrev = NULL;
    return node;
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
    m_rows[tileY][tileX].m_flags &= BRICKZ_CELL_UNOCCUPIED_MASK;
    m_rows[tileY][tileX].m_occupantId = -1;
}

inline void
CGruntzMapMgr::AcquireCellOccupancy(i32 tileX, i32 tileY, i32 playerIndex, i32 unitIndex) {
    m_rows[tileY][tileX].m_flags |= BRICKZ_CELL_OCCUPIED;
    m_rows[tileY][tileX].m_occupantId = (playerIndex << GRUNT_IDENTITY_PLAYER_SHIFT) | unitIndex;
}

static inline void TBombGridClear(CGameObject* obj) {
    CMapMgr* g = g_gameReg->m_tileGrid;
    i32 cy = obj->m_screenY >> TILE_SHIFT_PX;
    i32 cx = obj->m_screenX >> TILE_SHIFT_PX;
    if (static_cast<u32>(cx) < static_cast<u32>(g->m_width)
        && static_cast<u32>(cy) < static_cast<u32>(g->m_height)) {
        g->m_rowInts[cy][cx * 7] &= ~0x1000000;
    }
}

#endif // GRUNTZ_GRUNTZ_MAPCELLINLINE_H
