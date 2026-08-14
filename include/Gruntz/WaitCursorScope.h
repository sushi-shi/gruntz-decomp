#ifndef GRUNTZ_GRUNTZ_WAITCURSORSCOPE_H
#define GRUNTZ_GRUNTZ_WAITCURSORSCOPE_H

#include <rva.h>

#include <Mfc.h>

// RAII hourglass. Retail's /GX trylevel brackets every BeginWaitCursor /
// EndWaitCursor pair (state 0 set right after the Begin call, -1 right before
// the End call), and eight unwind funclets in the 0x1d.... band jmp into one
// shared out-of-line copy of this destructor at 0x00018430 - both of which a
// pair of bare calls cannot produce.
class CWaitCursorScope {
public:
    CWaitCursorScope() {
        afxCurrentWinApp->BeginWaitCursor();
    }

    RVA(0x00018430, 0xd)
    ~CWaitCursorScope() {
        afxCurrentWinApp->EndWaitCursor();
    }
};

#endif // GRUNTZ_GRUNTZ_WAITCURSORSCOPE_H
