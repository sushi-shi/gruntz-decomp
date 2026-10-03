#ifndef GRUNTZ_GRUNTZ_MAPCLIPINLINE_H
#define GRUNTZ_GRUNTZ_MAPCLIPINLINE_H

#include <Gruntz/MapMgr.h>

RVA(0x0002b340, 0xaa)
inline void CMapMgr::Clip(const RECT* src) {
    CRect bounds(0, 0, m_width, m_height);
    RECT clip;
    if (src != NULL) {
        clip = *src;
        clip.right++;
        clip.bottom++;
    } else {
        clip = CRect(0, 0, m_width, m_height);
    }
    RECT* dst = &m_bounds;
    if (!IntersectRect(dst, &clip, &bounds)) {
        *dst = clip;
    }
    m_gridSize.cx = dst->right - dst->left;
    m_gridSize.cy = dst->bottom - dst->top;
}

#endif // GRUNTZ_GRUNTZ_MAPCLIPINLINE_H
