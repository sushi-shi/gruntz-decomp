// ApiWrappers.cpp - game helpers that call Win32/CRT through the IAT,
// re-homed out of src/Stub/ApiCallers.cpp (matcher-4, low-RVA half).
//
// These are real game functions (not CRT): a browser-launch helper and a grunt
// combat-region scan. Placeholder class/field names (m_<hexoffset>); only the
// offsets + code bytes are load-bearing. Compiled base+/GX to mirror the
// original stub-unit environment.
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape); MFC-first (pulls afx)
#ifdef __clang__
// The label-step clang can't parse MFC's afxwin1.inl (CMenu::operator==/!= rely on the
// implicit-int return MSVC5 accepts). We only need afxwin.h's class DECLARATIONS for
// symbol mangling here, not the inline bodies, so skip the *.inl for clang; the real
// MSVC5 base compile keeps _AFX_ENABLE_INLINES (defined in afxver_.h) + the inlines.
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>               // real MFC CRgn/CGdiObject (the credits clip region: Attach)
#include <Dsndmgr/GruntzSoundZ.h> // canonical CGruntzSoundZ (g_mgrSettings->m_48 sound bank)
#include <Win32.h>
#include <ddraw.h> // real IDirectDrawSurface (the credits offscreen-DC object: GetDC/ReleaseDC)
#include <DDrawMgr/DDSurface.h> // real CDDSurface (the credits DC chain's surface holder: m_8)
#include <Bute/SymTab.h>        // real CSymTab (the credits section source: Insert)
#include <Gruntz/ParseSource.h> // real CParseSource (the resolved CREDITZ section: BeginParse/EndParse)
#include <Gruntz/SoundCueMgr.h> // real CSoundCueMgr (the config-cue sound player: ConfigureItem @0x1360d0)

#include <Ints.h>
#include <rva.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
class CGruntzMgr {
public:
    void StoreInputFlag(i32 a);
    i32 RunModalDialog(const char* a, void* b, i32 c);
    i32 StoreInputState(i32 a);
};

// The combat-scan grunts are CGrunts; Method_243c @0x243c is CGrunt::CreateHealthSprite.
// TU-local method-decl (Grunt.h is heavy), cast at the call.
SIZE_UNKNOWN(CGrunt);
VTBL(CGrunt, 0x001e8754); // vtable_names -> code (RTTI game class)
class CGrunt {
public:
    i32 CreateHealthSprite();
};

// SetupDlg's GetHandle @0x3bcf = CSaveGame::GetSlot, Apply @0x12f3 = CSaveGame::VerifySlot.
struct SaveSlot;
class CSaveGame {
public:
    SaveSlot* GetSlot(i32 idx);
    i32 VerifySlot(SaveSlot* s);
};

namespace m4 {

    // (0x8f120 LaunchWebBrowser re-homed to src/Gruntz/GruntzMgr.cpp - a free
    // __stdcall helper RVA-contiguous with the CGruntzMgr methods (gruntzmgr .obj),
    // called by CGruntzMgr::HandleCommand @0x862f0. func_2e6e moved with it.)

    // (0x78060 CheckCombatRegion re-homed to src/Gruntz/Play.cpp as CWorld::WorldTimeline
    // ::HudRect - the combat-region scan CPlay::DispatchHudClick/PostHudRect drive on
    // m_4w()->m_68 (WorldTimeline); Play.h already stubbed 0x78060 as HudRect. The
    // GruntCombatMgr view + its 5 combat sub-views folded onto CWorld::WorldTimeline's
    // nested combat structs; g_644c54 renamed to its canonical g_curPlayer.)

    // -------------------------------------------------------------------------
    // CreditzScreen::BuildText (0x00039a60) - pull the "CREDITZ" TXT blob from the
    // config store into the scroll CString, measure it against the offscreen DC,
    // and set up the scroll rect + per-frame scroll step. thiscall, returns BOOL.

    // The resolved CREDITZ section is the real CParseSource (<Gruntz/ParseSource.h>):
    // disasm-verified callees GetData = BeginParse @0x139960, Release = EndParse
    // @0x1399d0, m_length @+0x0c (matching layout). Both return an H used as a data
    // pointer. Was a hand-rolled CreditzText view; folded to the canonical class.

