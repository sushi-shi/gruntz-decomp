// NetMgrReportError.cpp - CNetMgr::ReportError, the Net module's DirectPlay
// HRESULT->error-string diagnostic reporter (C:\Proj\NetMgr\, the DirectPlay
// sibling of CDirectDrawMgr::GetErrorString and the DInput/DSound formatters).
//
// Maps a DirectPlay error code to a "<DPERR_NAME> (<code>) - <description>"
// string and, depending on three reporting-mode globals, beeps, formats it
// and/or pops a "Net Manager" message box. `this` is unused; the work is driven
// entirely by the (file, line, hr) arguments the call sites push.
//
// Same archetype as CDirectDrawMgr::GetErrorString, with three retail-body
// differences:
//   (1) the working buffers szCode/szMsg and the saved code/hr are GLOBALS
//       (g_szCode/g_szMsg/g_code/g_hr in .data), not stack locals - only the
//       formatted output line szLine is a stack buffer here;
//   (2) there is NO early "any output wanted?" return before the switch: the
//       (hr & 0xffff) save + beep run unconditionally, the switch ALWAYS runs,
//       and the three reporting-mode gates are consulted only afterwards; and
//   (3) the dispatch is a cmp/je binary-search tree (sparse DPERR values), not
//       a jump table, and the log path is sprintf-only (no separate logger
//       call), like the DInput sibling.
//
// The function self-identifies its module via the strings it references (every
// DPERR_* name + "Net Manager"); names of locals/globals are placeholders, the
// switch case VALUES and string contents are load-bearing.
#include <Net/CNetMgrError.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA + BOOL/HWND/LPCSTR/UINT come from the real
// <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's uType is
// MB_ICONEXCLAMATION (0x30); the old hand-rolled macro mislabeled that value as
// the "hand" icon, whose real windows.h value is 0x10.
#include <Win32.h>

// ---------------------------------------------------------------------------
// Module-global state (all in .data). Unlike the DDraw/DInput/DSound siblings
// (which buffer everything on the stack), the Net reporter keeps the error-code
// name, the description, and the saved (code, hr) in fixed globals.
// ---------------------------------------------------------------------------
DATA(0x002bf6e8)
extern "C" i32 g_logEnabled; // 0x6bf6e8  - drives the format-line path
DATA(0x002bf6ec)
extern "C" i32 g_msgBoxEnabled; // 0x6bf6ec  - drives the MessageBox path
DATA(0x002bf6f0)
extern "C" i32 g_beepEnabled; // 0x6bf6f0  - gates the startup beep
DATA(0x002bf6f4)
extern "C" i32 g_thirdEnabled; // 0x6bf6f4  - third "any output wanted" gate

extern "C" i32 g_hr;   // 0x6bf6f8  - the raw HRESULT, saved at entry
extern "C" i32 g_code; // 0x6bf6fc  - hr & 0xffff (the (%i) arg)

extern "C" char g_szCode[]; // 0x6bf700  - error-code name buffer
extern "C" char g_szMsg[];  // 0x6bf740  - description buffer

