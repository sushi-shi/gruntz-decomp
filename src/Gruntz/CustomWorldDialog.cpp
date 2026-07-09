#include <rva.h>
#include <Gruntz/GameRegistry.h>
// CustomWorldDialog.cpp - the "CUSTOM_WORLD" modal-dialog launcher (0x3ad90,
// __cdecl returning CString by value). Seeds the custom-world exchange globals
// from the game-manager singleton, runs the dialog through CGameRegistry::RunModalDialog,
// copies the chosen source name back through the optional out-param, and returns the
// selected-world name CString.
#include <Mfc.h> // CString (Empty / copy ctor / operator=)

#include <Wwd/WwdFile.h> // WwdHeader (validated 0x5f4-byte .WWD header; FillLevelInfoDialog)
#include <Gruntz/CustomWorldInfoDlg.h> // WwdWorldHolder/WwdLevelInfoSrc (IsValidWwd receiver)
#include <Ints.h>
#include <Globals.h>
#include <stdio.h>  // sprintf (FillLevelInfoDialog)
#include <stdlib.h> // atoi (FillLevelInfoDialog)

// The CUSTOM_WORLD picker dialog procedure (0x3ae60, defined below) handed to
// RunModalDialog as a pushed code address (reloc-masked DIR32).
extern "C" INT_PTR CALLBACK CustomWorldDlgProc(HWND, UINT, WPARAM, LPARAM);

// The active modeless dialog HWND, cached on entry (shared NetLobby global @0x64557c).
namespace NetLobby {
    extern HWND g_curDlg_64557c;
}
// The picker's level listbox (id 0x3fc) cached at WM_INITDIALOG (DAT_0062c274); the
// dialog proc owns this cluster-local global.
DATA(0x0022c274)
extern HWND g_customLevelList; // 0x62c274

// The dialog proc's command dispatchers (reloc-masked externals): fill the level
// list / info pane, load the picked world's info popup, commit the selection.
i32 FillCustomLevelList(HWND hDlg);      // 0x3af90 (CustomLevelList.cpp)
i32 LoadCustomWorldInfo(HWND hDlg);      // 0x3b7c0 (CustomWorldInfoDlg.cpp)
i32 FillLevelInfoDialog(HWND hDlg);      // 0x3b1a0
i32 LoadCustomWorldSelection(HWND hWnd); // 0x3b310 (below)

// The custom-world exchange block (separate engine globals; each reloc-masks to its
// own DAT_ symbol). 0x62c25c holds the selected-world name returned by value;
// 0x62c264 the source name copied back to the out-param; the three dwords carry the
// seeded manager state cleared after the dialog closes.
DATA(0x0022c25c)
extern CString g_str62c25c; // 0x62c25c
DATA(0x0022c264)
extern CString g_str62c264; // 0x62c264

// InitStr62c264 @0x03acb0 - the dynamic initializer that default-constructs the global
// CString g_str62c264 in place (explicit-ctor-call tail-jmp; see
// docs/patterns/explicit-ctor-call-inplace-tail-jmp.md). Re-homed from
// src/Stub/BoundaryLowerThunks.cpp (was StrFree3acb0).
RVA(0x0003acb0, 0xa)
void InitStr62c264() {
    g_str62c264.CString::CString();
}

DATA(0x0022c26c)
extern i32 g_dat62c26c; // 0x62c26c
DATA(0x0022c270)
extern i32 g_dat62c270; // 0x62c270

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
extern "C" CGameRegistry* g_mgrSettings;

