#ifndef IMAGE_IMAGECLIPMACROS_H
#define IMAGE_IMAGECLIPMACROS_H

#include <Mfc.h>

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

#endif // IMAGE_IMAGECLIPMACROS_H
