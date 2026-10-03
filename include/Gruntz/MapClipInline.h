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

inline i32 CMapMgr::InSearchBounds(i32 x, i32 y) const {
    x -= m_bounds.left;
    if (static_cast<u32>(x) >= static_cast<u32>(m_gridSize.cx)) {
        return 0;
    }
    y -= m_bounds.top;
    if (static_cast<u32>(y) >= static_cast<u32>(m_gridSize.cy)) {
        return 0;
    }
    return 1;
}

#endif // GRUNTZ_GRUNTZ_MAPCLIPINLINE_H
