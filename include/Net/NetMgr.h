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
#include <Gruntz/CObList.h>

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
// WINMM timeGetTime - the frame-sync / connect-wait clock; FF15 [IAT] indirect
// against the engine's cached WINMM import slot (*0x6c4650).
__declspec(dllimport) unsigned __stdcall timeGetTime(void);
// USER32 GetAsyncKeyState - the connect wait polls VK_ESCAPE (0x1b) to abort;
// FF15 [IAT] indirect against the cached USER32 slot (*0x6c4500).
__declspec(dllimport) short __stdcall GetAsyncKeyState(int vKey);
}

// ---------------------------------------------------------------------------
// Utils::WinAPI::ActiveWait - the engine busy-wait (?ActiveWait@WinAPI@Utils@@YAXI@Z,
// __cdecl). Defined in another TU (Utils/WinAPI.cpp); here it is an external
// no-body decl so the FrameSyncWait `call rel32` reloc-masks.
namespace Utils {
namespace WinAPI {
void ActiveWait(unsigned int milliseconds);
}
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
extern "C" void MultiDropPlayerCallback();   // OnDropPlayer (MULTI_DROPPLAYER)

// The pending drop's player id (-999 == none), an external engine global in
// .data at 0x611d88 (DIR32 reloc-masked).
extern "C" int g_dropPlayerId;    // 0x611d88

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

// ---------------------------------------------------------------------------
// The per-player command-resend slot ResetPlayerCommands operates on. The
// session sub-object (CNetMgr+0x520) keeps four of these in an inline array;
// FindCmdSlot returns the one whose owning player matches. Only the fields the
// reset reads are pinned. The three engine helpers it fires (slot methods,
// external incremental-link thunks) clear the slot's command range.
// ---------------------------------------------------------------------------
struct CNetCmdSlot {
    char  m_pad0[4];
    int   m_4;                          // +0x4  "slot already reset" guard
    char  m_pad8[0xc - 0x8];
    int  *m_c;                          // +0xc  -> command-list head value
    char  m_pad10[0x14 - 0x10];
    int   m_14;                         // +0x14  base command sequence number
    char  m_pad18[0x4c - 0x18];
    int   m_4c[3];                      // +0x4c  command-range A (reset to -1)
    int   m_58[3];                      // +0x58  command-range B (reset to -1)

    void Touch();                       // c1390  latch the slot (sets +4, +8)
    void RemoveCmd(int seq);            // c11b0  drop one queued command
    void ResetTriple(int *p);           // c10a0  splat &-1 over three dwords
};

// The DirectPlay session sub-object at CNetMgr+0x520. Two helpers are reached
// here (external __thiscall thunks): FindCmdSlot linear-scans the four inline
// command slots for the one whose player id matches; ResetCmdBuffers zeroes the
// head of each of those four slots.
struct CNetSession {
    CNetCmdSlot *FindCmdSlot(int playerId);   // c00a0
    void ResetCmdBuffers();                   // c0070
};

// The command-dispatch queue hanging off the CNetMgr's m_4 sub-object at +0x6c;
// ResetPlayerCommands fires its 2-arg dispatch helper (external thunk) for each
// command sequence number in the reset range.
struct CNetCmdQueue {
    void Dispatch(int cmdHead, int seq);      // 423b40
};

// The m_4 sub-object, seen here only for its +0x6c command-dispatch queue (the
// same object whose +0x4->+0x4 is the engine HWND - see CNetHwndHolder below).
struct CNetSubObject {
    char          m_pad0[0x6c];
    CNetCmdQueue *m_6c;                 // +0x6c  the command-dispatch queue
};

// ---------------------------------------------------------------------------
// The DirectPlay session interface CNetMgr keeps at +0x18 (an IDirectPlay4-shaped
// COM object). The 0x178xxx wrapper run reaches it through its vtable: each
// wrapper does `this->m_18->vtbl->Method(this->m_18, args...)` so the
// `call *off(ecx)` indirect form is what retail emits (the engine never link-
// resolves these - they are runtime COM slots). Only the slots the wrappers call
// are pinned; everything else is opaque padding. COM convention => __stdcall with
// the interface pointer as the first (hidden `this`) argument.
//   +0x38 (slot 14)  Enum2     (struct*, ctx)               -> HRESULT
//   +0x50 (slot 20)  GetData2  (id, lpData, lpSize, fl)     -> HRESULT
//   +0x68 (slot 26)  SetData5  (a, b, c, d, e)              -> HRESULT
// ---------------------------------------------------------------------------
struct IDirectPlay4Z {
    struct Vtbl {
        char m_pad0[0x38];
        long(__stdcall *Enum2)(IDirectPlay4Z *, void *desc, void *ctx);   // +0x38
        char m_pad3c[0x50 - 0x3c];
        long(__stdcall *GetData2)(IDirectPlay4Z *, int id, void *lpData,
                                  unsigned long *lpSize, unsigned long fl); // +0x50
        char m_pad54[0x68 - 0x54];
        long(__stdcall *SetData5)(IDirectPlay4Z *, int a, int b, int c,
                                  int d, int e);                          // +0x68
    } *vtbl;
};

// ---------------------------------------------------------------------------
// A managed player object the m_54 list holds. RemovePlayerObj (0x178e20) tears
// one down: it calls the object's vtable slot 1 (a self-destruct/scalar-deleting
// dtor taking a single flag arg) and then unlinks the object from the m_54
// CObList using the cached __POSITION the object stores at +0x20. Modeled as a
// polymorphic class so `obj->SelfDestruct(1)` emits the thiscall virtual
// dispatch (slot 1 == +0x4); the virtual is never defined so no vtable is
// emitted in this TU.
// ---------------------------------------------------------------------------
class CNetPlayerObj {
public:
    virtual void Slot00();              // +0x00
    virtual void SelfDestruct(int flag); // +0x04  slot 1 (self-destruct)

