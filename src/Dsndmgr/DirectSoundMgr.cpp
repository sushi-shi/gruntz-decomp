// DirectSoundMgr.cpp - DirectSoundMgr::GetErrorString, the Dsndmgr module's
// HRESULT->error-string diagnostic reporter (C:\Proj\Dsndmgr\, the DSound
// sibling of CDirectDrawMgr::GetErrorString / DDrawMgr's DIRSURF.CPP).
//
// Maps a DirectSound error code to a "<DSERR_NAME> (<code>) - <description>"
// string and, depending on three reporting-mode globals, beeps, logs it via
// OutputDebugStringA and/or pops a MessageBox. `this` is unused; the work is
// driven entirely by the (file, line, hr) arguments the call sites push.
//
// Same archetype as CDirectDrawMgr::GetErrorString. The retail body differs in
// (1) the DSound case set / strings (almost every case message is "No message"),
// (2) the case dispatch is a cmp/je binary-search tree (sparse DSERR values),
// not a jump table, and (3) the log path emits the formatted line through
// OutputDebugStringA (an FF15 [IAT] indirect call) rather than the DDraw
// version's direct DDrawLogLine call.
//
// The function self-identifies its module via the strings it references (every
// DSERR_* name + "DirectSoundMgr"); names of locals are placeholders, the
// switch case VALUES and string contents are load-bearing.
#include <Dsndmgr/DirectSoundMgr.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA / OutputDebugStringA + BOOL/HWND/LPCSTR/UINT come
// from the real <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's
// uType is MB_ICONEXCLAMATION (0x30); the old hand-rolled macro mislabeled that
// value as the "hand" icon, whose real windows.h value is 0x10.
#include <Win32.h>

// Reporting-mode globals (live in .data). g_logEnabled drives the
// OutputDebugStringA path, g_msgBoxEnabled the MessageBox path; g_beepEnabled
// gates the startup beep, g_thirdEnabled is a third "any output wanted" gate
// checked at entry.
DATA(0x253c54)
extern "C" int g_beepEnabled; // 0x653c54
DATA(0x253c4c)
extern "C" int g_logEnabled; // 0x653c4c
DATA(0x253c50)
extern "C" int g_msgBoxEnabled; // 0x653c50
DATA(0x253c58)
extern "C" int g_thirdEnabled; // 0x653c58

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x2293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// ---------------------------------------------------------------------------
// DirectSoundMgr::GetErrorString
RVA(0x138150, 0x33b)
void DirectSoundMgr::GetErrorString(char* file, int line, long hr) {
    char szCode[64];  // error-code name
    char szMsg[256];  // description
    char szLine[512]; // formatted output line

    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled) {
        return;
    }

    int code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case (int)0x80004001:
            strcpy(szCode, "DSERR_UNSUPPORTED");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80004005:
            strcpy(szCode, "DSERR_GENERIC");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80040110:
            strcpy(szCode, "DSERR_NOAGGREGATION");
            strcpy(szMsg, "No message");
            break;
        case (int)0x8007000e:
            strcpy(szCode, "DSERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case (int)0x80070057:
            strcpy(szCode, "DSERR_INVALIDPARAM");
            strcpy(szMsg, "No message");
            break;
        case (int)0x8878000a:
            strcpy(szCode, "DSERR_ALLOCATED");
            strcpy(szMsg, "No message");
            break;
        case (int)0x8878001e:
            strcpy(szCode, "DSERR_CONTROLUNAVAIL");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780032:
            strcpy(szCode, "DSERR_INVALIDCALL");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780046:
            strcpy(szCode, "DSERR_PRIOLEVELNEEDED");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780064:
            strcpy(szCode, "DSERR_BADFORMAT");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780078:
            strcpy(szCode, "DSERR_NODRIVER");
            strcpy(szMsg, "No message");
            break;
        case (int)0x88780096:
            strcpy(szCode, "DSERR_BUFFERLOST");
            strcpy(szMsg, "No message");
            break;
        case (int)0x887800a0:
            strcpy(szCode, "DSERR_OTHERAPPHASPRIO");
            strcpy(szMsg, "No message");
            break;
        case 0:
            strcpy(szCode, "DS_OK");
            strcpy(szMsg, "No error");
            break;
        default:
            break;
    }

    if (g_logEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
        OutputDebugStringA(szLine);
    }
    if (g_msgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA((HWND)0, szLine, "DirectSoundMgr", MB_ICONEXCLAMATION);
    }
}
