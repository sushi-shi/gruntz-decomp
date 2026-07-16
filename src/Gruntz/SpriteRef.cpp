// SpriteRef.cpp - the CSpriteRef bucket-element node of CSpriteRefTable (trace
// placeholder tomalla-42). Build() bakes the per-`kind` team-color triple
// into the node (three RGB565 pixels at +0x08/+0x0a/+0x0c) and caches the shade
// cache + alpha table; Free() returns the table to the cache. The class shape +
// the shared RGB-format shift globals come from <Gruntz/SpriteRefTable.h> /
// <DDrawMgr/ShadeTableCache.h>; CShadeTableCache::FindRemove is modeled NO-body so
// Free()'s `call` reloc-masks.
#include <Mfc.h> // afx-first (superset of Win32.h; the includes below pull MFC collections)
#include <DDrawMgr/PixelShift.h> // g_rUp/g_gUp/g_bUp/g_rDown/g_gDown/g_bDown

#include <Gruntz/SpriteRefTable.h>
#include <DDrawMgr/ShadeTableCache.h> // canonical CShadeTableCache (FindRemove @0x14fb80)
#include <Gruntz/GameRegistry.h>      // g_gameReg (m_saveSink) for the re-homed 0x0e35f0 dlg proc

#include <rva.h>

// The live screen RGB-format shift table at 0x683ea0..0x683eb4 - already named by
// CLightFxRender.cpp / ShadeTableCache.cpp. Reloc-masked DIR32 data refs.

// The shade cache that owns m_alphaKey; Free() hands the table back via FindRemove
// (0x14fb80). Modeled NO-body so the `call` reloc-masks.

// ---------------------------------------------------------------------------

// Bake the team-color triple for `kind` and cache the shade table. Returns 1, or
// 0 for an out-of-range kind. __thiscall, ret 0xc.
// @early-stop
// jump-table-data scoring artifact (docs/patterns/jumptable-data-overlap.md): the
// 927-byte function CODE is byte-exact (dispatch, all 17 case bodies in retail
// .text order, the RGB565 pack tail, both ret paths); the trailing 17-entry inline
// jump table scores fuzzy because cl emits local $L-label DIR32 relocs vs the
// delinked target's self-relocs. ~93.7% unit, the remainder is that data region.
RVA(0x000e2df0, 0x39f)
i32 CSpriteRef::Build(i32 cache, void* shade, i32 kind) {
    m_cache = cache;
    m_alphaKey = (i32)shade;
    u8 r1, g1, b1; // color 1 (192/255 shade) -> m_teamColor1
    u8 r2, g2, b2; // color 2 (full intensity) -> m_teamColor2
    u8 r3, g3, b3; // color 3 (128/255 shade)  -> m_teamColor3
    switch (kind) {
        case 0:
            r2 = 0xff;
            g2 = 0x80;
            b2 = 0x00;
            r1 = 0xc0;
            g1 = 0x60;
            b1 = 0x00;
            r3 = 0x80;
            g3 = 0x40;
            b3 = 0x00;
            break;
        case 1:
            r2 = 0x00;
            g2 = 0xff;
            b2 = 0x00;
            r1 = 0x00;
            g1 = 0xc0;
            b1 = 0x00;
            r3 = 0x00;
            g3 = 0x80;
            b3 = 0x00;
            break;
        case 2:
            r2 = 0x00;
            g2 = 0x00;
            b2 = 0xff;
            r1 = 0x00;
            g1 = 0x00;
            b1 = 0xc0;
            r3 = 0x00;
            g3 = 0x00;
            b3 = 0x80;
            break;
        case 3:
            r2 = 0xff;
            g2 = 0x00;
            b2 = 0x00;
            r1 = 0xc0;
            g1 = 0x00;
            b1 = 0x00;
            r3 = 0x80;
            g3 = 0x00;
            b3 = 0x00;
            break;
        case 6:
            r2 = 0xff;
            g2 = 0x00;
            b2 = 0x80;
            r1 = 0xc0;
            g1 = 0x00;
            b1 = 0x60;
            r3 = 0x80;
            g3 = 0x00;
            b3 = 0x40;
            break;
        case 5:
            r2 = 0xff;
            g2 = 0xff;
            b2 = 0x00;
            r1 = 0xc0;
            g1 = 0xc0;
            b1 = 0x00;
            r3 = 0x80;
            g3 = 0x80;
            b3 = 0x00;
            break;
        case 12:
            r2 = 0xff;
            g2 = 0x00;
            b2 = 0xff;
            r1 = 0xc0;
            g1 = 0x00;
            b1 = 0xc0;
            r3 = 0x80;
            g3 = 0x00;
            b3 = 0x80;
            break;
        case 8:
            r2 = 0x00;
            g2 = 0x00;
            b2 = 0x80;
            r1 = 0x00;
            g1 = 0x00;
            b1 = 0x60;
            r3 = 0x00;
            g3 = 0x00;
            b3 = 0x40;
            break;
        case 9:
            r2 = 0x00;
            g2 = 0x80;
            b2 = 0x00;
            r1 = 0x00;
            g1 = 0x60;
            b1 = 0x00;
            r3 = 0x00;
            g3 = 0x40;
            b3 = 0x00;
            break;
        case 10:
            r2 = 0x00;
            g2 = 0x80;
            b2 = 0x80;
            r1 = 0x00;
            g1 = 0x60;
            b1 = 0x60;
            r3 = 0x00;
            g3 = 0x40;
            b3 = 0x40;
            break;
        case 11:
            r2 = 0x80;
            g2 = 0x00;
            b2 = 0x00;
            r1 = 0x60;
            g1 = 0x00;
            b1 = 0x00;
            r3 = 0x40;
            g3 = 0x00;
            b3 = 0x00;
            break;
        case 4:
            r2 = 0x80;
            g2 = 0x00;
            b2 = 0x80;
            r1 = 0x60;
            g1 = 0x00;
            b1 = 0x60;
            r3 = 0x40;
            g3 = 0x00;
            b3 = 0x40;
            break;
        case 13:
            r2 = 0x80;
            g2 = 0x80;
            b2 = 0x00;
            r1 = 0x60;
            g1 = 0x60;
            b1 = 0x00;
            r3 = 0x40;
            g3 = 0x40;
            b3 = 0x00;
            break;
        case 14:
            r2 = 0x80;
            g2 = 0x80;
            b2 = 0x80;
            r1 = 0x60;
            g1 = 0x60;
            b1 = 0x60;
            r3 = 0x40;
            g3 = 0x40;
            b3 = 0x40;
            break;
        case 15:
            r2 = 0x00;
            g2 = 0xff;
            b2 = 0xff;
            r1 = 0x00;
            g1 = 0xc0;
            b1 = 0xc0;
            r3 = 0x00;
            g3 = 0x80;
            b3 = 0x80;
            break;
        case 16:
            r2 = 0xff;
            g2 = 0xff;
            b2 = 0xff;
            r1 = 0xc0;
            g1 = 0xc0;
            b1 = 0xc0;
            r3 = 0x80;
            g3 = 0x80;
            b3 = 0x80;
            break;
        case 7:
            r2 = 0x40;
            g2 = 0x40;
            b2 = 0x40;
            r1 = 0x20;
            g1 = 0x20;
            b1 = 0x20;
            r3 = 0x20;
            g3 = 0x20;
            b3 = 0x20;
            break;
        default:
            return 0;
    }
    m_teamColor1 = static_cast<u16>(((static_cast<u8>((static_cast<u8>(r1) >> static_cast<u8>(g_rDown))) << g_rUp)
                         | (static_cast<u8>((static_cast<u8>(g1) >> static_cast<u8>(g_gDown))) << g_gUp) | static_cast<u8>((static_cast<u8>(b1) >> static_cast<u8>(g_bDown)))));
    m_teamColor2 = static_cast<u16>(((static_cast<u8>((static_cast<u8>(g2) >> static_cast<u8>(g_gDown))) << g_gUp)
                         | (static_cast<u8>((static_cast<u8>(r2) >> static_cast<u8>(g_rDown))) << g_rUp) | static_cast<u8>((static_cast<u8>(b2) >> static_cast<u8>(g_bDown)))));
    m_teamColor3 = static_cast<u16>(((static_cast<u8>((static_cast<u8>(r3) >> static_cast<u8>(g_rDown))) << g_rUp)
                         | (static_cast<u8>((static_cast<u8>(g3) >> static_cast<u8>(g_gDown))) << g_gUp) | static_cast<u8>((static_cast<u8>(b3) >> static_cast<u8>(g_bDown)))));
    return 1;
}

