// DirectInputMgr2.cpp - DirectInputMgr2::GetErrorString, the DinMgr2 module's
// HRESULT->error-string diagnostic reporter (C:\Proj\DinMgr2\, the DInput
// sibling of CDirectDrawMgr::GetErrorString / DDrawMgr's DIRSURF.CPP).
//
// Maps a DirectInput error code to a "<DIERR_NAME> (<code>) - <description>"
// string and, depending on three reporting-mode globals, beeps, formats it
// and/or pops a MessageBox. `this` is unused; the work is driven entirely by
// the (file, line, hr) arguments the call sites push.
//
// Same archetype as CDirectDrawMgr::GetErrorString, with three differences in
// the retail body: (1) the DInput case set / strings, (2) the case dispatch is
// a cmp/je binary-search tree (sparse DIERR values), not a jump table, and
// (3) the log path only sprintf-formats the line - there is NO separate logger
// call (the DDraw version's DDrawLogLine has no DinMgr2 counterpart here).
//
// The function self-identifies its module via the strings it references (every
// DIERR_* name + "DirectInputMgr2"); names of locals are placeholders, the
// switch case VALUES and string contents are load-bearing.
#include <DinMgr2/DirectInputMgr2.h>
#include <rva.h>
#include <stdio.h>   // engine sprintf (reloc-masked)
#include <string.h>  // inline strcpy (rep movs / repne scasb)

// ---------------------------------------------------------------------------
// Minimal Win32 surface. Do NOT pull in <windows.h> - keep the visible symbol
// SET small. Only the two USER32 imports the function calls, as a
// __declspec(dllimport) __stdcall block (reproduces the FF15 [IAT] indirect
// call form).
// ---------------------------------------------------------------------------
typedef int   BOOL;
typedef void *HWND;
typedef const char *LPCSTR;
typedef unsigned int UINT;

extern "C" {
__declspec(dllimport) BOOL __stdcall MessageBeep(UINT uType);
__declspec(dllimport) int  __stdcall MessageBoxA(HWND hWnd, LPCSTR lpText,
                                                 LPCSTR lpCaption, UINT uType);
}

#define MB_ICONHAND 0x30   // the uType / beep code the reporter passes (0x30)

// Reporting-mode globals (live in .data). g_logEnabled drives the format-line
// path, g_msgBoxEnabled the MessageBox path; g_beepEnabled gates the startup
// beep, g_thirdEnabled is a third "any output wanted" gate checked at entry.
DATA(0x253aac)
extern "C" int g_beepEnabled;     // 0x653aac
DATA(0x253aa4)
extern "C" int g_logEnabled;      // 0x653aa4
DATA(0x253aa8)
extern "C" int g_msgBoxEnabled;   // 0x653aa8
DATA(0x253ab0)
extern "C" int g_thirdEnabled;    // 0x653ab0

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x2293f4)
extern "C" char g_emptyString[];  // 0x6293f4

// ---------------------------------------------------------------------------
// DirectInputMgr2::GetErrorString
RVA(0x133590, 0x5be)
void DirectInputMgr2::GetErrorString(char *file, int line, long hr)
{
    char szCode[64];    // error-code name
    char szMsg[256];    // description
    char szLine[512];   // formatted output line

    if (g_beepEnabled)
        MessageBeep(MB_ICONHAND);
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled)
        return;

    int code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
    case (int)0x80004001: strcpy(szCode, "DIERR_UNSUPPORTED");           strcpy(szMsg, "The function called is not supported at this time."); break;
    case (int)0x80004002: strcpy(szCode, "DIERR_NOINTERFACE");           strcpy(szMsg, "The specified interface is not supported by the object."); break;
    case (int)0x80004005: strcpy(szCode, "DIERR_GENERIC");               strcpy(szMsg, "An undetermined error occured inside the DInput subsystem."); break;
    case (int)0x80040154: strcpy(szCode, "DIERR_DEVICENOTREG");          strcpy(szMsg, "The device or device instance or effect is not registered with DirectInput."); break;
    case (int)0x80040200: strcpy(szCode, "DIERR_INSUFFICIENTPRIVS");     strcpy(szMsg, "No message"); break;
    case (int)0x80070002: strcpy(szCode, "DIERR_NOTFOUND");              strcpy(szMsg, "The requested object does not exist."); break;
    case (int)0x80070005: strcpy(szCode, "DIERR_READONLY");              strcpy(szMsg, "The specified property cannot be changed."); break;
    case (int)0x8007000c: strcpy(szCode, "DIERR_NOTACQUIRED");           strcpy(szMsg, "The operation cannot be performed unless the device is acquired."); break;
    case (int)0x8007000e: strcpy(szCode, "DIERR_OUTOFMEMORY");           strcpy(szMsg, "No message"); break;
    case (int)0x80070015: strcpy(szCode, "DIERR_NOTINITIALIZED");        strcpy(szMsg, "This object has not been initialized."); break;
    case (int)0x8007001e: strcpy(szCode, "DIERR_INPUTLOST");             strcpy(szMsg, "Access to the device has been lost.  It must be re-acquired."); break;
    case (int)0x80070057: strcpy(szCode, "DIERR_INVALIDPARAM");          strcpy(szMsg, "No message"); break;
    case (int)0x80070077: strcpy(szCode, "DIERR_BADDRIVERVER");          strcpy(szMsg, "The object could not be created due to an incompatible driver version or mismatched or incomplete driver components."); break;
    case (int)0x800700aa: strcpy(szCode, "DIERR_ACQUIRED");              strcpy(szMsg, "The operation cannot be performed while the device is acquired."); break;
    case (int)0x8007047e: strcpy(szCode, "DIERR_OLDDIRECTINPUTVERSION"); strcpy(szMsg, "The application requires a newer version of DirectInput."); break;
    case (int)0x800704df: strcpy(szCode, "DIERR_ALREADYINITIALIZED");    strcpy(szMsg, "This object is already initialized."); break;
    case 0:          strcpy(szCode, "DD_OK");                       strcpy(szMsg, "No error"); break;
    default: break;
    }

    if (g_logEnabled) {
        if (file == 0 || line <= 0)
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        else
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
    }
    if (g_msgBoxEnabled) {
        if (file == 0 || line <= 0)
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        else
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        MessageBoxA((HWND)0, szLine, "DirectInputMgr2", MB_ICONHAND);
    }
}
