#ifndef GRUNTZ_NET_LATENCYLIST_H
#define GRUNTZ_NET_LATENCYLIST_H

#include <Net/KeyedList.h>
#include <rva.h>

struct CLatencyItem {
    CString m_text;
    i32 m_id;
    i32 m_param;
    CString GetName();
};
SIZE_UNKNOWN();

class CLatencyList : public CKeyedList {
public:
    CLatencyList(i32 nBlockSize) : CKeyedList(nBlockSize) {}

    i32 Dispatch(i32 mode);

    i32 Populate1();
    i32 Populate2();
    i32 Populate3();
    i32 Populate4();
    i32 Populate5();

    i32 FillCombo(HWND hDlg, i32 ctrlId);

    i32 SelectItem(HWND hDlg, i32 id, i32 lo, i32 hi);
};
SIZE(0x20);

#endif // GRUNTZ_NET_LATENCYLIST_H