// Hand the alpha table back to its cache and clear the cached pointers.
RVA(0x000e32e0, 0x25)
void CSpriteRef::Free() {
    CShadeTableCache* cache = (CShadeTableCache*)m_cache;
    if (cache && m_alphaKey) {
        cache->FindRemove((CShadeTable*)m_alphaKey);
        m_cache = 0;
        m_alphaKey = 0;
    }
}

// -------------------------------------------------------------------------
// 0x0e35f0. __stdcall dialog
// proc: WM_INITDIALOG seeds the selection index + active save-sink from the game
// registry; WM_COMMAND handles Cancel / the shared save-menu draw. The two callees
// and the selection global are the canonical savegame symbols (DrawSaveGameMenu
// @0xe3f40, FillSaveDialog @0xe3c60, g_savedMenuCmd @0x213a9c), reloc-masked.
extern "C" CGameRegistry* g_gameReg;
DATA(0x0024c86c)
i32 g_dlg64c86c = 0;       // DAT_0064c86c (the active save-sink; owner-TU definition)
extern i32 g_savedMenuCmd; // 0x213a9c (canonical, DATA-bound in savegame)
class CSaveGame;
i32 DrawSaveGameMenu(HWND hDlg, i32 wParam, CSaveGame* cur); // 0xe3f40 (savegame)
void FillSaveDialog(HWND hDlg, CSaveGame* sg);               // 0xe3c60 (savegame)
RVA(0x000e35f0, 0x77)
i32 CALLBACK winapi_0e35f0_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (DrawSaveGameMenu(hDlg, wParam, (CSaveGame*)g_dlg64c86c) != 0) {
                return 1;
            }
            // falls through to the shared "return 0" default
        default:
            return 0;
        case 0x110: {
            i32 v = (i32)g_gameReg->m_saveSink;
            g_savedMenuCmd = -1;
            g_dlg64c86c = v;
            FillSaveDialog(hDlg, (CSaveGame*)v);
            return 1;
        }
    }
}
