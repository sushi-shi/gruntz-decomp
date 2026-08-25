#ifndef GRUNTZ_GRUNTZ_WAITCURSORSCOPE_H
#define GRUNTZ_GRUNTZ_WAITCURSORSCOPE_H

#include <rva.h>

#include <Mfc.h>

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
