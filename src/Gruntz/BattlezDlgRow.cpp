// BattlezDlgRow.cpp - CBattlezDlg row enable/disable toggle (RVA 0x15fe0).
//
// For a battlez roster row: query its four child controls, and if the row's combo
// (control A) reports a selection, enable controls B/C/D and raise the row's
// +0x20 flag; otherwise disable them. Field names are placeholders; only offsets +
// code bytes are load-bearing.
#include <Gruntz/GruntzMgr.h> // m_slots' real type (the slot array is m_options[])
#include <rva.h>

#include <Mfc.h> // afx-first (TU pulls MFC via unified CObject; superset of Win32.h)

#include <Gruntz/Dialogs.h> // canonical CBattlezDlg (CDialog subclass) + CBattlezSlot

// SendMessageA is reached through a game-owned function pointer (ff 15).

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
        GruntzPlayer* rec = &m_slots->m_options[row];
        if (::SendMessageA(a->m_hWnd, 0x147, 0, 0) != 0) {
            b->EnableWindow(1);
            d->EnableWindow(1);
            rec->m_liveGate = 1;
            return c->EnableWindow(1);
        }
        b->EnableWindow(0);
        d->EnableWindow(0);
        rec->m_liveGate = 0;
        c = reinterpret_cast<CWnd*>(c->EnableWindow(0));
    }
    return reinterpret_cast<i32>(c);
}
