// WwdFile.cpp - the far-flung WwdFile / CPlaneRender strays (5 leaves, strictly
// RVA-ascending; each sits OUTSIDE the level-load region and awaits its own
// birth-position attribution):
//   CPlaneRender::WrapCoord        0x0000a000
//   CPlaneRender::SnapToTileCenter 0x000311e0
//   WwdFile::ValidateMainBlock     0x0003b470
//   WwdFile::GetMapBaseName        0x0003bb50
//   CPlaneRender::GetTileHandle    0x000d53a0
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
#include <Gruntz/GameRegistry.h>
#include <rva.h>

#include <Mfc.h>    // CString (ValidateMainBlock/GetMapBaseName take one by value)
#include <stdlib.h> // atoi
#include <string.h> // inline strcpy/strlen (rep movs / repne scas)
#include <Globals.h>

// The shared map-name scratch buffer GetMapBaseName strcpy's the path into
// (0x62c010), plus its 4-byte predecessor slot (0x62c00c) the extension-truncation
// store indexes through. Reloc-masked DATA pins.

// ---------------------------------------------------------------------------
// The game registry global (?g_gameReg@@3PAUCGameReg@@A @ VA 0x64556c). Only the
// chain ValidateMainBlock walks is modeled here (m_slot -> m_wwdPath, a filename);
// the full CGameReg layout lives in src/Gruntz/StatusBarMgr.cpp. Offsets are the
// only load-bearing thing (campaign doctrine), so a TU-local view is matching-neutral.
// authentic: reduced local view of the cross-TU CGameReg world slot; only the +0x24
// path field ValidateMainBlock reads is modeled (offset-faithful, mangling-neutral).
struct WwdGameRegSlot {
    char pad_0[0x24];
    char* m_wwdPath; // +0x24  a WWD path / numeric-tail string CheckHeader validates
};
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

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

// ---------------------------------------------------------------------------
// WwdFile::ValidateMainBlock (static, __cdecl: ignores `this`, caller-cleaned
// `ret`; Ghidra mis-derived the void/no-arg `QAEXXZ` prototype).
// Takes a CString BY VALUE (the callee runs its dtor on every exit). Returns -1
// for the three reject paths, else the integer parsed from the first digit run
// of the validated header:
//   1. the CString must be non-empty (its length, at pszData-8, != 0);
//   2. ((WwdGameRegSlot*)g_gameReg->m_world)->m_wwdPath (a filename) must be non-null;
//   3. CheckHeader(that filename) into a 0x100 stack buffer must succeed.
// Then skip leading non-digits and atoi() the first digit run. The CString is
// unused beyond its non-empty check; `this` is never touched -> static.
RVA(0x0003b470, 0x13a)
i32 WwdFile::ValidateMainBlock(CString name) {
    char header[0x100];

    if (name.GetLength() == 0) {
        return -1;
    }
    if (((WwdGameRegSlot*)g_gameReg->m_world)->m_wwdPath == 0) {
        return -1;
    }

    if (WwdFile_CheckHeader(((WwdGameRegSlot*)g_gameReg->m_world)->m_wwdPath, header) == 0) {
        return -1;
    }

    char* p = header;
    char c = *p;
    while (c != 0 && (c < '0' || c > '9')) {
        c = *++p;
    }
    return atoi(p);
}

// ---------------------------------------------------------------------------
// WwdFile::GetMapBaseName (static __cdecl, returns CString by value)
// Copy the path into the shared 0x62c010 scratch buffer, drop the 4-char
// extension (write a NUL at len-4 via the preceding 0x62c00c slot), then return
// the filename portion after the last backslash. Empty/short (<= 4 char) paths
// come back unchanged. The arg CString is taken by value (callee destroys it),
// and a working-copy CString temp carries the result, so cl emits the /GX frame.
// @early-stop
// /GX CString-temp wall: the inline strcpy/strlen, the extension truncation, the
// last-'\\' scan, the by-value arg + result CString teardown and the return-copy
// are byte-faithful; residue is the EH scope-table cookie + the descending
// trylevel numbering across the two CString temps (not source-steerable).
RVA(0x0003bb50, 0x128)
CString WwdFile::GetMapBaseName(CString path) {
    CString result = path;
    i32 len = path.GetLength();
    if (len == 0) {
        return result;
    }
    if (len <= 4) {
        return result;
    }
    strcpy(g_mapNameBuf, path);
    i32 blen = strlen(g_mapNameBuf);
    if (blen >= 5) {
        g_mapNamePre[blen] = 0; // g_mapNameBuf[blen - 4] = 0 (drop the ".ext")
        i32 blen2 = strlen(g_mapNameBuf);
        if (blen2 >= 1) {
            i32 i = blen2 - 1;
            if (i >= 0) {
                while (g_mapNameBuf[i] != '\\') {
                    i--;
                    if (i < 0) {
                        break;
                    }
                }
            }
            result = &g_mapNameBuf[i + 1];
        }
    }
    return result;
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
// --- WwdFile.cpp local views ---
SIZE_UNKNOWN(WwdGameRegSlot);
