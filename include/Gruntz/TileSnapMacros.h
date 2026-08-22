#ifndef GRUNTZ_TILESNAPMACROS_H
#define GRUNTZ_TILESNAPMACROS_H

#define SNAP_OBJECT_TO_TILE_CENTER(object)                                                         \
    object->m_screenX = (object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;                        \
    object->m_screenY = (object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;

#endif // GRUNTZ_TILESNAPMACROS_H
