#ifndef GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H
#define GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Gruntz/CoordClampMacros.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ImageSets.h>

static inline CGameLevel* LevelOf(CDDrawSurfaceMgr* holder) {
    return holder->m_level;
}

static inline TileCollisionKind LookupTileType(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    CLAMP_PIXEL_TO_PLANE(x, y, g);
    Coord position(x, y);
    Coord tile(position.m_x >> g->m_tileShift.m_x, position.m_y >> g->m_tileShift.m_y);
    Coord tileOrigin(tile.m_x << g->m_tileShift.m_x, tile.m_y << g->m_tileShift.m_y);
    Coord sub = position - tileOrigin;
    i32 cell = g->GetTileHandle(tile.m_x, tile.m_y);
    if (cell == UNINIT_FILL || cell == -1) {
        return TILEKIND_PASSABLE;
    }

    CUniformTileImageSet* tc = static_cast<CUniformTileImageSet*>(
        level->m_imageSets.GetAt(cell & WWD_TILE_IMAGE_SET_INDEX_MASK)
    );
    return tc->GetCollisionAt(sub.m_x, sub.m_y);
}

static inline TileCollisionKind LookupTileTypeDirect(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    CLAMP_PIXEL_TO_PLANE(x, y, g);
    Coord position(x, y);
    Coord tile(position.m_x >> g->m_tileShift.m_x, position.m_y >> g->m_tileShift.m_y);
    Coord tileOrigin(tile.m_x << g->m_tileShift.m_x, tile.m_y << g->m_tileShift.m_y);
    Coord sub = position - tileOrigin;
    i32 cell = g->m_tileHandles[g->m_tileRowOffsets[tile.m_y] + tile.m_x];
    if (cell == UNINIT_FILL || cell == -1) {
        return TILEKIND_PASSABLE;
    }

    CUniformTileImageSet* tc = static_cast<CUniformTileImageSet*>(
        level->m_imageSets.GetAt(cell & WWD_TILE_IMAGE_SET_INDEX_MASK)
    );
    return tc->GetCollisionAt(sub.m_x, sub.m_y);
}

static __inline i32 VtblResolve(CTileImageSet* imageSet) {
    return IDX(imageSet->GetCollisionAt(0, 0));
}

static __inline TileCollisionKind PbResolveCell(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* plane = level->m_mainPlane;
    CLAMP_TILE_TO_PLANE(x, y, plane);
    i32 cell = plane->m_tileHandles[plane->m_tileRowOffsets[y] + x];
    if (cell == UNINIT_FILL || cell == WWD_TILE_CLEAR) {
        return TILEKIND_PASSABLE;
    }

    CTileImageSet* set =
        static_cast<CTileImageSet*>(level->m_imageSets[cell & WWD_TILE_IMAGE_SET_INDEX_MASK]);
    return set->GetCollisionAt(0, 0);
}

static __inline TileCollisionKind PbResolveCellHandle(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* plane = level->m_mainPlane;
    CLAMP_TILE_TO_PLANE(x, y, plane);
    i32 cell = plane->GetTileHandle(x, y);
    if (cell == UNINIT_FILL || cell == WWD_TILE_CLEAR) {
        return TILEKIND_PASSABLE;
    }
    CTileImageSet* set =
        static_cast<CTileImageSet*>(level->m_imageSets[cell & WWD_TILE_IMAGE_SET_INDEX_MASK]);
    return set->GetCollisionAt(0, 0);
}

#endif // GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H