    // m_text is the real MFC CString (Assign @0x1b9e74 = CString::operator=(const
    // char*), verified); was a hand-rolled CreditzStr view. Its LPCTSTR (the +0x00
    // m_pszData) is what DrawTextA reads.
    // m_clipRgn is the real MFC CRgn (Attach @0x1c6a05 = CGdiObject::Attach, the
    // clip region built from CreateRectRgn); was a hand-rolled CreditzRgn view.
    // The offscreen-DC object is the real IDirectDrawSurface (<ddraw.h>): the two
    // dispatched slots are GetDC (slot 17, +0x44: HDC*) and ReleaseDC (slot 26,
    // +0x68: HDC) - exact IDirectDrawSurface vtable offsets (the batch-1 DcSink
    // precedent). Was a hand-rolled CreditzDcVtbl PMF view naming only those two
    // slots; folded to the real SDK interface (ddraw.h supplies the full vtable, so
    // no fabricated fillers). Byte-neutral COM dispatch (`mov eax,[dc]; call [eax+N]`).
    // The section source (m_sectionSrc) is the real CSymTab (<Bute/SymTab.h>): the
    // "GetSection(name,tag)" lookup is CSymTab::Insert(key, arg) @0x13a000 (insert-or-
    // resolve), returning the section's CParseSource as an H (used as a pointer). Was
    // a hand-rolled CreditzSectionSrc view; folded to the canonical class.

    extern double g_5e96f8;
    extern double g_5e96f0;
    extern double g_5e9708;

    struct CreditzScreen {
        char m_pad0[0xc];
        char* m_dcChain; // +0xc  (chain to the offscreen-DC object)
        char m_pad10[0x2c - 0x10];
        CSymTab* m_sectionSrc; // +0x2c  the section symbol table (Insert = section lookup)
        char m_pad30[0x1c8 - 0x30];
        RECT m_scrollRect;   // +0x1c8
        RECT m_textRect;     // +0x1d8
        CRgn m_clipRgn;      // +0x1e8  (real MFC CRgn; vptr + m_hObject)
        CString m_text;      // +0x1f0  (real MFC CString; m_pszData at +0x00)
        i32 m_1f4;           // +0x1f4
        i32 m_1f8;           // +0x1f8
        i32 m_1fc;           // +0x1fc
        double m_scrollStep; // +0x200
        i32 BuildText();
    };

    // @early-stop
    // scheduling tail (~99.7%): for the final (double)(unsigned)m_1f4 conversion the
    // int64 temp's two halves (low=eax, high=0) are emitted in the opposite order
    // relative to the fld/fmul (retail stores high=0 before the fld, low after the
    // fmul; cl reverses). All other bytes identical (llvm-objdump -dr). The GetDC/
    // ReleaseDC region is now byte-exact after realizing CreditzDc as a real
    // __stdcall COM interface (was 98% with the __cdecl free-fn-ptr view).
    RVA(0x00039a60, 0x179)
    i32 CreditzScreen::BuildText() {
        // CSymTab::Insert resolves the "CREDITZ" section of FOURCC type 'TXT'
        // (== 0x545854, a tag value not an address); it returns the section's
        // CParseSource as an H, used here as a pointer.
        CParseSource* sect = (CParseSource*)m_sectionSrc->Insert("CREDITZ", (void*)'TXT');
        if (sect) {
            char* src = (char*)sect->BeginParse();
            if (!src) {
                return 0;
            }
            i32 len = sect->m_length;
            char* buf = (char*)operator new(len + 1);
            if (!buf) {
                return 0;
            }
            memcpy(buf, src, len);
            buf[len] = 0;
            m_text = buf;
            sect->EndParse();
            operator delete(buf);
        }
        m_clipRgn.Attach(CreateRectRgn(0x32, 0, 0x24e, 0x1e0));
        char* b = *(char**)(m_dcChain + 4);
        char* c2 = *(char**)(b + 0x14);
        CDDSurface* d = *(CDDSurface**)(c2 + 0x2c);
        HDC hdc = 0;
        d->m_8->GetDC(&hdc);
        if (hdc) {
            i32 h = DrawTextA(hdc, (const char*)m_text, -1, &m_textRect, 0x450);
            SetRect(&m_scrollRect, 0x32, 0x1e0, 0x24e, h + 0x1e0);
            d->m_8->ReleaseDC(hdc);
        }
        m_1f8 = 0;
        m_1fc = 0;
        m_1f4 = (i32)(g_5e96f8 / g_5e96f0);
        m_scrollStep = (g_5e96f8 * g_5e9708) / (double)(unsigned)m_1f4;
        return 1;
    }

