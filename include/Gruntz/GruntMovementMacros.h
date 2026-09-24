#ifndef GRUNTZ_GRUNTMOVEMENTMACROS_H
#define GRUNTZ_GRUNTMOVEMENTMACROS_H

#include <Gruntz/GruntMovementInline.h>

#define SETDIR(cell, nx, ny)                                                                       \
    do {                                                                                           \
        newPos.m_y = (ny);                                                                         \
        newPos.m_x = (nx);                                                                         \
        this->m_entranceCell = (cell);                                                             \
    } while (0)

#define MV_VEC(V) m_entranceCell = g_gruntDir##V

#define MV_N                                                                                       \
    MV_VEC(North);                                                                                 \
    m_lastTilePx.m_y -= 0x10

#define MV_S                                                                                       \
    MV_VEC(South);                                                                                 \
    m_lastTilePx.m_y += 0x10

#define MV_E                                                                                       \
    MV_VEC(East);                                                                                  \
    m_lastTilePx.m_x += 0x10

#define MV_W                                                                                       \
    MV_VEC(West);                                                                                  \
    m_lastTilePx.m_x -= 0x10

#define MV_NE                                                                                      \
    MV_VEC(NorthEast);                                                                             \
    m_lastTilePx.m_x += 0x10;                                                                      \
    m_lastTilePx.m_y -= 0x10

#define MV_NW                                                                                      \
    MV_VEC(NorthWest);                                                                             \
    m_lastTilePx.m_x -= 0x10;                                                                      \
    m_lastTilePx.m_y -= 0x10

#define MV_SE                                                                                      \
    MV_VEC(SouthEast);                                                                             \
    m_lastTilePx.m_x += 0x10;                                                                      \
    m_lastTilePx.m_y += 0x10

#define MV_SW                                                                                      \
    MV_VEC(SouthWest);                                                                             \
    m_lastTilePx.m_x -= 0x10;                                                                      \
    m_lastTilePx.m_y += 0x10

#define REGION_INIT()                                                                              \
    do {                                                                                           \
        RECT a;                                                                                    \
        a.left = -1;                                                                               \
        a.top = -1;                                                                                \
        a.right = 1;                                                                               \
        a.bottom = 1;                                                                              \
        m_vehicleContactRect = a;                                                                  \
        a.left = 0;                                                                                \
        a.top = 0;                                                                                 \
        a.right = 0;                                                                               \
        a.bottom = 0;                                                                              \
        m_vehicleContactExclusionRect = a;                                                         \
    } while (0)

#define GRUNT_NOT_AT_SAVED_SCREEN_POS(grunt)                                                       \
    grunt->m_object->m_screenPosition.m_x != grunt->m_lastTilePx.m_x                               \
        || grunt->m_object->m_screenPosition.m_y != grunt->m_lastTilePx.m_y

#define SET_GRUNT_ARRIVAL_TARGET(target)                                                           \
    SetEntrancePos(1, 1);                                                                          \
    m_arrivalCell.m_x = target->m_playerIndex;                                                     \
    m_arrivalCell.m_y = target->m_unitIndex

#define GRUNT_AT_SAVED_SCREEN_POS(grunt)                                                           \
    grunt->m_object->m_screenPosition.m_x == grunt->m_lastTilePx.m_x                               \
        && grunt->m_object->m_screenPosition.m_y == grunt->m_lastTilePx.m_y

#define GRUNT_OBJECT_AT_SAVED_SCREEN_POS(object, grunt)                                            \
    object->m_screenPosition.m_x == grunt->m_lastTilePx.m_x                                        \
        && object->m_screenPosition.m_y == grunt->m_lastTilePx.m_y

#define COMMIT_GRUNT_NEIGHBOR(target)                                                              \
    CommitNeighbor(                                                                                \
        target->m_playerIndex,                                                                     \
        target->m_unitIndex,                                                                       \
        target->LastTilePx().m_x,                                                                  \
        target->LastTilePx().m_y                                                                   \
    )

#define DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(object, pixelX, pixelY)                                  \
    i32 pixelX = (object->m_screenPosition.m_x & ~TILE_MASK_PX) + TILE_HALF_PX;                    \
    i32 pixelY = (object->m_screenPosition.m_y & ~TILE_MASK_PX) + TILE_HALF_PX;

#define GRUNT_SCREEN_X_NOT_AT_SAVED_POS(object, grunt)                                             \
    object->m_screenPosition.m_x != grunt->m_lastTilePx.m_x

#define GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(object, grunt)                                             \
    object->m_screenPosition.m_y != grunt->m_lastTilePx.m_y

#endif // GRUNTZ_GRUNTMOVEMENTMACROS_H
