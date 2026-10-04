#ifndef GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H
#define GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ImageSets.h>

static inline TileCollisionKind
LookupTileCollisionAtPixel(CGameLevel* level, i32 pixelX, i32 pixelY) {
    CLevelPlane* plane = level->m_mainPlane;
    CLAMP_PIXEL_TO_PLANE(pixelX, pixelY, plane);
    i32 tileX = pixelX >> plane->m_shiftX;
    i32 tileY = pixelY >> plane->m_shiftY;
    i32 tilePixelX = pixelX - (tileX << plane->m_shiftX);
    i32 tilePixelY = pixelY - (tileY << plane->m_shiftY);
    i32 tileHandle = plane->GetTileHandle(tileX, tileY);
    return level->CollisionAtHandle(tileHandle, tilePixelX, tilePixelY);
}

static inline TileCollisionKind
LookupTileCollisionAtPixelDirect(CGameLevel* level, i32 pixelX, i32 pixelY) {
    CLevelPlane* plane = level->m_mainPlane;
    CLAMP_PIXEL_TO_PLANE(pixelX, pixelY, plane);
    i32 tileX = pixelX >> plane->m_shiftX;
    i32 tileY = pixelY >> plane->m_shiftY;
    i32 tilePixelX = pixelX - (tileX << plane->m_shiftX);
    i32 tilePixelY = pixelY - (tileY << plane->m_shiftY);
    i32 tileHandle = plane->m_tileHandles[plane->m_tileRowOffsets[tileY] + tileX];
    return level->CollisionAtHandle(tileHandle, tilePixelX, tilePixelY);
}

static __inline i32 GetTileOriginCollisionCode(CTileImageSet* imageSet) {
    return IDX(imageSet->GetCollisionAt(0, 0));
}

static __inline TileCollisionKind
LookupTileOriginCollisionDirect(CGameLevel* level, i32 tileX, i32 tileY) {
    CLAMP_TILE_TO_PLANE(tileX, tileY, level->m_mainPlane);
    CLevelPlane* plane = level->m_mainPlane;
    i32 tileHandle = plane->m_tileHandles[plane->m_tileRowOffsets[tileY] + tileX];
    return level->CollisionAtHandle(tileHandle, 0, 0);
}

static __inline TileCollisionKind
LookupTileOriginCollision(CGameLevel* level, i32 tileX, i32 tileY) {
    CLAMP_TILE_TO_PLANE(tileX, tileY, level->m_mainPlane);
    i32 tileHandle = level->m_mainPlane->GetTileHandle(tileX, tileY);
    return level->CollisionAtHandle(tileHandle, 0, 0);
}

#endif // GRUNTZ_GRUNTZ_LEVELCOLLISIONINLINE_H