    // -------------------------------------------------------------------------
    // GruntHitMgr::FindGruntAt (0x00075c60) - given a pixel point + tile-span rect
    // (or an explicit source rect), scan the surrounding tile cells of the level
    // grid; return the first live grunt whose 15x15 screen box hits the rect, and
    // report its packed (col,row) via out-params. thiscall, returns HitGrunt*.

    struct HitGruntPos { // HitGrunt::m_pos
        char m_pad0[0x5c];
        i32 m_screenX; // +0x5c  screen x
        i32 m_screenY; // +0x60  screen y
    };
    struct HitGrunt {
        char m_pad0[0x10];
        HitGruntPos* m_pos; // +0x10
        char m_pad14[0x1fc - 0x14];
        i32 m_1fc; // +0x1fc
    };
    struct HitTileRect { // tile-span rect param
        i32 m_left;
        i32 m_top;
        i32 m_right;
        i32 m_bottom;
    };
    struct HitGrid { // g_mgrSettings->m_cellGrid (the level cell grid)
        char m_pad0[8];
        char** m_cells; // +0x8  array of 0x1c-byte-cell rows
        i32 m_width;    // +0xc  width
        i32 m_height;   // +0x10 height
    };
    struct SoundCue { // config-section result handed back by GetSection
        char m_pad0[0x10];
        CSoundCueMgr* m_player; // +0x10
        i32 m_lastTime;         // +0x14
        i32 m_interval;         // +0x18
    };
    struct CfgAccessor { // embedded at MgrM28+0x10
        // GetSection @0x13a000 IS CSymTab::Insert; cast at the call.
    };
    struct MgrM28 { // g_mgrSettings->m_world->m_configHost (config accessor host)
        char m_pad0[0x10];
        CfgAccessor m_cfg; // +0x10 (GetSection receiver)
        char m_pad11[0x30 - 0x11];
        MgrM28* m_30; // +0x30
    };
    struct MgrM30 {
        char m_pad0[0x24];
        void* m_levelInfoSrc; // +0x24  (unused; the +0x160530 check is a free fn)
        MgrM28* m_configHost; // +0x28
    };
    struct MgrWnd { // g_mgrSettings->m_wnd (window holder)
        char m_pad0[4];
        HWND m_hWnd; // +0x4
    };
    struct MgrSettings { // g_mgrSettings (0x64556c)
        char m_pad0[4];
        MgrWnd* m_wnd; // +0x4
        char m_pad8[0x30 - 8];
        MgrM30* m_world; // +0x30
        char m_pad34[0x48 - 0x34];
        CGruntzSoundZ* m_48; // +0x48  sound-bank manager (canonical)
        char m_pad4c[0x70 - 0x4c];
        HitGrid* m_cellGrid; // +0x70
        char m_pad74[0xbc - 0x74];
        i32 m_bc; // +0xbc
        char m_padc0[0x124 - 0xc0];
        i32 m_scrollSpeed; // +0x124
        // Send2bb7 @0x2bb7 IS CGruntzMgr::RunModalDialog; cast at the call.
        // Voice4174 @0x4174 IS CGruntzMgr::StoreInputState; cast at the call.
        // Chip40c0 @0x40c0 IS CGruntzMgr::StoreInputFlag; cast at the call.
    };
    extern "C" MgrSettings* g_mgrSettings; // 0x64556c

    struct GruntHitMgr {
        char m_pad0[0x1c];
        HitGrunt* m_grunts[1]; // +0x1c  grunt slots, indexed by (col + row*15)
        HitGrunt* FindGruntAt(i32 px, i32 py, HitTileRect* tr, i32* outCol, i32* outRow, RECT* src);
    };

