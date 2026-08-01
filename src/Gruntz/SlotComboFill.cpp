#include <Net/LatencyList.h> // CLatencyList / CLatencyItem (+ <Mfc.h>: CPtrList/CString/windows.h)
#include <rva.h>
#include <MsgParam.h> // the window-message parameter's pointer/word pair

// The control lookup + the four combo messages go through the engine's cached USER32
// function-pointer globals (::GetDlgItem / ::SendMessageA), not the raw imports:
// retail has no free callee-saved register here (this/next/data hold them), so each
// ::SendMessageA call is an uncached memory-indirect `ff 15 [::SendMessageA]` (the
// global is called directly, never hoisted into a register).
// @early-stop
RVA(0x00037ff0, 0xe7)
i32 CLatencyList::FillCombo(HWND hDlg, i32 ctrlId) {
    if (m_list.GetCount() > 0) {
        HWND combo = ::GetDlgItem(hDlg, ctrlId);
        if (combo != 0) {
            ::SendMessageA(combo, CB_RESETCONTENT, 0, 0);
            POSITION pos = m_list.GetHeadPosition();
            while (pos != 0) {
                CLatencyItem* rec = static_cast<CLatencyItem*>(m_list.GetNext(pos));
                i32 data = ((rec->m_param & 0xffff) << 16) | (rec->m_id & 0xffff);
                i32 idx;
                {
                    MsgParam name;
                    idx = ::SendMessageA(
                        combo,
                        CB_ADDSTRING,
                        0,
                        (name.m_str = static_cast<LPCTSTR>(rec->GetName()), name.m_lparam)
                    );
                }
                if (idx != -1) {
                    ::SendMessageA(combo, CB_SETITEMDATA, idx, data);
                }
            }
            return m_list.GetCount();
        }
    }
    return 0;
}

// 0x38120 (re-homed from src/Stub/BoundaryTail.cpp): CLatencyItem::GetName - return
// the row-label CString member (offset 0) by value. Called by FillCombo above.
RVA(0x00038120, 0x1d)
CString CLatencyItem::GetName() {
    return m_text;
}

RVA(0x00038150, 0x91)
i32 CLatencyList::SelectItem(HWND hDlg, i32 id, i32 lo, i32 hi) {
    HWND list = ::GetDlgItem(hDlg, id);
    if (!list) {
        return 0;
    }
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = ::SendMessageA;
    i32 searching = 1;
    i32 i = 0;
    while (searching) {
        i32 data = pSend(list, 0x150, i, 0);
        if (data != -1) {
            i32 itemLo = data & 0xffff;
            i32 itemHi = static_cast<u32>(data) >> 0x10;
            if (itemLo == lo && itemHi == hi) {
                if (pSend(list, 0x147, 0, 0) != i) {
                    pSend(list, 0x14e, i, 0);
                }
                return 1;
            }
        } else {
            searching = 0;
        }
        i++;
    }
    return 0;
}
