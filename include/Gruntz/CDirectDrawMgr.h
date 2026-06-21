// CDirectDrawMgr.h - the WAP32 DirectDraw manager (DDrawMgr module,
// C:\Proj\DDrawMgr\). Minimal reconstruction: only the surface needed to
// byte-match CDirectDrawMgr::GetErrorString, the HRESULT->error-string
// diagnostic formatter. GetErrorString ignores `this`, so the class body is
// only the method declaration (no member layout is load-bearing here).
#ifndef GRUNTZ_CDIRECTDRAWMGR_H
#define GRUNTZ_CDIRECTDRAWMGR_H

class CDirectDrawMgr {
public:
    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__ and
    // a DirectDraw HRESULT, builds a "<DDERR_NAME> (<code>) - <description>"
    // string and (depending on the three reporting-mode globals) beeps, logs it
    // and/or shows it in a message box. STATIC: it ignores `this` (the call sites
    // leave ECX set from a prior thiscall, but the body never reads it) and is
    // __cdecl/caller-cleaned (plain `ret`; call sites `add esp,0xc`).
    static void GetErrorString(char* file, int line, long hr);
};

#endif // GRUNTZ_CDIRECTDRAWMGR_H
