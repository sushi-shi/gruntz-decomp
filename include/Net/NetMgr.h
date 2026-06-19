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

#include <Utils/RegistryHelper.h>

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
//   operator+(const CString&, LPCTSTR)  (AFXAPI)
//   CString::~CString()                 (__thiscall)
// ---------------------------------------------------------------------------
class AfxString {
public:
    AfxString();
    ~AfxString();
    operator const char *() const { return m_pchData; }
    char *m_pchData;
};

AfxString __stdcall operator+(const AfxString &lhs, const char *rhs);

// ---------------------------------------------------------------------------
// The game-manager singleton - only its +0x38 RegistryHelper is
// touched here (the config persistence target). Modeled as a tiny struct with
// the member at the right offset.
// ---------------------------------------------------------------------------
struct CGameMgr {
    char                  m_pad0[0x38];
    Utils::RegistryHelper *m_38;        // +0x38  the per-game registry config writer
};

extern CGameMgr *g_pGameMgr;

// ---------------------------------------------------------------------------
// The multiplayer command dispatcher (reached through an
// incremental-link thunk so its `call rel32` reloc-masks). __stdcall (cleans
// its own args - the call site has no `add esp`). Args: the command-name
// string, a per-command CALLBACK function pointer, and a flag; returns the
// dispatched message id. Each handler passes a distinct callback (the engine
// routines, also via incremental-link thunks so the
// `push &callback` reloc-masks); modeled as external no-body functions whose
// address is taken.
// ---------------------------------------------------------------------------
typedef void (*MultiCallbackFn)();
extern "C" int __stdcall MultiDispatch(const char *cmd, MultiCallbackFn cb, int flag);

// Per-command callbacks (address-taken only; bodies are external/no-body).
extern "C" void MultiOptionzCallback();
extern "C" void MultiPauseCallback();
extern "C" void MultiOutOfSyncCallback();

// ---------------------------------------------------------------------------
// Reentrancy guards (file-scope globals).
//   g_optionzGuard   (OnMultiOptions)
//   g_pauseGuard     (OnMultiPause)
//   OnOutOfSync uses a per-this flag at +0x574 (not a shared guard global)
//   g_sharedFlag     (cleared by all three)
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
    void OnMultiOptions();
    void OnMultiPause();
    void OnOutOfSync();
    void ApplyCmdDelayDefaults();

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

    // Engine-label backlog stubs.
    void Stub_0b5460();
    void Stub_0b6000();
    void Stub_0b78b0();
    void Stub_0b7b10();
    void Stub_0b82e0();
    void Stub_0b8b10();
    void Stub_0b8cf0();
    void Stub_0b9750();
    void Stub_0bc070();
    void Stub_0bc110();
    void Stub_0bc460();
    void Stub_0bca50();
    void Stub_0bcf20();
    void Stub_0bd000();
    void Stub_0bd030();
    void Stub_0bd0b0();
    void Stub_0bd180();
    void Stub_1776a0();
    void Stub_1780b0();
    void Stub_178280();
    void Stub_1782d0();
    void Stub_178e20();
    void Stub_178e90();
    void Stub_178eb0();
    void Stub_178ef0();
    void Stub_178fc0();
    void Stub_179090();
    void Stub_179130();
};

// The HWND chain the message handlers walk: m_4 -> +0x4 -> +0x4 (the HWND).
struct CNetHwndHolder {
    char  m_pad0[4];
    void *m_4;                          // +0x4
};

#endif // NET_NETMGR_H
