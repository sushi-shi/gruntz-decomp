// NetMgr.cpp - CNetMgr (DirectPlay networking manager) handlers, config writer,
// and the DirectPlay error reporter.
// Matched methods:
//   CNetMgr::OnMultiOptions       @ RVA 0x0badd0 (67 B)
//   CNetMgr::OnMultiPause         @ RVA 0x0bad40 (108 B)
//   CNetMgr::OnOutOfSync          @ RVA 0x0bae40 (132 B)
//   CNetMgr::ApplyCmdDelayDefaults@ RVA 0x0b85a0 (210 B)
//
// The three message handlers fire a multiplayer command through the engine
// dispatcher (MultiDispatch, external, via incremental-link thunk), guarded by
// a reentrancy flag, and (Pause/OutOfSync) forward a WM_COMMAND to the engine
// window via PostMessageA when the dispatch result matches. ApplyCmdDelayDefaults
// persists the command-timing config (m_5a4/m_5a8) to the game's RegistryHelper.
//
// CNetMgr::ReportError maps DirectPlay HRESULTs to DPERR symbol names + human
// descriptions, writing to the TU's static error-reporting buffers.
#include "NetMgr.h"
#include <string.h>

CGameMgr *g_pGameMgr;                     // @0x64556c

// File-scope reentrancy guards.
static int g_optionzGuard;               // @0x648d08
static int g_pauseGuard;                 // @0x648d04
static int g_sharedFlag;                 // @0x648ce0

// ---------------------------------------------------------------------------
// CNetMgr::ReportError - error-report globals  (pinned @data).
// The function writes its results into these TU-static buffers.
// The 4 report-mode flags control output formatting.
// ---------------------------------------------------------------------------
extern "C" int __cdecl EngFormat(char *dest, const char *fmt, ...);  // @0x11f890

static int   g_repFlagE8;          // @0x6bf6e8  (report-mode flag)
static int   g_repFlagEC;          // @0x6bf6ec  (report-mode flag)
static int   g_repBeepFlag;        // @0x6bf6f0  (MessageBeep gate)
static int   g_repFlagF4;          // @0x6bf6f4  (report-mode flag)
static long  g_repFullHr;          // @0x6bf6f8  (full HRESULT raw value)
static int   g_repHrLow;           // @0x6bf6fc  (HRESULT & 0xFFFF)
static char  g_repSym[0x40];       // @0x6bf700  (DPERR symbol name buffer)
static char  g_repDesc[0x40];      // @0x6bf740  (human-readable description buffer)

// Format strings
static const char s_fmtDebug[]       = "%s, line %i: %s (%i) - %s\n";  // @0x619a20
static const char s_fmtDebugShort[]  = "%s (%i) - %s\n";               // @0x619a10
static const char s_fmtMsgBox[]      = "%s, line %i\n\n%s (%i)\n\n%s"; // @0x6199f4
static const char s_fmtMsgBoxShort[] = "%s (%i)\n\n%s";                // @0x6199e8
static const char s_netMgrModule[]   = "Net Manager";                  // @0x624280

// Utility strings
static const char s_unknownErrMsg[]  = "Unknown Error Message";         // @0x619ec0
static const char s_unknownErrCode[] = "Unknown Error Code";            // @0x619eac
static const char s_noError[]        = "No error";                     // @0x619ad4

