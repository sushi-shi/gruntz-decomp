#ifndef GRUNTZ_TILESNAPMACROS_H
#define GRUNTZ_TILESNAPMACROS_H

#define SNAP_OBJECT_TO_TILE_CENTER(object)                                                         \
    object->m_screenX = (object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;                        \
    object->m_screenY = (object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;

#define SNAP_OBJECT_TO_TILE_CENTER_COPY(object, snapX, snapY)                                      \
    i32 snapX = (object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;                                \
    i32 snapY = (object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;                                \
    object->m_screenX = snapX;                                                                     \
    object->m_screenY = snapY;

#define SNAP_OBJECT_TO_TILE_CENTER_DOUBLE_POS(object, snapX, snapY, posX, posY)                    \
    SNAP_OBJECT_TO_TILE_CENTER_COPY(object, snapX, snapY)                                          \
    posX = static_cast<double>(snapX);                                                             \
    posY = static_cast<double>(snapY);

#endif // GRUNTZ_TILESNAPMACROS_H