    // @early-stop
    // 75% - nested-loop regalloc wall: identical instruction selection/logic, but
    // MSVC5 assigns the point/rect args to a permuted register set (a0->edi,
    // a1->esi vs retail a0->ebx/a1->edi) and spills one fewer local, so the frame
    // is 0x1c vs retail 0x20 and every [esp+N] stack offset shifts. Not steerable
    // from source (llvm-objdump -dr: same mnemonics, shifted operands).
    RVA(0x00075c60, 0x1ba)
    HitGrunt*
    GruntHitMgr::FindGruntAt(i32 px, i32 py, HitTileRect* tr, i32* outCol, i32* outRow, RECT* src) {
        i32 tcol = px >> 5;
        i32 trow = py >> 5;
        RECT rc;
        if (src) {
            CopyRect(&rc, src);
        } else {
            SetRect(
                &rc,
                px - tr->m_left * 32 - 7,
                py - tr->m_top * 32 - 7,
                tr->m_right * 32 + px + 7,
                tr->m_bottom * 32 + py + 7
            );
        }
        i32 xEnd = tr->m_right + tcol + 1;
        for (i32 x = tcol - tr->m_left - 1; (u32)x <= (u32)xEnd; x++) {
            i32 yEnd = tr->m_bottom + trow + 1;
            for (i32 y = trow - tr->m_top - 1; (u32)y <= (u32)yEnd; y++) {
                if ((u32)x >= (u32)g_mgrSettings->m_cellGrid->m_width) {
                    continue;
                }
                HitGrid* grid = g_mgrSettings->m_cellGrid;
                if ((u32)y >= (u32)grid->m_height) {
                    continue;
                }
                i32 val;
                if ((u32)x < (u32)grid->m_width && (u32)y < (u32)grid->m_height) {
                    val = *(i32*)(grid->m_cells[y] + x * 0x1c + 4);
                } else {
                    val = -1;
                }
                if (val == -1) {
                    continue;
                }
                i32 col = val & 0xff;
                i32 row = (val >> 8) & 0xff;
                HitGrunt* g = m_grunts[col + row * 15];
                if (!g) {
                    continue;
                }
                if (!g->m_1fc) {
                    continue;
                }
                i32 sx = g->m_pos->m_screenX - 7;
                i32 sy = g->m_pos->m_screenY - 7;
                if (rc.left <= sx + 0xe && rc.right >= sx && rc.top <= sy + 0xe
                    && rc.bottom >= sy) {
                    *outCol = row;
                    *outRow = col;
                    return g;
                }
            }
        }
        return 0;
    }

    // (0x3b1a0 FillLevelInfoDialog re-homed to src/Gruntz/CustomWorldDialog.cpp - its
    // only caller is CustomWorldDlgProc @0x3ae60 (LBN_SELCHANGE), which lives there, and
    // it is RVA-adjacent (0x3ae60 < 0x3b1a0 < 0x3b310). The LevelInfo view folded onto
    // the real WwdHeader; g_62c25c/g_62c264 are the CString exchange globals that TU
    // already models.)

    // -------------------------------------------------------------------------
    // SetupDlgCommand (0x0009e390) - the Battlez setup dialog's WM_COMMAND
    // handler. Three contiguous control-ID ranges (map/delete/select buttons)
    // each map to a 0..9 index; the matching action toggles the dialog enabled
    // state around a config-store call, and the "select" range starts the game.
    // A free __cdecl(HWND, cmdId, dlg) helper.

    extern "C" i32 g_64c864; // 0x64c864 (last selected slot handle)

    struct SetupDlg {
    }; // GetHandle @0x3bcf = CSaveGame::GetSlot, Apply @0x12f3 = CSaveGame::VerifySlot

    extern "C" void proc_401e3d();            // dialog proc thunk (RVA 0x1e3d)
    extern "C" void proc_40121c();            // dialog proc thunk (RVA 0x121c)
    void func_2ee6(HWND hwnd, SetupDlg* dlg); // cdecl thunk 0x2ee6

