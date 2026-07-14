#ifndef GRUNTZ_WAITCURSORAPP_H
#define GRUNTZ_WAITCURSORAPP_H

// WaitCursorApp.h - the MFC app's hourglass pair, reached through
// AfxGetModuleState()->m_pCurrentWinApp (0x1d3631 == AfxGetModuleState).
//
// BeginWaitCursor (0x1beafb) / EndWaitCursor (0x1beb10) are the real
// CCmdTarget methods (NAFXCW), inherited by the running CWinApp. afxwin.h's
// CWinApp/CCmdTarget carry them, but its inline bodies don't parse under the
// clang label pass for these lean Win32/MFC front-end TUs, so the pair is
// modeled as two __thiscall slots on the app pointer m_pCurrentWinApp exposes
// (the same shape StartUpPrompt.cpp used as a .cpp-local `WaitApp`, now shared).
// Reloc-masked: the two `call rel32`s mask against the retail 0x1beafb/0x1beb10.
#include <Mfc.h> // AfxGetModuleState + AFX_MODULE_STATE::m_pCurrentWinApp (afx.h)
#include <rva.h> // SIZE_UNKNOWN

// A method-only facet cast onto the running CWinApp; it holds no storage of its own.
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
