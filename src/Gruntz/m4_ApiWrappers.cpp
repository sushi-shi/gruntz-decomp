// m4_ApiWrappers.cpp - game helpers that call Win32/CRT through the IAT,
// re-homed out of src/Stub/ApiCallers.cpp (matcher-4, low-RVA half).
//
// These are real game functions (not CRT): a browser-launch helper and a grunt
// combat-region scan. Placeholder class/field names (m_<hexoffset>); only the
// offsets + code bytes are load-bearing. Compiled base+/GX to mirror the
// original stub-unit environment.
#include <Win32.h>

#include <Ints.h>
#include <rva.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace m4 {

    // -------------------------------------------------------------------------
    // LaunchWebBrowser (0x0008f120) - read HKCR\http\shell\open\command, sanitise
    // the browser command line, and CreateProcess it with the supplied URL.
    // A free __stdcall(char* url) helper.

    // Game helper at RVA 0x2e6e (cdecl): normalises an argv token in-place.
    extern "C" i32 func_2e6e(const char* tok, i32 flag, i32* out);

    RVA(0x0008f120, 0x264)
    i32 __stdcall LaunchWebBrowser(char* url) {
        LONG len = 0x104;
        char cmd[0x104];
        if (RegQueryValueA((HKEY)0x80000000, "http\\shell\\open\\command", cmd, &len)) {
            return 0;
        }
        if (strlen(cmd) < 3) {
            return 0;
        }
        i32 quoted = 0;
        _strlwr(cmd);
        if (strstr(cmd, "IEXPLORE.EXE")) {
            func_2e6e("IEXPLORE.EXE", 1, &quoted);
        }
        char* dash = strchr(cmd, '-');
        i32 dn = dash - cmd + 1;
        if (dash) {
            if (dn <= 2) {
                return 0;
            }
        }
        if (dash) {
            cmd[dn - 2] = 0;
        }
        char* slash = strchr(cmd, '/');
        i32 sn = slash - cmd + 1;
        if (slash) {
            if (sn <= 2) {
                return 0;
            }
        }
        if (slash) {
            cmd[sn - 2] = 0;
        }
        char cmdline[0x104];
        sprintf(cmdline, "%s %s", cmd, url);
        STARTUPINFOA si;
        memset(&si, 0, sizeof(si));
        PROCESS_INFORMATION pi;
        si.cb = sizeof(si);
        return CreateProcessA(0, cmdline, 0, 0, FALSE, 0, 0, 0, &si, &pi);
    }

    // -------------------------------------------------------------------------
    // GruntCombatMgr::CheckCombatRegion (0x00078060) - transform a world rect to
    // screen space, then scan the manager's grunt slots: any grunt whose 30x30
    // screen box intersects the rect gets combat state (re)armed. thiscall,
    // likely a CGruntzMgr method.

    struct GruntCombatMgr;

    struct CombatViewport {  // CombatView::m_5c points here (a CRect at +0x40)
        char m_pad0[0x40];
        RECT m_rect;  // +0x40
    };
    struct CombatView {  // (this->m_22c)->m_24
        char m_pad0[0x10];
        i32 m_10;  // +0x10
        i32 m_14;  // +0x14
        char m_pad18[0x5c - 0x18];
        CombatViewport* m_5c;  // +0x5c
    };
    struct CombatViewHost {  // this->m_22c
        char m_pad0[0x24];
        CombatView* m_24;  // +0x24
    };
    struct GruntPos {  // CombatGrunt::m_10
        char m_pad0[0x5c];
        i32 m_5c;  // +0x5c (screen x)
        i32 m_60;  // +0x60 (screen y)
    };
    struct CombatGrunt {  // element of GruntCombatMgr::m_1c
        char m_pad0[0x10];
        GruntPos* m_10;  // +0x10
        char m_pad14[0x1fc - 0x14];
        i32 m_1fc;  // +0x1fc
        char m_pad200[0x880 - 0x200];
        i32 m_880;  // +0x880
        i32 m_884;  // +0x884
        i32 m_888;  // +0x888
        i32 m_88c;  // +0x88c
        void Method_243c();  // RVA 0x243c
    };
    struct CButeMgr {
        i32 GetInt(const char* section, const char* key, i32 def);  // RVA 0x1721e0
    };
    extern CButeMgr g_buteMgr;  // 0x6453d8
    extern "C" i32 g_644c54;    // 0x644c54
    extern "C" i32 g_645588;    // 0x645588

    struct GruntCombatMgr {
        char m_pad0[0x1c];
        CombatGrunt* m_1c[16];  // +0x1c
        char m_pad5c[0x22c - 0x5c];
        CombatViewHost* m_22c;  // +0x22c
        void Method_36ed();     // RVA 0x36ed
        void Method_29cd(i32 a, i32 j, i32 c, i32 d);  // RVA 0x29cd
        void CheckCombatRegion(i32 a, i32 b, i32 c, i32 d, i32 flag);
    };

    // @early-stop
    // regalloc/CSE wall: identical logic + instruction selection, but MSVC5 pins
    // `this`->ebx (retail: ebp) and `view` is dropped from eax after the m_24
    // load; that pressure lets cl CSE (view->m_5c->rect.left - m_10) once whereas
    // retail reloads view->m_5c per rect pair. Consistent ebx<->ebp swap + the
    // reload-vs-CSE shape are the only residuals (llvm-objdump -dr). ~80%.
    RVA(0x00078060, 0x18d)
    void GruntCombatMgr::CheckCombatRegion(i32 a, i32 b, i32 c, i32 d, i32 flag) {
        CombatView* view = m_22c->m_24;
        a += view->m_5c->m_rect.left - view->m_10;
        b += view->m_5c->m_rect.top - view->m_14;
        c += view->m_5c->m_rect.left - view->m_10;
        d += view->m_5c->m_rect.top - view->m_14;
        for (i32 i = 0; i < 4; i++) {
            for (i32 j = 0; j < 15; j++) {
                CombatGrunt* g = m_1c[j];
                if (g) {
                    i32 cx = g->m_10->m_5c;
                    i32 cy = g->m_10->m_60;
                    RECT box;
                    SetRect(&box, cx - 0xf, cy - 0xf, cx + 0xf, cy + 0xf);
                    if (a <= box.right && c >= box.left && b <= box.bottom && d >= box.top) {
                        if (i == g_644c54) {
                            if (flag == 0 && g->m_1fc != 0) {
                                Method_36ed();
                                flag = 1;
                            }
                            Method_29cd(g_644c54, j, 1, 1);
                        } else {
                            g->Method_243c();
                            g->m_888 = g_buteMgr.GetInt("Grunt", "CombatTimeout", 0x1388);
                            g->m_88c = 0;
                            g->m_880 = g_645588;
                            g->m_884 = 0;
                        }
                    }
                }
            }
        }
    }

    // -------------------------------------------------------------------------
    // CreditzScreen::BuildText (0x00039a60) - pull the "CREDITZ" TXT blob from the
    // config store into the scroll CString, measure it against the offscreen DC,
    // and set up the scroll rect + per-frame scroll step. thiscall, returns BOOL.

    struct CreditzText {  // config section handle (this->m_2c->GetSection result)
        char m_pad0[0xc];
        i32 m_c;              // +0xc  byte length
        char* GetData();      // RVA 0x139960
        void Release();       // RVA 0x1399d0
    };

    struct CreditzStr {  // MFC-style CString (data ptr at +0)
        char* m_data;
        void Assign(const char* s);  // RVA 0x1b9e74
    };
    struct CreditzRgn {  // CRgn / CGdiObject at this->m_1e8
        char m_pad0[8];
        i32 Attach(void* hrgn);  // RVA 0x1c6a05
    };
    struct CreditzDcVtbl;
    struct CreditzDc {
        CreditzDcVtbl* vtbl;
    };
    struct CreditzDcVtbl {
        char m_pad0[0x44];
        void (*GetDC)(CreditzDc*, i32* out);     // +0x44
        char m_pad48[0x68 - 0x48];
        void (*ReleaseDC)(CreditzDc*, i32 hdc);  // +0x68
    };
    struct CreditzSectionSrc {  // this->m_2c
        CreditzText* GetSection(const char* name, i32 tag);  // RVA 0x13a000
    };

    extern double g_5e96f8;
    extern double g_5e96f0;
    extern double g_5e9708;

    struct CreditzScreen {
        char m_pad0[0xc];
        char* m_c;  // +0xc  (chain to the offscreen-DC object)
        char m_pad10[0x2c - 0x10];
        CreditzSectionSrc* m_2c;  // +0x2c
        char m_pad30[0x1c8 - 0x30];
        RECT m_1c8;         // +0x1c8
        RECT m_1d8;         // +0x1d8
        CreditzRgn m_1e8;   // +0x1e8
        CreditzStr m_1f0;   // +0x1f0
        i32 m_1f4;          // +0x1f4
        i32 m_1f8;          // +0x1f8
        i32 m_1fc;          // +0x1fc
        double m_200;       // +0x200
        i32 BuildText();
    };

    // @early-stop
    // 98% - scheduling tail: for the final (double)(unsigned)m_1f4 conversion the
    // int64 temp's two halves (low=eax, high=0) are emitted in the opposite order
    // relative to the fld/fmul (retail stores high=0 before the fld, low after the
    // fmul; cl reverses). All other bytes identical (llvm-objdump -dr).
    RVA(0x00039a60, 0x179)
    i32 CreditzScreen::BuildText() {
        CreditzText* sect = m_2c->GetSection("CREDITZ", 0x545854);
        if (sect) {
            char* src = sect->GetData();
            if (!src) {
                return 0;
            }
            i32 len = sect->m_c;
            char* buf = (char*)operator new(len + 1);
            if (!buf) {
                return 0;
            }
            memcpy(buf, src, len);
            buf[len] = 0;
            m_1f0.Assign(buf);
            sect->Release();
            operator delete(buf);
        }
        m_1e8.Attach(CreateRectRgn(0x32, 0, 0x24e, 0x1e0));
        char* b = *(char**)(m_c + 4);
        char* c2 = *(char**)(b + 0x14);
        char* d = *(char**)(c2 + 0x2c);
        CreditzDc* dc = *(CreditzDc**)(d + 8);
        i32 hdc = 0;
        dc->vtbl->GetDC(dc, &hdc);
        if (hdc) {
            i32 h = DrawTextA((HDC)hdc, m_1f0.m_data, -1, &m_1d8, 0x450);
            SetRect(&m_1c8, 0x32, 0x1e0, 0x24e, h + 0x1e0);
            CreditzDc* dc2 = *(CreditzDc**)(d + 8);
            dc2->vtbl->ReleaseDC(dc2, hdc);
        }
        m_1f8 = 0;
        m_1fc = 0;
        m_1f4 = (i32)(g_5e96f8 / g_5e96f0);
        m_200 = (g_5e96f8 * g_5e9708) / (double)(unsigned)m_1f4;
        return 1;
    }

    // -------------------------------------------------------------------------
    // GruntHitMgr::FindGruntAt (0x00075c60) - given a pixel point + tile-span rect
    // (or an explicit source rect), scan the surrounding tile cells of the level
    // grid; return the first live grunt whose 15x15 screen box hits the rect, and
    // report its packed (col,row) via out-params. thiscall, returns HitGrunt*.

    struct HitGruntPos {  // HitGrunt::m_10
        char m_pad0[0x5c];
        i32 m_5c;  // +0x5c  screen x
        i32 m_60;  // +0x60  screen y
    };
    struct HitGrunt {
        char m_pad0[0x10];
        HitGruntPos* m_10;  // +0x10
        char m_pad14[0x1fc - 0x14];
        i32 m_1fc;  // +0x1fc
    };
    struct HitTileRect {  // tile-span rect param
        i32 m_0;
        i32 m_4;
        i32 m_8;
        i32 m_c;
    };
    struct HitGrid {  // g_mgrSettings->m_70 (the level cell grid)
        char m_pad0[8];
        char** m_8;  // +0x8  array of 0x1c-byte-cell rows
        i32 m_c;     // +0xc  width
        i32 m_10;    // +0x10 height
    };
    struct LevelInfo {  // filled by the config accessor at RVA 0x160530 (0x5f4 B)
        char m_pad0[0x10];
        char m_10[0x40];         // +0x10  version/number string (scanned for a digit)
        char m_50[0x40];         // +0x50  (dialog item 0x428)
        char m_90[0x5f4 - 0x90]; // +0x90  (dialog item 0x429)
    };
    struct LevelInfoSrc {  // g_mgrSettings->m_30->m_24
        i32 GetInfo(void* key, LevelInfo* out);  // RVA 0x160530
    };
    struct SoundPlayer {
        void Play1360d0(i32 a, i32 b, i32 c, i32 d);  // thiscall thunk 0x1360d0
    };
    struct SoundCue {  // config-section result handed back by GetSection
        char m_pad0[0x10];
        SoundPlayer* m_10;  // +0x10
        i32 m_14;           // +0x14
        i32 m_18;           // +0x18
    };
    struct CfgAccessor {  // embedded at MgrM28+0x10
        i32 GetSection(const char* name, SoundCue** out);  // thiscall thunk 0x1b8438
    };
    struct MgrM28 {  // g_mgrSettings->m_30->m_28 (config accessor host)
        char m_pad0[0x10];
        CfgAccessor m_10;  // +0x10 (GetSection receiver)
        char m_pad11[0x30 - 0x11];
        MgrM28* m_30;  // +0x30
    };
    struct MgrM30 {
        char m_pad0[0x24];
        LevelInfoSrc* m_24;  // +0x24
        MgrM28* m_28;         // +0x28
    };
    struct MgrM48 {  // g_mgrSettings->m_48 (scroll target)
        void Set138950(i32 pos);  // thiscall thunk 0x138950
    };
    struct MgrWnd {  // g_mgrSettings->m_4 (window holder)
        char m_pad0[4];
        HWND m_4;  // +0x4
    };
    struct MgrM48;  // g_mgrSettings->m_48 (scroll target)
    struct MgrSettings {  // g_mgrSettings (0x64556c)
        char m_pad0[4];
        MgrWnd* m_4;  // +0x4
        char m_pad8[0x30 - 8];
        MgrM30* m_30;  // +0x30
        char m_pad34[0x48 - 0x34];
        MgrM48* m_48;  // +0x48
        char m_pad4c[0x70 - 0x4c];
        HitGrid* m_70;  // +0x70
        char m_pad74[0xbc - 0x74];
        i32 m_bc;  // +0xbc
        char m_padc0[0x124 - 0xc0];
        i32 m_124;  // +0x124
        i32 Send2bb7(const char* section, void* proc, i32 flag);  // thiscall thunk 0x2bb7
        void Voice4174(i32 pos);       // thiscall thunk 0x4174
        void Chip40c0(i32 pos);        // thiscall thunk 0x40c0
    };
    extern "C" MgrSettings* g_mgrSettings;  // 0x64556c

    struct GruntHitMgr {
        char m_pad0[0x1c];
        HitGrunt* m_1c[1];  // +0x1c  grunt slots, indexed by (col + row*15)
        HitGrunt* FindGruntAt(i32 px, i32 py, HitTileRect* tr, i32* outCol, i32* outRow, RECT* src);
    };

    // @early-stop
    // 75% - nested-loop regalloc wall: identical instruction selection/logic, but
    // MSVC5 assigns the point/rect args to a permuted register set (a0->edi,
    // a1->esi vs retail a0->ebx/a1->edi) and spills one fewer local, so the frame
    // is 0x1c vs retail 0x20 and every [esp+N] stack offset shifts. Not steerable
    // from source (llvm-objdump -dr: same mnemonics, shifted operands).
    RVA(0x00075c60, 0x1ba)
    HitGrunt* GruntHitMgr::FindGruntAt(i32 px, i32 py, HitTileRect* tr, i32* outCol, i32* outRow, RECT* src) {
        i32 tcol = px >> 5;
        i32 trow = py >> 5;
        RECT rc;
        if (src) {
            CopyRect(&rc, src);
        } else {
            SetRect(&rc, px - tr->m_0 * 32 - 7, py - tr->m_4 * 32 - 7, tr->m_8 * 32 + px + 7,
                    tr->m_c * 32 + py + 7);
        }
        i32 xEnd = tr->m_8 + tcol + 1;
        for (i32 x = tcol - tr->m_0 - 1; (u32)x <= (u32)xEnd; x++) {
            i32 yEnd = tr->m_c + trow + 1;
            for (i32 y = trow - tr->m_4 - 1; (u32)y <= (u32)yEnd; y++) {
                if ((u32)x >= (u32)g_mgrSettings->m_70->m_c) {
                    continue;
                }
                HitGrid* grid = g_mgrSettings->m_70;
                if ((u32)y >= (u32)grid->m_10) {
                    continue;
                }
                i32 val;
                if ((u32)x < (u32)grid->m_c && (u32)y < (u32)grid->m_10) {
                    val = *(i32*)(grid->m_8[y] + x * 0x1c + 4);
                } else {
                    val = -1;
                }
                if (val == -1) {
                    continue;
                }
                i32 col = val & 0xff;
                i32 row = (val >> 8) & 0xff;
                HitGrunt* g = m_1c[col + row * 15];
                if (!g) {
                    continue;
                }
                if (!g->m_1fc) {
                    continue;
                }
                i32 sx = g->m_10->m_5c - 7;
                i32 sy = g->m_10->m_60 - 7;
                if (rc.left <= sx + 0xe && rc.right >= sx && rc.top <= sy + 0xe && rc.bottom >= sy) {
                    *outCol = row;
                    *outRow = col;
                    return g;
                }
            }
        }
        return 0;
    }

    // -------------------------------------------------------------------------
    // FillLevelInfoDialog (0x0003b1a0) - populate the level-info dialog items from
    // the config store; on a bad level file, stamp every item "Bad Level File".
    // A free __cdecl(HWND) helper.

    extern "C" i32 func_2176(HWND hDlg);  // RVA 0x2176 (cdecl game helper)
    extern "C" void* g_62c25c;            // 0x62c25c  config key value
    extern "C" char* g_62c264;            // 0x62c264  dialog item 0x408 text

    RVA(0x0003b1a0, 0x118)
    i32 FillLevelInfoDialog(HWND hDlg) {
        if (!GetDlgItem(hDlg, 0x3fc)) {
            return 0;
        }
        if (!func_2176(hDlg)) {
            return 0;
        }
        char num[0x20];
        LevelInfo info;
        if (g_mgrSettings->m_30->m_24->GetInfo(g_62c25c, &info)) {
            char* p = info.m_10;
            while (*p && (*p < '0' || *p > '9')) {
                p++;
            }
            sprintf(num, "%d", atoi(p));
            SetDlgItemTextA(hDlg, 0x408, g_62c264);
            SetDlgItemTextA(hDlg, 0x428, info.m_50);
            SetDlgItemTextA(hDlg, 0x40c, num);
            SetDlgItemTextA(hDlg, 0x429, info.m_90);
        } else {
            SetDlgItemTextA(hDlg, 0x408, "Bad Level File");
            SetDlgItemTextA(hDlg, 0x428, "Bad Level File");
            SetDlgItemTextA(hDlg, 0x40c, "Bad Level File");
            SetDlgItemTextA(hDlg, 0x429, "Bad Level File");
        }
        return 1;
    }

    // -------------------------------------------------------------------------
    // SetupDlgCommand (0x0009e390) - the Battlez setup dialog's WM_COMMAND
    // handler. Three contiguous control-ID ranges (map/delete/select buttons)
    // each map to a 0..9 index; the matching action toggles the dialog enabled
    // state around a config-store call, and the "select" range starts the game.
    // A free __cdecl(HWND, cmdId, dlg) helper.

    extern "C" i32 g_64c864;  // 0x64c864 (last selected slot handle)

    struct SetupDlg {  // arg3 - the settings dialog object
        i32 GetHandle(i32 idx);  // thiscall thunk 0x3bcf
        i32 Apply(i32 h);        // thiscall thunk 0x12f3
    };

    extern "C" void proc_401e3d();  // dialog proc thunk (RVA 0x1e3d)
    extern "C" void proc_40121c();  // dialog proc thunk (RVA 0x121c)
    void func_2ee6(HWND hwnd, SetupDlg* dlg);  // cdecl thunk 0x2ee6

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
        case 0x49a: idx = 0; break;
        case 0x49b: idx = 1; break;
        case 0x49c: idx = 2; break;
        case 0x49d: idx = 3; break;
        case 0x49e: idx = 4; break;
        case 0x49f: idx = 5; break;
        case 0x4a0: idx = 6; break;
        case 0x4a1: idx = 7; break;
        case 0x4a2: idx = 8; break;
        case 0x4a3: idx = 9; break;
        }
        if (idx != -1) {
            i32 h = dlg->GetHandle(idx);
            g_64c864 = h;
            if (h) {
                EnableWindow(hwnd, FALSE);
                g_mgrSettings->Send2bb7("GAME_INFO", (void*)proc_401e3d, 0);
                EnableWindow(hwnd, TRUE);
            }
            return 0;
        }
        idx = -1;
        switch (cmdId) {
        case 0x4a4: idx = 0; break;
        case 0x4a5: idx = 1; break;
        case 0x4a6: idx = 2; break;
        case 0x4a7: idx = 3; break;
        case 0x4a8: idx = 4; break;
        case 0x4a9: idx = 5; break;
        case 0x4aa: idx = 6; break;
        case 0x4ab: idx = 7; break;
        case 0x4ac: idx = 8; break;
        case 0x4ad: idx = 9; break;
        }
        if (idx != -1) {
            i32 h = dlg->GetHandle(idx);
            g_64c864 = h;
            if (h) {
                EnableWindow(hwnd, FALSE);
                i32 r = g_mgrSettings->Send2bb7("GAME_DELETE", (void*)proc_40121c, 0);
                EnableWindow(hwnd, TRUE);
                if (r) {
                    func_2ee6(hwnd, dlg);
                }
            }
            return 0;
        }
        idx = -1;
        switch (cmdId) {
        case 0x490: idx = 0; break;
        case 0x491: idx = 1; break;
        case 0x492: idx = 2; break;
        case 0x493: idx = 3; break;
        case 0x494: idx = 4; break;
        case 0x495: idx = 5; break;
        case 0x496: idx = 6; break;
        case 0x497: idx = 7; break;
        case 0x498: idx = 8; break;
        case 0x499: idx = 9; break;
        }
        if (idx != -1) {
            i32 h = dlg->GetHandle(idx);
            if (h) {
                EnableWindow(hwnd, FALSE);
                i32 r = dlg->Apply(h);
                EnableWindow(hwnd, TRUE);
                if (r) {
                    g_mgrSettings->m_bc = h;
                    PostMessageA(g_mgrSettings->m_4->m_4, 0x111, 0x807e, 0);
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

    extern i32 g_sndEnabled;        // 0x61ab20 (?g_sndEnabled@@3HA)
    extern i32 g_sndCueTag;         // 0x61ab24 (?g_sndCueTag@@3HA)
    extern "C" i32 g_killCueClock;  // 0x6bf3c0

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
        case 0: newpos--; break;
        case 1: newpos++; break;
        case 2: newpos -= 10; break;
        case 3: newpos += 10; break;
        case 4: break;
        case 5: break;
        default: return;
        }
        si.fMask = SIF_POS;
        si.nPos = newpos;
        SetScrollInfo(hwnd, SB_CTL, &si, TRUE);
        if (hwnd == GetDlgItem(hwnd, 0x472)) {
            g_mgrSettings->m_48->Set138950(newpos);
            return;
        }
        if (hwnd == GetDlgItem(hwnd, 0x478)) {
            g_mgrSettings->m_124 = newpos;
            return;
        }
        if (hwnd == GetDlgItem(hwnd, 0x476)) {
            g_mgrSettings->Voice4174(newpos);
            if (code == 5) {
                return;
            }
            MgrM28* m28 = g_mgrSettings->m_30->m_28;
            if (m28->m_30) {
                return;
            }
            SoundCue* cue = 0;
            m28->m_10.GetSection("GAME_VOICE", &cue);
            if (!cue) {
                return;
            }
            if (!g_sndEnabled) {
                return;
            }
            if (g_killCueClock - cue->m_14 < cue->m_18) {
                return;
            }
            cue->m_14 = g_killCueClock;
            cue->m_10->Play1360d0(newpos, 0, 0, 0);
            return;
        }
        if (hwnd == GetDlgItem(hwnd, 0x470)) {
            g_mgrSettings->Chip40c0(newpos);
            if (code == 5) {
                return;
            }
            MgrM28* m28 = g_mgrSettings->m_30->m_28;
            if (m28->m_30) {
                return;
            }
            SoundCue* cue = 0;
            m28->m_10.GetSection("GAME_CHIPFALLOUT", &cue);
            if (!cue) {
                return;
            }
            if (!g_sndEnabled) {
                return;
            }
            if (g_killCueClock - cue->m_14 < cue->m_18) {
                return;
            }
            cue->m_14 = g_killCueClock;
            cue->m_10->Play1360d0(g_sndCueTag, 0, 0, 0);
            return;
        }
    }

}  // namespace m4
