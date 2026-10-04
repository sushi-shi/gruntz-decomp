#include <StdAfx.h>

#include <Ints.h>

#include <Net/LatencyList.h>

#include <Enums.h>

#include <stddef.h>
#include <windowsx.h>

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

i32 CLatencyList::FillCombo(HWND hDlg, i32 ctrlId) {

    if (static_cast<i32>(m_list.size()) <= 0) {
        return 0;
    }
    HWND combo = GetDlgItem(hDlg, ctrlId);
    if (combo == NULL) {
        return 0;
    }
    ComboBox_ResetContent(combo);
    std::list<CKeyedNode*>::iterator pos = m_list.begin();
    while (pos != m_list.end()) {
        CKeyedNode* rec = static_cast<CKeyedNode*>(*(pos++));
        i32 data = MAKELONG(rec->GetCommandDelay(), rec->GetResendInterval());
        i32 idx = ComboBox_AddString(combo, rec->GetName().c_str());
        if (idx != CB_ERR) {
            ComboBox_SetItemData(combo, idx, data);
        }
    }
    return static_cast<i32>(m_list.size());
}

std::string CKeyedNode::GetName() {
    return m_key;
}

i32 CLatencyList::SelectItem(HWND hDlg, i32 id, i32 lo, i32 hi) {
    HWND list = GetDlgItem(hDlg, id);
    if (!list) {
        return 0;
    }
    i32 searching = 1;
    i32 i = 0;
    while (searching) {
        i32 data = ComboBox_GetItemData(list, i);
        if (data != CB_ERR) {
            i32 itemLo = LOWORD(data);
            i32 itemHi = HIWORD(data);
            if (itemLo == lo && itemHi == hi) {
                if (ComboBox_GetCurSel(list) != i) {
                    ComboBox_SetCurSel(list, i);
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

i32 CLatencyList::GetSelItemData(HWND hDlg, i32 id, i32* outLo, i32* outHi) {
    HWND list = GetDlgItem(hDlg, id);
    if (!list) {
        return 0;
    }
    i32 sel = ComboBox_GetCurSel(list);
    if (sel == CB_ERR) {
        return 0;
    }
    i32 data = ComboBox_GetItemData(list, sel);
    if (data == CB_ERR) {
        return 0;
    }
    *outLo = LOWORD(data);
    *outHi = HIWORD(data);
    return 1;
}
