#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <MsgParam.h>
#include <Net/LatencyList.h>

// @early-stop
RVA(0x00037ff0, 0xe7)
i32 CLatencyList::FillCombo(HWND hDlg, i32 ctrlId) {
    if (m_list.GetCount() > 0) {
        HWND combo = GetDlgItem(hDlg, ctrlId);
        if (combo != NULL) {
            SendMessageA(combo, CB_RESETCONTENT, 0, 0);
            POSITION pos = m_list.GetHeadPosition();
            while (pos != NULL) {
                CKeyedNode* rec = static_cast<CKeyedNode*>(m_list.GetNext(pos));
                i32 data = ((rec->m_drainReload & 0xffff) << 16) | (rec->m_commandDelay & 0xffff);
                i32 idx;
                {
                    MsgParam name;
                    idx = SendMessageA(
                        combo,
                        CB_ADDSTRING,
                        0,
                        (name.m_str = static_cast<LPCTSTR>(rec->GetName()), name.m_lparam)
                    );
                }
                if (idx != -1) {
                    SendMessageA(combo, CB_SETITEMDATA, idx, data);
                }
            }
            return m_list.GetCount();
        }
    }
    return 0;
}

RVA(0x00038120, 0x1d)
CString CKeyedNode::GetName() {
    return m_key;
}

RVA(0x00038150, 0x91)
i32 CLatencyList::SelectItem(HWND hDlg, i32 id, i32 lo, i32 hi) {
    HWND list = GetDlgItem(hDlg, id);
    if (!list) {
        return 0;
    }
    LRESULT(WINAPI * pSend)(HWND, UINT, WPARAM, LPARAM) = SendMessageA;
    i32 searching = 1;
    i32 i = 0;
    while (searching) {
        i32 data = pSend(list, CB_GETITEMDATA, i, 0);
        if (data != -1) {
            i32 itemLo = data & 0xffff;
            i32 itemHi = static_cast<u32>(data) >> 0x10;
            if (itemLo == lo && itemHi == hi) {
                if (pSend(list, CB_GETCURSEL, 0, 0) != i) {
                    pSend(list, CB_SETCURSEL, i, 0);
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
