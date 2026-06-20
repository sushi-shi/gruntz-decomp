// DirectInputMgr2.h - the WAP32 DirectInput manager (DinMgr2 module,
// C:\Proj\DinMgr2\). Minimal reconstruction: only the surface needed to
// byte-match DirectInputMgr2::GetErrorString, the HRESULT->error-string
// diagnostic formatter (the DInput sibling of CDirectDrawMgr::GetErrorString).
// GetErrorString ignores `this`, so the class body is only the method
// declaration (no member layout is load-bearing here).
#ifndef DINMGR2_DIRECTINPUTMGR2_H
#define DINMGR2_DIRECTINPUTMGR2_H

class DirectInputMgr2 {
public:
    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__ and
    // a DirectInput HRESULT, builds a "<DIERR_NAME> (<code>) - <description>"
    // string and (depending on the three reporting-mode globals) beeps, formats
    // it and/or shows it in a message box ("DirectInputMgr2" caption). STATIC: it
    // ignores `this` (the call sites leave ECX set from a prior thiscall, but the
    // body never reads it) and is __cdecl/caller-cleaned (plain `ret`; call sites
    // `add esp,0xc`).
    static void GetErrorString(char *file, int line, long hr);
};

#endif // DINMGR2_DIRECTINPUTMGR2_H
