// SlotComboFill.cpp - CMultiSlotList::Method2a45 (0x37ff0), the /GX combo-box
// populate for the multiplayer-slot list. It resets the (hDlg, ctrlId) combo,
// walks its node list (head @+0x4, count @+0xc), and for each node's slot record
// adds the record's GetName() text (a returned CString temp) then stashes a packed
// MAKELONG(m_4,m_8) as the item data.
//
// Homed in its own eh unit with the real (head@4/count@0xc) list layout - the
// CObList placeholder in Dialogs.cpp's CMultiSlotList (used by the parked
// BuildSlotList) has the wrong count offset, so this reconstruction models the
// list directly and is RVA-matched (the caller's reloc-masked call resolves by
// RVA regardless of the divergent placeholder mangling). GetDlgItem/SendMessageA
// are Win32 imports; GetName (0x38120 via the 0x256d ILT thunk) and ~CString are
// reloc-masked. Only offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (GetName returns one by value) + windows.h
#include <rva.h>

// A slot record hung off each node (+0x8); m_4/m_8 pack into the combo item data.
struct SlotRec {
    char m_pad0[4];
    i32 m_4;           // +0x04
    i32 m_8;           // +0x08
    CString GetName(); // 0x38120 __thiscall, returns CString by value
};
// A node in the slot list: {pNext@0, ?@4, rec@8}.
struct SlotNode {
    SlotNode* m_next; // +0x00
    char m_pad4[4];   // +0x04
    SlotRec* m_8;     // +0x08
};

class CMultiSlotList {
public:
    i32 Method2a45(i32 hDlg, i32 ctrlId); // 0x37ff0

    char m_pad0[4];
    SlotNode* m_4; // +0x04  list head
    char m_pad8[4];
    i32 m_c; // +0x0c  count
};

// @early-stop
// regalloc coin-flip wall (docs/patterns/zero-register-pinning.md): the prologue,
// m_c gate, GetDlgItem, CB_RESETCONTENT, node walk, MAKELONG pack, GetName CString
// temp (tight scope = retail teardown order), CB_ADDSTRING/CB_SETITEMDATA and the
// m_c return are all faithful. Retail keeps `this`->edi, the loop-carried `next`->esi
// (register), `data`->ebp, and calls SendMessageA via `call [__imp_]` (no free reg);
// our cl puts `this`->esi, spills `next` to [esp+0x24], `data`->esi, and caches the
// import in ebp (`mov ebp,[imp]; call ebp`). One `this`-register choice cascades into
// the next-spill + import-cache; no source lever under /O2. ~59%.
RVA(0x00037ff0, 0xe7)
i32 CMultiSlotList::Method2a45(i32 hDlg, i32 ctrlId) {
    if (m_c <= 0) {
        return 0;
    }
    HWND combo = GetDlgItem((HWND)hDlg, ctrlId);
    if (combo == 0) {
        return 0;
    }
    SendMessageA(combo, CB_RESETCONTENT, 0, 0);
    SlotNode* node = m_4;
    while (node != 0) {
        SlotNode* next = node->m_next;
        SlotRec* rec = node->m_8;
        i32 data = ((rec->m_8 & 0xffff) << 16) | (rec->m_4 & 0xffff);
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
    return m_c;
}

// ---------------------------------------------------------------------------
// Class metadata (SIZE sweep) - hosted at TU EOF; labels.py scans tree-wide.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(SlotNode);
SIZE_UNKNOWN(SlotRec);
