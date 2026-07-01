// MultiStartDlgWorld.cpp - CMultiStartDlg::SetupWorldCombo (0xc1840), a /GX EH
// method of the multiplayer-start dialog. It fills the 0x4ff "world" combo from
// the GAME_MULTI registry path (each entry name uppercased through a scratch
// CString), makes the combo's edit child read-only, selects the first item, and
// subclasses the edit child's window-proc (saving the old proc in g_64bdc0).
//
// Homed in its own eh unit (not Dialogs.cpp) so it can't perturb that TU's parked
// dtors; it reuses the shared Dialogs.h class models. Every callee is a NAFXCW /
// engine / import thunk that reloc-masks; the "GAME_MULTI" $SG literal reloc-masks
// against the matched string symbol; only offsets + code bytes are load-bearing.
#include <Gruntz/Dialogs.h>
#include <rva.h>

// The old edit-child window-proc snapshot the subclass saves (reloc-masked DATA).
DATA(0x0024bdc0)
extern i32 g_64bdc0; // DAT_0064bdc0

// The subclass window-proc installed on the combo's edit child (0x4c1a10). Only
// its address is taken (push offset -> DIR32 reloc-masks).
extern "C" i32 __stdcall WndProc_c1a10(HWND__* hWnd, u32 msg, u32 wParam, i32 lParam);

// The GAME_MULTI registry path -> a name registry (m_5c is the CNetDlgHost* the
// ctor stored as i32; its m_34 is the registry). ResolvePath returns a symbol
// table iterated by FirstSym/NextSym2 (first entry) then NextSym3 (advance).
struct MpSymItem {
    char* m_0; // +0x00  entry name (LPCSTR)
};
struct MpSymTable {
    void* FirstSym();                  // 0x13a2b0 __thiscall
    MpSymItem* NextSym2(void* pos);    // 0x13a2f0 __thiscall
    MpSymItem* NextSym3(MpSymItem* p); // 0x13a310 __thiscall
};
struct MpWorldReg {
    MpSymTable* ResolvePath(const char* path); // 0x13c030 __thiscall
};
struct MpDlgHost {
    char m_pad0[0x34];
    MpWorldReg* m_34; // +0x34
};

RVA(0x000c1840, 0x16e)
i32 CMultiStartDlg::SetupWorldCombo() {
    CWnd* combo = GetDlgItem(0x4ff);
    if (combo == 0) {
        return 0;
    }
    MpSymTable* st = ((MpDlgHost*)m_5c)->m_34->ResolvePath("GAME_MULTI");
    if (st == 0) {
        return 0;
    }
    MpSymItem* item = st->NextSym2(st->FirstSym());
    while (item != 0) {
        CString name(item->m_0);
        name.MakeUpper();
        SendMessageA(combo->m_hWnd, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)name);
        item = st->NextSym3(item);
    }
    CWnd* combo2 = GetDlgItem(0x4ff);
    CWnd* child = CWnd::FromHandle(GetWindow(combo2->m_hWnd, GW_CHILD));
    if (child == 0) {
        return 0;
    }
    SendMessageA(child->m_hWnd, EM_SETREADONLY, 1, 0);
    SendMessageA(combo->m_hWnd, CB_SETCURSEL, 0, 0);
    HWND__* h = child->m_hWnd;
    g_64bdc0 = GetWindowLongA(h, GWL_WNDPROC);
    SetWindowLongA(h, GWL_WNDPROC, (i32)WndProc_c1a10);
    Sub_c3e30();
    return 1;
}
