// SlotComboFill.cpp - CLatencyList::FillCombo (0x37ff0), the /GX combo-box populate
// for the multiplayer connection-latency slot list. It resets the (hDlg, ctrlId)
// combo, walks the CPtrList node list (head @+0x4, count @+0xc), and for each node's
// CLatencyItem payload adds GetName() text (a returned CString temp) then stashes a
// packed MAKELONG(m_param,m_id) as the item data.
//
// The container is the canonical CLatencyList (<Net/LatencyList.h>) - the former
// per-TU CMultiSlotList/SlotNode/SlotRec views here folded onto it (wave 3). The
// node walk uses the real MFC CPtrList node list (CPtrList::CNode, protected, reached
// through the CLatencyList base). GetDlgItem/SendMessageA are Win32 imports; GetName
// (0x38120 via the 0x256d ILT thunk) and ~CString are reloc-masked. Only offsets +
// code bytes are load-bearing.
#include <Net/LatencyList.h> // CLatencyList / CLatencyItem (+ <Mfc.h>: CPtrList/CString/windows.h)
#include <rva.h>

// The engine's cached USER32 imports, held as function-pointer globals (the dialog
// helpers call THESE, not the raw USER32 imports - reloc-masked indirect calls).
// Both reference their canonical DATA homes (no local DATA here): ::GetDlgItem is the
// C++-mangled ?::GetDlgItem@@3P6GPAUHWND__@@PAU1@H@ZA (home CustomWorldDialog.cpp
// @0x2c4564) and ::SendMessageA is the extern-"C" _g_pSendMessageA (home GruntzMgr.cpp
// @0x2c44a4) - so both DIR32 references reloc against the tree-winning symbol at the
// right RVA (the old extern-"C" _g_pGetDlgItem + DATA(0x006c44a4) VA-typo mis-bound them).

// The control lookup + the four combo messages go through the engine's cached USER32
// function-pointer globals (::GetDlgItem / ::SendMessageA), not the raw imports:
// retail has no free callee-saved register here (this/next/data hold them), so each
// ::SendMessageA call is an uncached memory-indirect `ff 15 [::SendMessageA]` (the
// global is called directly, never hoisted into a register).
// @early-stop
// regalloc coin-flip wall (docs/patterns/zero-register-pinning.md), ~61%: with the
// USER32 fn-ptr globals now correct (reloc names match), the residual is the callee-
// saved assignment - retail keeps `this`->edi, the loop-carried `next`->esi, `data`->
// ebp; our cl puts `this`->esi, spills `next`, `data`->esi. One `this`-register choice
// cascades into the next-spill; no source lever under /O2.
RVA(0x00037ff0, 0xe7)
i32 CLatencyList::FillCombo(i32 hDlg, i32 ctrlId) {
    if (m_nCount <= 0) {
        return 0;
    }
    HWND combo = ::GetDlgItem((HWND)hDlg, ctrlId);
    if (combo == 0) {
        return 0;
    }
    ::SendMessageA(combo, CB_RESETCONTENT, 0, 0);
    CPtrList::CNode* node = m_pNodeHead;
    while (node != 0) {
        CPtrList::CNode* next = node->pNext;
        CLatencyItem* rec = (CLatencyItem*)node->data;
        i32 data = ((rec->m_param & 0xffff) << 16) | (rec->m_id & 0xffff);
        i32 idx;
        {
            CString name = rec->GetName();
            idx = ::SendMessageA(combo, CB_ADDSTRING, 0, reinterpret_cast<long>((LPCTSTR)name));
        }
        if (idx != -1) {
            ::SendMessageA(combo, CB_SETITEMDATA, idx, data);
        }
        node = next;
    }
    return m_nCount;
}

// 0x38120 (re-homed from src/Stub/BoundaryTail.cpp): CLatencyItem::GetName - return
// the row-label CString member (offset 0) by value. Called by FillCombo above.
// @early-stop
// /O2 dead-local wall (~66%): the copyctor-into-retslot logic is exact, but retail
// reserves + zeroes one extra stack dword (`push reg; mov [slot],0`) that our /O2
// elides as dead. Confirmed NOT /O1 (o1 profile scored 34%). The origin of the kept
// zero store (a return-value cookie / source temp) is not yet spellable; the copy
// itself is byte-exact.
RVA(0x00038120, 0x1d)
CString CLatencyItem::GetName() {
    return m_text;
}

// CLatencyList::SelectItem (0x38150) - re-homed from src/Gruntz/Dialogs.cpp
// (matcher-1 de-fragmentation); it is the immediate retail .text neighbour of
// GetName above and a CLatencyList method (CMultiStartDlg::BuildSlotList/UpdateSlot
// call it, reloc-masked). Find the list item in control `id` of dialog `hDlg` whose
// item-data equals MAKELONG(lo, hi) and select it (LB_SETCURSEL); returns 1 if
// found, else 0. The method ignores `this` (a dialog-item scan) - ecx is unused.
// The control lookup + the three list messages go through the engine's cached USER32
// function-pointer globals (::GetDlgItem / ::SendMessageA), not the raw imports:
// retail loads ::SendMessageA once into edi (`mov edi,[::SendMessageA]; call edi`),
// so the local pSend caches it here.
RVA(0x00038150, 0x91)
i32 CLatencyList::SelectItem(i32 hDlg, i32 id, i32 lo, i32 hi) {
    HWND list = ::GetDlgItem((HWND)hDlg, id);
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
