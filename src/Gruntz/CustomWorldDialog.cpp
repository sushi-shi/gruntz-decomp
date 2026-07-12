// CustomWorldDialog.cpp - the custom-world picker feature file (C:\Proj\Gruntz):
// ONE original obj `[0x3ac30 .. 0x3bc78]` (dossier #16, waveM-judgment). Holds the
// CUSTOM_WORLD modal launcher + its DlgProc, the level-list filler, the level-info
// pane filler, the CUSTOM_WORLDINFO popup + its loader, the custom-path builder,
// and the two WwdFile static helpers (ValidateMainBlock / GetMapBaseName) whose
// birth positions are woven into this block.
//
// Merge evidence (initialized-.data privates + text weave): private cell 0x20cfa4
// is shared by FillLevelInfoDialog + CustomWorldInfoDlgProc; 0x20cfbc by
// LoadCustomWorldSelection + LoadCustomWorldInfo + BuildCustomWwdPath; 0x20cfc4 by
// LoadCustomWorldSelection + LoadCustomWorldInfo; the adjacent .bss static band
// 0x22c25c-0x22c274 is read intermixed by the ex-customworlddialog and
// ex-customworldinfodlg fns. The ex units customworldinfodlg / customlevellist /
// customwwdpath dissolved here; wwdfile's two in-block statics moved in.
// /GX per the EH prologues at 0x3b470 / 0x3b940 / 0x3bb50.
//
// The head statics: g_pathStr @0x62c25c (frag i513 + atexit-style reset 0x3ac30),
// g_str62c264 @0x62c264 (frag i514 -> InitStr62c264 0x3acb0), g_levelStr @0x62c260
// (frag i515 -> FreeLevelStr 0x3ad30). All three are MFC CStrings: the ex-GameKeyStr
// view's methods were the real CString entry points (Set=??4 0x1b9e74,
// Append=?+= 0x1ba0c8, Reset=?Empty 0x1b9c69, Free1b9b93=??0 0x1b9b93 -
// config/library_labels.csv, all anchored), so GameKeyStr dissolved to CString.
#include <Mfc.h> // afx-first: CString + the dialog API

#include <Gruntz/CustomWorldInfoDlg.h> // WwdWorldHolder/WwdLevelInfoSrc (IsValidWwd receiver)
#include <Gruntz/GameRegistry.h> // the canonical manager singleton view (m_gameWnd/m_world/m_owner)
#include <Gruntz/GruntzMgr.h>    // the MFC view of the same singleton (IsBattlezMapFile)
#include <Wwd/WwdFile.h>         // WwdHeader + WwdFile statics + WwdFile_CheckHeader
#include <Globals.h>             // g_dat62c268 / g_mapNameBuf / g_mapNamePre
#include <Ints.h>
#include <rva.h>

#include <direct.h> // _getcwd (BuildCustomWwdPath)
#include <io.h>     // _finddata_t / _findfirst / _findnext (FillCustomLevelList)
#include <stdio.h>  // sprintf
#include <stdlib.h> // atoi
#include <string.h> // strstr / inline strcpy-strlen

// The picker's dialog procs (defined below in RVA order).
extern "C" INT_PTR CALLBACK CustomWorldDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK CustomWorldInfoDlgProc(HWND, UINT, WPARAM, LPARAM);

// The active modeless dialog HWND, cached on entry (shared NetLobby global @0x64557c).
namespace NetLobby {
    extern HWND g_curDlg_64557c;
}
// The picker's level listbox (id 0x3fc) cached at WM_INITDIALOG (DAT_0062c274).
DATA(0x0022c274)
extern HWND g_customLevelList; // 0x62c274

// The custom-world exchange globals (this file's statics; see the header comment).
// g_pathStr carries the selected world's full path (returned by value from the
// launcher); g_levelStr the popup's level name; g_str62c264 the source name copied
// back to the launcher's out-param.
DATA(0x0022c25c)
extern CString g_pathStr; // 0x62c25c
DATA(0x0022c260)
extern CString g_levelStr; // 0x62c260
DATA(0x0022c264)
extern CString g_str62c264; // 0x62c264

