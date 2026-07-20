#ifndef GRUNTZ_WAITCURSORAPP_H
#define GRUNTZ_WAITCURSORAPP_H

#include <Mfc.h> // AfxGetModuleState + AFX_MODULE_STATE::m_pCurrentWinApp (afx.h)
#include <rva.h> // SIZE_UNKNOWN

SIZE_UNKNOWN(CWaitCursorApp);
struct CWaitCursorApp {
    void BeginWaitCursor(); // 0x1beafb  CCmdTarget::BeginWaitCursor
    // EndWaitCursor is void in NAFXCW, but retail propagates its trailing eax as the
    // caller's return in some sites (CustomLevelDlg's `return EndWaitCursor()` tail),
    // so it is typed i32 here to reproduce that `call; ret`; void-context callers
    // (CustomWorldDialog) discard the value, byte-identically.
    i32 EndWaitCursor(); // 0x1beb10  CCmdTarget::EndWaitCursor
};

#endif // GRUNTZ_WAITCURSORAPP_H
