#ifndef IMAGE_IMAGECLIPMACROS_H
#define IMAGE_IMAGECLIPMACROS_H

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <Globals.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ResolveNode.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/WwdGameObjectFlags.h>

inline b32 ClipImageRect(
    RECT* rect,
    CSize* size,
    const CResolveNode* info,
    const CDDrawSurfacePair* dst,
    const CDDrawSurfaceMgr* owner
) {
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {
        const RECT* viewport = &owner->m_level->m_viewportRect;
        rect->left = Max(rect->left, viewport->left);
        rect->right = Min(rect->right, viewport->right);
        rect->top = Max(rect->top, viewport->top);
        rect->bottom = Min(rect->bottom, viewport->bottom);
    } else if (info->m_clip.left == COORD_UNSET) {
        rect->left = Max(rect->left, 0L);
        rect->right = Min(rect->right, static_cast<LONG>(dst->m_width - 1));
        rect->top = Max(rect->top, 0L);
        rect->bottom = Min(rect->bottom, static_cast<LONG>(dst->m_height - 1));
    } else {
        rect->left = Max(rect->left, info->m_clip.left);
        rect->right = Min(rect->right, info->m_clip.right);
        rect->top = Max(rect->top, info->m_clip.top);
        rect->bottom = Min(rect->bottom, info->m_clip.bottom);
    }

    *size = CSize(rect->right - rect->left + 1, rect->bottom - rect->top + 1);
    return size->cx > 0 && size->cy > 0;
}

inline Coord ResolveImagePosition(
    const CResolveNode* info,
    const POINT& origin,
    const POINT& anchor,
    b32 mirrorX,
    b32 mirrorY
) {
    Coord offset = Coord(origin.x, origin.y) + info->m_plotOffset;
    if (mirrorX != false) {
        offset.m_x = -offset.m_x;
    }
    if (mirrorY != false) {
        offset.m_y = -offset.m_y;
    }
    return info->m_screenPosition + offset - Coord(anchor.x, anchor.y);
}
#define DECLARE_IMAGE_DEST_EXTENTS(info, x, y, right, bottom)                                      \
    if (info->m_flags & IDX(WWD_GAME_OBJECT_FLAG_WORLD_SPACE)) {                                   \
        info->m_level->m_mainPlane->WorldToViewport(&x, &y);                                       \
    }                                                                                              \
    i32 right = m_width + x - 1;                                                                   \
    i32 bottom = m_height + y - 1

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

#define IMAGE_POSITION_COMPONENT(origin, anchor, plot, screen)                                     \
    ((origin) - (anchor) + (plot) + (screen))
#define IMAGE_MIRROR_COMPONENT(screen, origin, plot, anchor)                                       \
    ((screen) - (origin) - (plot) - (anchor))

#endif // IMAGE_IMAGECLIPMACROS_H
