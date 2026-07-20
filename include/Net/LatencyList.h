#ifndef GRUNTZ_NET_LATENCYLIST_H
#define GRUNTZ_NET_LATENCYLIST_H

#include <Net/KeyedList.h> // the REAL container: CKeyedList : CPtrList + AddNode/m_mode
#include <rva.h>

struct CLatencyItem {
    CString m_text;    // +0x00  row label ("Very Low Latency [ping < 50]", ...)
    i32 m_id;          // +0x04  row id / ping column
    i32 m_param;       // +0x08  row param column
    CString GetName(); // 0x38120  returns m_text by value
};
SIZE_UNKNOWN(CLatencyItem);

SIZE(CLatencyList, 0x20);
class CLatencyList : public CKeyedList {
public:
    // Forward to the CKeyedList(nBlockSize) ctor (chains CPtrList(0x1b4867), zeros the
    // mode). Inlines to new(0x20) + CPtrList ctor + `mov [obj+0x1c],0`.
    CLatencyList(i32 nBlockSize) : CKeyedList(nBlockSize) {}

    // 0x37910: latch `mode` at +0x1c and route to Populate<mode> (mode 1..5), report
    // whether the populator succeeded (0 for an out-of-range mode).
    i32 Dispatch(i32 mode);

    // The five per-mode populators (reached via ILT thunks from Dispatch; each
    // appends eight AddNode rows and folds the success into a BOOL).
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
};

#endif // GRUNTZ_NET_LATENCYLIST_H
