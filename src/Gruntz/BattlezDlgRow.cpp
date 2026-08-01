#include <Gruntz/GruntzMgr.h>
#include <rva.h>

#include <Mfc.h>

#include <Gruntz/Dialogs.h>

RVA(0x00015fe0, 0xbe)
void CBattlezDlg::ToggleRow(i32 row) {
    CWnd* a = GetCtrlA(row);
    CWnd* b = GetCtrlB(row);
    CWnd* d = GetCtrlD(row);
    CWnd* c = GetCtrlC(row);
    if (row == 0) {
        return;
    }
    GruntzPlayer* rec = &m_slots->m_options[row];
    if (::SendMessageA(a->m_hWnd, 0x147, 0, 0) != 0) {
        b->EnableWindow(1);
        d->EnableWindow(1);
        rec->m_liveGate = 1;
        c->EnableWindow(1);
        return;
    }
    b->EnableWindow(0);
    d->EnableWindow(0);
    rec->m_liveGate = 0;
    c->EnableWindow(0);
}
