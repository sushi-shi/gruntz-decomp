#include <Mfc.h>
#include <Gruntz/PortalPath.h> // g_nameFmt (ex .cpp extern)
#undef _AFX_ENABLE_INLINES     // skip afxwin1.inl (MFC4.2 implicit-int inlines clang rejects)
#include <afxwin.h>            // real MFC CCmdTarget::Begin/EndWaitCursor (via m_pCurrentWinApp)
#include <Gruntz/GameRegMfcPtr.h>

#include <Gruntz/CustomWorldInfoDlg.h> // the TU's dialog-proc / helper extern surface
#include <Gruntz/GameRegistry.h> // the canonical manager singleton view (m_gameWnd/m_world/m_owner)
#include <Gruntz/GruntzMgr.h>    // the MFC view of the same singleton (IsBattlezMapFile)
#include <Gruntz/GameLevel.h> // CGameLevel::ReadWwdHeaderName / IsValidWwd (the m_level receiver)
#include <Wwd/WwdFile.h>      // WwdHeader + the WwdFile statics
#include <Ints.h>
#include <rva.h>

#include <direct.h>                   // _getcwd (BuildCustomWwdPath)
#include <io.h>                       // _finddata_t / _findfirst / _findnext (FillCustomLevelList)
#include <stdio.h>                    // sprintf
#include <stdlib.h>                   // atoi
#include <string.h>                   // strstr / inline strcpy-strlen
#include <Gruntz/CustomWorldDialog.h> // own exported globals (ex Globals.h)
#include <Net/NetLobby.h>             // NetLobby::g_curDlg

// LoadCustomWorldInfo's DialogBoxParamA takes CustomWorldInfoDlgProc's ADDRESS, and the
// retail /INCREMENTAL link routes it through the proc's ILT jmp-thunk 0x305d (jmp 0x3b600),
// so the DIR32 stored is 0x305d, not the body 0x3b600. Bind the address-taken symbol to
// the THUNK rva (same idiom as GruntzApp's _ErrorDialogProcThunk @0x33c8) so have==want.

DATA(0x0022c010)
char g_mapNameBuf[0x200] = {0}; // 0x62c010  GetMapBaseName filename scratch
DATA(0x0022c25c)
CString g_pathStr;
DATA(0x0022c260)
CString g_levelStr;
DATA(0x0022c264)
CString g_str62c264;
DATA(0x0022c268)
CDDrawSurfaceMgr* g_dat62c268 = 0; // 0x62c268  the manager's world slot, seeded for the popup
DATA(0x0022c26c)
HWND g_customWorldParent = 0; // 0x62c26c  launcher parent-window exchange
DATA(0x0022c270)
HINSTANCE g_customWorldInst = 0; // 0x62c270  launcher instance exchange
DATA(0x0022c274)
HWND g_customLevelList = 0; // 0x62c274  the picker's level listbox (id 0x3fc)

DATA(0x0020cf90)
char g_dotDot[] = ".."; // 0x60cf90
DATA(0x0020cf94)
char g_customGlob[] = "*.WWD"; // 0x60cf94

namespace m4 {}

// The three file-scope CStrings each emit wrapper/constructor/registrar/
// destructor helpers. The _$E<n> suffixes are unique placeholders; the
// compiler-private numbers are unstable.

// ===========================================================================
// run the custom-world dialog. Seed 0x62c26c with the supplied parent (or the
// manager's game-window HWND when 0), 0x62c268 with m_30, 0x62c270 with
// m_8->m_c (the HINSTANCE); run the dialog; on cancel (result 0) clear the
// returned name; clear the three seed slots; copy g_str62c264 into the out-param
// when present; return the selected-world path by value.
// ===========================================================================
// @early-stop
// ~95.7% under this obj's real /GX profile. Remaining residual is the cross-TU reloc-name tail:
// the CString::Empty/operator=/copy-ctor + RunModalDialog relocs resolve to the
// delinker's Ghidra simple-labels which a foreign TU cannot name-match; they go
// exact once those library/manager functions are reconstructed. Logic complete.
RVA(0x0003ad90, 0x97)
CString RunCustomWorldDialog(HWND parent, CString* outSource) {
    g_pathStr.Empty();
    HWND v = parent;
    if (parent == 0) {
        v = g_gameReg->m_gameWnd->m_hwnd;
    }
    g_customWorldParent = v;
    g_dat62c268 = g_gameReg->m_world;
    // m_owner (CGameApp*, CGameMgr+0x8) -> +0xc HINSTANCE (raw offset read).
    g_customWorldInst = g_gameReg->m_owner->m_hInstance;
    if (g_gameReg->RunModalDialog("CUSTOM_WORLD", CustomWorldDlgProc, 0) == 0) {
        g_pathStr.Empty();
    }
    g_dat62c268 = 0;
    g_customWorldParent = 0;
    g_customWorldInst = 0;
    if (outSource != 0) {
        *outSource = g_str62c264;
    }
    return g_pathStr;
}

