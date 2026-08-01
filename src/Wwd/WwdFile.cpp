#include <Wwd/WwdFile.h>
#include <rva.h>

#include <Mfc.h>
#include <Gruntz/CoordNode.h> // Coord - SnapToTileCenter's {x,y} out pair

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::WrapCoord (__thiscall, ret 0x8). Maps a world coordinate
// (*px, *py) into the plane's local draw space: wrap each axis into its pixel
// modulus (when the plane wraps on that axis: flag bit2=X, bit3=Y), pull it back
// near the visible origin, then subtract the plane origin and add the scroll
// view offset. Pure integer arithmetic + member reads; no calls.
//
// @early-stop
RVA(0x0000a000, 0xac)
void CDDrawWorkerHost::WrapCoord(LONG* px, LONG* py) {
    if (m_flags & 0x4) { // wrap X
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

    if (m_flags & 0x8) { // wrap Y
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

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::SnapToTileCenter (__thiscall, ret 0xc). Floor each axis to its
// tile boundary (>>shift <<shift) and add half a tile (signed /2).
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

// ===========================================================================
// Class-metadata annotations for the Wwd classes (EOF-hosted: WwdFile.h is pulled
// into GameLevel.h and this is a large /O2 TU with several @early-stop bodies, so
// keep every completeness typedef after the last function to avoid a reschedule).
//
// VTBL skips (logged, none catalogable here):
//   (the former CDDrawWorkerHost / CWwdStream / CPlaneRenderPoly shells are dissolved:
//    CDDrawWorkerHost == CDDrawWorkerHost, CWwdStream == CFileMemBase, and the "poly"
//    dispatch view is the canonical class's own slot 5.)
// ===========================================================================
// --- WwdFile.h header classes ---
