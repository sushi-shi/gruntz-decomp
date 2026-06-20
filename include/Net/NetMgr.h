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
// KERNEL32 Sleep - the version-mismatch handler pauses 250ms before bailing.
// FF15 [IAT] indirect form against the engine's cached KERNEL32 import slot.
__declspec(dllimport) void __stdcall Sleep(unsigned ms);
}

// ---------------------------------------------------------------------------
// CString - the minimal MFC CString model (a single char* @+0). The config
// writer builds value-name strings "m_section + _Suffix" via the engine global
// operator+ overload (AFXAPI == __stdcall) and destroys the temporaries via
// the engine CString dtor. Both are external/no-body so their `call rel32`
// displacements reloc-mask.
//   operator+(const CString&, LPCTSTR)  (AFXAPI)
//   CString::~CString()                 (__thiscall)
// ---------------------------------------------------------------------------
#include <Gruntz/CString.h>

CString __stdcall operator+(const CString &lhs, const char *rhs);

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
// Engine-global version state (in .data, external; reloc-masked). The Net
// module compares the host packet's version pair against the two locals
// (g_localVersion @0x60fa70, g_remoteVersion @0x60fa74), and a config dword
// (g_cfgWord @0x645550) plus the global CButeMgr's +0x4 word go into the
// version-announce packet.
// ---------------------------------------------------------------------------
extern "C" int g_localVersion;    // 0x60fa70
extern "C" int g_remoteVersion;   // 0x60fa74
extern "C" int g_cfgWord;         // 0x645550
extern "C" int g_buteMgrField4;   // *(g_buteMgr + 4) - the CButeMgr config word

// The host-version packet HandleVersionCheck inspects: it carries the host's
// version pair at +0x18 / +0x1c (compared against the two locals). Only those
// two dwords are pinned.
struct CNetVersionMsg {
    char m_pad0[0x18];
    int  m_18;                          // +0x18  host remote-version word
    int  m_1c;                          // +0x1c  host local-version word
};

// The 0x20-byte version-announce packet AnnounceVersion builds on the stack and
// ships through the engine stat dispatcher as stat 0x417. Field offsets pinned
// by the writes; the rest is zero-filled.
struct CNetVersionPacket {
    unsigned char m_0;                  // +0x00  flag byte (bit7 set)
    char          m_pad1[7];
    int           m_8;                  // +0x08  CButeMgr config word
    int           m_c;                  // +0x0c  g_cfgWord
    int           m_10;                 // +0x10  stat id (0x417)
    char          m_pad14[4];
    int           m_18;                 // +0x18  g_remoteVersion
    int           m_1c;                 // +0x1c  g_localVersion
};

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
// One per-player slot in the m_4 sub-object's slot array (stride 0x238). Only
// the three dwords GetMaxAckLatency reads are pinned: two "slot active" gate
// flags and the slot's current latency value.
struct CNetPlayerSlot {
    char  m_pad0[0x164];
    DWORD m_164;                        // +0x164  slot-active gate A
    char  m_pad168[0x170 - 0x168];
    DWORD m_170;                        // +0x170  slot-active gate B
    char  m_pad174[0x37c - 0x174];
    DWORD m_37c;                        // +0x37c  the slot's latency value
};

// A payload entry found through the m_58 player list. FindPlayerById matches on
// the entry's +0x4 id field.
struct CNetPlayerEntry {
    char  m_pad0[4];
    int   m_4;                          // +0x4  the entry's id (the lookup key)
};

// A singly-linked node in the m_58 player list: +0x0 next, +0x8 payload.
struct CNetPlayerNode {
    CNetPlayerNode  *m_next;            // +0x0
    char             m_pad4[4];
    CNetPlayerEntry *m_8;               // +0x8  the payload entry
};

class CNetMgr {
public:
    void OnMultiOptions();
    void OnMultiPause();
    void OnOutOfSync();
    void ApplyCmdDelayDefaults();
    unsigned GetMaxAckLatency();
    void ReportAckLatency();
    CNetPlayerEntry *FindPlayerById(int id);

    // Version-check cluster (engine CNetMgr base; ~0xbd0xx). HandleVersionCheck
    // compares the host packet's version pair against the two locals and, on a
    // mismatch, reports + announces; AnnounceVersion ships the version packet as
    // stat 0x417 through the engine dispatcher.
    void HandleVersionCheck(CNetVersionMsg *msg);
    void AnnounceVersion(int param);

    // External per-instance command/stat dispatcher (reached through an
    // incremental-link thunk so its `call rel32` reloc-masks; __thiscall, ecx =
    // this). No body here - the engine routine is external.
    void SendNetStat(int id, unsigned value, int flag);
    // The version-report diagnostic (logs a message string + zero) and the two
    // stat dispatchers the version cluster fires (a 2-arg stat-id/flag form and
    // a 4-arg stat packet form). All __thiscall engine routines reached through
    // incremental-link thunks; no body here.
    void ReportVersionMsg(const char *msg, int zero);
    void SendStatFlag(int id, int flag);
    void SendStatPacket(int param, const void *packet, int size, int flag);

    char       m_pad0[4];              // +0x000
    void      *m_4;                     // +0x004
    char       m_pad8[0x1c - 0x8];
    int        m_1c;                    // +0x01c
    char       m_pad20[0x58 - 0x20];
    CNetPlayerNode *m_58;               // +0x58  head of the player list
    char       m_pad5c[0x528 - 0x5c];
    int        m_528;                   // +0x528  branch selector
    char       m_pad52c[0x570 - 0x52c];
    int        m_570;                   // +0x570  version-mismatch latch
    int        m_574;                   // +0x574
    char       m_pad578[0x580 - 0x578];
    int        m_580;                   // +0x580  "connected" flag (gates report)
    int        m_584;                   // +0x584
    char       m_pad588[0x598 - 0x588];
    CString  m_598;                   // +0x598
    char       m_pad59c[0x5a4 - 0x59c];
    DWORD      m_5a4;                   // +0x5a4
    DWORD      m_5a8;                   // +0x5a8
    char       m_pad5ac[0x5f0 - 0x5ac];
    DWORD      m_5f0[4];                // +0x5f0  per-channel latency values

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
    void Stub_1780b0();
    void Stub_178280();
    void Stub_1782d0();
    void Stub_178e20();
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