RVA(0x0003ae60, 0xec)
INT_PTR CALLBACK CustomWorldDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    NetLobby::g_curDlg = hDlg;
    switch (msg) {
        case 0x110: // WM_INITDIALOG
            g_customLevelList = GetDlgItem(hDlg, 0x3fc);
            if (g_customLevelList) {
                m4::FillCustomLevelList(hDlg);
            }
            return 1;
        case 0x111: // WM_COMMAND
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 0x42a) {
                LoadCustomWorldInfo(hDlg);
                return 1;
            }
            if (wParam == 1) {
                LoadCustomWorldSelection(hDlg);
                EndDialog(hDlg, 1);
                return 1;
            }
            if (g_customLevelList != 0 && lParam == reinterpret_cast<LPARAM>(g_customLevelList)) {
                if (HIWORD(wParam) == 1) {
                    FillLevelInfoDialog(hDlg);
                    return 1;
                }
                if (HIWORD(wParam) == 2) {
                    PostMessageA(hDlg, 0x111, 1, 0);
                    return 1;
                }
            }
            break;
    }
    return 0;
}

namespace m4 {

    // Game Win32 pointer table (reloc-masked indirect calls).

    // The directory walk is the real CRT _findfirst/_findnext (0x11f900 / 0x11fa30,
    // FID-carved) over <io.h>'s _finddata_t.

    // The "reentrancy lock" guarding the directory walk is the MFC wait cursor:
    // AfxGetApp()->Begin/EndWaitCursor (<Gruntz/WaitCursorApp.h>).

    // The settings-manager singleton == *g_gameReg (the real CGruntzMgr, the MFC view of
    // the file-global g_gameReg above); IsBattlezMapFile takes the display name by value
    // (callee destroys). The namespaced re-declaration that used to sit here was a
    // C++-linkage SHADOW of the file-scope extern "C" g_gameReg - at the identical type,
    // so it bought nothing and emitted ?g_gameReg@m4@@3PAVCGruntzMgr@@A: a second symbol
    // for 0x24556c that nothing could ever define. Plain lookup finds ::g_gameReg.

    // (The display-name format literal g_nameFmt "%s" @0x60c5b8 is DEFINED in
    // src/Gruntz/BootyStateActivate.cpp, whose .data run holds it - the "Cursez:"
    // literal follows it there; declared at file scope below this namespace. The
    // glob + ".." literals are this TU's own, DEFINED at file scope above.)

    // @early-stop
    // regalloc + frame-layout wall. Complete correct reconstruction: the listbox
    // reset, the "Custom" gate, the _findfirst glob, the singleton lock (its /GX
    // unwind), the do-while _findnext walk with the per-entry sprintf + by-value
    // IsHidden query + extension strip + LB_ADDSTRING, and the unlock all align by
    // shape (llvm-objdump -dr). Residual is MSVC5's stack-buffer placement (the glob
    // vs _finddata_t vs display-name locals land at different [esp+N] than retail's, and
    // the CString-temp arg slot is reused) shifting the operand displacements - not
    // steerable from source.
    RVA(0x0003af90, 0x194)
    i32 FillCustomLevelList(HWND hWnd) {
        HWND lb = ::GetDlgItem(hWnd, 0x3fc);
        if (!lb) {
            return 0;
        }
        ::SendMessageA(lb, 0x184, 0, 0); // LB_RESETCONTENT
        if (CustomGate("Custom")) {
            return 0;
        }
        char pattern[260];
        strcpy(pattern, g_customGlob);
        _finddata_t fd;
        i32 h = _findfirst(pattern, &fd);
        i32 found = (h != -1);
        AfxGetApp()->BeginWaitCursor();
        if (found) {
            do {
                char disp[260];
                sprintf(disp, "%s", fd.name);
                if (!g_gameReg->IsBattlezMapFile(CString(disp))) {
                    i32 len = strlen(disp);
                    if (len > 4) {
                        disp[len - 4] = 0;
                    }
                    ::SendMessageA(lb, 0x180, 0, reinterpret_cast<LPARAM>(disp)); // LB_ADDSTRING
                }
            } while (_findnext(h, &fd) != -1);
        }
        CustomGate(g_dotDot);
        AfxGetApp()->EndWaitCursor();
        return 1;
    }

} // namespace m4

