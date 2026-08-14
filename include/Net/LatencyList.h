#ifndef GRUNTZ_NET_LATENCYLIST_H
#define GRUNTZ_NET_LATENCYLIST_H

#include <rva.h>

#include <Net/KeyedList.h>

class CLatencyList : public CKeyedList {
public:
    CLatencyList(i32 nBlockSize) : CKeyedList(nBlockSize) {}

    i32 Dispatch(i32 mode);

    i32 PopulateIpxOptions();
    i32 PopulateTcpIpOptions();
    i32 PopulateModemOptions();
    i32 PopulateSerialOptions();
    i32 PopulateGenericOptions();

    i32 FillCombo(HWND hDlg, i32 ctrlId);

    i32 SelectItem(HWND hDlg, i32 id, i32 lo, i32 hi);
};

// The read side of FillCombo's packed item data: unpacks the two 16-bit halves
// back out of the selected entry.
i32 __stdcall GetSelItemData(HWND hDlg, i32 id, i32* outLo, i32* outHi);

#endif // GRUNTZ_NET_LATENCYLIST_H
