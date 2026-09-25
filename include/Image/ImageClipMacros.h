#ifndef IMAGE_IMAGECLIPMACROS_H
#define IMAGE_IMAGECLIPMACROS_H

#include <Wwd/WwdGameObjectFlags.h>

#define DECLARE_CLIPPED_IMAGE_RECT(rectType, rect, info, dst, x, y, right, bottom, width, height)  \
    rectType rect;                                                                                 \
    rect.left = x;                                                                                 \
    rect.top = y;                                                                                  \
    rect.right = right;                                                                            \
    rect.bottom = bottom;                                                                          \
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {                                   \
        BlitRect clipA = m_ownerCtx->m_level->m_viewportRect;                                      \
        RECT clip;                                                                                 \
        CopyRect(&clip, static_cast<const RECT*>(&clipA));                                         \
        if (x < clip.left) {                                                                       \
            rect.left += clip.left - x;                                                            \
        }                                                                                          \
        if (right > clip.right) {                                                                  \
            rect.right += clip.right - right;                                                      \
        }                                                                                          \
        if (y < clip.top) {                                                                        \
            rect.top += clip.top - y;                                                              \
        }                                                                                          \
        if (bottom > clip.bottom) {                                                                \
            rect.bottom += clip.bottom - bottom;                                                   \
        }                                                                                          \
    } else if (info->m_clip.left == COORD_UNSET) {                                                 \
        if (x < 0) {                                                                               \
            rect.left = 0;                                                                         \
        }                                                                                          \
        if (right >= dst->m_width) {                                                               \
            rect.right = dst->m_width - 1;                                                         \
        }                                                                                          \
        if (y < 0) {                                                                               \
            rect.top = 0;                                                                          \
        }                                                                                          \
        if (bottom >= dst->m_height) {                                                             \
            rect.bottom = dst->m_height - 1;                                                       \
        }                                                                                          \
    } else {                                                                                       \
        if (x < info->m_clip.left) {                                                               \
            rect.left = info->m_clip.left;                                                         \
        }                                                                                          \
        if (right > info->m_clip.right) {                                                          \
            rect.right = info->m_clip.right;                                                       \
        }                                                                                          \
        if (y < info->m_clip.top) {                                                                \
            rect.top = info->m_clip.top;                                                           \
        }                                                                                          \
        if (bottom > info->m_clip.bottom) {                                                        \
            rect.bottom = info->m_clip.bottom;                                                     \
        }                                                                                          \
    }                                                                                              \
    i32 width = rect.right - rect.left + 1;                                                        \
    i32 height = rect.bottom - rect.top + 1;                                                       \
    if (width <= 0 || height <= 0) {                                                               \
        info->m_dirty.m_armed = -1;                                                                \
        return;                                                                                    \
    }

#endif // IMAGE_IMAGECLIPMACROS_H
