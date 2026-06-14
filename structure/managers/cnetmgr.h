#ifndef MANAGERS_CNETMGR_H
#define MANAGERS_CNETMGR_H

/*
 * CNetMgr — DirectPlay networking / multiplayer manager.
 * .?AVCNetMgr@@  (size 0x4 known: a CObject-derived polymorphic object — only the
 * vtable pointer @+0 is recovered; instance fields are still unknown.)
 *
 * Leaked source TU:  C:\Proj\NetMgr\NetMgr.cpp  (shared header c:\proj\incs\netmgr.h)
 *
 * Layout ported from tomalla (attributed). tomalla recovered only the static
 * error-state globals + the ReportError signature + ctor/dtor addresses; the
 * instance vtable + fields beyond the vptr are unknown. @address values = 1.0.1.77.
 *
 * Net stack: DPLAYX.dll (DirectPlay). Lockstep model
 * ("Using CmdDelay of %d and ResendDelay of %d."). Full DPERR_* stringify table
 * embedded. Lobby launch via IDirectPlayLobby (LOBBYLAUNCH).
 */

typedef void *HWND;     // Win32 handle (4-byte pointer); avoid pulling <afxwin.h>.
typedef long  HRESULT;  // COM status code.

// MFC root base (vptr only); CNetMgr's vptr lands at +0x00 from it.
class CObject { public: virtual ~CObject() {} };

class CNetMgr : public CObject
{
public:
    // ctor (inlined) 0x4b55fe / dtor 0x4b5ff0 (1.0.1.77). Instance fields unknown.
    static void ReportError(char *szFile, int lineNumber, HRESULT hResult, HWND hWnd);

    //@address: 006c0640
    static bool is_error_log;
    //@address: 006c0644
    static bool is_error_messagebox;
    //@address: 006c0648
    static bool is_error_beep;
    //@address: 006c064c
    static bool is_error_unknown_flag;
    //@address: 006c0650
    static int  error_hresult;
    //@address: 006c0654
    static int  error_hresult_low;
    //@address: 006c0658
    static char error_string_code[64];
    //@address: 006c0698
    static char error_string_message[256];
};                                          // 0x4 (vptr only; fields unknown)

#endif /* MANAGERS_CNETMGR_H */