// The launcher's parent-window/instance exchange slots (seeded from the manager,
// cleared after the dialog closes; the popup's DialogBoxParamA reads them).
DATA(0x0022c26c)
extern HWND g_customWorldParent; // 0x62c26c
DATA(0x0022c270)
extern HINSTANCE g_customWorldInst; // 0x62c270

// The launcher's command dispatchers (defined below in RVA order).
namespace m4 {
    i32 FillCustomLevelList(HWND hWnd); // 0x3af90
}
i32 LoadCustomWorldInfo(HWND hDlg);      // 0x3b7c0
i32 FillLevelInfoDialog(HWND hDlg);      // 0x3b1a0
i32 LoadCustomWorldSelection(HWND hWnd); // 0x3b310

// Manager member chains the seed reads (placeholder offsets).
struct GmInner4 {
    char m_pad0[4];
    i32 m_4; // +0x04
};
struct GmInner8 {
    char m_pad0[0xc];
    i32 m_c; // +0x0c
};

// The game-manager singleton (*0x64556c). Only the seed members + the modal-dialog
// runner are modeled; RunModalDialog (0x90260, __thiscall) is reloc-masked. The obj
// names the pointer _g_mgrSettings (extern "C"), matching the codebase convention
// for 0x64556c.
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg;

// The "game root dir" the loaders resolve is just the current working directory:
// 0x11fc10 is the CRT _getcwd (LIBCMT __getcwd), the same routine BuildCustomWwdPath
// (below) and FecCrypt call; the former GetGameDir decl was a fake alias of it.
// The OpenFile(OF_EXIST) existence probe (FUN_00004282) is reloc-masked.
i32 FileExists(char* path); // 0x1189c0 (heapdiag; "PathFileExists 0x4282" was a thunk to it)

// FreeGlobal62c25c @0x03ac30 - reset the g_pathStr global in place (the
// explicit-ctor-call tail-jmp to ??0CString@@QAE@XZ; the file's leading static,
// frag i513). Re-homed from OrphanLeaves.cpp (the unit split was an aggregation
// artifact - dossier #16).
RVA(0x0003ac30, 0xa)
void FreeGlobal62c25c() {
    g_pathStr.CString::CString();
}

// InitStr62c264 @0x03acb0 - the dynamic initializer that default-constructs the
// global CString g_str62c264 in place (explicit-ctor-call tail-jmp; see
// docs/patterns/explicit-ctor-call-inplace-tail-jmp.md).
RVA(0x0003acb0, 0xa)
void InitStr62c264() {
    g_str62c264.CString::CString();
}

// FreeLevelStr @0x03ad30 - reconstruct the global g_levelStr in place (same
// ??0CString@@QAE@XZ tail-jmp as its two sibling statics).
RVA(0x0003ad30, 0xa)
void FreeLevelStr() {
    g_levelStr.CString::CString();
}

