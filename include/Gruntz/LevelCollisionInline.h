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
    if (x < 0) {
        x = 0;
    } else if (x >= g->m_planePixelWidth) {
        x = g->m_planePixelWidth - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_planePixelHeight) {
        y = g->m_planePixelHeight - 1;
    }
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->GetTileHandle(tx, ty);
    if (cell == UNINIT_FILL || cell == -1) {
        return TILEKIND_PASSABLE;
    }

    CUniformTileImageSet* tc = static_cast<CUniformTileImageSet*>(
        level->m_imageSets.GetAt(cell & WWD_TILE_IMAGE_SET_INDEX_MASK)
    );
    return tc->GetCollisionAt(subX, subY);
}

static inline TileCollisionKind LookupTileTypeDirect(CGameLevel* level, i32 x, i32 y) {
    CDDrawWorkerHost* g = level->m_mainPlane;
    if (x < 0) {
        x = 0;
    } else if (x >= g->m_planePixelWidth) {
        x = g->m_planePixelWidth - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= g->m_planePixelHeight) {
        y = g->m_planePixelHeight - 1;
    }
    i32 tx = x >> g->m_shiftX;
    i32 ty = y >> g->m_shiftY;
    i32 subX = x - (tx << g->m_shiftX);
    i32 subY = y - (ty << g->m_shiftY);
    i32 cell = g->m_tileHandles[g->m_tileRowOffsets[ty] + tx];
    if (cell == UNINIT_FILL || cell == -1) {
        return TILEKIND_PASSABLE;
    }

    CUniformTileImageSet* tc = static_cast<CUniformTileImageSet*>(
        level->m_imageSets.GetAt(cell & WWD_TILE_IMAGE_SET_INDEX_MASK)
    );
    return tc->GetCollisionAt(subX, subY);
}

static __inline i32 VtblResolve(CTileImageSet* imageSet) {
    return IDX(imageSet->GetCollisionAt(0, 0));
}

static __inline TileCollisionKind PbResolveCell(CGameLevel* level, i32 x, i32 y) {
    if (x < 0) {
        x = 0;
    } else if (x >= level->m_mainPlane->m_tileColumns) {
        x = level->m_mainPlane->m_tileColumns - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= level->m_mainPlane->m_tileRows) {
        y = level->m_mainPlane->m_tileRows - 1;
    }
    CDDrawWorkerHost* plane = level->m_mainPlane;
    i32 cell = plane->m_tileHandles[plane->m_tileRowOffsets[y] + x];
    if (cell == UNINIT_FILL || cell == s_tileClear) {
        return TILEKIND_PASSABLE;
    }

    CTileImageSet* set =
        static_cast<CTileImageSet*>(level->m_imageSets[cell & WWD_TILE_IMAGE_SET_INDEX_MASK]);
    return set->GetCollisionAt(0, 0);
}

static __inline TileCollisionKind PbResolveCellHandle(CGameLevel* level, i32 x, i32 y) {
    if (x < 0) {
        x = 0;
    } else if (x >= level->m_mainPlane->m_tileColumns) {
        x = level->m_mainPlane->m_tileColumns - 1;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= level->m_mainPlane->m_tileRows) {
        y = level->m_mainPlane->m_tileRows - 1;
    }
    i32 cell = level->m_mainPlane->GetTileHandle(x, y);
    if (cell == UNINIT_FILL || cell == s_tileClear) {
        return TILEKIND_PASSABLE;
    }
    CTileImageSet* set =
        static_cast<CTileImageSet*>(level->m_imageSets[cell & WWD_TILE_IMAGE_SET_INDEX_MASK]);
    return set->GetCollisionAt(0, 0);
}

#endif // GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H
