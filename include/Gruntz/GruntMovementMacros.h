#ifndef GRUNTZ_GRUNTMOVEMENTMACROS_H
#define GRUNTZ_GRUNTMOVEMENTMACROS_H

// CGrunt::IsAtSavedScreenPos (0x29a80) expanded at the sites where retail
// inlined it, plus the partial one-axis tests.  The call/expand split itself is
// documented at <Gruntz/GruntMovementInline.h>, which owns the twin.
// The receiver-only variants (GRUNT_SELF_*, PIXEL_PAIR_NOT_AT_SELF_*) were
// folded onto the parameterized forms 2026-08-22 by passing `this`, byte-neutral
// (0 rows moved); nine further variants had no use site at all.

#define GRUNT_AT_SAVED_SCREEN_POS(grunt)                                                           \
    grunt->m_object->m_screenX == grunt->m_lastTilePx.m_x                                          \
        && grunt->m_object->m_screenY == grunt->m_lastTilePx.m_y

#define GRUNT_NOT_AT_SAVED_SCREEN_POS(grunt)                                                       \
    grunt->m_object->m_screenX != grunt->m_lastTilePx.m_x                                          \
        || grunt->m_object->m_screenY != grunt->m_lastTilePx.m_y

#define GRUNT_OBJECT_AT_SAVED_SCREEN_POS(object, grunt)                                            \
    object->m_screenX == grunt->m_lastTilePx.m_x && object->m_screenY == grunt->m_lastTilePx.m_y

#define GRUNT_SCREEN_Y_AT_SAVED_POS(object, grunt) object->m_screenY == grunt->m_lastTilePx.m_y
#define GRUNT_SCREEN_X_NOT_AT_SAVED_POS(object, grunt) object->m_screenX != grunt->m_lastTilePx.m_x
#define GRUNT_SCREEN_Y_NOT_AT_SAVED_POS(object, grunt) object->m_screenY != grunt->m_lastTilePx.m_y

#define GRUNT_OBJECT_NOT_AT_SELF_SAVED_SCREEN_POS(object)                                          \
    object->m_screenX != m_lastTilePx.m_x || object->m_screenY != m_lastTilePx.m_y

#define GRUNT_X_AT_SAVED_POS(x, grunt) x == grunt->m_lastTilePx.m_x
#define DECLARE_SNAPPED_SCREEN_PIXEL_PAIR(object, pixelX, pixelY)                                  \
    i32 pixelX = (object->m_screenX & ~TILE_MASK_PX) + TILE_HALF_PX;                               \
    i32 pixelY = (object->m_screenY & ~TILE_MASK_PX) + TILE_HALF_PX;

#define PIXEL_PAIR_NOT_AT_POSITION(pixelX, pixelY, savedX, savedY)                                 \
    pixelX != savedX || pixelY != savedY

#endif // GRUNTZ_GRUNTMOVEMENTMACROS_H
