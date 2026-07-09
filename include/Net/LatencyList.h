// LatencyList.h - the multiplayer "connection latency preset" list (the network
// start dialog, C:\Proj\NetMgr / Gruntz). ONE real class, formerly viewed under
// four placeholder names across four TUs (all folded here, wave 3):
//   * CMultiSlotList  (Dialogs.cpp)      - the m_slotList container + SelectItem
//   * CMultiSlotList  (SlotComboFill.cpp)- FillCombo (the combo populate)
//   * CConnSlotList   (Net/ConnSlotList) - the AddItem populators (0x37c30/e10/f00)
//   * CBzKindDispatch (BzKindDispatch)   - the mode dispatcher (0x37910)
// PROOF of one class: the dispatcher 0x37910 (called on the dialog's m_slotList)
// routes modes 1..5 to the five populators 0x37b40/c30/d20/e10/f00 (thunks 0x3760/
// 3008/41ce/1eab/1cc1 -> those bodies), and FillCombo/SelectItem run on the SAME
// container. The container's ctor 0x1b4867 (block size 0xa) is the plain MFC CObList
// ctor - the object has NO own vtable (it uses CObList's 0x5eb054), so it is a
// CObList subclass adding only the mode int at +0x1c.
//
// BuildSlotList (0x0c1e60) news a 0x20-byte object into CMultiStartDlg::m_slotList,
// dispatches the connection-type mode via 0x37910, then fills/selects the 0x527
// combo. Each populator appends eight rows via AddItem (0x37a70), which news a
// 12-byte {CString text; int id; int param} node (CLatencyItem) and AddTail's it;
// the populators differ only in the per-row `param` column. Field/method names are
// placeholders (campaign doctrine); only the offsets + code bytes are load-bearing.
#ifndef GRUNTZ_NET_LATENCYLIST_H
#define GRUNTZ_NET_LATENCYLIST_H

#include <Mfc.h> // real MFC CObList (base) + CString (node text / GetName)
#include <rva.h>

// The node payload hung off each CObList node's `data` pointer (heap, 0xc bytes).
// GetName (0x38120) returns m_text by value; FillCombo packs (m_param<<16)|m_id as
// the combo item-data.
struct CLatencyItem {
    CString m_text; // +0x00  row label ("Very Low Latency [ping < 50]", ...)
    i32 m_id;       // +0x04  row id / ping column
    i32 m_param;    // +0x08  row param column
    CString GetName(); // 0x38120  returns m_text by value
};
SIZE_UNKNOWN(CLatencyItem);

// The connection-latency slot list: a CObList of CLatencyItem rows + the latched
// connection-type mode at +0x1c. Allocated with block size 0xa.
SIZE(CLatencyList, 0x20);
class CLatencyList : public CObList {
public:
    // Inline ctor: chain the MFC CObList(nBlockSize) ctor (0x1b4867), then zero the
    // mode. No own vtable, so cl emits no extra stamp (matches new(0x20) + CObList
    // ctor + `mov [obj+0x1c],0`).
    CLatencyList(i32 nBlockSize) : CObList(nBlockSize) {
        m_mode = 0;
    }

    // 0x37a70: new a {text,id,param} CLatencyItem and AddTail it; returns the node
    // (non-zero on success). Reloc-masked external (no body here).
    i32 AddItem(const char* text, i32 id, i32 param);

    // 0x37910: latch `mode` at +0x1c and route to Populate<mode> (mode 1..5), report
    // whether the populator succeeded (0 for an out-of-range mode).
    i32 Dispatch(i32 mode);

    // The five per-mode populators (reached via ILT thunks from Dispatch; each
    // appends eight AddItem rows and folds the success into a BOOL).
    i32 Populate1(); // 0x37b40  mode 1
    i32 Populate2(); // 0x37c30  mode 2  (ramped ladder 0,10,10,20,30,30,30,30)
    i32 Populate3(); // 0x37d20  mode 3
    i32 Populate4(); // 0x37e10  mode 4  (fixed-30 ladder)
    i32 Populate5(); // 0x37f00  mode 5  (== Populate4 body)

    // 0x37ff0: reset the (hDlg,ctrlId) combo, walk the node list, and for each row
    // add GetName() text keyed by MAKELONG(m_param,m_id) item-data. Returns m_nCount.
    i32 FillCombo(i32 hDlg, i32 ctrlId);

    // 0x38150: find the combo item whose data == MAKELONG(lo,hi) and select it
    // (ignores `this` - a pure dialog-item scan). Returns 1 if found.
    i32 SelectItem(i32 hDlg, i32 id, i32 lo, i32 hi);

    i32 m_mode; // +0x1c  latched connection-type mode (Dispatch arg)
};

#endif // GRUNTZ_NET_LATENCYLIST_H
