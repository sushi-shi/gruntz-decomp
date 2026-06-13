#ifndef MANAGERS_CNETMGR_H
#define MANAGERS_CNETMGR_H

/*
 * CNetMgr — DirectPlay networking / multiplayer manager.
 * .?AVCNetMgr@@
 *
 * Leaked source TU:  C:\Proj\NetMgr\NetMgr.cpp  (shared header c:\proj\incs\netmgr.h)
 *
 * Layout PORTED FROM tomalla (refs/tomalla-gruntz/dx/cnetmgr.h), attributed.
 * The class name IS in RTTI (.?AVCNetMgr@@). tomalla recovered only the static
 * error-state globals + the ReportError signature + ctor/dtor addresses; the
 * instance vtable + fields are still @todo. @address values = 1.0.1.77.
 *
 * Net stack: DPLAYX.dll (DirectPlay). Lockstep model
 * ("Using CmdDelay of %d and ResendDelay of %d."). Full DPERR_* stringify table
 * embedded. Lobby launch via IDirectPlayLobby (LOBBYLAUNCH).
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

class CNetMgr : public CObject
{
public:
    //@todo: vftable, instance fields etc.
    //destructor:          004b5ff0   (1.0.1.77)
    //constructor (inlined): 004b55fe (1.0.1.77)

    static void ReportError(char* szFile, int lineNumber, HRESULT hResult, HWND hWnd);

    //@address: 006c0640
    static bool is_error_log;
    //@address: 006c0644
    static bool is_error_messagebox;
    //@address: 006c0648
    static bool is_error_beep;
    //@address: 006c064c
    static bool is_error_unknown_flag;
    //@address: 006c0650
    static int error_hresult;
    //@address: 006c0654
    static int error_hresult_low;
    //@address: 006c0658
    static char error_string_code[64];
    //@address: 006c0698
    static char error_string_message[256];
};

#endif /* MANAGERS_CNETMGR_H */
