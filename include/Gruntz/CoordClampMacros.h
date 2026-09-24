#ifndef GRUNTZ_COORDCLAMPMACROS_H
#define GRUNTZ_COORDCLAMPMACROS_H

#define CLAMP_PIXEL_TO_PLANE(pixelX, pixelY, plane)                                                \
    do {                                                                                           \
        if ((pixelX) < 0) {                                                                        \
            (pixelX) = 0;                                                                          \
        } else if ((pixelX) >= (plane)->m_planePixelSize.cx) {                                     \
            (pixelX) = (plane)->m_planePixelSize.cx - 1;                                           \
        }                                                                                          \
        if ((pixelY) < 0) {                                                                        \
            (pixelY) = 0;                                                                          \
        } else if ((pixelY) >= (plane)->m_planePixelSize.cy) {                                     \
            (pixelY) = (plane)->m_planePixelSize.cy - 1;                                           \
        }                                                                                          \
    } while (0)

#define CLAMP_TILE_TO_PLANE(tileX, tileY, plane)                                                   \
    do {                                                                                           \
        if ((tileX) < 0) {                                                                         \
            (tileX) = 0;                                                                           \
        } else if ((tileX) >= (plane)->m_tileGridSize.cx) {                                        \
            (tileX) = (plane)->m_tileGridSize.cx - 1;                                              \
        }                                                                                          \
        if ((tileY) < 0) {                                                                         \
            (tileY) = 0;                                                                           \
        } else if ((tileY) >= (plane)->m_tileGridSize.cy) {                                        \
            (tileY) = (plane)->m_tileGridSize.cy - 1;                                              \
        }                                                                                          \
    } while (0)

#endif // GRUNTZ_COORDCLAMPMACROS_H