// ===========================================================================
// run the custom-world dialog. Seed 0x62c26c with the supplied parent (or the
// manager's game-window HWND when 0), 0x62c268 with m_30, 0x62c270 with
// m_8->m_c (the HINSTANCE); run the dialog; on cancel (result 0) clear the
// returned name; clear the three seed slots; copy g_str62c264 into the out-param
// when present; return the selected-world path by value.
// ===========================================================================
// @early-stop
// ~95.7% under this obj's real /GX profile (the old ~73% "extra dead stack dword"
// residual was the missing EH-adjacent frame slot - resolved by the merged unit's
// eh flags, dossier #16). Remaining residual is the cross-TU reloc-name tail:
// the CString::Empty/operator=/copy-ctor + RunModalDialog relocs resolve to the
// delinker's Ghidra simple-labels which a foreign TU cannot name-match; they go
// exact once those library/manager functions are reconstructed. Logic complete.
RVA(0x0003ad90, 0x97)
CString RunCustomWorldDialog(i32 id, CString* outSource) {
    g_pathStr.Empty();
    i32 v = id;
    if (id == 0) {
        v = ((GmInner4*)g_gameReg->m_gameWnd)->m_4;
    }
    g_customWorldParent = (HWND)v;
    g_dat62c268 = (i32)g_gameReg->m_world;
    g_customWorldInst = (HINSTANCE)((GmInner8*)g_gameReg->m_owner)->m_c;
    if (g_gameReg->RunModalDialog("CUSTOM_WORLD", (void*)CustomWorldDlgProc, 0) == 0) {
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

// ===========================================================================
// CustomWorldDlgProc @0x03ae60 - the CUSTOM_WORLD level-picker dialog proc.
// ===========================================================================
// WM_INITDIALOG caches the level listbox (id 0x3fc) and fills it. WM_COMMAND: the
// Cancel button (2) ends the dialog; button 0x42a pops the world-info popup; the OK
// button (1) commits the selection and ends the dialog; a notification from the
// listbox re-fills the info pane on select-change (LBN_SELCHANGE=1) or simulates OK
// on double-click (LBN_DBLCLK=2 -> PostMessage WM_COMMAND/1).
RVA(0x0003ae60, 0xec)
extern "C" INT_PTR CALLBACK CustomWorldDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    NetLobby::g_curDlg_64557c = hDlg;
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
            if (g_customLevelList != 0 && lParam == (LPARAM)g_customLevelList) {
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

// ===========================================================================
// FillCustomLevelList @0x3af90 - fill the custom-level listbox.
// ===========================================================================
// clear listbox 0x3fc, bail if the "Custom" gate is set, then walk the
// custom-level directory glob (_findfirst/_findnext under a shared singleton lock),
// format each entry's display name, ask the settings manager whether to hide it,
// and LB_ADDSTRING it with its 4-char extension stripped. __cdecl(HWND).
namespace m4 {

    // Game Win32 pointer table (reloc-masked indirect calls).
    extern HWND(WINAPI* g_pGetDlgItem)(HWND, int);                       // 0x006c4564
    extern LRESULT(WINAPI* g_pSendMessageA)(HWND, UINT, WPARAM, LPARAM); // 0x006c44a4

    // The directory walk is the real CRT _findfirst/_findnext (0x11f900 / 0x11fa30,
    // FID-carved) over <io.h>'s _finddata_t.
    extern "C" i32 CustomGate(const char* name); // 0x0018d290

    // The shared reentrancy lock guarding the directory walk.
    struct WalkLock {
        void Lock();   // 0x001beafb
        void Unlock(); // 0x001beb10
    };
    struct WalkOwner {
        char m_pad0[4];
        WalkLock* m_4; // +0x04
    };
    extern "C" WalkOwner* GetWalkOwner1d3631(); // 0x001d3631

    // The settings-manager singleton == *g_gameReg (the real CGruntzMgr, the MFC view
    // of the file-global g_gameReg above); IsBattlezMapFile takes the display
    // name by value (callee destroys). C++-namespaced here (its OWN symbol) so the one
    // file-scope extern "C" _g_gameReg keeps the single global-scope DATA binding and the
    // two typed views coexist without an extern "C" type clash (clang -emit-llvm).
    extern CGruntzMgr* g_gameReg; // 0x0064556c

    // The custom-level glob + display-name format + "already loaded" strings.
    extern char g_customGlob[]; // 0x0060cf94
    extern char g_customDone[]; // 0x0060cf90
    extern char g_nameFmt[];    // 0x0060c5b8

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
        HWND lb = g_pGetDlgItem(hWnd, 0x3fc);
        if (!lb) {
            return 0;
        }
        g_pSendMessageA(lb, 0x184, 0, 0); // LB_RESETCONTENT
        if (CustomGate("Custom")) {
            return 0;
        }
        char pattern[260];
        strcpy(pattern, g_customGlob);
        _finddata_t fd;
        i32 h = _findfirst(pattern, &fd);
        i32 found = (h != -1);
        GetWalkOwner1d3631()->m_4->Lock();
        if (found) {
            do {
                char disp[260];
                sprintf(disp, g_nameFmt, fd.name);
                if (!g_gameReg->IsBattlezMapFile(CString(disp))) {
                    i32 len = strlen(disp);
                    if (len > 4) {
                        disp[len - 4] = 0;
                    }
                    g_pSendMessageA(lb, 0x180, 0, (LPARAM)disp); // LB_ADDSTRING
                }
            } while (_findnext(h, &fd) != -1);
        }
        CustomGate(g_customDone);
        GetWalkOwner1d3631()->m_4->Unlock();
        return 1;
    }

} // namespace m4

// FillLevelInfoDialog reaches USER32 through the game's cached fn-pointers (bare
// 0x6c45xx absolutes, no import symbols) - the same pattern as g_pPostMessageA.
DATA(0x002c4564)
extern HWND(WINAPI* g_pGetDlgItem)(HWND, int);
DATA(0x002c4554)
extern BOOL(WINAPI* g_pSetDlgItemTextA)(HWND, int, LPCSTR);
// The listbox-selection precheck (0x2176, cdecl).
extern "C" i32 func_2176(HWND hDlg);

// ===========================================================================
// FillLevelInfoDialog @0x3b1a0 - populate the four level-info dialog items from the
// selected world's .WWD header (path in g_pathStr); on a bad/invalid file, stamp
// every item "Bad Level File". A free __cdecl(HWND) helper the picker DlgProc's
// LBN_SELCHANGE notification calls.
// ===========================================================================
// @early-stop
// regalloc/scheduling wall (~94%): the IsValidWwd this-chain + method call and every
// cached-ptr USER32 call now match retail's instruction selection (llvm-objdump -dr).
// Residual is a symmetric esi<->edi coloring swap (retail edi=hDlg / esi=SetDlgItemTextA
// ptr; cl picks esi=hDlg / edi=ptr) plus the ptr load being hoisted to the `setText`
// local vs retail's lazy per-branch CSE. Binary-correct shape kept over the coincidentally
// higher-% free-call+import form the stub carried (correctness-not-artifacts).
RVA(0x0003b1a0, 0x118)
i32 FillLevelInfoDialog(HWND hDlg) {
    if (!g_pGetDlgItem(hDlg, 0x3fc)) {
        return 0;
    }
    if (!func_2176(hDlg)) {
        return 0;
    }
    char num[0x20];
    WwdHeader info;
    BOOL(WINAPI * setText)(HWND, int, LPCSTR) = g_pSetDlgItemTextA;
    if (((WwdWorldHolder*)g_gameReg->m_world)->m_24->IsValidWwd((const char*)g_pathStr, &info)) {
        char* p = info.levelName;
        while (*p && (*p < '0' || *p > '9')) {
            p++;
        }
        sprintf(num, "%d", atoi(p));
        setText(hDlg, 0x408, (const char*)g_str62c264);
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

// ===========================================================================
// 0x3b310: build "<gameroot>\Custom\<selected>.WWD" for the custom-world list's
// current selection into g_pathStr and stash the bare name in g_str62c264;
// returns 1 iff a valid item is selected and its .WWD file exists.
// ===========================================================================
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
    if (SendMessageA(lb, 0x189, sel, (LPARAM)itemText) == -1) {
        return 0;
    }
    if (!_getcwd(dirBuf, 0xfe)) {
        return 0;
    }
    g_pathStr = dirBuf;
    g_pathStr += "\\Custom\\";
    g_pathStr += itemText;
    g_pathStr += ".WWD";
    if (!FileExists((char*)(const char*)g_pathStr)) {
        g_pathStr.Empty();
        return 0;
    }
    g_str62c264 = itemText;
    return 1;
}

// ---------------------------------------------------------------------------
// The manager world slot's +0x24 as ValidateMainBlock reads it (a filename the
// header probe validates). authentic: reduced local view of the cross-TU world
// slot; only the +0x24 path field is modeled (offset-faithful, mangling-neutral).
struct WwdGameRegSlot {
    char pad_0[0x24];
    char* m_wwdPath; // +0x24  a WWD path / numeric-tail string CheckHeader validates
};

// ---------------------------------------------------------------------------
// WwdFile::ValidateMainBlock (static, __cdecl: ignores `this`, caller-cleaned
// `ret`; Ghidra mis-derived the void/no-arg `QAEXXZ` prototype).
// Takes a CString BY VALUE (the callee runs its dtor on every exit). Returns -1
// for the three reject paths, else the integer parsed from the first digit run
// of the validated header:
//   1. the CString must be non-empty (its length, at pszData-8, != 0);
//   2. ((WwdGameRegSlot*)g_gameReg->m_world)->m_wwdPath must be non-null;
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

// ===========================================================================
// CustomWorldInfoDlgProc @0x03b600 - the CUSTOM_WORLDINFO level-info popup proc.
// ===========================================================================
// WM_INITDIALOG validates the selected .WWD (the g_pathStr full path exists AND the
// world's level-info source parses its header); on success it fills the four info
// items - the level name (0x408 = g_levelStr), the header's author/paths sub-strings
// (0x428/0x429 = the +0x50/+0x90 slices of the name block), and a numeric field
// (0x40c) parsed out of the +0x10 version string; on any failure every item reads
// "Bad Level File". WM_COMMAND ends the dialog on OK (1).
RVA(0x0003b600, 0x15f)
INT_PTR CALLBACK CustomWorldInfoDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110: { // WM_INITDIALOG
            WwdHeader info;
            char num[0x20];
            i32 bad = 1;
            if (g_dat62c268 != 0 && FileExists((char*)(const char*)g_pathStr)
                && ((WwdWorldHolder*)g_dat62c268)
                       ->m_24->IsValidWwd((const char*)g_pathStr, &info)) {
                SetDlgItemTextA(hDlg, 0x408, (const char*)g_levelStr);
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

// ===========================================================================
// LoadCustomWorldInfo (0x3b7c0) - reads the level name from the dialog's listbox
// (id 0x3fc), builds "<gameDir>\Custom\<level>.WWD" through the exchange CStrings,
// and if the file exists pops the CUSTOM_WORLDINFO dialog.
// ===========================================================================
RVA(0x0003b7c0, 0x12c)
i32 LoadCustomWorldInfo(HWND hDlg) {
    char szLevel[0x100];
    char szDir[0x100];

    HWND hList = GetDlgItem(hDlg, 0x3fc);
    if (!hList) {
        return 0;
    }
    i32 sel = (i32)SendMessageA(hList, 0x188 /*LB_GETCURSEL*/, 0, 0);
    if (sel == -1) {
        return 0;
    }
    if ((i32)SendMessageA(hList, 0x189 /*LB_GETTEXT*/, sel, (LPARAM)szLevel) == -1) {
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
    if (!FileExists((char*)(const char*)g_pathStr)) {
        g_pathStr.Empty();
        return 0;
    }
    DialogBoxParamA(
        g_customWorldInst,
        "CUSTOM_WORLDINFO",
        g_customWorldParent,
        (DLGPROC)CustomWorldInfoDlgProc,
        0
    );
    return 1;
}

// ===========================================================================
// BuildCustomWwdPath @0x03b940 - the custom-level path resolver (__cdecl, returns
// a CString by value). Given a bare WWD name, rewrite it to an absolute custom
// path "<cwd>\CUSTOM\<NAME>.WWD" (upper-cased, ".WWD" appended if absent). Names
// that are empty, already contain a backslash (already a path), or hit a getcwd
// failure are returned unchanged. /GX: the by-value CString param + the inner
// `orig` copy are destructible.
// ===========================================================================
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

SIZE_UNKNOWN(GmInner4);
SIZE_UNKNOWN(GmInner8);
SIZE_UNKNOWN(WwdGameRegSlot);