    // @early-stop
    // jump-table-data scoring artifact (docs/patterns/jumptable-data-overlap.md):
    // the switch dispatch, all three 10-way index tables, case bodies and the two
    // return epilogues are byte-exact (llvm-objdump -dr), but objdiff scores the 3
    // inline jump-table regions (~120 B) as mismatched — cl's base obj references
    // local `$L####` case labels (addend 0) while the delinked target carries
    // `?SetupDlgCommand+offset` self-relocs, which objdiff cannot pair. ~79%.
    RVA(0x0009e390, 0x243)
    i32 SetupDlgCommand(HWND hwnd, i32 cmdId, SetupDlg* dlg) {
        i32 idx = -1;
        switch (cmdId) {
            case 0x49a:
                idx = 0;
                break;
            case 0x49b:
                idx = 1;
                break;
            case 0x49c:
                idx = 2;
                break;
            case 0x49d:
                idx = 3;
                break;
            case 0x49e:
                idx = 4;
                break;
            case 0x49f:
                idx = 5;
                break;
            case 0x4a0:
                idx = 6;
                break;
            case 0x4a1:
                idx = 7;
                break;
            case 0x4a2:
                idx = 8;
                break;
            case 0x4a3:
                idx = 9;
                break;
        }
        if (idx != -1) {
            i32 h = (i32)((::CSaveGame*)dlg)->GetSlot(idx);
            g_64c864 = h;
            if (h) {
                EnableWindow(hwnd, FALSE);
                ((CGruntzMgr*)g_mgrSettings)->RunModalDialog("GAME_INFO", (void*)proc_401e3d, 0);
                EnableWindow(hwnd, TRUE);
            }
            return 0;
        }
        idx = -1;
        switch (cmdId) {
            case 0x4a4:
                idx = 0;
                break;
            case 0x4a5:
                idx = 1;
                break;
            case 0x4a6:
                idx = 2;
                break;
            case 0x4a7:
                idx = 3;
                break;
            case 0x4a8:
                idx = 4;
                break;
            case 0x4a9:
                idx = 5;
                break;
            case 0x4aa:
                idx = 6;
                break;
            case 0x4ab:
                idx = 7;
                break;
            case 0x4ac:
                idx = 8;
                break;
            case 0x4ad:
                idx = 9;
                break;
        }
        if (idx != -1) {
            i32 h = (i32)((::CSaveGame*)dlg)->GetSlot(idx);
            g_64c864 = h;
            if (h) {
                EnableWindow(hwnd, FALSE);
                i32 r = ((CGruntzMgr*)g_mgrSettings)
                            ->RunModalDialog("GAME_DELETE", (void*)proc_40121c, 0);
                EnableWindow(hwnd, TRUE);
                if (r) {
                    func_2ee6(hwnd, dlg);
                }
            }
            return 0;
        }
        idx = -1;
        switch (cmdId) {
            case 0x490:
                idx = 0;
                break;
            case 0x491:
                idx = 1;
                break;
            case 0x492:
                idx = 2;
                break;
            case 0x493:
                idx = 3;
                break;
            case 0x494:
                idx = 4;
                break;
            case 0x495:
                idx = 5;
                break;
            case 0x496:
                idx = 6;
                break;
            case 0x497:
                idx = 7;
                break;
            case 0x498:
                idx = 8;
                break;
            case 0x499:
                idx = 9;
                break;
        }
        if (idx != -1) {
            i32 h = (i32)((::CSaveGame*)dlg)->GetSlot(idx);
            if (h) {
                EnableWindow(hwnd, FALSE);
                i32 r = ((::CSaveGame*)dlg)->VerifySlot((::SaveSlot*)h);
                EnableWindow(hwnd, TRUE);
                if (r) {
                    g_mgrSettings->m_bc = h;
                    PostMessageA(g_mgrSettings->m_wnd->m_hWnd, 0x111, 0x807e, 0);
                    EndDialog(hwnd, 1);
                }
                return 1;
            }
        }
        return 0;
    }

