#include <rva.h>

#include <Net/NetMgrReportError.h>

#include <Enums.h>
#include <Net/NetMgr.h>

#include <dplay.h>
#include <stdio.h>
#include <string.h>

DATA(0x002bf6e8)
b32 g_logEnabled = FALSE;
DATA(0x002bf6ec)
b32 g_msgBoxEnabled = FALSE;
DATA(0x002bf6f0)
b32 g_beepEnabled = FALSE;
DATA(0x002bf6f4)
b32 g_unknownOptionEnabled = FALSE;
DATA(0x002bf6f8)
HRESULT g_hr = 0;
DATA(0x002bf6fc)
i32 g_code = 0; // should be a DWORD
DATA(0x002bf700)
char g_szCode[0x40];
DATA(0x002bf740)
char g_szMsg[0x100];

RVA(0x00177670, 0x27)
void CNetMgr::SetReportMode(b32 log, b32 msgBox, b32 beep, b32 unknownOption) {
    g_logEnabled = log;
    g_msgBoxEnabled = msgBox;
    g_beepEnabled = beep;
    g_unknownOptionEnabled = unknownOption;
}

inline static void SetError(const char* szCode, const char* szDesc) {
    strcpy(g_szCode, szCode);
    strcpy(g_szMsg, szDesc);
}

