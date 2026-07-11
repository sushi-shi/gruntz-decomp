// WwdFile.cpp - the far-flung CPlaneRender strays (3 leaves, strictly
// RVA-ascending; each sits OUTSIDE the level-load region and awaits its own
// birth-position attribution):
//   CPlaneRender::WrapCoord        0x0000a000
//   CPlaneRender::SnapToTileCenter 0x000311e0
//   CPlaneRender::GetTileHandle    0x000d53a0
//
// WwdFile::ValidateMainBlock (0x3b470) + GetMapBaseName (0x3bb50) re-homed to
// CustomWorldDialog.cpp (dossier #16: their birth positions are woven into the
// custom-world obj).
//
// The level-load core was re-homed by retail .text birth position (interval
// dossier 0x15ccd0, wave1-C): IsValidWwd/CheckHeader/InflateMainBlock/
// CompressMainBlock + CGameLevelPlanes::ReadPlane/ReadObjectPlane are GameLevel.cpp
// content (the wwdfile pockets inside [0x15ccd0..0x161322]); the plane/render
// bodies (SetTileSize(FromImageSet)/Draw/CenterScrollA+B/InitScrollRects/
// ValidateTiles/ResolveColorKey/Save/Load/RebuildPlanes/ReadPlaneObjects + the
// serialize dispatcher) live in the plane TU, src/Gruntz/LevelPlane.cpp
// ([0x161350..0x163a00]). The shared class declarations stay in <Wwd/WwdFile.h>.
#include <Wwd/WwdFile.h>
#include <rva.h>

#include <Mfc.h>
#include <Globals.h>

// ---------------------------------------------------------------------------
// CPlaneRender::WrapCoord (__thiscall, ret 0x8). Maps a world coordinate
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
void CPlaneRender::WrapCoord(i32* px, i32* py) {
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
    *px = *px + m_viewX;
    *py = *py + m_viewY;
}

// ---------------------------------------------------------------------------
// CPlaneRender::SnapToTileCenter (__thiscall, ret 0xc). Floor each axis to its
// tile boundary (>>shift <<shift) and add half a tile (signed /2).
// @early-stop
// ~51%, logic byte-exact (same sar/shl/cltd/sub/sar/add selection). Residual is a
// whole-function regalloc/coloring wall: retail keeps the two shift counts in
// caller-saved eax/edx (3 callee-saved pushes) and stores both results last; this
// build colors a shift count into ebx (a 4th push, ebp) and flips the axis order.
// Not source-steerable (member-load scheduling / coloring; matching-patterns.md).
RVA(0x000311e0, 0x4c)
void CPlaneRender::SnapToTileCenter(i32* out, i32 x, i32 y) {
    i32 sx = m_shiftX;
    i32 sy = m_shiftY;
    i32 rx = ((x >> sx) << sx) + m_tilePxW / 2;
    i32 ry = ((y >> sy) << sy) + m_tilePxH / 2;
    out[0] = rx;
    out[1] = ry;
}

// GetTileHandle (0x0d53a0): index the tile-handle grid by (row, col) -
// m_tileGrid[m_colOffsets[col] + row]. Out-of-line (retail emits it standalone;
// the inline member folded into its callers and never emitted).
RVA(0x000d53a0, 0x19)
i32 CPlaneRender::GetTileHandle(i32 row, i32 col) {
    return m_tileGrid[m_colOffsets[col] + row];
}

// ===========================================================================
// Class-metadata annotations for the Wwd classes (EOF-hosted: WwdFile.h is pulled
// into GameLevel.h and this is a large /O2 TU with several @early-stop bodies, so
// keep every completeness typedef after the last function to avoid a reschedule).
//
// VTBL skips (logged, none catalogable here):
//   CPlane / CWwdStream / CPlaneRenderPoly - external/abstract engine shells;
//     their virtuals are declared-not-defined so cl emits no ??_7 vtable and the
//     retail RVA is not modeled in-TU.
// ===========================================================================
// --- WwdFile.h header classes ---
SIZE(WwdHeader, 0x5f4);     // on-disk WWD header (RE'd 0x5F4 bytes)
SIZE(WwdInputStream, 0x10); // 16-byte file-stream object (full layout to +0xc)
SIZE_UNKNOWN(CPlaneGeom);   // WwdFile's plane-geom (CPlay.h's render-geom facet is CPlayPlaneGeom)
SIZE_UNKNOWN(CPlaneScroll);
SIZE_UNKNOWN(CPlaneSurfDesc);
SIZE_UNKNOWN(CPlaneSurf);
SIZE_UNKNOWN(CPlanePalArr);
SIZE_UNKNOWN(CPlanePalOwner);
SIZE_UNKNOWN(CPlanePalHost);
SIZE_UNKNOWN(CPlaneMapData);
SIZE_UNKNOWN(CWwdStream);       // abstract serialize-stream slot view
SIZE_UNKNOWN(CPlaneRenderPoly); // slot-dispatch view
SIZE_UNKNOWN(CPlaneRender);
SIZE_UNKNOWN(CGameLevelPlanes);
SIZE_UNKNOWN(WwdFile); // namespace-class (method-only)