    // -------------------------------------------------------------------------
    // ScrollDialog (0x00037260) - the options-dialog scrollbar handler. Reads the
    // control's SCROLLINFO, adjusts nPos by the SB_* code, writes it back, then
    // routes the new value to the matching setting (music/sfx volume sliders):
    // control 0x472/0x478 store directly; 0x476/0x470 also (re)trigger a GAME_VOICE
    // / GAME_CHIPFALLOUT sample if the kill-cue clock has elapsed. A free
    // __cdecl(HWND, wParam, code, pos) helper.

    extern i32 g_sndEnabled;       // 0x61ab20 (?g_sndEnabled@@3HA)
    extern i32 g_sndCueTag;        // 0x61ab24 (?g_sndCueTag@@3HA)
    extern "C" i32 g_killCueClock; // 0x6bf3c0

    // @early-stop
    // regalloc wall + jump-table-data artifact (docs/patterns/jumptable-data-overlap.md).
    // Logic + instruction selection identical, but MSVC5 caches `code` in ebp and
    // `newpos` in edi across the whole body, whereas retail keeps `newpos` in ebp,
    // holds `code` in eax only for the switch, and RE-READS code from [esp+0x38] in
    // the voice/chip blocks; that register permutation shifts most operand bytes
    // (consistent ebp<->edi/eax swap, llvm-objdump -dr). ~85%.
    RVA(0x00037260, 0x1fd)
    void ScrollDialog(HWND hwnd, i32 a2, i32 code, i32 pos) {
        if (!hwnd) {
            return;
        }
        SCROLLINFO si;
        si.cbSize = sizeof(si);
        si.fMask = SIF_POS;
        GetScrollInfo(hwnd, SB_CTL, &si);
        i32 newpos;
        if (code == 5) {
            newpos = pos;
        } else {
            newpos = si.nPos;
            if (code == 4) {
                newpos = pos;
            }
        }
        switch (code) {
            case 0:
                newpos--;
                break;
            case 1:
                newpos++;
                break;
            case 2:
                newpos -= 10;
                break;
            case 3:
                newpos += 10;
                break;
            case 4:
                break;
            case 5:
                break;
            default:
                return;
        }
        si.fMask = SIF_POS;
        si.nPos = newpos;
        SetScrollInfo(hwnd, SB_CTL, &si, TRUE);
        if (hwnd == GetDlgItem(hwnd, 0x472)) {
            g_mgrSettings->m_48->SetXMidiVolume(newpos);
            return;
        }
        if (hwnd == GetDlgItem(hwnd, 0x478)) {
            g_mgrSettings->m_scrollSpeed = newpos;
            return;
        }
        if (hwnd == GetDlgItem(hwnd, 0x476)) {
            ((CGruntzMgr*)g_mgrSettings)->StoreInputState(newpos);
            if (code == 5) {
                return;
            }
            MgrM28* m28 = g_mgrSettings->m_world->m_configHost;
            if (m28->m_30) {
                return;
            }
            SoundCue* cue = 0;
            ((CSymTab*)&m28->m_cfg)->Insert("GAME_VOICE", (void*)&cue);
            if (!cue) {
                return;
            }
            if (!g_sndEnabled) {
                return;
            }
            if (g_killCueClock - cue->m_lastTime < cue->m_interval) {
                return;
            }
            cue->m_lastTime = g_killCueClock;
            cue->m_player->ConfigureItem(newpos, 0, 0, 0);
            return;
        }
        if (hwnd == GetDlgItem(hwnd, 0x470)) {
            ((CGruntzMgr*)g_mgrSettings)->StoreInputFlag(newpos);
            if (code == 5) {
                return;
            }
            MgrM28* m28 = g_mgrSettings->m_world->m_configHost;
            if (m28->m_30) {
                return;
            }
            SoundCue* cue = 0;
            ((CSymTab*)&m28->m_cfg)->Insert("GAME_CHIPFALLOUT", (void*)&cue);
            if (!cue) {
                return;
            }
            if (!g_sndEnabled) {
                return;
            }
            if (g_killCueClock - cue->m_lastTime < cue->m_interval) {
                return;
            }
            cue->m_lastTime = g_killCueClock;
            cue->m_player->ConfigureItem(g_sndCueTag, 0, 0, 0);
            return;
        }
    }

} // namespace m4
