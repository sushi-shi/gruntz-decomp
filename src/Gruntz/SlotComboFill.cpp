#include <Net/LatencyList.h> // CLatencyList / CLatencyItem (+ <Mfc.h>: CPtrList/CString/windows.h)
#include <rva.h>

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
    if (m_list.GetCount() <= 0) {
        return 0;
    }
    HWND combo = ::GetDlgItem(reinterpret_cast<HWND>(hDlg), ctrlId);
    if (combo == 0) {
        return 0;
    }
    ::SendMessageA(combo, CB_RESETCONTENT, 0, 0);
    POSITION pos = m_list.GetHeadPosition();
    while (pos != 0) {
        CLatencyItem* rec = static_cast<CLatencyItem*>(m_list.GetNext(pos));
        i32 data = ((rec->m_param & 0xffff) << 16) | (rec->m_id & 0xffff);
        i32 idx;
        {
            CString name = rec->GetName();
            idx = ::SendMessageA(combo, CB_ADDSTRING, 0, reinterpret_cast<long>(static_cast<LPCTSTR>(name)));
        }
        if (idx != -1) {
            ::SendMessageA(combo, CB_SETITEMDATA, idx, data);
        }
    }
    return m_list.GetCount();
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

RVA(0x00038150, 0x91)
i32 CLatencyList::SelectItem(i32 hDlg, i32 id, i32 lo, i32 hi) {
    HWND list = ::GetDlgItem(reinterpret_cast<HWND>(hDlg), id);
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
