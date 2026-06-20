// CDirectDrawMgr.cpp - CDirectDrawMgr::GetErrorString, the DDrawMgr module's
// HRESULT->error-string diagnostic reporter (C:\Proj\DDrawMgr\DIRSURF.CPP).
//
// Maps a DirectDraw error code to a "<DDERR_NAME> (<code>) - <description>"
// string and, depending on three reporting-mode globals, beeps, logs it through
// the engine logger and/or pops a MessageBox. `this` is unused; the work is
// driven entirely by the (file, line, hr) arguments the call sites push.
//
// The function self-identifies its module via the strings it references
// (every DDERR_* name + "DirectDrawMgr"); names of locals are placeholders, the
// switch case VALUES and string contents are load-bearing.
#include <Gruntz/CDirectDrawMgr.h>
#include <rva.h>
#include <stdio.h>   // engine sprintf (reloc-masked)
#include <string.h>  // inline strcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA + BOOL/HWND/LPCSTR/UINT come from the real
// <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's uType is
// MB_ICONEXCLAMATION (0x30); the old hand-rolled macro mislabeled that value as
// the "hand" icon, whose real windows.h value is 0x10.
#include <Win32.h>

// Reporting-mode globals (live in .data). g_logEnabled drives the engine logger
// path, g_msgBoxEnabled the MessageBox path; g_beepEnabled gates the startup
// beep, g_thirdEnabled is a third "any output wanted" gate checked at entry.
DATA(0x283ec0)
extern "C" int g_beepEnabled;     // 0x683ec0
DATA(0x283eb8)
extern "C" int g_logEnabled;      // 0x683eb8
DATA(0x283ebc)
extern "C" int g_msgBoxEnabled;   // 0x683ebc
DATA(0x283ec4)
extern "C" int g_thirdEnabled;    // 0x683ec4

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x2293f4)
extern "C" char g_emptyString[];  // 0x6293f4

// The engine logger that consumes the formatted line (DDrawMgr-local helper).
extern void __cdecl DDrawLogLine(char *line);   // 0x141cb0

