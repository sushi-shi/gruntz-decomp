// SlotComboFill.cpp - CLatencyList::FillCombo (0x37ff0), the /GX combo-box populate
// for the multiplayer connection-latency slot list. It resets the (hDlg, ctrlId)
// combo, walks the CObList node list (head @+0x4, count @+0xc), and for each node's
// CLatencyItem payload adds GetName() text (a returned CString temp) then stashes a
// packed MAKELONG(m_param,m_id) as the item data.
//
// The container is the canonical CLatencyList (<Net/LatencyList.h>) - the former
// per-TU CMultiSlotList/SlotNode/SlotRec views here folded onto it (wave 3). The
// node walk uses the real MFC CObList node list (CObList::CNode, protected, reached
// through the CLatencyList base). GetDlgItem/SendMessageA are Win32 imports; GetName
// (0x38120 via the 0x256d ILT thunk) and ~CString are reloc-masked. Only offsets +
// code bytes are load-bearing.
#include <Net/LatencyList.h> // CLatencyList / CLatencyItem (+ <Mfc.h>: CObList/CString/windows.h)
#include <rva.h>

// @early-stop
// regalloc coin-flip wall (docs/patterns/zero-register-pinning.md): the prologue,
// m_nCount gate, GetDlgItem, CB_RESETCONTENT, node walk, MAKELONG pack, GetName CString
// temp (tight scope = retail teardown order), CB_ADDSTRING/CB_SETITEMDATA and the
// m_nCount return are all faithful. Retail keeps `this`->edi, the loop-carried `next`->esi
// (register), `data`->ebp, and calls SendMessageA via `call [__imp_]` (no free reg);
// our cl puts `this`->esi, spills `next` to [esp+0x24], `data`->esi, and caches the
// import in ebp (`mov ebp,[imp]; call ebp`). One `this`-register choice cascades into
// the next-spill + import-cache; no source lever under /O2. ~59%.
RVA(0x00037ff0, 0xe7)
i32 CLatencyList::FillCombo(i32 hDlg, i32 ctrlId) {
    if (m_nCount <= 0) {
        return 0;
    }
    HWND combo = GetDlgItem((HWND)hDlg, ctrlId);
    if (combo == 0) {
        return 0;
    }
    SendMessageA(combo, CB_RESETCONTENT, 0, 0);
    CObList::CNode* node = m_pNodeHead;
    while (node != 0) {
        CObList::CNode* next = node->pNext;
        CLatencyItem* rec = (CLatencyItem*)node->data;
        i32 data = ((rec->m_param & 0xffff) << 16) | (rec->m_id & 0xffff);
        i32 idx;
        {
            CString name = rec->GetName();
            idx = SendMessageA(combo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)name);
        }
        if (idx != -1) {
            SendMessageA(combo, CB_SETITEMDATA, idx, data);
        }
        node = next;
    }
    return m_nCount;
}
