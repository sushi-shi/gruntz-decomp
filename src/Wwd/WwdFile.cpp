#include <Wwd/WwdFile.h>
#include <rva.h>

#include <Mfc.h>

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::WrapCoord (__thiscall, ret 0x8). Maps a world coordinate
// (*px, *py) into the plane's local draw space: wrap each axis into its pixel
// modulus (when the plane wraps on that axis: flag bit2=X, bit3=Y), pull it back
// near the visible origin, then subtract the plane origin and add the scroll
// view offset. Pure integer arithmetic + member reads; no calls.
//
// @early-stop
// 91.5%, logic byte-exact. Residual: the SECOND flag test - retail emits a direct
// `test BYTE [ecx+8],8` (memory) and loads py into eax in the branch shadow; this
// build loads the flag byte into al (`mov al; test al,8`) and parks py in edx.
// A whole-function regalloc/scheduling choice (which physical reg holds py); not
// source-steerable. Documented scheduling wall (matching-patterns.md §entropy).
RVA(0x0000a000, 0xac)
void CDDrawWorkerHost::WrapCoord(i32* px, i32* py) {
    if (m_flags & 0x4) { // wrap X
        i32 x = *px;
        if (x < 0) {
            *px = m_wrapW + x;
        } else if (x >= m_wrapW) {
            *px = x - m_wrapW;
        }
        if (m_extentX >= m_wrapW && *px < m_originX && *px <= m_extentX - m_wrapW) {
            *px = m_wrapW + *px;
        }
    }

    if (m_flags & 0x8) { // wrap Y
        i32 y = *py;
        if (y < 0) {
            *py = m_wrapH + y;
        } else if (y >= m_wrapH) {
            *py = y - m_wrapH;
        }
        if (m_extentY >= m_wrapH && *py < m_originY && *py <= m_extentY - m_wrapH) {
            *py = m_wrapH + *py;
        }
    }

    *px = *px - m_originX;
    *py = *py - m_originY;
    *px = *px + m_bounds50.left;
    *py = *py + m_bounds50.top;
}

// ---------------------------------------------------------------------------
// CDDrawWorkerHost::SnapToTileCenter (__thiscall, ret 0xc). Floor each axis to its
// tile boundary (>>shift <<shift) and add half a tile (signed /2).
// @early-stop
// ~51%, logic byte-exact (same sar/shl/cltd/sub/sar/add selection). Residual is a
// whole-function regalloc/coloring wall: retail keeps the two shift counts in
// caller-saved eax/edx (3 callee-saved pushes) and stores both results last; this
// build colors a shift count into ebx (a 4th push, ebp) and flips the axis order.
// Not source-steerable (member-load scheduling / coloring; matching-patterns.md).
// @interleaver CDDrawWorkerHost::SnapToTileCenter emitted-in <boundary: unreconstructed>
// (REHOME D10 not-homeable: BOUNDARY COMDAT - retail neighbours are freenodepool
// FreeNodePool::Push @0x311b0 (before) + ddrawsubmgr CQueueDrainHost::Drain @0x31250
// (after), NOT one reconstructed host both sides. Home hint battlezmapconfig is a
// scattered god-TU (proximity only). This is one of WwdFile.cpp's 3 far-flung
// CPlaneRender strays awaiting individual birth-position attribution; leave + flag.)
RVA(0x000311e0, 0x4c)
void CDDrawWorkerHost::SnapToTileCenter(i32* out, i32 x, i32 y) {
    i32 sx = m_shiftX;
    i32 sy = m_shiftY;
    i32 rx = ((x >> sx) << sx) + m_tilePxW / 2;
    i32 ry = ((y >> sy) << sy) + m_tilePxH / 2;
    out[0] = rx;
    out[1] = ry;
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
//   (the former CPlane / CWwdStream / CPlaneRenderPoly shells are dissolved:
//    CPlane == CDDrawWorkerHost, CWwdStream == CFileMemBase, and the "poly"
//    dispatch view is the canonical class's own slot 5.)
// ===========================================================================
// --- WwdFile.h header classes ---