// ---------------------------------------------------------------------------
// CDirectDrawMgr::GetErrorString
RVA(0x141400, 0x835)
void CDirectDrawMgr::GetErrorString(char *file, int line, long hr)
{
    char szCode[64];    // local_340 - error-code name
    char szMsg[256];    // local_300 - description
    char szLine[512];   // local_200 - formatted output line

    if (g_beepEnabled)
        MessageBeep(MB_ICONEXCLAMATION);
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled)
        return;

    int code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
    case (int)0x80004001: strcpy(szCode, "DDERR_UNSUPPORTED");           strcpy(szMsg, "Action not supported"); break;
    case (int)0x80004005: strcpy(szCode, "DDERR_GENERIC");               strcpy(szMsg, "Generic failure"); break;
    case (int)0x8007000e: strcpy(szCode, "DDERR_OUTOFMEMORY");           strcpy(szMsg, "No message"); break;
    case (int)0x80070057: strcpy(szCode, "DDERR_INVALIDPARAMS");         strcpy(szMsg, "No message"); break;
    case (int)0x88760064: strcpy(szCode, "DDERR_INVALIDCAPS");           strcpy(szMsg, "One or more of the caps bits passed to the callback are incorrect"); break;
    case (int)0x88760078: strcpy(szCode, "DDERR_INVALIDMODE");           strcpy(szMsg, "No message"); break;
    case (int)0x88760082: strcpy(szCode, "DDERR_INVALIDOBJECT");         strcpy(szMsg, "No message"); break;
    case (int)0x88760091: strcpy(szCode, "DDERR_INVALIDPIXELFORMAT");    strcpy(szMsg, "Pixel format was invalid as specified."); break;
    case (int)0x88760096: strcpy(szCode, "DDERR_INVALIDRECT");           strcpy(szMsg, "No message"); break;
    case (int)0x887600a0: strcpy(szCode, "DDERR_LOCKEDSURFACES");        strcpy(szMsg, "No message"); break;
    case (int)0x887600aa: strcpy(szCode, "DDERR_NO3D");                  strcpy(szMsg, "No message"); break;
    case (int)0x887600b4: strcpy(szCode, "DDERR_NOALPHAHW");             strcpy(szMsg, "No message"); break;
    case (int)0x887600d2: strcpy(szCode, "DDERR_NOCOLORCONVHW");         strcpy(szMsg, "No message"); break;
    case (int)0x887600d4: strcpy(szCode, "DDERR_NOCOOPERATIVELEVELSET"); strcpy(szMsg, "Create function called without DirectDraw object method SetCooperativeLevel being called"); break;
    case (int)0x887600e1: strcpy(szCode, "DDERR_NOEXCLUSIVEMODE");       strcpy(szMsg, "No message"); break;
    case (int)0x887600f0: strcpy(szCode, "DDERR_NOGDI");                 strcpy(szMsg, "There is no GDI present"); break;
    case (int)0x887600fa: strcpy(szCode, "DDERR_NOMIRRORHW");            strcpy(szMsg, "Operation could not be carried out because there is no hardware present or available."); break;
    case (int)0x887600ff: strcpy(szCode, "DDERR_NOTFOUND");              strcpy(szMsg, "Request item was not found"); break;
    case (int)0x88760104: strcpy(szCode, "DDERR_NOOVERLAYHW");           strcpy(szMsg, "No message"); break;
    case (int)0x88760118: strcpy(szCode, "DDERR_NORASTEROPHW");          strcpy(szMsg, "No message"); break;
    case (int)0x88760122: strcpy(szCode, "DDERR_NOROTATEHW");            strcpy(szMsg, "No message"); break;
    case (int)0x88760136: strcpy(szCode, "DDERR_NOSTRETCHHW");           strcpy(szMsg, "No message"); break;
    case (int)0x88760140: strcpy(szCode, "DDERR_NOT8BITCOLOR");          strcpy(szMsg, "No message"); break;
    case (int)0x8876014a: strcpy(szCode, "DDERR_NOTEXTUREHW");           strcpy(szMsg, "No message"); break;
    case (int)0x8876014f: strcpy(szCode, "DDERR_NOVSYNCHW");             strcpy(szMsg, "No message"); break;
    case (int)0x88760154: strcpy(szCode, "DDERR_NOZBUFFERHW");           strcpy(szMsg, "No message"); break;
    case (int)0x88760168: strcpy(szCode, "DDERR_OUTOFCAPS");             strcpy(szMsg, "No message"); break;
    case (int)0x8876017c: strcpy(szCode, "DDERR_OUTOFVIDEOMEMORY");      strcpy(szMsg, "No message"); break;
    case (int)0x88760183: strcpy(szCode, "DDERR_PALETTEBUSY");          strcpy(szMsg, "No message"); break;
    case (int)0x887601ae: strcpy(szCode, "DDERR_SURFACEBUSY");          strcpy(szMsg, "No message"); break;
    case (int)0x887601b8: strcpy(szCode, "DDERR_SURFACEISOBSCURED");    strcpy(szMsg, "No message"); break;
    case (int)0x887601c2: strcpy(szCode, "DDERR_SURFACELOST");          strcpy(szMsg, "No message"); break;
    case (int)0x887601cc: strcpy(szCode, "DDERR_SURFACENOTATTACHED");   strcpy(szMsg, "The requested surface is not attached"); break;
    case (int)0x887601e0: strcpy(szCode, "DDERR_TOOBIGSIZE");          strcpy(szMsg, "No message"); break;
    case (int)0x887601ea: strcpy(szCode, "DDERR_TOOBIGWIDTH");         strcpy(szMsg, "No message"); break;
    case (int)0x88760219: strcpy(szCode, "DDERR_VERTICALBLANKINPROGRESS"); strcpy(szMsg, "No message"); break;
    case (int)0x8876021c: strcpy(szCode, "DDERR_WASTILLDRAWING");      strcpy(szMsg, "The previous Blt which is transfering information to or from this Surface is incomplete"); break;
    case (int)0x88760233: strcpy(szCode, "DDERR_NODIRECTDRAWHW");      strcpy(szMsg, "No message"); break;
    case (int)0x88760232: strcpy(szCode, "DDERR_DIRECTDRAWALREADYCREATED"); strcpy(szMsg, "No message"); break;
    case (int)0x88760230: strcpy(szCode, "DDERR_XALIGN");             strcpy(szMsg, "Rectangle provided was not horizontally aligned on a DWORD boundary"); break;
    case (int)0x8876023a: strcpy(szCode, "DDERR_HWNDSUBCLASSED");      strcpy(szMsg, "No message"); break;
    case (int)0x8876023b: strcpy(szCode, "DDERR_HWNDALREADYSET");      strcpy(szMsg, "No message"); break;
    case (int)0x8876023d: strcpy(szCode, "DDERR_NOPALETTEHW");         strcpy(szMsg, "No hardware support for 16 or 256 color palettes"); break;
    case (int)0x88760234: strcpy(szCode, "DDERR_PRIMARYSURFACEALREADYEXISTS"); strcpy(szMsg, "This process already has created a primary surface"); break;
    case (int)0x88760245: strcpy(szCode, "DDERR_EXCLUSIVEMODEALREADYSET"); strcpy(szMsg, "No message"); break;
    case (int)0x88760248: strcpy(szCode, "DDERR_LOCKEDSURFACES");      strcpy(szMsg, "No message"); break;
    case 0:          strcpy(szCode, "DD_OK");                     strcpy(szMsg, "No error"); break;
    default: break;
    }

    if (g_logEnabled) {
        if (file == 0 || line <= 0)
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        else
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        DDrawLogLine(szLine);
    }
    if (g_msgBoxEnabled) {
        if (file == 0 || line <= 0)
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        else
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        MessageBoxA((HWND)0, szLine, "DirectDrawMgr", MB_ICONEXCLAMATION);
    }
}