// (the dead DATA(0x002c4554) fn-decl binding is gone - it never emitted a row)

// ===========================================================================
// FillLevelInfoDialog @0x3b1a0 - populate the four level-info dialog items from the
// selected world's .WWD header (path in g_pathStr); on a bad/invalid file, stamp
// every item "Bad Level File". A free __cdecl(HWND) helper the picker DlgProc's
// LBN_SELCHANGE notification calls.
// ===========================================================================
RVA(0x0003b1a0, 0x118)
i32 FillLevelInfoDialog(HWND hDlg) {
    if (!::GetDlgItem(hDlg, 0x3fc)) {
        return 0;
    }
    if (!func_2176(hDlg)) {
        return 0;
    }
    char num[0x20];
    WwdHeader info;
    BOOL(WINAPI * setText)(HWND, int, LPCSTR) = ::SetDlgItemTextA;
    if (g_gameReg->m_world->m_level->IsValidWwd(static_cast<const char*>(g_pathStr), &info)) {
        char* p = info.levelName;
        while (*p && (*p < '0' || *p > '9')) {
            p++;
        }
        sprintf(num, "%d", atoi(p));
        setText(hDlg, 0x408, static_cast<const char*>(g_str62c264));
        setText(hDlg, 0x428, info.levelName + 0x40);
        setText(hDlg, 0x40c, num);
        setText(hDlg, 0x429, info.levelName + 0x80);
    } else {
        setText(hDlg, 0x408, "Bad Level File");
        setText(hDlg, 0x428, "Bad Level File");
        setText(hDlg, 0x40c, "Bad Level File");
        setText(hDlg, 0x429, "Bad Level File");
    }
    return 1;
}

