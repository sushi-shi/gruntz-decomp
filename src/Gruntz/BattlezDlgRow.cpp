// BattlezDlgRow.cpp - CBattlezDlg row enable/disable toggle (RVA 0x15fe0).
//
// For a battlez roster row: query its four child controls, and if the row's combo
// (control A) reports a selection, enable controls B/C/D and raise the row's
// +0x20 flag; otherwise disable them. Field names are placeholders; only offsets +
// code bytes are load-bearing.
#include <rva.h>

#include <Mfc.h> // afx-first (TU pulls MFC via unified CObject; superset of Win32.h)

#include <Gruntz/Wnd.h>

// SendMessageA is reached through a game-owned function pointer (ff 15).

class CBattlezDlg {
public:
    CWnd* GetCtrlA(i32 row); // 0x01e7e
    CWnd* GetCtrlB(i32 row); // 0x02770
    CWnd* GetCtrlC(i32 row); // 0x033a0
    CWnd* GetCtrlD(i32 row); // 0x02c52
    i32 ToggleRow(i32 row);

    char m_pad00[0x5c];
    char* m_slots; // +0x5c row-record (slot-array) base
};

// @early-stop
// /O2 regalloc wall: body byte-exact except MSVC pins control B->ebx, D->ebp and
// spills A/C, while the recompile pins B->ebp, C->ebx and spills A/D (the returned
// `c` is live-out so the recompile keeps it in a callee-saved reg; retail spills
// it). Logic + externs match retail. ~82%; final sweep.
RVA(0x00015fe0, 0xbe)
i32 CBattlezDlg::ToggleRow(i32 row) {
    CWnd* a = GetCtrlA(row);
    CWnd* b = GetCtrlB(row);
    CWnd* d = GetCtrlD(row);
    CWnd* c = GetCtrlC(row);
    if (row != 0) {
        char* rec = m_slots + row * 0x238 + 0x150;
        if (::SendMessageA(a->m_hWnd, 0x147, 0, 0) != 0) {
            b->EnableWindow(1);
            d->EnableWindow(1);
            *(i32*)(rec + 0x20) = 1;
            return c->EnableWindow(1);
        }
        b->EnableWindow(0);
        d->EnableWindow(0);
        *(i32*)(rec + 0x20) = 0;
        c = (CWnd*)c->EnableWindow(0);
    }
    return (i32)c;
}
