#ifndef GRUNTZ_GRUNTMOVEMENTMACROS_H
#define GRUNTZ_GRUNTMOVEMENTMACROS_H

#define GRUNT_AT_SAVED_SCREEN_POS(grunt)                                                           \
    grunt->m_object->m_screenX == grunt->m_lastTilePx.m_x                                          \
        && grunt->m_object->m_screenY == grunt->m_lastTilePx.m_y

#define GRUNT_NOT_AT_SAVED_SCREEN_POS(grunt)                                                       \
    grunt->m_object->m_screenX != grunt->m_lastTilePx.m_x                                          \
        || grunt->m_object->m_screenY != grunt->m_lastTilePx.m_y

#define GRUNT_OBJECT_AT_SAVED_SCREEN_POS(object, grunt)                                            \
    object->m_screenX == grunt->m_lastTilePx.m_x && object->m_screenY == grunt->m_lastTilePx.m_y

#define GRUNT_OBJECT_NOT_AT_SAVED_SCREEN_POS(object, grunt)                                        \
    object->m_screenX != grunt->m_lastTilePx.m_x || object->m_screenY != grunt->m_lastTilePx.m_y

#define GRUNT_SCREEN_X_AT_SAVED_POS(object, grunt) object->m_screenX == grunt->m_lastTilePx.m_x
#define GRUNT_SCREEN_Y_AT_SAVED_POS(object, grunt) object->m_screenY == grunt->m_lastTilePx.m_y
#define GRUNT_SCREEN_X_NOT_AT_SAVED_POS(object, grunt) object->m_screenX != grunt->m_lastTilePx.m_x
#define GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(object, grunt) object->m_screenY != grunt->m_lastTilePx.m_y

#define GRUNT_SELF_AT_SAVED_SCREEN_POS                                                             \
    m_object->m_screenX == m_lastTilePx.m_x && m_object->m_screenY == m_lastTilePx.m_y
#define GRUNT_SELF_NOT_AT_SAVED_SCREEN_POS                                                         \
    m_object->m_screenX != m_lastTilePx.m_x || m_object->m_screenY != m_lastTilePx.m_y
#define GRUNT_OBJECT_AT_SELF_SAVED_SCREEN_POS(object)                                              \
    object->m_screenX == m_lastTilePx.m_x && object->m_screenY == m_lastTilePx.m_y
#define GRUNT_OBJECT_NOT_AT_SELF_SAVED_SCREEN_POS(object)                                          \
    object->m_screenX != m_lastTilePx.m_x || object->m_screenY != m_lastTilePx.m_y

#define GRUNT_SELF_SCREEN_X_AT_SAVED_POS m_object->m_screenX == m_lastTilePx.m_x
#define GRUNT_SELF_SCREEN_Y_AT_SAVED_POS m_object->m_screenY == m_lastTilePx.m_y
#define GRUNT_SELF_SCREEN_X_NOT_AT_SAVED_POS m_object->m_screenX != m_lastTilePx.m_x
#define GRUNT_SELF_SCREEN_Y_NOT_AT_SAVED_POS m_object->m_screenY != m_lastTilePx.m_y

#define GRUNT_X_AT_SAVED_POS(x, grunt) x == grunt->m_lastTilePx.m_x
#define GRUNT_Y_AT_SAVED_POS(y, grunt) y == grunt->m_lastTilePx.m_y

#define SET_GRUNT_ARRIVAL_TARGET(target)                                                           \
    SetEntrancePos(1, 1);                                                                          \
    m_arrivalCell.m_x = target->m_tileOwnerHi;                                                     \
    m_arrivalCell.m_y = target->m_tileOwnerLo

#define COMMIT_GRUNT_NEIGHBOR(target)                                                              \
    CommitNeighbor(                                                                                \
        target->m_tileOwnerHi,                                                                     \
        target->m_tileOwnerLo,                                                                     \
        target->m_lastTilePx.m_x,                                                                  \
        target->m_lastTilePx.m_y                                                                   \
    )

#define COMMIT_GRUNT_NEIGHBOR_COPY(target, coord)                                                  \
    Coord coord = target->m_lastTilePx;                                                            \
    CommitNeighbor(target->m_tileOwnerHi, target->m_tileOwnerLo, coord.m_x, coord.m_y)

#define DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(object, pixelX, pixelY)                                  \
    i32 pixelX = (object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;                               \
    i32 pixelY = (object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;

#define PIXEL_PAIR_NOT_AT_SAVED_SCREEN_POS(pixelX, pixelY, grunt)                                  \
    pixelX != grunt->m_lastTilePx.m_x || pixelY != grunt->m_lastTilePx.m_y

#define PIXEL_PAIR_NOT_AT_SELF_SAVED_SCREEN_POS(pixelX, pixelY)                                    \
    pixelX != m_lastTilePx.m_x || pixelY != m_lastTilePx.m_y

#define PIXEL_PAIR_NOT_AT_POSITION(pixelX, pixelY, savedX, savedY)                                 \
    pixelX != savedX || pixelY != savedY

#endif // GRUNTZ_GRUNTMOVEMENTMACROS_H
