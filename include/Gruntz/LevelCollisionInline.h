#ifndef GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H
#define GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ImageSets.h>

static inline CGameLevel* LevelOf(CDDrawSurfaceMgr* holder) {
    return holder->m_level;
}

static inline TileCollisionKind LookupTileType(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    CLAMP_PIXEL_TO_PLANE(x, y, g);
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->GetTileHandle(tx, ty);
    return level->CollisionAtHandle(cell, subX, subY);
}

static inline TileCollisionKind LookupTileTypeDirect(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    CLAMP_PIXEL_TO_PLANE(x, y, g);
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->m_tileHandles[g->m_tileRowOffsets[ty] + tx];
    return level->CollisionAtHandle(cell, subX, subY);
}

static __inline i32 VtblResolve(CTileImageSet* imageSet) {
    return IDX(imageSet->GetCollisionAt(0, 0));
}

static __inline TileCollisionKind PbResolveCell(CGameLevel* level, i32 x, i32 y) {
    CLAMP_TILE_TO_PLANE(x, y, level->m_mainPlane);
    CDDrawWorkerHost* plane = level->m_mainPlane;
    i32 cell = plane->m_tileHandles[plane->m_tileRowOffsets[y] + x];
    return level->CollisionAtHandle(cell, 0, 0);
}

static __inline TileCollisionKind PbResolveCellHandle(CGameLevel* level, i32 x, i32 y) {
    CLAMP_TILE_TO_PLANE(x, y, level->m_mainPlane);
    i32 cell = level->m_mainPlane->GetTileHandle(x, y);
    return level->CollisionAtHandle(cell, 0, 0);
}

#endif // GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H
