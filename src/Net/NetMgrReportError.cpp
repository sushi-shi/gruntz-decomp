#include <rva.h>

#include <Net/NetMgrReportError.h>

#include <Enums.h>
#include <Net/EmptyString.h>
#include <Net/NetMgr.h>

#include <dplay.h>
#include <stdio.h>
#include <string.h>

DATA(0x002bf6e8)
i32 g_logEnabled = 0;
DATA(0x002bf6ec)
i32 g_msgBoxEnabled = 0;
DATA(0x002bf6f0)
i32 g_beepEnabled = 0;
DATA(0x002bf6f4)
i32 g_thirdEnabled = 0;
DATA(0x002bf6f8)
i32 g_hr = 0;
DATA(0x002bf6fc)
i32 g_code = 0;
DATA(0x002bf700)
char g_szCode[0x40];
DATA(0x002bf740)
char g_szMsg[0x100];

DATA(0x002293f4)
char g_emptyString[] = "";

RVA(0x00177670, 0x27)
void CNetMgr::SetReportMode(i32 log, i32 msgBox, i32 beep, i32 third) {
    g_logEnabled = log;
    g_msgBoxEnabled = msgBox;
    g_beepEnabled = beep;
    g_thirdEnabled = third;
}

RVA(0x001776a0, 0xa01)
void CNetMgr::ReportError(char* file, i32 line, i32 hr, void* hWnd) {
    char szLine[512];

    g_code = hr & 0xffff;
    g_hr = hr;
    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }

    strcpy(g_szMsg, "Unknown Error Message");
    sprintf(g_szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    // Eight of these arms keep their raw HRESULT: DPERR_APPNOTSTARTED,
    // BUFFERTOOLARGE, CANTCREATEPROCESS, INVALIDINTERFACE, NOTLOBBIED,
    // PLAYERLOST, SESSIONLOST and UNKNOWNAPPLICATION are not in the DPLAY.H
    // that MSVC 5.0 ships - retail built against a later DirectPlay SDK than
    // its own compiler bundled. The names survive in the strings either way.
    switch (hr) {
        case static_cast<i32>(DPERR_UNSUPPORTED):
            strcpy(g_szCode, "DPERR_UNSUPPORTED");
            strcpy(g_szMsg, "The function is not available in this implementation.");
            break;
        case static_cast<i32>(DPERR_GENERIC):
            strcpy(g_szCode, "DPERR_GENERIC");
            strcpy(g_szMsg, "An undefined error condition occurred.");
            break;
        case static_cast<i32>(DPERR_OUTOFMEMORY):
            strcpy(g_szCode, "DPERR_OUTOFMEMORY");
            strcpy(g_szMsg, "There is insufficient memory to perform the requested operation.");
            break;
        case static_cast<i32>(DPERR_INVALIDPARAMS):
            strcpy(g_szCode, "DPERR_INVALIDPARAMS");
            strcpy(g_szMsg, "One or more of the parameters passed to the function are invalid.");
            break;
        case static_cast<i32>(DPERR_ALREADYINITIALIZED):
            strcpy(g_szCode, "DPERR_ALREADYINITIALIZED");
            strcpy(g_szMsg, "This object is already initialized.");
            break;
        case static_cast<i32>(DPERR_ACCESSDENIED):
            strcpy(g_szCode, "DPERR_ACCESSDENIED");
            strcpy(g_szMsg, "The session is full or an incorrect password was supplied.");
            break;
        case static_cast<i32>(DPERR_ACTIVEPLAYERS):
            strcpy(g_szCode, "DPERR_ACTIVEPLAYERS");
            strcpy(
                g_szMsg,
                "The requested operation cannot be performed because there are existing active "
                "players."
            );
            break;
        case static_cast<i32>(DPERR_BUFFERTOOSMALL):
            strcpy(g_szCode, "DPERR_BUFFERTOOSMALL");
            strcpy(
                g_szMsg,
                "The supplied buffer is not large enough to contain the requested data."
            );
            break;
        case static_cast<i32>(DPERR_CANTADDPLAYER):
            strcpy(g_szCode, "DPERR_CANTADDPLAYER");
            strcpy(g_szMsg, "The player cannot be added to the session.");
            break;
        case static_cast<i32>(DPERR_CANTCREATEGROUP):
            strcpy(g_szCode, "DPERR_CANTCREATEGROUP");
            strcpy(g_szMsg, "A new group cannot be created.");
            break;
        case static_cast<i32>(DPERR_CANTCREATEPLAYER):
            strcpy(g_szCode, "DPERR_CANTCREATEPLAYER");
            strcpy(g_szMsg, "A new player cannot be created.");
            break;
        case static_cast<i32>(DPERR_CANTCREATESESSION):
            strcpy(g_szCode, "DPERR_CANTCREATESESSION");
            strcpy(g_szMsg, "A new session cannot be created.");
            break;
        case static_cast<i32>(DPERR_CAPSNOTAVAILABLEYET):
            strcpy(g_szCode, "DPERR_CAPSNOTAVAILABLEYET");
            strcpy(
                g_szMsg,
                "The capabilities of the DirectPlay object have not been determined yet."
            );
            break;
        case static_cast<i32>(DPERR_EXCEPTION):
            strcpy(g_szCode, "DPERR_EXCEPTION");
            strcpy(g_szMsg, "An exception occurred when processing the request.");
            break;
        case static_cast<i32>(DPERR_INVALIDFLAGS):
            strcpy(g_szCode, "DPERR_INVALIDFLAGS");
            strcpy(g_szMsg, "The flags passed to this function are invalid.");
            break;
        case static_cast<i32>(DPERR_INVALIDOBJECT):
            strcpy(g_szCode, "DPERR_INVALIDOBJECT");
            strcpy(g_szMsg, "The DirectPlay object pointer is invalid.");
            break;
        case static_cast<i32>(DPERR_INVALIDPLAYER):
            strcpy(g_szCode, "DPERR_INVALIDPLAYER");
            strcpy(
                g_szMsg,
                "The player ID is not recognized as a valid player ID for this game session."
            );
            break;
        case static_cast<i32>(DPERR_NOCAPS):
            strcpy(g_szCode, "DPERR_NOCAPS");
            strcpy(
                g_szMsg,
                "The communication link underneath DirectPlay is not capable of this function."
            );
            break;
        case static_cast<i32>(DPERR_NOCONNECTION):
            strcpy(g_szCode, "DPERR_NOCONNECTION");
            strcpy(g_szMsg, "No communication link was established.");
            break;
        case static_cast<i32>(DPERR_NOMESSAGES):
            strcpy(g_szCode, "DPERR_NOMESSAGES");
            strcpy(g_szMsg, "There are no messages to be received.");
            break;
        case static_cast<i32>(DPERR_NONAMESERVERFOUND):
            strcpy(g_szCode, "DPERR_NONAMESERVERFOUND");
            strcpy(
                g_szMsg,
                "No name server (host) could be found or created. A host must exist in order to "
                "create a player."
            );
            break;
        case static_cast<i32>(DPERR_NOPLAYERS):
            strcpy(g_szCode, "DPERR_NOPLAYERS");
            strcpy(g_szMsg, "There are no active players in the session.");
            break;
        case static_cast<i32>(DPERR_NOSESSIONS):
            strcpy(g_szCode, "DPERR_NOSESSIONS");
            strcpy(g_szMsg, "There are no existing sessions for this game.");
            break;
        case static_cast<i32>(DPERR_SENDTOOBIG):
            strcpy(g_szCode, "DPERR_SENDTOOBIG");
            strcpy(
                g_szMsg,
                "The message buffer passed to the IDirectPlay2::Send method is larger than allowed."
            );
            break;
        case static_cast<i32>(DPERR_TIMEOUT):
            strcpy(g_szCode, "DPERR_TIMEOUT");
            strcpy(g_szMsg, "The operation could not be completed in the specified time.");
            break;
        case static_cast<i32>(DPERR_UNAVAILABLE):
            strcpy(g_szCode, "DPERR_UNAVAILABLE");
            strcpy(g_szMsg, "The requested function is not available at this time.");
            break;
        case static_cast<i32>(DPERR_BUSY):
            strcpy(g_szCode, "DPERR_BUSY");
            strcpy(g_szMsg, "The DirectPlay message queue is full.");
            break;
        case static_cast<i32>(DPERR_USERCANCEL):
            strcpy(g_szCode, "DPERR_USERCANCEL");
            strcpy(
                g_szMsg,
                "The user canceled the connection process during a call to the IDirectPlay2::Open "
                "method."
            );
            break;
        case static_cast<i32>(0x8877012c):
            strcpy(g_szCode, "DPERR_PLAYERLOST");
            strcpy(g_szMsg, "A player has lost the connection to the session.");
            break;
        case static_cast<i32>(0x88770136):
            strcpy(g_szCode, "DPERR_SESSIONLOST");
            strcpy(g_szMsg, "The connection to the session has been lost.");
            break;
        case static_cast<i32>(0x887703e8):
            strcpy(g_szCode, "DPERR_BUFFERTOOLARGE");
            strcpy(g_szMsg, "The data buffer is too large to store.");
            break;
        case static_cast<i32>(0x887703f2):
            strcpy(g_szCode, "DPERR_CANTCREATEPROCESS");
            strcpy(g_szMsg, "Can't launch the application.");
            break;
        case static_cast<i32>(0x887703fc):
            strcpy(g_szCode, "DPERR_APPNOTSTARTED");
            strcpy(g_szMsg, "The application has not been started yet.");
            break;
        case static_cast<i32>(0x88770406):
            strcpy(g_szCode, "DPERR_INVALIDINTERFACE");
            strcpy(g_szMsg, "The interface parameter is invalid.");
            break;
        case static_cast<i32>(0x8877041a):
            strcpy(g_szCode, "DPERR_UNKNOWNAPPLICATION");
            strcpy(g_szMsg, "An unknown application was specified.");
            break;
        case static_cast<i32>(0x8877042e):
            strcpy(g_szCode, "DPERR_NOTLOBBIED");
            strcpy(
                g_szMsg,
                "Returned by IDirectPlayLobby::Connect if the application was not launched using "
                "IDirectPlayLobby::RunApplication"
            );
            break;
        case DP_OK:
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
        MessageBoxA(static_cast<HWND>(hWnd), szLine, "Net Manager", MB_ICONEXCLAMATION);
    }
}
