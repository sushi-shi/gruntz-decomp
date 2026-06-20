// CNetMgrError.h - the surface needed to byte-match CNetMgr::ReportError, the
// Net module's DirectPlay HRESULT->error-string diagnostic reporter
// (C:\Proj\NetMgr\, the DirectPlay sibling of CDirectDrawMgr::GetErrorString /
// DirectInputMgr2::GetErrorString / DirectSoundMgr::GetErrorString).
//
// Same "error formatter" archetype, but with two structural differences from
// the three DDraw/DInput/DSound siblings:
//   (1) the working buffers and the saved code/hr are GLOBALS (DAT_006bf700 /
//       DAT_006bf740 / DAT_006bf6fc / DAT_006bf6f8), not stack locals - only
//       the formatted output line is a stack buffer here; and
//   (2) there is NO early "any output wanted?" return before the switch: the
//       (hr & 0xffff) save + beep run first, the switch always runs, and the
//       three reporting-mode gates are only consulted afterwards.
//
// ReportError ignores `this` (call sites leave ECX from a prior thiscall but
// the body never reads it); the class body is only the method declaration.
#ifndef NET_CNETMGRERROR_H
#define NET_CNETMGRERROR_H

class CNetMgr {
public:
    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__, a
    // DirectPlay HRESULT, and a parent window handle, maps the HRESULT to a
    // "<DPERR_NAME> (<code>) - <description>" line and (depending on three
    // reporting-mode globals) beeps, formats it and/or shows it in a "Net
    // Manager" message box owned by `hWnd`. STATIC: it ignores `this` and is
    // __cdecl/caller-cleaned (plain `ret`; call sites `add esp,0x10`).
    static void ReportError(char *file, int line, long hr, void *hWnd);
};

#endif // NET_CNETMGRERROR_H
