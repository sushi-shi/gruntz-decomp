#ifndef IMAGE_IMAGECLIPMACROS_H
#define IMAGE_IMAGECLIPMACROS_H

#include <Mfc.h>

#include <Wwd/WwdGameObjectFlags.h>
#include <MakeRect.h>

#define DECLARE_CLIPPED_IMAGE_RECT(rectType, rect, info, dst, originX, originY, farX, farY, size)  \
    rectType rect = MakeRect(originX, originY, farX, farY);                                        \
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {                                   \
        BlitRect clipA = m_ownerCtx->m_level->m_viewportRect;                                      \
        RECT clip;                                                                                 \
        CopyRect(&clip, static_cast<const RECT*>(&clipA));                                         \
        if (originX < clip.left) {                                                                 \
            rect.left += clip.left - originX;                                                      \
        }                                                                                          \
        if (farX > clip.right) {                                                                   \
            rect.right += clip.right - farX;                                                       \
        }                                                                                          \
        if (originY < clip.top) {                                                                  \
            rect.top += clip.top - originY;                                                        \
        }                                                                                          \
        if (farY > clip.bottom) {                                                                  \
            rect.bottom += clip.bottom - farY;                                                     \
        }                                                                                          \
    } else if (info->m_clip.left == COORD_UNSET) {                                                 \
        if (originX < 0) {                                                                         \
            rect.left = 0;                                                                         \
        }                                                                                          \
        if (farX >= dst->m_width) {                                                                \
            rect.right = dst->m_width - 1;                                                         \
        }                                                                                          \
        if (originY < 0) {                                                                         \
            rect.top = 0;                                                                          \
        }                                                                                          \
        if (farY >= dst->m_height) {                                                               \
            rect.bottom = dst->m_height - 1;                                                       \
        }                                                                                          \
    } else {                                                                                       \
        if (originX < info->m_clip.left) {                                                         \
            rect.left = info->m_clip.left;                                                         \
        }                                                                                          \
        if (farX > info->m_clip.right) {                                                           \
            rect.right = info->m_clip.right;                                                       \
        }                                                                                          \
        if (originY < info->m_clip.top) {                                                          \
            rect.top = info->m_clip.top;                                                           \
        }                                                                                          \
        if (farY > info->m_clip.bottom) {                                                          \
            rect.bottom = info->m_clip.bottom;                                                     \
        }                                                                                          \
    }                                                                                              \
    CSize size(rect.right - rect.left + 1, rect.bottom - rect.top + 1);                            \
    if (size.cx <= 0 || size.cy <= 0) {                                                            \
        info->m_dirty.m_armed = -1;                                                                \
        return;                                                                                    \
    }

#endif // IMAGE_IMAGECLIPMACROS_H