RVA(0x0003b310, 0x10d)
i32 LoadCustomWorldSelection(HWND hWnd) {
    char itemText[256];
    char dirBuf[256];
    HWND lb = GetDlgItem(hWnd, 0x3fc);
    if (!lb) {
        return 0;
    }
    i32 sel = SendMessageA(lb, 0x188, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if (SendMessageA(lb, 0x189, sel, reinterpret_cast<LPARAM>(itemText)) == -1) {
        return 0;
    }
    if (!_getcwd(dirBuf, 0xfe)) {
        return 0;
    }
    g_pathStr = dirBuf;
    g_pathStr += "\\Custom\\";
    g_pathStr += itemText;
    g_pathStr += ".WWD";
    if (!FileExists(const_cast<char*>(static_cast<const char*>(g_pathStr)))) {
        g_pathStr.Empty();
        return 0;
    }
    g_str62c264 = itemText;
    return 1;
}

RVA(0x0003b470, 0x13a)
i32 WwdFile::ValidateMainBlock(CString name) {
    char header[0x100];

    if (name.GetLength() == 0) {
        return -1;
    }
    // The world slot's +0x24 is the level (CDDrawSurfaceMgr::m_level) - the SAME
    // receiver FillLevelInfoDialog drives IsValidWwd on. It is not just a null guard:
    // retail leaves it in ECX as the __thiscall receiver of the sibling reader below
    // (0x3b4de `mov ecx,[edx+0x24]`, then 0x3b515 pushes only the two stack args), so
    // the header temp gets edx. Modelling 0x160660 as a free __stdcall taking the level
    // POINTER as its `const char* name` (excused as "retail's own pun") was the bug -
    // that callee CFile::Open()s the arg, so it can only ever be the file name.
    CGameLevel* lvl = g_gameReg->m_world->m_level;
    if (lvl == 0) {
        return -1;
    }

    if (!lvl->ReadWwdHeaderName(name, header)) {
        return -1;
    }

    char* p = header;
    char c = *p;
    while (c != 0 && (c < '0' || c > '9')) {
        c = *++p;
    }
    return atoi(p);
}

RVA(0x0003b600, 0x15f)
INT_PTR CALLBACK CustomWorldInfoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110: { // WM_INITDIALOG
            WwdHeader info;
            char num[0x20];
            i32 bad = 1;
            if (g_dat62c268 != 0
                && FileExists(const_cast<char*>(static_cast<const char*>(g_pathStr)))
                && g_dat62c268->m_level->IsValidWwd(static_cast<const char*>(g_pathStr), &info)) {
                SetDlgItemTextA(hDlg, 0x408, static_cast<const char*>(g_levelStr));
                SetDlgItemTextA(hDlg, 0x428, info.levelName + 0x40);
                char* p = info.levelName;
                while (*p && (*p < '0' || *p > '9')) {
                    p++;
                }
                sprintf(num, "%d", atoi(p));
                SetDlgItemTextA(hDlg, 0x40c, num);
                SetDlgItemTextA(hDlg, 0x429, info.levelName + 0x80);
                bad = 0;
            }
            if (bad) {
                SetDlgItemTextA(hDlg, 0x408, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x428, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x40c, "Bad Level File");
                SetDlgItemTextA(hDlg, 0x429, "Bad Level File");
            }
            return 1;
        }
        case 0x111: // WM_COMMAND
            if (wParam == 1) {
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}

RVA(0x0003b7c0, 0x12c)
i32 LoadCustomWorldInfo(HWND hDlg) {
    char szLevel[0x100];
    char szDir[0x100];

    HWND hList = GetDlgItem(hDlg, 0x3fc);
    if (!hList) {
        return 0;
    }
    i32 sel = static_cast<i32>(SendMessageA(hList, 0x188 /*LB_GETCURSEL*/, 0, 0));
    if (sel == -1) {
        return 0;
    }
    if (static_cast<i32>(
            SendMessageA(hList, 0x189 /*LB_GETTEXT*/, sel, reinterpret_cast<LPARAM>(szLevel))
        )
        == -1) {
        return 0;
    }
    g_levelStr = szLevel;
    if (!_getcwd(szDir, 0xfe)) {
        return 0;
    }
    g_pathStr = szDir;
    g_pathStr += "\\Custom\\";
    g_pathStr += szLevel;
    g_pathStr += ".WWD";
    if (!FileExists(const_cast<char*>(static_cast<const char*>(g_pathStr)))) {
        g_pathStr.Empty();
        return 0;
    }
    DialogBoxParamA(
        g_customWorldInst,
        "CUSTOM_WORLDINFO",
        g_customWorldParent,
        CustomWorldInfoDlgProc, // retail stores the ILT thunk 0x305d, which jmps here
        0
    );
    return 1;
}

RVA(0x0003b940, 0x19d)
CString BuildCustomWwdPath(CString name) {
    if (name.GetLength() == 0) {
        return name;
    }
    if (strstr(name, "\\") != 0) {
        return name;
    }
    char cwd[254];
    if (_getcwd(cwd, 254) == 0) {
        return name;
    }
    CString orig = name;
    name = cwd;
    name += "\\CUSTOM\\";
    name += orig;
    name.MakeUpper();
    if (strstr(name, ".WWD") == 0) {
        name += ".WWD";
    }
    return name;
}

// ===========================================================================
// WwdFile::GetMapBaseName (static __cdecl, returns CString by value)
// Copy the path into the shared 0x62c010 scratch buffer, drop the 4-char
// extension (write a NUL at len-4 via the preceding 0x62c00c slot), then return
// the filename portion after the last backslash. Empty/short (<= 4 char) paths
// come back unchanged. The arg CString is taken by value (callee destroys it),
// and a working-copy CString temp carries the result, so cl emits the /GX frame.
// ===========================================================================
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
        g_mapNameBuf[blen - 4] = 0;
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
