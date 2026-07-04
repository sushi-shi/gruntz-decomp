#include <rva.h>
#include <Gruntz/GameRegistry.h>
// CustomWorldDialog.cpp - the "CUSTOM_WORLD" modal-dialog launcher (0x3ad90,
// __cdecl returning CString by value). Seeds the custom-world exchange globals
// from the game-manager singleton, runs the dialog through CGameRegistry::RunModalDialog,
// copies the chosen source name back through the optional out-param, and returns the
// selected-world name CString.
#include <Mfc.h> // CString (Empty / copy ctor / operator=)

#include <Ints.h>
#include <Globals.h>

// The CUSTOM_WORLDINFO dialog procedure handed to RunModalDialog (pushed code
// address, reloc-masked DIR32 against the named LAB_ symbol).
extern "C" void CustomWorldInfoDlgProc(); // LAB_0040357b

// The custom-world exchange block (separate engine globals; each reloc-masks to its
// own DAT_ symbol). 0x62c25c holds the selected-world name returned by value;
// 0x62c264 the source name copied back to the out-param; the three dwords carry the
// seeded manager state cleared after the dialog closes.
DATA(0x0022c25c)
extern CString g_str62c25c; // 0x62c25c
DATA(0x0022c264)
extern CString g_str62c264; // 0x62c264
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
    if (g_mgrSettings->RunModalDialog("CUSTOM_WORLD", (void*)CustomWorldInfoDlgProc, 0) == 0) {
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
