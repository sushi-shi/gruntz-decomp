// NetMgr.h - the engine CNetMgr (DirectPlay networking manager) - minimal
// reconstruction sufficient to byte-match a cluster of its state/message
// handlers and config writers. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS and code bytes are load-bearing.
//
// The DirectPlay COM interfaces and the DPLAYX exports are EXTERNAL - never
// matched here. The handlers in this TU touch only:
//   - a reentrancy-guard global pair (per-message guards + a shared one),
//   - the multiplayer command dispatcher (an external engine routine reached
//     through an incremental-link thunk),
//   - PostMessageA (USER32, via the cached IAT pointer the engine keeps),
//   - the per-game RegistryHelper (reached as the game-manager singleton's
//     +0x38 member) to persist command-timing config.
#ifndef NET_NETMGR_H
#define NET_NETMGR_H

#include "../Utils/RegistryHelper.h"

// ---------------------------------------------------------------------------
// Minimal Win32 surface (no <windows.h> - keep the visible symbol set small;
// see docs/matching-patterns.md). Only PostMessageA is needed, as a
// __declspec(dllimport) __stdcall decl so the call site emits the FF15 [IAT]
// indirect form against the engine's cached USER32 import slot.
// ---------------------------------------------------------------------------
typedef void *         HWND;
typedef unsigned int   UINT;
typedef unsigned int   WPARAM;
typedef long           LPARAM;
typedef int            BOOL;

extern "C" {
__declspec(dllimport) BOOL __stdcall PostMessageA(HWND hWnd, UINT Msg,
                                                  WPARAM wParam, LPARAM lParam);
}

// ---------------------------------------------------------------------------
// AfxString - the minimal MFC CString model (a single char* @+0). The config
// writer builds value-name strings "m_section + _Suffix" via the engine global
// operator+ overload (AFXAPI == __stdcall) and destroys the temporaries via
// the engine CString dtor. Both are external/no-body so their `call rel32`
// displacements reloc-mask.
//   0x1b9f81  operator+(const CString&, LPCTSTR)  (AFXAPI, ret 0xc)
//   0x1b9cde  CString::~CString()                 (__thiscall, ret)
// ---------------------------------------------------------------------------
class AfxString {
public:
    AfxString();
    ~AfxString();                       // @0x1b9cde
    operator const char *() const { return m_pchData; }
    char *m_pchData;
};

AfxString __stdcall operator+(const AfxString &lhs, const char *rhs); // @0x1b9f81

// ---------------------------------------------------------------------------
// The game-manager singleton (@0x64556c) - only its +0x38 RegistryHelper is
// touched here (the config persistence target). Modeled as a tiny struct with
// the member at the right offset.
// ---------------------------------------------------------------------------
struct CGameMgr {
    char                  m_pad0[0x38];
    Utils::RegistryHelper *m_38;        // +0x38  the per-game registry config writer
};

extern CGameMgr *g_pGameMgr;            // @0x64556c

// ---------------------------------------------------------------------------
// The multiplayer command dispatcher (engine @0x4bc250, reached through an
// incremental-link thunk so its `call rel32` reloc-masks). __stdcall (cleans
// its own args - the call site has no `add esp`). Args: the command-name
// string, a per-command CALLBACK function pointer, and a flag; returns the
// dispatched message id. Each handler passes a distinct callback (the engine
// @0x4bda70/0x4bd850/0x4bddd0 routines, also via incremental-link thunks so the
// `push &callback` reloc-masks); modeled as external no-body functions whose
// address is taken.
// ---------------------------------------------------------------------------
typedef void (*MultiCallbackFn)();
extern "C" int __stdcall MultiDispatch(const char *cmd, MultiCallbackFn cb, int flag);

// Per-command callbacks (address-taken only; bodies are external/no-body).
extern "C" void MultiOptionzCallback();    // @0x4bda70 (via thunk 0x4027fc)
extern "C" void MultiPauseCallback();      // @0x4bd850 (via thunk 0x40113b)
extern "C" void MultiOutOfSyncCallback();  // @0x4bddd0 (via thunk 0x40301c)

// ---------------------------------------------------------------------------
// Reentrancy guards (file-scope globals).
//   g_optionzGuard  @0x648d08   (OnMultiOptions)
//   g_pauseGuard    @0x648d04   (OnMultiPause)
//   g_outOfSyncGuard@0x648d04 ... no: OnOutOfSync uses a per-this flag @+0x574
//   g_sharedFlag    @0x648ce0   (cleared by all three)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// CNetMgr - the DirectPlay networking manager. Only the members the matched
// methods touch are pinned:
//   +0x004 m_4   : a sub-object whose +0x4 is a window-handle holder; the
//                  message handlers do (((T*)m_4)->m_4)->m_4 to reach the HWND.
//   +0x01c m_1c  : the wParam value posted with the resync message.
//   +0x574 m_574 : OnOutOfSync's per-instance reentrancy guard.
//   +0x584 m_584 : a state word the handlers clear on entry.
//   +0x598 m_598 : a CString - the config section/value-name prefix
//                  ("m_598 + _CmdDelay" etc.).
//   +0x5a4 m_5a4 : the _CmdDelay value persisted by ApplyCmdDelayDefaults.
//   +0x5a8 m_5a8 : the _Resend value persisted by ApplyCmdDelayDefaults.
// ---------------------------------------------------------------------------
class CNetMgr {
public:
    void OnMultiOptions();              // @0x0badd0
    void OnMultiPause();                // @0x0bad40
    void OnOutOfSync();                 // @0x0bae40
    void ApplyCmdDelayDefaults();       // @0x0b85a0

    char       m_pad0[4];              // +0x000
    void      *m_4;                     // +0x004
    char       m_pad8[0x1c - 0x8];
    int        m_1c;                    // +0x01c
    char       m_pad20[0x574 - 0x20];
    int        m_574;                   // +0x574
    char       m_pad578[0x584 - 0x578];
    int        m_584;                   // +0x584
    char       m_pad588[0x598 - 0x588];
    AfxString  m_598;                   // +0x598
    char       m_pad59c[0x5a4 - 0x59c];
    DWORD      m_5a4;                   // +0x5a4
    DWORD      m_5a8;                   // +0x5a8
};

// The HWND chain the message handlers walk: m_4 -> +0x4 -> +0x4 (the HWND).
struct CNetHwndHolder {
    char  m_pad0[4];
    void *m_4;                          // +0x4
};

#endif // NET_NETMGR_H
