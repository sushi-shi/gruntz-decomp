#include <rva.h>

#include <Wwd/WwdFile.h>

#include <Mfc.h>

#include <Gruntz/CoordNode.h>

// @early-stop
RVA(0x0000a000, 0xac)
void CDDrawWorkerHost::WrapCoord(LONG* px, LONG* py) {
    if (m_flags & 0x4) {
        LONG x = *px;
        if (x < 0) {
            *px = m_wrapW + x;
        } else if (x >= m_wrapW) {
            *px = x - m_wrapW;
        }
        if (m_viewRect.right >= m_wrapW && *px < m_viewRect.left
            && *px <= m_viewRect.right - m_wrapW) {
            *px = m_wrapW + *px;
        }
    }

    if (m_flags & 0x8) {
        LONG y = *py;
        if (y < 0) {
            *py = m_wrapH + y;
        } else if (y >= m_wrapH) {
            *py = y - m_wrapH;
        }
        if (m_viewRect.bottom >= m_wrapH && *py < m_viewRect.top
            && *py <= m_viewRect.bottom - m_wrapH) {
            *py = m_wrapH + *py;
        }
    }

    *px = *px - m_viewRect.left;
    *py = *py - m_viewRect.top;
    *px = *px + m_bounds50.left;
    *py = *py + m_bounds50.top;
}

// @early-stop
RVA(0x000311e0, 0x4c)
void CDDrawWorkerHost::SnapToTileCenter(Coord* out, i32 x, i32 y) {
    i32 sx = m_shiftX;
    i32 sy = m_shiftY;
    i32 rx = ((x >> sx) << sx) + m_tilePxW / 2;
    i32 ry = ((y >> sy) << sy) + m_tilePxH / 2;
    out->m_x = rx;
    out->m_y = ry;
}

RVA(0x000d53a0, 0x19)
i32 CDDrawWorkerHost::GetTileHandle(i32 row, i32 col) {
    return m_tileGrid[m_colOffsets[col] + row];
}