RVA(0x001776a0, 0xa01)
void CNetMgr::ReportError(const char* file, i32 line, HRESULT hr, HWND hWnd) {
    char szLine[512];

    g_code = static_cast<i32>(HRESULT_CODE(hr));
    g_hr = hr;

    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }

    strcpy(g_szMsg, "Unknown Error Message");
    sprintf(g_szCode, "Unknown Error Code");
    strcpy(szLine, "");

    switch (hr) {
        case DPERR_UNSUPPORTED:
            SetError("DPERR_UNSUPPORTED", "The function is not available in this implementation.");
            break;
        case DPERR_GENERIC:
            SetError("DPERR_GENERIC", "An undefined error condition occurred.");
            break;
        case DPERR_OUTOFMEMORY:
            SetError(
                "DPERR_OUTOFMEMORY",
                "There is insufficient memory to perform the requested operation."
            );
            break;
        case DPERR_INVALIDPARAMS:
            SetError(
                "DPERR_INVALIDPARAMS",
                "One or more of the parameters passed to the function are invalid."
            );
            break;
        case DPERR_ALREADYINITIALIZED:
            SetError("DPERR_ALREADYINITIALIZED", "This object is already initialized.");
            break;
        case DPERR_ACCESSDENIED:
            SetError(
                "DPERR_ACCESSDENIED",
                "The session is full or an incorrect password was supplied."
            );
            break;
        case DPERR_ACTIVEPLAYERS:
            SetError(
                "DPERR_ACTIVEPLAYERS",
                "The requested operation cannot be performed because there are existing active "
                "players."
            );
            break;
        case DPERR_BUFFERTOOSMALL:
            SetError(
                "DPERR_BUFFERTOOSMALL",
                "The supplied buffer is not large enough to contain the requested data."
            );
            break;
        case DPERR_CANTADDPLAYER:
            SetError("DPERR_CANTADDPLAYER", "The player cannot be added to the session.");
            break;
        case DPERR_CANTCREATEGROUP:
            SetError("DPERR_CANTCREATEGROUP", "A new group cannot be created.");
            break;
        case DPERR_CANTCREATEPLAYER:
            SetError("DPERR_CANTCREATEPLAYER", "A new player cannot be created.");
            break;
        case DPERR_CANTCREATESESSION:
            SetError("DPERR_CANTCREATESESSION", "A new session cannot be created.");
            break;
        case DPERR_CAPSNOTAVAILABLEYET:
            SetError(
                "DPERR_CAPSNOTAVAILABLEYET",
                "The capabilities of the DirectPlay object have not been determined yet."
            );
            break;
        case DPERR_EXCEPTION:
            SetError("DPERR_EXCEPTION", "An exception occurred when processing the request.");
            break;
        case DPERR_INVALIDFLAGS:
            SetError("DPERR_INVALIDFLAGS", "The flags passed to this function are invalid.");
            break;
        case DPERR_INVALIDOBJECT:
            SetError("DPERR_INVALIDOBJECT", "The DirectPlay object pointer is invalid.");
            break;
        case DPERR_INVALIDPLAYER:
            SetError(
                "DPERR_INVALIDPLAYER",
                "The player ID is not recognized as a valid player ID for this game session."
            );
            break;
        case DPERR_NOCAPS:
            SetError(
                "DPERR_NOCAPS",
                "The communication link underneath DirectPlay is not capable of this function."
            );
            break;
        case DPERR_NOCONNECTION:
            SetError("DPERR_NOCONNECTION", "No communication link was established.");
            break;
        case DPERR_NOMESSAGES:
            SetError("DPERR_NOMESSAGES", "There are no messages to be received.");
            break;
        case DPERR_NONAMESERVERFOUND:
            SetError(
                "DPERR_NONAMESERVERFOUND",
                "No name server (host) could be found or created. A host must exist in order to "
                "create a player."
            );
            break;
        case DPERR_NOPLAYERS:
            SetError("DPERR_NOPLAYERS", "There are no active players in the session.");
            break;
        case DPERR_NOSESSIONS:
            SetError("DPERR_NOSESSIONS", "There are no existing sessions for this game.");
            break;
        case DPERR_SENDTOOBIG:
            SetError(
                "DPERR_SENDTOOBIG",
                "The message buffer passed to the IDirectPlay2::Send method is larger than allowed."
            );
            break;
        case DPERR_TIMEOUT:
            SetError(
                "DPERR_TIMEOUT",
                "The operation could not be completed in the specified time."
            );
            break;
        case DPERR_UNAVAILABLE:
            SetError("DPERR_UNAVAILABLE", "The requested function is not available at this time.");
            break;
        case DPERR_BUSY:
            SetError("DPERR_BUSY", "The DirectPlay message queue is full.");
            break;
        case DPERR_USERCANCEL:
            SetError(
                "DPERR_USERCANCEL",
                "The user canceled the connection process during a call to the IDirectPlay2::Open "
                "method."
            );
            break;
        case DPERR_PLAYERLOST:
            SetError("DPERR_PLAYERLOST", "A player has lost the connection to the session.");
            break;
        case DPERR_SESSIONLOST:
            SetError("DPERR_SESSIONLOST", "The connection to the session has been lost.");
            break;
        case DPERR_BUFFERTOOLARGE:
            SetError("DPERR_BUFFERTOOLARGE", "The data buffer is too large to store.");
            break;
        case DPERR_CANTCREATEPROCESS:
            SetError("DPERR_CANTCREATEPROCESS", "Can't launch the application.");
            break;
        case DPERR_APPNOTSTARTED:
            SetError("DPERR_APPNOTSTARTED", "The application has not been started yet.");
            break;
        case DPERR_INVALIDINTERFACE:
            SetError("DPERR_INVALIDINTERFACE", "The interface parameter is invalid.");
            break;
        case DPERR_UNKNOWNAPPLICATION:
            SetError("DPERR_UNKNOWNAPPLICATION", "An unknown application was specified.");
            break;
        case DPERR_NOTLOBBIED:
            SetError(
                "DPERR_NOTLOBBIED",
                "Returned by IDirectPlayLobby::Connect if the application was not launched using "
                "IDirectPlayLobby::RunApplication"
            );
            break;
        case DP_OK:
            SetError("DP_OK", "No error");
            break;
        default:
            break;
    }

    if (!g_logEnabled && !g_msgBoxEnabled && !g_unknownOptionEnabled) {
        return;
    }

    if (g_logEnabled) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", g_szCode, g_code, g_szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, g_szCode, g_code, g_szMsg);
        }
    }

    if (g_msgBoxEnabled) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", g_szCode, g_code, g_szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, g_szCode, g_code, g_szMsg);
        }

        MessageBoxA(hWnd, szLine, "Net Manager", MB_ICONEXCLAMATION);
    }

    // The unknown third option (g_unknownOptionEnabled) was probably only present in the debug build.
}