// Empty mutable string in .data copied into the working line up front.
DATA(0x002293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// ---------------------------------------------------------------------------
// CNetMgr::ReportError
RVA(0x001776a0, 0xa01)
void CNetMgr::ReportError(char* file, i32 line, i32 hr, void* hWnd) {
    char szLine[512]; // the only stack buffer (the formatted output line)

    g_code = hr & 0xffff;
    g_hr = hr;
    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }

    strcpy(g_szMsg, "Unknown Error Message");
    sprintf(g_szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case (i32)0x80004001:
            strcpy(g_szCode, "DPERR_UNSUPPORTED");
            strcpy(g_szMsg, "The function is not available in this implementation.");
            break;
        case (i32)0x80004005:
            strcpy(g_szCode, "DPERR_GENERIC");
            strcpy(g_szMsg, "An undefined error condition occurred.");
            break;
        case (i32)0x8007000e:
            strcpy(g_szCode, "DPERR_OUTOFMEMORY");
            strcpy(g_szMsg, "There is insufficient memory to perform the requested operation.");
            break;
        case (i32)0x80070057:
            strcpy(g_szCode, "DPERR_INVALIDPARAMS");
            strcpy(g_szMsg, "One or more of the parameters passed to the function are invalid.");
            break;
        case (i32)0x88770005:
            strcpy(g_szCode, "DPERR_ALREADYINITIALIZED");
            strcpy(g_szMsg, "This object is already initialized.");
            break;
        case (i32)0x8877000a:
            strcpy(g_szCode, "DPERR_ACCESSDENIED");
            strcpy(g_szMsg, "The session is full or an incorrect password was supplied.");
            break;
        case (i32)0x88770014:
            strcpy(g_szCode, "DPERR_ACTIVEPLAYERS");
            strcpy(
                g_szMsg,
                "The requested operation cannot be performed because there are existing active "
                "players."
            );
            break;
        case (i32)0x8877001e:
            strcpy(g_szCode, "DPERR_BUFFERTOOSMALL");
            strcpy(
                g_szMsg,
                "The supplied buffer is not large enough to contain the requested data."
            );
            break;
        case (i32)0x88770028:
            strcpy(g_szCode, "DPERR_CANTADDPLAYER");
            strcpy(g_szMsg, "The player cannot be added to the session.");
            break;
        case (i32)0x88770032:
            strcpy(g_szCode, "DPERR_CANTCREATEGROUP");
            strcpy(g_szMsg, "A new group cannot be created.");
            break;
        case (i32)0x8877003c:
            strcpy(g_szCode, "DPERR_CANTCREATEPLAYER");
            strcpy(g_szMsg, "A new player cannot be created.");
            break;
        case (i32)0x88770046:
            strcpy(g_szCode, "DPERR_CANTCREATESESSION");
            strcpy(g_szMsg, "A new session cannot be created.");
            break;
        case (i32)0x88770050:
            strcpy(g_szCode, "DPERR_CAPSNOTAVAILABLEYET");
            strcpy(
                g_szMsg,
                "The capabilities of the DirectPlay object have not been determined yet."
            );
            break;
        case (i32)0x8877005a:
            strcpy(g_szCode, "DPERR_EXCEPTION");
            strcpy(g_szMsg, "An exception occurred when processing the request.");
            break;
        case (i32)0x88770078:
            strcpy(g_szCode, "DPERR_INVALIDFLAGS");
            strcpy(g_szMsg, "The flags passed to this function are invalid.");
            break;
        case (i32)0x88770082:
            strcpy(g_szCode, "DPERR_INVALIDOBJECT");
            strcpy(g_szMsg, "The DirectPlay object pointer is invalid.");
            break;
        case (i32)0x88770096:
            strcpy(g_szCode, "DPERR_INVALIDPLAYER");
            strcpy(
                g_szMsg,
                "The player ID is not recognized as a valid player ID for this game session."
            );
            break;
        case (i32)0x887700a0:
            strcpy(g_szCode, "DPERR_NOCAPS");
            strcpy(
                g_szMsg,
                "The communication link underneath DirectPlay is not capable of this function."
            );
            break;
        case (i32)0x887700aa:
            strcpy(g_szCode, "DPERR_NOCONNECTION");
            strcpy(g_szMsg, "No communication link was established.");
            break;
        case (i32)0x887700be:
            strcpy(g_szCode, "DPERR_NOMESSAGES");
            strcpy(g_szMsg, "There are no messages to be received.");
            break;
        case (i32)0x887700c8:
            strcpy(g_szCode, "DPERR_NONAMESERVERFOUND");
            strcpy(
                g_szMsg,
                "No name server (host) could be found or created. A host must exist in order to "
                "create a player."
            );
            break;
        case (i32)0x887700d2:
            strcpy(g_szCode, "DPERR_NOPLAYERS");
            strcpy(g_szMsg, "There are no active players in the session.");
            break;
        case (i32)0x887700dc:
            strcpy(g_szCode, "DPERR_NOSESSIONS");
            strcpy(g_szMsg, "There are no existing sessions for this game.");
            break;
        case (i32)0x887700e6:
            strcpy(g_szCode, "DPERR_SENDTOOBIG");
            strcpy(
                g_szMsg,
                "The message buffer passed to the IDirectPlay2::Send method is larger than allowed."
            );
            break;
        case (i32)0x887700f0:
            strcpy(g_szCode, "DPERR_TIMEOUT");
            strcpy(g_szMsg, "The operation could not be completed in the specified time.");
            break;
        case (i32)0x887700fa:
            strcpy(g_szCode, "DPERR_UNAVAILABLE");
            strcpy(g_szMsg, "The requested function is not available at this time.");
            break;
        case (i32)0x8877010e:
            strcpy(g_szCode, "DPERR_BUSY");
            strcpy(g_szMsg, "The DirectPlay message queue is full.");
            break;
        case (i32)0x88770118:
            strcpy(g_szCode, "DPERR_USERCANCEL");
            strcpy(
                g_szMsg,
                "The user canceled the connection process during a call to the IDirectPlay2::Open "
                "method."
            );
            break;
        case (i32)0x8877012c:
            strcpy(g_szCode, "DPERR_PLAYERLOST");
            strcpy(g_szMsg, "A player has lost the connection to the session.");
            break;
        case (i32)0x88770136:
            strcpy(g_szCode, "DPERR_SESSIONLOST");
            strcpy(g_szMsg, "The connection to the session has been lost.");
            break;
        case (i32)0x887703e8:
            strcpy(g_szCode, "DPERR_BUFFERTOOLARGE");
            strcpy(g_szMsg, "The data buffer is too large to store.");
            break;
        case (i32)0x887703f2:
            strcpy(g_szCode, "DPERR_CANTCREATEPROCESS");
            strcpy(g_szMsg, "Can't launch the application.");
            break;
        case (i32)0x887703fc:
            strcpy(g_szCode, "DPERR_APPNOTSTARTED");
            strcpy(g_szMsg, "The application has not been started yet.");
            break;
        case (i32)0x88770406:
            strcpy(g_szCode, "DPERR_INVALIDINTERFACE");
            strcpy(g_szMsg, "The interface parameter is invalid.");
            break;
        case (i32)0x8877041a:
            strcpy(g_szCode, "DPERR_UNKNOWNAPPLICATION");
            strcpy(g_szMsg, "An unknown application was specified.");
            break;
        case (i32)0x8877042e:
            strcpy(g_szCode, "DPERR_NOTLOBBIED");
            strcpy(
                g_szMsg,
                "Returned by IDirectPlayLobby::Connect if the application was not launched using "
                "IDirectPlayLobby::RunApplication"
            );
            break;
        case 0:
            strcpy(g_szCode, "DP_OK");
            strcpy(g_szMsg, "No error");
            break;
        default:
            break;
    }

    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled) {
        return;
    }

    if (g_logEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", g_szCode, g_code, g_szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, g_szCode, g_code, g_szMsg);
        }
    }
    if (g_msgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", g_szCode, g_code, g_szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, g_szCode, g_code, g_szMsg);
        }
        MessageBoxA((HWND)hWnd, szLine, "Net Manager", MB_ICONEXCLAMATION);
    }
}