// ===========================================================================
// run the custom-world dialog. Seed +0x62c26c with the supplied id (or
// the manager's resolved default when 0), +0x62c268 with m_30, +0x62c270 with
// m_8->m_c; run the dialog; on cancel (result 0) clear the returned name; clear the
// three seed dwords; copy g_str62c264 into the out-param when present; return the
// selected-world name by value.
// ===========================================================================
// @early-stop
// ~73% (extra-dead-local frame wall + cross-TU reloc-name tail): the control flow
// and every operation match retail in order (verified instruction-by-instruction).
// Two residuals, neither source-steerable from here: (1) retail reserves and zeroes
// one extra dead stack dword (`push ecx; mov [esp+4],0`) - an artifact of the
// original source's local layout that shifts every [esp+N] frame offset by 4; a
// named-return-value local instead ADDS a CString temp+dtor (59%), so the direct
// `return g_str62c25c` is the closest shape. (2) Five callee/global relocs
// (CString::Empty/operator=/copy-ctor, CGameRegistry::RunModalDialog, the dialog proc)
// resolve to the delinker's Ghidra simple-labels ("Empty"/"operator="/"CString"/
// "RunModalDialog"/"LoadGruntzPalette") which a foreign TU cannot name-match; they
// go exact once those library/manager functions are reconstructed. Logic complete;
// deferred to the final sweep.
RVA(0x0003ad90, 0x97)
CString RunCustomWorldDialog(i32 id, CString* outSource) {
    g_str62c25c.Empty();
    i32 v = id;
    if (id == 0) {
        v = ((GmInner4*)g_mgrSettings->m_gameWnd)->m_4;
    }
    g_dat62c26c = v;
    g_dat62c268 = (i32)g_mgrSettings->m_world;
    g_dat62c270 = ((GmInner8*)g_mgrSettings->m_owner)->m_c;
    if (g_mgrSettings->RunModalDialog("CUSTOM_WORLD", (void*)CustomWorldDlgProc, 0) == 0) {
        g_str62c25c.Empty();
    }
    g_dat62c268 = 0;
    g_dat62c26c = 0;
    g_dat62c270 = 0;
    if (outSource != 0) {
        *outSource = g_str62c264;
    }
    return g_str62c25c;
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
                FillCustomLevelList(hDlg);
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
// selected world's .WWD header (path in g_str62c25c); on a bad/invalid file, stamp
// every item "Bad Level File". A free __cdecl(HWND) helper the picker DlgProc's
// LBN_SELCHANGE notification calls. Re-homed from src/Stub/ApiWrappers.cpp; the
// LevelInfo placeholder view folded onto the real WwdHeader (the +0x10 levelName
// block, sliced +0x40/+0x80) and the IsValidWwd call restored to its true form:
// a __thiscall on g_mgrSettings->m_world->m_24 (the sibling CustomWorldInfoDlgProc
// pattern), with the USER32 calls routed through the cached fn-pointers.
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
    if (((WwdWorldHolder*)g_mgrSettings->m_world)
            ->m_24->IsValidWwd((const char*)g_str62c25c, &info)) {
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

// The game-root-dir resolver + the OpenFile(OF_EXIST) probe RunCustomWorldSelection
// funnels through (reloc-masked externals).
i32 GetGameRootDir_11fc10(char* buf, i32 size); // 0x11fc10 (__cdecl)
i32 FileExists_4282(const char* path);          // 0x4282  (__cdecl)

// ===========================================================================
// 0x3b310: build "<gameroot>\Custom\<selected>.WWD" for the custom-world list's
// current selection into g_str62c25c and stash the bare name in g_str62c264;
// returns 1 iff a valid item is selected and its .WWD file exists. Re-homed from
// src/Stub/ApiCallers.cpp; de-hacked the GameCStr placeholder view to the real
// CString exchange globals (in-place operator= / operator+= / Empty).
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
    if (!GetGameRootDir_11fc10(dirBuf, 0xfe)) {
        return 0;
    }
    g_str62c25c = dirBuf;
    g_str62c25c += "\\Custom\\";
    g_str62c25c += itemText;
    g_str62c25c += ".WWD";
    if (!FileExists_4282(g_str62c25c)) {
        g_str62c25c.Empty();
        return 0;
    }
    g_str62c264 = itemText;
    return 1;
}

SIZE_UNKNOWN(GmInner4);
SIZE_UNKNOWN(GmInner8);
