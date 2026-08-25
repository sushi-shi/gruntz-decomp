#include <rva.h>

#include <Net/LatencyList.h>

#include <Mfc.h>

#include <Enums.h>
#include <MsgParam.h>

#include <stddef.h>

RVA(0x00037b40, 0xb3)
i32 CLatencyList::PopulateIpxOptions() {
    if (!AddNode("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddNode("Very Low Latency [ping < 50]", 2, 10)) {
        return 0;
    }
    if (!AddNode("Low Latency [ping < 100]", 4, 10)) {
        return 0;
    }
    if (!AddNode("Medium Latency [ping < 200]", 6, 10)) {
        return 0;
    }
    if (!AddNode("Medium-High [ping < 250]", 8, 10)) {
        return 0;
    }
    if (!AddNode("High Latency [ping < 400]", 12, 10)) {
        return 0;
    }
    if (!AddNode("Very High Latency [ping < 550]", 16, 10)) {
        return 0;
    }
    return AddNode("Last Resort", 24, 10) != NULL;
}

RVA(0x00037c30, 0xb3)
i32 CLatencyList::PopulateTcpIpOptions() {
    if (!AddNode("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddNode("Very Low Latency [ping < 50]", 2, 10)) {
        return 0;
    }
    if (!AddNode("Low Latency [ping < 100]", 4, 10)) {
        return 0;
    }
    if (!AddNode("Medium Latency [ping < 200]", 6, 20)) {
        return 0;
    }
    if (!AddNode("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddNode("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddNode("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddNode("Last Resort", 24, 30) != NULL;
}

RVA(0x00037d20, 0xb3)
i32 CLatencyList::PopulateModemOptions() {
    if (!AddNode("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddNode("Very Low Latency [ping < 50]", 2, 30)) {
        return 0;
    }
    if (!AddNode("Low Latency [ping < 100]", 4, 30)) {
        return 0;
    }
    if (!AddNode("Medium Latency [ping < 200]", 6, 30)) {
        return 0;
    }
    if (!AddNode("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddNode("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddNode("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddNode("Last Resort", 24, 30) != NULL;
}
RVA(0x00037e10, 0xb3)
i32 CLatencyList::PopulateSerialOptions() {
    if (!AddNode("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddNode("Very Low Latency [ping < 50]", 2, 30)) {
        return 0;
    }
    if (!AddNode("Low Latency [ping < 100]", 4, 30)) {
        return 0;
    }
    if (!AddNode("Medium Latency [ping < 200]", 6, 30)) {
        return 0;
    }
    if (!AddNode("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddNode("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddNode("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddNode("Last Resort", 24, 30) != NULL;
}

RVA(0x00037f00, 0xb3)
i32 CLatencyList::PopulateGenericOptions() {
    if (!AddNode("Automatic", 0, 0)) {
        return 0;
    }
    if (!AddNode("Very Low Latency [ping < 50]", 2, 30)) {
        return 0;
    }
    if (!AddNode("Low Latency [ping < 100]", 4, 30)) {
        return 0;
    }
    if (!AddNode("Medium Latency [ping < 200]", 6, 30)) {
        return 0;
    }
    if (!AddNode("Medium-High [ping < 250]", 8, 30)) {
        return 0;
    }
    if (!AddNode("High Latency [ping < 400]", 12, 30)) {
        return 0;
    }
    if (!AddNode("Very High Latency [ping < 550]", 16, 30)) {
        return 0;
    }
    return AddNode("Last Resort", 24, 30) != NULL;
}

RVA(0x00037ff0, 0xe7)
i32 CLatencyList::FillCombo(HWND hDlg, i32 ctrlId) {
    if (m_list.GetCount() <= 0) {
        return 0;
    }
    HWND combo = GetDlgItem(hDlg, ctrlId);
    if (combo != NULL) {
        SendMessageA(combo, CB_RESETCONTENT, 0, 0);
        POSITION pos = m_list.GetHeadPosition();
        while (pos != NULL) {
            CKeyedNode* rec = static_cast<CKeyedNode*>(m_list.GetNext(pos));
            // * 0x10000 (not << 16): keeps both 0xffff masks - cl folds the
            // shifted mask at C1, but strength-reduces the multiply only after.
            i32 data =
                ((rec->m_resendInterval & 0xffff) * 0x10000) | (rec->m_commandDelay & 0xffff);
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

RVA(0x00038220, 0x73)
i32 CLatencyList::GetSelItemData(HWND hDlg, i32 id, i32* outLo, i32* outHi) {
    HWND list = GetDlgItem(hDlg, id);
    if (!list) {
        return 0;
    }
    i32 sel = SendMessageA(list, CB_GETCURSEL, 0, 0);
    if (sel == -1) {
        return 0;
    }
    i32 data = SendMessageA(list, CB_GETITEMDATA, sel, 0);
    if (data == -1) {
        return 0;
    }
    *outLo = data & 0xffff;
    *outHi = static_cast<u32>(data) >> 0x10;
    return 1;
}
