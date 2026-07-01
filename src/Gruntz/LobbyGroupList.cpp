// LobbyGroupList.cpp - the /GX listbox-populate method (0x178470) of the network
// lobby's group/session manager (the DPlay-ish enum cluster around 0x178xxx:
// ClearGroupList/ReadGroupSel/EnumPlayersInto/...). It resets the target listbox,
// walks the manager's embedded object list (CObList head @+0x20, iterated through
// the position latch @+0x7c), and for each interface object - unless the caller's
// filter (flags bit1 -> IsInterface2, bit2 -> IsInterface1) rejects it - adds its
// GetName() text and stashes the object pointer as the item data.
//
// The list-node GetNext advance is an inlined helper; it lands at two call sites
// (skip vs add) with different scratch registers. Every callee (IsInterface1/2,
// GetName, ~CString, SendMessageA import) reloc-masks; only offsets + code bytes
// are load-bearing (names are placeholders).
#include <Mfc.h> // real MFC CString (GetName returns one by value) + windows.h
#include <rva.h>

// A list element: a DPlay-ish interface object with name + kind probes.
struct LobbyIface {
    i32 IsInterface1(); // 0x1794b0 __thiscall
    i32 IsInterface2(); // 0x1794e0 __thiscall
    CString GetName();  // 0x179300 __thiscall, returns CString by value
};
// A CObList node: {pNext@0, pPrev@4, data@8}.
struct LobbyNode {
    LobbyNode* m_next;  // +0x00
    char m_pad4[4];     // +0x04  pPrev
    LobbyIface* m_data; // +0x08
};

class CLobbyGroupMgr {
public:
    void PopulateGroupList(HWND hList, i32 flags); // 0x178470

    // Inlined CObList::GetNext(m_7c)-with-null-guard: return the current node's
    // object and advance the position latch; null position -> null.
    LobbyIface* GetNext() {
        LobbyNode* p = m_7c;
        if (p == 0) {
            return 0;
        }
        m_7c = p->m_next;
        return p->m_data;
    }

    char m_pad0[0x20];
    LobbyNode* m_20; // +0x20  list head node (CObList m_pNodeHead)
    char m_pad24[0x7c - 0x24];
    LobbyNode* m_7c; // +0x7c  iteration position latch
};

// @early-stop
// regalloc/stack-slot coin-flip wall (docs/patterns/zero-register-pinning.md): every
// instruction, offset, callee, EH state and the inlined-GetNext-at-two-sites structure
// is faithful, but retail assigns `this`->edi / `idx`->ebp and lays the CString temp at
// [esp+0x10] / the hoisted flags&1 (reusing the dead hList arg slot) at [esp+0x24];
// our cl swaps BOTH pairs (this->ebp/idx->edi, CString@[esp+0x24]/flags&1@[esp+0x10]).
// The two swaps are correlated allocation choices with no source lever (an explicit
// `i32 f1=flags&1` made it WORSE, 62%; the tight CString scope is already retail's
// teardown order). ~78.6%.
RVA(0x00178470, 0x11e)
void CLobbyGroupMgr::PopulateGroupList(HWND hList, i32 flags) {
    if (hList == 0) {
        return;
    }
    SendMessageA(hList, LB_RESETCONTENT, 0, 0);
    m_7c = m_20;
    LobbyIface* obj = GetNext();
    while (obj != 0) {
        if (((flags & 1) && obj->IsInterface2()) || ((flags & 2) && obj->IsInterface1())) {
            obj = GetNext();
        } else {
            i32 idx;
            {
                CString name = obj->GetName();
                idx = SendMessageA(hList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)name);
            }
            if (idx != -1) {
                SendMessageA(hList, LB_SETITEMDATA, idx, (LPARAM)obj);
            }
            obj = GetNext();
        }
    }
}