// DPERR symbol + description strings
static const char s_dperrUnsupported[]  = "DPERR_UNSUPPORTED";         // @0x624d24
static const char s_descUnsupported[]   = "The function is not available in this implementation."; // @0x624cec
static const char s_dperrGeneric[]      = "DPERR_GENERIC";             // @0x624cdc
static const char s_descGeneric[]       = "An undefined error condition occurred."; // @0x624cb4
static const char s_dperrOutOfMem[]     = "DPERR_OUTOFMEMORY";         // @0x624ca0
static const char s_descOutOfMem[]      = "There is insufficient memory to perform the requested operation."; // @0x624c5c
static const char s_dperrInvalidParams[]= "DPERR_INVALIDPARAMS";       // @0x624c48
static const char s_descInvalidParams[] = "One or more of the parameters passed to the function are invalid."; // @0x624c04
static const char s_dperrAlreadyInit[]  = "DPERR_ALREADYINITIALIZED";  // @0x624be8
static const char s_descAlreadyInit[]   = "This object is already initialized."; // @0x619a94
static const char s_dperrAccessDenied[] = "DPERR_ACCESSDENIED";        // @0x624bd4
static const char s_descAccessDenied[]  = "The session is full or an incorrect password was supplied."; // @0x624b98
static const char s_dperrActivePlayers[]= "DPERR_ACTIVEPLAYERS";       // @0x624b84
static const char s_descActivePlayers[] = "The requested operation cannot be performed because there are existing active players."; // @0x624b2c
static const char s_dperrBufferSmall[]  = "DPERR_BUFFERTOOSMALL";      // @0x624b14
static const char s_descBufferSmall[]   = "The supplied buffer is not large enough to contain the requested data."; // @0x624acc
static const char s_dperrCantAddPlayer[]= "DPERR_CANTADDPLAYER";       // @0x624ab8
static const char s_descCantAddPlayer[] = "The player cannot be added to the session."; // @0x624a8c
static const char s_dperrCantCreateGroup[] = "DPERR_CANTCREATEGROUP";  // @0x624a74
static const char s_descCantCreateGroup[]= "A new group cannot be created."; // @0x624a54
static const char s_dperrCantCreatePlayer[]="DPERR_CANTCREATEPLAYER";  // @0x624a3c
static const char s_descCantCreatePlayer[]="A new player cannot be created."; // @0x624a1c
static const char s_dperrCantCreateSession[]="DPERR_CANTCREATESESSION"; // @0x624a04
static const char s_descCantCreateSession[]="A new session cannot be created."; // @0x6249e0
static const char s_dperrCapsNotAvail[] = "DPERR_CAPSNOTAVAILABLEYET"; // @0x6249c4
static const char s_descCapsNotAvail[]  = "The capabilities of the DirectPlay object have not been determined yet."; // @0x62497c
static const char s_dperrException[]    = "DPERR_EXCEPTION";           // @0x62496c
static const char s_descException[]     = "An exception occurred when processing the request."; // @0x624938
static const char s_dperrInvalidFlags[] = "DPERR_INVALIDFLAGS";        // @0x624924
static const char s_descInvalidFlags[]  = "The flags passed to this function are invalid."; // @0x6248f4
static const char s_dperrInvalidObject[]= "DPERR_INVALIDOBJECT";       // @0x6248e0
static const char s_descInvalidObject[] = "The DirectPlay object pointer is invalid."; // @0x6248b4
static const char s_dperrInvalidPlayer[]= "DPERR_INVALIDPLAYER";       // @0x6248a0
static const char s_descInvalidPlayer[] = "The player ID is not recognized as a valid player ID for this game session."; // @0x624854
static const char s_dperrNoCaps[]       = "DPERR_NOCAPS";              // @0x624844
static const char s_descNoCaps[]        = "The communication link underneath DirectPlay is not capable of this function."; // @0x6247f4
static const char s_dperrNoConnection[] = "DPERR_NOCONNECTION";        // @0x6247e0
static const char s_descNoConnection[]  = "No communication link was established."; // @0x6247b8
static const char s_dperrNoMessages[]   = "DPERR_NOMESSAGES";          // @0x6247a4
static const char s_descNoMessages[]    = "There are no messages to be received."; // @0x62477c
static const char s_dperrNoNameServer[] = "DPERR_NONAMESERVERFOUND";   // @0x624764
static const char s_descNoNameServer[]  = "No name server (host) could be found or created. A host must exist in order to create a player."; // @0x624704
static const char s_dperrNoPlayers[]    = "DPERR_NOPLAYERS";           // @0x6246f4
static const char s_descNoPlayers[]     = "There are no active players in the session."; // @0x6246c8
static const char s_dperrNoSessions[]   = "DPERR_NOSESSIONS";          // @0x6246b4
static const char s_descNoSessions[]    = "There are no existing sessions for this game."; // @0x624684
static const char s_dperrSendTooBig[]   = "DPERR_SENDTOOBIG";          // @0x624670
static const char s_descSendTooBig[]    = "The message buffer passed to the IDirectPlay2::Send method is larger than allowed."; // @0x62461c
static const char s_dperrTimeout[]      = "DPERR_TIMEOUT";             // @0x62460c
static const char s_descTimeout[]       = "The operation could not be completed in the specified time."; // @0x6245d0
static const char s_dperrUnavailable[]  = "DPERR_UNAVAILABLE";         // @0x6245bc
static const char s_descUnavailable[]   = "The requested function is not available at this time."; // @0x624584
static const char s_dperrBusy[]         = "DPERR_BUSY";                // @0x624578
static const char s_descBusy[]          = "The DirectPlay message queue is full."; // @0x624550
static const char s_dperrUserCancel[]   = "DPERR_USERCANCEL";          // @0x62453c
static const char s_descUserCancel[]    = "The user canceled the connection process during a call to the IDirectPlay2::Open method."; // @0x6244e0
static const char s_dperrPlayerLost[]   = "DPERR_PLAYERLOST";          // @0x6244cc
static const char s_descPlayerLost[]    = "A player has lost the connection to the session."; // @0x624498
static const char s_dperrSessionLost[]  = "DPERR_SESSIONLOST";         // @0x624484
static const char s_descSessionLost[]   = "The connection to the session has been lost."; // @0x624454
static const char s_dperrBufferLarge[]  = "DPERR_BUFFERTOOLARGE";      // @0x62443c
static const char s_descBufferLarge[]   = "The data buffer is too large to store."; // @0x624414
static const char s_dperrCantCreateProc[]="DPERR_CANTCREATEPROCESS";   // @0x6243fc
static const char s_descCantCreateProc[]="Can't launch the application."; // @0x6243dc
static const char s_dperrAppNotStarted[]="DPERR_APPNOTSTARTED";        // @0x6243c8
static const char s_descAppNotStarted[] = "The application has not been started yet."; // @0x62439c
static const char s_dperrInvalidIf[]    = "DPERR_INVALIDINTERFACE";    // @0x624384
static const char s_descInvalidIf[]     = "The interface parameter is invalid."; // @0x624360
static const char s_dpOkStr[]           = "DP_OK";                    // @0x624358
static const char s_dperrNotLobbied[]   = "DPERR_NOTLOBBIED";          // @0x624344
static const char s_descNotLobbied[]    = "Returned by IDirectPlayLobby::Connect if the application was not launched using IDirectPlayLobby::RunApplication"; // @0x6242d0
static const char s_dperrUnknownApp[]   = "DPERR_UNKNOWNAPPLICATION";  // @0x6242b4
static const char s_descUnknownApp[]    = "An unknown application was specified."; // @0x62428c

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiOptions  @ 0x0badd0  (__thiscall, ret).
// Reentrancy-guarded fire of the MULTI_OPTIONZ command. Clears m_584, dispatches
// (return value ignored), then clears the shared flag.
//
// @address: 0x0badd0
// @size:    0x43
// ---------------------------------------------------------------------------
void CNetMgr::OnMultiOptions()
{
    if (g_optionzGuard)
        return;

    m_584 = 0;
    g_optionzGuard = 1;
    MultiDispatch("MULTI_OPTIONZ", MultiOptionzCallback, 0);
    g_optionzGuard = 0;
    g_sharedFlag = 0;
}

