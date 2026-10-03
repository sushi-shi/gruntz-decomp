#ifndef GRUNTZ_GRUNTZ_WAITCURSORSCOPE_H
#define GRUNTZ_GRUNTZ_WAITCURSORSCOPE_H

#include <Ints.h>

class CWaitCursorScope {
public:
    CWaitCursorScope() {
        afxCurrentWinApp->BeginWaitCursor();
    }

    ~CWaitCursorScope() {
        afxCurrentWinApp->EndWaitCursor();
    }
};

#endif