    char        m_pad4[0x20 - 0x4];     // +0x04
    __POSITION *m_20;                   // +0x20  cached list position
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

    // The DirectPlay session-management wrapper run (engine CNetMgr base;
    // ~0x178xxx). Each thin wrapper calls one IDirectPlay4 vtable slot on the
    // m_18 interface and, on a nonzero HRESULT, routes it through the static
    // ReportError diagnostic with this TU's __FILE__/__LINE__.
    int   RemovePlayerObj(CNetPlayerObj *obj);         // 0x178e20
    void *GetPlayerData(int id);                       // 0x178eb0
    long  SetGroupData2(CNetPlayerEntry *a, CNetPlayerEntry *b,
                        int c, int d, int e);           // 0x178ef0
    long  SetData(int a, int b, int c, int d, int e);  // 0x178fc0
    long  SetGroupDataFrom(CNetPlayerEntry *a, int c, int d, int e); // 0x179090
    int   EnumSessions(void *desc, void *ctx);         // 0x179130

    // The diagnostic error reporter (lives in the netmgrerror TU; static
    // __cdecl). Declared here so the wrappers can route HRESULTs through it.
    static void ReportError(char *file, int line, long hr, void *hWnd);

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

    // ---- 0xbc0xx cluster ---------------------------------------------------
    // The cluster's matched methods (defined in NetMgr.cpp).
    unsigned FrameSyncWait();                 // 0xbc070
    void     OnDropPlayer();                  // 0xbc110
    int      WaitForConnect();                // 0xbca50
    int      ResetPlayerCommands(int id);     // 0xbcf20

    // External engine helpers the cluster fires through incremental-link thunks
    // (__thiscall on `this` unless noted; bodies external/no-body so the
    // `call rel32` reloc-masks).
    //   SendStat3      (b9410) the 3-arg stat sender (id, value, flag)
    //   ReportNetError (b7e30) status-bar diagnostic (string, level)
    //   ReportStatusId (b7ec0) status-bar diagnostic by string-resource id
    //   PollSession    (b95f0) pump the DirectPlay receive queue, no args
    //   AckDropPlayer  (ba590) finalize a dropped player (id)
    //   ResetCmdBuffers(c0070) zero the four per-slot command buffers, no args
    void SendStat3(int id, unsigned value, int flag);
    void ReportNetError(const char *msg, int level);
    void ReportStatusId(UINT strId, int level);
    void PollSession();
    void AckDropPlayer(int id);

    char       m_pad0[4];              // +0x000
    void      *m_4;                     // +0x004
    char       m_pad8[0x18 - 0x8];
    IDirectPlay4Z *m_18;                // +0x018  the DirectPlay session interface
    int        m_1c;                    // +0x01c
    char       m_pad20[0x58 - 0x20];
    // The managed-player-object list (a by-value CObList embedded at +0x54). Its
    // 0x1c-byte body spans +0x54..+0x70; the +0x58 head node ptr below is the
    // CObList's own m_pHead (what FindPlayerById walks). Modeled as the head
    // pointer here; RemovePlayerObj reaches the embedded CObList via a +0x54 cast.
    CNetPlayerNode *m_58;               // +0x58  head of the player list (CObList m_pHead)
    char       m_pad5c[0x520 - 0x5c];
    CNetSession *m_520;                 // +0x520  the DirectPlay session sub-object
    CNetMgr   *m_524;                   // +0x524  peer net-manager (owns the player list)
    int        m_528;                   // +0x528  branch selector
    int        m_52c;                   // +0x52c  "session terminated" flag
    char       m_pad530[0x534 - 0x530];
    int        m_534;                   // +0x534  host-mode flag
    int        m_538;                   // +0x538  "removed from game" flag
    char       m_pad53c[0x56c - 0x53c];
    int        m_56c;                   // +0x56c  "game full" flag
    int        m_570;                   // +0x570  version-mismatch latch
    int        m_574;                   // +0x574
    int        m_578;                   // +0x578  sync-toggle gate
    char       m_pad57c[0x580 - 0x57c];
    int        m_580;                   // +0x580  "connected" flag (gates report)
    int        m_584;                   // +0x584
    char       m_pad588[0x58c - 0x588];
    int        m_58c;                   // +0x58c  "abort" flag set during connect-wait
    char       m_pad590[0x598 - 0x590];
    CString  m_598;                   // +0x598
    char       m_pad59c[0x5a4 - 0x59c];
    DWORD      m_5a4;                   // +0x5a4
    DWORD      m_5a8;                   // +0x5a8
    int        m_5ac;                   // +0x5ac  "game closed" flag
    char       m_pad5b0[0x5bc - 0x5b0];
    int        m_5bc;                   // +0x5bc  the local player descriptor ptr
    int        m_5c0;                   // +0x5c0  the local player id (from +0x5bc[+4])
    char       m_pad5c4[0x5e0 - 0x5c4];
    unsigned   m_5e0;                   // +0x5e0  last frame-sync delta
    unsigned   m_5e4;                   // +0x5e4  last frame-sync timestamp
    char       m_pad5e8[0x5f0 - 0x5e8];
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
    void Stub_0bc460();
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