// ---------------------------------------------------------------------------
// CNetMgr::OnMultiPause  @ 0x0bad40  (__thiscall, ret).
// Reentrancy-guarded fire of MULTI_PAUSE. When the dispatch returns 0x4cc,
// forwards WM_COMMAND(0x80d7, m_1c) to the engine window.
//
// @address: 0x0bad40
// @size:    0x6c
// ---------------------------------------------------------------------------
void CNetMgr::OnMultiPause()
{
    if (g_pauseGuard)
        return;

    m_584 = 0;
    g_pauseGuard = 1;
    int r = MultiDispatch("MULTI_PAUSE", MultiPauseCallback, 0);
    g_pauseGuard = 0;
    g_sharedFlag = 0;

    if (r == 0x4cc) {
        void *hwnd = ((CNetHwndHolder *)((CNetHwndHolder *)m_4)->m_4)->m_4;
        PostMessageA((HWND)hwnd, 0x111, 0x80d7, m_1c);
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::OnOutOfSync  @ 0x0bae40  (__thiscall, ret).
// Per-instance reentrancy-guarded fire of MULTI_OUTOFSYNC. Switches on the
// dispatch result: 0x4cc -> the same WM_COMMAND(0x80d7, m_1c) as Pause;
// 0x4cd -> nothing; otherwise -> WM_COMMAND(0x8023, 0).
//
// @address: 0x0bae40
// @size:    0x84
// ---------------------------------------------------------------------------
void CNetMgr::OnOutOfSync()
{
    if (m_574)
        return;

    m_574 = 1;
    m_584 = 0;
    int r = MultiDispatch("MULTI_OUTOFSYNC", MultiOutOfSyncCallback, 0);
    g_sharedFlag = 0;

    switch (r) {
    case 0x4cc: {
        void *hwnd = ((CNetHwndHolder *)((CNetHwndHolder *)m_4)->m_4)->m_4;
        PostMessageA((HWND)hwnd, 0x111, 0x80d7, m_1c);
        break;
    }
    case 0x4cd:
        break;
    default: {
        void *hwnd = ((CNetHwndHolder *)((CNetHwndHolder *)m_4)->m_4)->m_4;
        PostMessageA((HWND)hwnd, 0x111, 0x8023, 0);
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// CNetMgr::ApplyCmdDelayDefaults  @ 0x0b85a0  (__thiscall, ret; C++ EH frame).
// Persists the command-timing config to the game RegistryHelper (the singleton's
// +0x38 member). Builds three value-name strings "m_598 + _Suffix" via operator+
// (each a stack CString temp), writes m_5a4 under "_CmdDelay" and m_5a8 under
// "_Resend"; the "_DynCmdDelay" temp is built but its write is elided here. The
// three temporaries' dtors run under the C++ EH frame (=> /GX).
//
// @address: 0x0b85a0
// @size:    0xd2
// ---------------------------------------------------------------------------
void CNetMgr::ApplyCmdDelayDefaults()
{
    Utils::RegistryHelper *reg = g_pGameMgr->m_38;

    AfxString cmdDelayName  = m_598 + "_CmdDelay";
    AfxString resendName    = m_598 + "_Resend";
    AfxString dynCmdName    = m_598 + "_DynCmdDelay";

    reg->SetValueDword((char *)(const char *)cmdDelayName, m_5a4);
    reg->SetValueDword((char *)(const char *)resendName, m_5a8);
}

// ---------------------------------------------------------------------------
// CNetMgr::ReportError  @ 0x1776a0  (__thiscall, ret; 2561 B).
// Maps a DirectPlay HRESULT to its DPERR symbol name + human-readable
// description, writing the results into module-level global buffers.
// Outputs via OutputDebugStringA and/or MessageBoxA according to the
// report-mode flags. This is the largest matched function in the codebase.
//
// @address: 0x1776a0
// @size:    0xa01
// ---------------------------------------------------------------------------
void CNetMgr::ReportError(const char *file, int line, long hr)
{
    char out[0x100];  // output formatting buffer

    int hrLow = hr & 0xFFFF;
    g_repFullHr = hr;
    g_repHrLow = hrLow;

    // Optional beep
    if (g_repBeepFlag)
        MessageBeep(0x30);

    // Initialize global buffers with defaults
    strcpy(g_repDesc, s_unknownErrMsg);
    EngFormat(g_repSym, s_unknownErrCode);

    // Map HRESULT -> DPERR symbol + description (both to global buffers)
    if (hr == 0x80004001) {                              // E_NOTIMPL
        strcpy(g_repSym, s_dperrUnsupported);
        strcpy(g_repDesc, s_descUnsupported);
    } else if (hr == 0x80004005) {                       // E_FAIL
        strcpy(g_repSym, s_dperrGeneric);
        strcpy(g_repDesc, s_descGeneric);
    } else if (hr == 0x8007000e) {                       // E_OUTOFMEMORY
        strcpy(g_repSym, s_dperrOutOfMem);
        strcpy(g_repDesc, s_descOutOfMem);
    } else if (hr == 0x80070057) {                       // E_INVALIDARG
        strcpy(g_repSym, s_dperrInvalidParams);
        strcpy(g_repDesc, s_descInvalidParams);
    } else if (hr == 0x88770005) {                       // DPERR_ALREADYINITIALIZED
        strcpy(g_repSym, s_dperrAlreadyInit);
        strcpy(g_repDesc, s_descAlreadyInit);
    } else if (hr == 0x8877000a) {                       // DPERR_ACCESSDENIED
        strcpy(g_repSym, s_dperrAccessDenied);
        strcpy(g_repDesc, s_descAccessDenied);
    } else if (hr == 0x88770014) {                       // DPERR_ACTIVEPLAYERS
        strcpy(g_repSym, s_dperrActivePlayers);
        strcpy(g_repDesc, s_descActivePlayers);
    } else if (hr == 0x8877001e) {                       // DPERR_BUFFERTOOSMALL
        strcpy(g_repSym, s_dperrBufferSmall);
        strcpy(g_repDesc, s_descBufferSmall);
    } else if (hr == 0x88770028) {                       // DPERR_CANTADDPLAYER
        strcpy(g_repSym, s_dperrCantAddPlayer);
        strcpy(g_repDesc, s_descCantAddPlayer);
    } else if (hr == 0x88770032) {                       // DPERR_CANTCREATEGROUP
        strcpy(g_repSym, s_dperrCantCreateGroup);
        strcpy(g_repDesc, s_descCantCreateGroup);
    } else if (hr == 0x8877003c) {                       // DPERR_CANTCREATEPLAYER
        strcpy(g_repSym, s_dperrCantCreatePlayer);
        strcpy(g_repDesc, s_descCantCreatePlayer);
    } else if (hr == 0x88770046) {                       // DPERR_CANTCREATESESSION
        strcpy(g_repSym, s_dperrCantCreateSession);
        strcpy(g_repDesc, s_descCantCreateSession);
    } else if (hr == 0x88770050) {                       // DPERR_CAPSNOTAVAILABLEYET
        strcpy(g_repSym, s_dperrCapsNotAvail);
        strcpy(g_repDesc, s_descCapsNotAvail);
    } else if (hr == 0x8877005a) {                       // DPERR_EXCEPTION
        strcpy(g_repSym, s_dperrException);
        strcpy(g_repDesc, s_descException);
    } else if (hr == 0x88770078) {                       // DPERR_INVALIDFLAGS
        strcpy(g_repSym, s_dperrInvalidFlags);
        strcpy(g_repDesc, s_descInvalidFlags);
    } else if (hr == 0x88770082) {                       // DPERR_INVALIDOBJECT
        strcpy(g_repSym, s_dperrInvalidObject);
        strcpy(g_repDesc, s_descInvalidObject);
    } else if (hr == 0x88770096) {                       // DPERR_INVALIDPLAYER
        strcpy(g_repSym, s_dperrInvalidPlayer);
        strcpy(g_repDesc, s_descInvalidPlayer);
    } else if (hr == 0x887700a0) {                       // DPERR_NOCAPS
        strcpy(g_repSym, s_dperrNoCaps);
        strcpy(g_repDesc, s_descNoCaps);
    } else if (hr == 0x887700aa) {                       // DPERR_NOCONNECTION
        strcpy(g_repSym, s_dperrNoConnection);
        strcpy(g_repDesc, s_descNoConnection);
    } else if (hr == 0x887700be) {                       // DPERR_NOMESSAGES
        strcpy(g_repSym, s_dperrNoMessages);
        strcpy(g_repDesc, s_descNoMessages);
    } else if (hr == 0x887700c8) {                       // DPERR_NONAMESERVERFOUND
        strcpy(g_repSym, s_dperrNoNameServer);
        strcpy(g_repDesc, s_descNoNameServer);
    } else if (hr == 0x887700d2) {                       // DPERR_NOPLAYERS
        strcpy(g_repSym, s_dperrNoPlayers);
        strcpy(g_repDesc, s_descNoPlayers);
    } else if (hr == 0x887700dc) {                       // DPERR_NOSESSIONS
        strcpy(g_repSym, s_dperrNoSessions);
        strcpy(g_repDesc, s_descNoSessions);
    } else if (hr == 0x887700e6) {                       // DPERR_SENDTOOBIG
        strcpy(g_repSym, s_dperrSendTooBig);
        strcpy(g_repDesc, s_descSendTooBig);
    } else if (hr == 0x887700f0) {                       // DPERR_TIMEOUT
        strcpy(g_repSym, s_dperrTimeout);
        strcpy(g_repDesc, s_descTimeout);
    } else if (hr == 0x887700fa) {                       // DPERR_UNAVAILABLE
        strcpy(g_repSym, s_dperrUnavailable);
        strcpy(g_repDesc, s_descUnavailable);
    } else if (hr == 0x8877010e) {                       // DPERR_BUSY
        strcpy(g_repSym, s_dperrBusy);
        strcpy(g_repDesc, s_descBusy);
    } else if (hr == 0x88770118) {                       // DPERR_USERCANCEL
        strcpy(g_repSym, s_dperrUserCancel);
        strcpy(g_repDesc, s_descUserCancel);
    } else if (hr == 0x8877012c) {                       // DPERR_PLAYERLOST
        strcpy(g_repSym, s_dperrPlayerLost);
        strcpy(g_repDesc, s_descPlayerLost);
    } else if (hr == 0x88770136) {                       // DPERR_SESSIONLOST
        strcpy(g_repSym, s_dperrSessionLost);
        strcpy(g_repDesc, s_descSessionLost);
    } else if (hr == 0x887703e8) {                       // DPERR_BUFFERTOOLARGE
        strcpy(g_repSym, s_dperrBufferLarge);
        strcpy(g_repDesc, s_descBufferLarge);
    } else if (hr == 0x887703f2) {                       // DPERR_CANTCREATEPROCESS
        strcpy(g_repSym, s_dperrCantCreateProc);
        strcpy(g_repDesc, s_descCantCreateProc);
    } else if (hr == 0x887703fc) {                       // DPERR_APPNOTSTARTED
        strcpy(g_repSym, s_dperrAppNotStarted);
        strcpy(g_repDesc, s_descAppNotStarted);
    } else if (hr == 0x88770406) {                       // DPERR_INVALIDINTERFACE
        strcpy(g_repSym, s_dperrInvalidIf);
        strcpy(g_repDesc, s_descInvalidIf);
    } else if (hr == 0x8877041a) {                       // DPERR_UNKNOWNAPPLICATION
        strcpy(g_repSym, s_dperrUnknownApp);
        strcpy(g_repDesc, s_descUnknownApp);
    } else if (hr == 0x8877042e) {                       // DPERR_NOTLOBBIED
        strcpy(g_repSym, s_dperrNotLobbied);
        strcpy(g_repDesc, s_descNotLobbied);
    } else if (hr == 0) {                                // DP_OK
        strcpy(g_repSym, s_dpOkStr);
        strcpy(g_repDesc, s_noError);
    }

    // Output via debugger channel
    if (g_repFlagE8) {
        if (file && line > 0)
            EngFormat(out, s_fmtDebug, file, line, g_repSym, hrLow, g_repDesc);
        else
            EngFormat(out, s_fmtDebugShort, g_repSym, hrLow, g_repDesc);
        OutputDebugStringA(out);
    }

    // Output via message box channel
    if (g_repFlagF4) {
        if (file && line > 0)
            EngFormat(out, s_fmtMsgBox, file, line, g_repSym, hrLow, g_repDesc);
        else
            EngFormat(out, s_fmtMsgBoxShort, g_repSym, hrLow, g_repDesc);
        MessageBoxA(0, out, s_netMgrModule, 0x30);
    }
}
