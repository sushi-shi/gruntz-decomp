// DirectSoundMgr.h - the WAP32 DirectSound manager (Dsndmgr module,
// C:\Proj\Dsndmgr\). Minimal reconstruction: only the surface needed to
// byte-match DirectSoundMgr::GetErrorString, the HRESULT->error-string
// diagnostic formatter (the DSound sibling of CDirectDrawMgr::GetErrorString).
// GetErrorString ignores `this`, so the class body is only the method
// declaration (no member layout is load-bearing here).
#ifndef DSNDMGR_DIRECTSOUNDMGR_H
#define DSNDMGR_DIRECTSOUNDMGR_H

class DirectSoundMgr {
public:
    // Diagnostic error reporter. Given the calling site's __FILE__/__LINE__ and
    // a DirectSound HRESULT, builds a "<DSERR_NAME> (<code>) - <description>"
    // string and (depending on the three reporting-mode globals) beeps, logs it
    // via OutputDebugStringA and/or shows it in a message box ("DirectSoundMgr"
    // caption). STATIC: it ignores `this` (the call sites leave ECX set from a
    // prior thiscall, but the body never reads it) and is __cdecl/caller-cleaned
    // (plain `ret`; call sites `add esp,0xc`).
    static void GetErrorString(char* file, int line, long hr);
};

#endif // DSNDMGR_DIRECTSOUNDMGR_H
