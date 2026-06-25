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

#include <Ints.h>

// <Mfc.h> brings <windows.h> USER32 (PostMessageA / Sleep / GetAsyncKeyState - the
// connect wait polls VK_ESCAPE to abort; HWND / UINT / ...) and the central WINMM
// timeGetTime decl (the frame-sync / connect-wait clock).
#include <Mfc.h>
#include <Utils/RegistryHelper.h>
#include <Gruntz/CObList.h>

// ---------------------------------------------------------------------------
// Utils::WinAPI::ActiveWait - the engine busy-wait (?ActiveWait@WinAPI@Utils@@YAXI@Z,
// __cdecl). Defined in another TU (Utils/WinAPI.cpp); here it is an external
// no-body decl so the FrameSyncWait `call rel32` reloc-masks.
namespace Utils {
    namespace WinAPI {
        void ActiveWait(u32 milliseconds);
    }
} // namespace Utils

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

CString __stdcall operator+(const CString& lhs, const char* rhs);

// ---------------------------------------------------------------------------
// The game-manager singleton - only its +0x38 RegistryHelper is
// touched here (the config persistence target). Modeled as a tiny struct with
// the member at the right offset.
// ---------------------------------------------------------------------------
struct CGameMgr {
    char m_pad0[0x38];
    Utils::RegistryHelper* m_38; // +0x38  the per-game registry config writer
};

extern CGameMgr* g_pGameMgr;

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
extern "C" i32 __stdcall MultiDispatch(const char* cmd, MultiCallbackFn cb, i32 flag);

// Per-command callbacks (address-taken only; bodies are external/no-body).
extern "C" void MultiOptionzCallback();
extern "C" void MultiPauseCallback();
extern "C" void MultiOutOfSyncCallback();
extern "C" void MultiDropPlayerCallback(); // OnDropPlayer (MULTI_DROPPLAYER)

// The pending drop's player id (-999 == none), an external engine global in
// .data at 0x611d88 (DIR32 reloc-masked).
extern "C" i32 g_dropPlayerId; // 0x611d88

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
extern "C" i32 g_localVersion;  // 0x60fa70
extern "C" i32 g_remoteVersion; // 0x60fa74
extern "C" i32 g_cfgWord;       // 0x645550
extern "C" i32 g_buteMgrField4; // *(g_buteMgr + 4) - the CButeMgr config word

// The host-version packet HandleVersionCheck inspects: it carries the host's
// version pair at +0x18 / +0x1c (compared against the two locals). Only those
// two dwords are pinned.
struct CNetVersionMsg {
    char m_pad0[0x18];
    i32 m_18; // +0x18  host remote-version word
    i32 m_1c; // +0x1c  host local-version word
};

// The 0x20-byte version-announce packet AnnounceVersion builds on the stack and
// ships through the engine stat dispatcher as stat 0x417. Field offsets pinned
// by the writes; the rest is zero-filled.
struct CNetVersionPacket {
    u8 m_0; // +0x00  flag byte (bit7 set)
    char m_pad1[7];
    i32 m_8;  // +0x08  CButeMgr config word
    i32 m_c;  // +0x0c  g_cfgWord
    i32 m_10; // +0x10  stat id (0x417)
    char m_pad14[4];
    i32 m_18; // +0x18  g_remoteVersion
    i32 m_1c; // +0x1c  g_localVersion
};

// ---------------------------------------------------------------------------
// CNetMgr - the DirectPlay networking manager. Only the members the matched
// methods touch are pinned:
//   +0x004 m_4              : a sub-object whose +0x4 is a window-handle holder;
//                             the message handlers do (((T*)m_4)->m_4)->m_4 to
//                             reach the HWND.
//   +0x01c m_1c             : the lParam value posted with the resync message.
//   +0x574 m_outOfSyncGuard : OnOutOfSync's per-instance reentrancy guard.
//   +0x584 m_584            : a state word the handlers clear on entry.
//   +0x598 m_configSection  : a CString - the config section/value-name prefix
//                             ("m_configSection + _CmdDelay" etc.).
//   +0x5a4 m_cmdDelay       : the _CmdDelay value persisted by ApplyCmdDelayDefaults.
//   +0x5a8 m_resend         : the _Resend value persisted by ApplyCmdDelayDefaults.
// ---------------------------------------------------------------------------
// One per-player slot in the m_4 sub-object's slot array (stride 0x238). Only
// the three dwords GetMaxAckLatency reads are pinned: two "slot active" gate
// flags and the slot's current latency value.
struct CNetPlayerSlot {
    char m_pad0[0x164];
    DWORD m_164; // +0x164  slot-active gate A
    char m_pad168[0x170 - 0x168];
    DWORD m_170; // +0x170  slot-active gate B
    char m_pad174[0x37c - 0x174];
    DWORD m_37c; // +0x37c  the slot's latency value
};

// A payload entry found through the m_58 player list. FindPlayerById matches on
// the entry's +0x4 id field.
struct CNetPlayerEntry {
    char m_pad0[4];
    i32 m_4; // +0x4  the entry's id (the lookup key)
};

// The 0x10-byte stat-packet header the SendStat* helpers fire through the
// DirectPlay set-data slots. Only the three written fields are pinned: a flag
// byte (bit7 set on every send), the stat id, and the value/player-id word.
struct CNetStatPacket {
    u8 m_0; // +0x0  flag byte (bit7 set)
    char m_pad1[3];
    i32 m_4;        // +0x4  stat id
    i32 m_8;        // +0x8  value / player id
    char m_padc[4]; // +0xc  (0x10 total)
};

// A singly-linked node in the m_58 player list: +0x0 next, +0x8 payload.
struct CNetPlayerNode {
    CNetPlayerNode* m_next; // +0x0
    char m_pad4[4];
    CNetPlayerEntry* m_8; // +0x8  the payload entry
};

// ---------------------------------------------------------------------------
// The per-player command-resend slot ResetPlayerCommands operates on. The
// session sub-object (CNetMgr+0x520) keeps four of these in an inline array;
// FindCmdSlot returns the one whose owning player matches. Only the fields the
// reset reads are pinned. The three engine helpers it fires (slot methods,
// external incremental-link thunks) clear the slot's command range.
// ---------------------------------------------------------------------------
struct CNetCmdSlot {
    char m_pad0[4];
    i32 m_4; // +0x4  "slot already reset" guard
    char m_pad8[0xc - 0x8];
    i32* m_c; // +0xc  -> command-list head value
    char m_pad10[0x14 - 0x10];
    i32 m_14; // +0x14  base command sequence number
    char m_pad18[0x4c - 0x18];
    i32 m_4c[3]; // +0x4c  command-range A (reset to -1)
    i32 m_58[3]; // +0x58  command-range B (reset to -1)

    void Touch();             // c1390  latch the slot (sets +4, +8)
    void RemoveCmd(i32 seq);  // c11b0  drop one queued command
    void ResetTriple(i32* p); // c10a0  splat &-1 over three dwords
};

// The DirectPlay session sub-object at CNetMgr+0x520. Two helpers are reached
// here (external __thiscall thunks): FindCmdSlot linear-scans the four inline
// command slots for the one whose player id matches; ResetCmdBuffers zeroes the
// head of each of those four slots.
struct CNetSession {
    CNetCmdSlot* FindCmdSlot(i32 playerId); // c00a0
    void ResetCmdBuffers();                 // c0070
};

// The command-dispatch queue hanging off the CNetMgr's m_4 sub-object at +0x6c;
// ResetPlayerCommands fires its 2-arg dispatch helper (external thunk) for each
// command sequence number in the reset range.
struct CNetCmdQueue {
    void Dispatch(i32 cmdHead, i32 seq); // 423b40
};

// The m_4 sub-object, seen here only for its +0x6c command-dispatch queue (the
// same object whose +0x4->+0x4 is the engine HWND - see CNetHwndHolder below).
struct CNetSubObject {
    char m_pad0[0x6c];
    CNetCmdQueue* m_6c; // +0x6c  the command-dispatch queue
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
        char m_pad0[0x34];
        i32(__stdcall* EnumPlayers)(
            IDirectPlay4Z*,
            void* desc,
            void* a,
            void* callback,
            void* ctx,
            void* flags
        );                                                            // +0x34 (slot 13)
        i32(__stdcall* Enum2)(IDirectPlay4Z*, void* desc, void* ctx); // +0x38
        char m_pad3c[0x50 - 0x3c];
        i32(__stdcall* GetData2)(
            IDirectPlay4Z*,
            i32 id,
            void* lpData,
            u32* lpSize,
            u32 fl
        ); // +0x50
        char m_pad54[0x58 - 0x54];
        i32(__stdcall* GetPlayerData2)(IDirectPlay4Z*, void* in, void* out); // +0x58 (slot 22)
        char m_pad5c[0x60 - 0x5c];
        i32(__stdcall* EnumGroups)(IDirectPlay4Z*, void* desc, i32 flags); // +0x60 (slot 24)
        char m_pad64[0x68 - 0x64];
        i32(__stdcall* SetData5)(IDirectPlay4Z*, i32 a, i32 b, i32 c, i32 d, i32 e); // +0x68
    }* vtbl;
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
    virtual void Slot00();               // +0x00
    virtual void SelfDestruct(i32 flag); // +0x04  slot 1 (self-destruct)

    char m_pad4[0x20 - 0x4]; // +0x04
    __POSITION* m_20;        // +0x20  cached list position
};

// ---------------------------------------------------------------------------
// One node of the three managed CObList collections at CNetMgr+0x1c/+0x38/+0x54.
// Each node holds a payload sub-object at +0x8 whose vtable slot 1 (+0x4) is the
// self-destruct (scalar-deleting dtor with a flag arg) the clear-loops fire.
// (The list-node CNode shape: +0x0 next, +0x4 prev, +0x8 payload pointer.)
// ---------------------------------------------------------------------------
struct CNetListNode {
    CNetListNode* m_next;  // +0x00  next node
    char m_pad4[4];        // +0x04  prev node (unused)
    CNetPlayerObj* m_data; // +0x08  payload sub-object (polymorphic; slot1 self-destruct)
};

// ---------------------------------------------------------------------------
// The per-player enumeration callback EnumPlayersInto hands to the DirectPlay
// EnumPlayers slot (its address is taken => DIR32 reloc-masked). The body lives
// elsewhere in the NetMgr TU (a recovery gap with no carved boundary); declared
// no-body here so the `push &NetEnumPlayerCb` reloc-masks.
// ---------------------------------------------------------------------------
extern "C" void NetEnumPlayerCb();

// ---------------------------------------------------------------------------
// A COM interface CNetMgr keeps at +0x14 alongside the IDirectPlay4 at +0x18.
// Destroy releases both: the +0x14 interface via its vtable slot 2 (the
// IUnknown::Release form), the +0x18 via slot 4 then slot 2. Only those slots
// are pinned; everything else is opaque padding. __stdcall (COM convention).
// ---------------------------------------------------------------------------
struct INetReleasable {
    struct Vtbl {
        char m_pad0[8];
        i32(__stdcall* Release)(INetReleasable*); // +0x08 (slot 2)
        char m_padc[0x10 - 0xc];
        i32(__stdcall* Slot10)(INetReleasable*); // +0x10 (slot 4)
    }* vtbl;
};

class CNetMgr {
public:
    void OnMultiOptions();
    void OnMultiPause();
    void OnOutOfSync();
    void ApplyCmdDelayDefaults();
    u32 GetMaxAckLatency();
    void ReportAckLatency();
    CNetPlayerEntry* FindPlayerById(i32 id);

    // The DirectPlay session-management wrapper run (engine CNetMgr base;
    // ~0x178xxx). Each thin wrapper calls one IDirectPlay4 vtable slot on the
    // m_18 interface and, on a nonzero HRESULT, routes it through the static
    // ReportError diagnostic with this TU's __FILE__/__LINE__.
    i32 RemovePlayerObj(CNetPlayerObj* obj);                                        // 0x178e20
    void* GetPlayerData(i32 id);                                                    // 0x178eb0
    i32 SetGroupData2(CNetPlayerEntry* a, CNetPlayerEntry* b, i32 c, i32 d, i32 e); // 0x178ef0
    i32 SetData(i32 a, i32 b, i32 c, i32 d, i32 e);                                 // 0x178fc0
    i32 SetGroupDataFrom(CNetPlayerEntry* a, i32 c, i32 d, i32 e);                  // 0x179090
    i32 EnumSessions(void* desc, void* ctx);                                        // 0x179130

    // The session-list cluster (engine CNetMgr base; ~0x178xxx). The three managed
    // collections at +0x1c/+0x38/+0x54 each have a clear-loop that self-destructs
    // every node's payload then RemoveAll's the list and zeroes a count/id pair.
    void Destroy();          // 0x178230  full teardown (clears all three lists + COM)
    void ClearGroupList();   // 0x178430  +0x1c list -> clear +0x7c/+0x70
    void ClearPlayerList();  // 0x178750  +0x38 list -> clear +0x80/+0x74
    void ClearSessionList(); // 0x178c70  +0x54 list -> clear +0x84/+0x78
    // Two list-box selection readers: read the current selection's item-data and
    // latch it into a per-list field if it is in range (Win32 SendMessageA).
    i32 ReadGroupSel(void* hList);  // 0x178590  -> latch into +0x70 (count +0x28)
    i32 ReadPlayerSel(void* hList); // 0x178820  -> latch into +0x74 (count +0x44)
    // Two IDirectPlay4 enumeration wrappers: enumerate sessions/players into the
    // COM interface at +0x18 and, on a nonzero HRESULT, route it through ReportError.
    i32 EnumPlayersInto(void* a, void* b); // 0x178610 (@early-stop, scheduling wall)

    // Backlog (not yet reconstructed - left as @stub in NetMgr.cpp):
    //   0x178360  AddGroupNode  - operator-new'd 0x10 node (2-phase vtbl + CString
    //                             member + /GX EH frame) AddTail'd onto the +0x1c list
    //   0x1788a0  EnumGroupsInto- 0x50-desc EnumGroups COM call + GetPlayerData2 +
    //                             operator-new/RezFree juggling; calls sibling 0x1786d0
    void Stub_178360(); // 0x178360
    void Stub_1788a0(); // 0x1788a0

    // The diagnostic error reporter (lives in the netmgrerror TU; static
    // __cdecl). Declared here so the wrappers can route HRESULTs through it.
    static void ReportError(char* file, i32 line, i32 hr, void* hWnd);

    // Version-check cluster (engine CNetMgr base; ~0xbd0xx). HandleVersionCheck
    // compares the host packet's version pair against the two locals and, on a
    // mismatch, reports + announces; AnnounceVersion ships the version packet as
    // stat 0x417 through the engine dispatcher.
    void HandleVersionCheck(CNetVersionMsg* msg);
    void AnnounceVersion(i32 param);

    // The stat-send family (matched in NetMgr.cpp). All ship a 0x10-byte
    // CNetStatPacket (or a caller packet) to the local player's peer group
    // through the DirectPlay set-data wrappers; SendStatFlag/SendNetStat build
    // the header then funnel through SendStatBuf.
    //   SendStatBuf   (b91f0) the core: SetGroupDataFrom(localPlayer, flag, pkt, 0x10)
    //   SendStatFlag  (b9240) build {id, localPlayer.id} then SendStatBuf
    //   SendNetStat   (b9290) build {id, value} then SendStatBuf
    //   SendStatFrom  (b92e0) SetGroupDataFrom wrapper, null-guarded packet
    //   SendStatPair  (b9330) SetGroupData2 wrapper, null-guarded packet
    i32 SendStatBuf(CNetStatPacket* pkt, i32 flag);                    // 0xb91f0
    void SendStatFlag(i32 id, i32 flag);                               // 0xb9240
    void SendNetStat(i32 id, u32 value, i32 flag);                     // 0xb9290
    i32 SendStatFrom(CNetStatPacket* pkt, i32 b, i32 c);               // 0xb92e0
    i32 SendStatPair(CNetPlayerEntry* recipient, CNetStatPacket* pkt, i32 c); // 0xb9330
    // The version-report diagnostic (logs a message string + zero) and the
    // 4-arg stat-packet dispatcher AnnounceVersion fires. Both __thiscall engine
    // routines reached through incremental-link thunks; no body here.
    void ReportVersionMsg(const char* msg, i32 zero);
    void SendStatPacket(i32 param, const void* packet, i32 size, i32 flag);

    // ---- 0xbc0xx cluster ---------------------------------------------------
    // The cluster's matched methods (defined in NetMgr.cpp).
    u32 FrameSyncWait();             // 0xbc070
    void OnDropPlayer();             // 0xbc110
    i32 WaitForConnect();            // 0xbca50
    i32 ResetPlayerCommands(i32 id); // 0xbcf20

    // External engine helpers the cluster fires through incremental-link thunks
    // (__thiscall on `this` unless noted; bodies external/no-body so the
    // `call rel32` reloc-masks).
    //   SendStat3      (b9410) the 3-arg stat sender (id, value, flag)
    //   ReportNetError (b7e30) status-bar diagnostic (string, level)
    //   ReportStatusId (b7ec0) status-bar diagnostic by string-resource id
    //   PollSession    (b95f0) pump the DirectPlay receive queue, no args
    //   AckDropPlayer  (ba590) finalize a dropped player (id)
    //   ResetCmdBuffers(c0070) zero the four per-slot command buffers, no args
    void SendStat3(i32 id, u32 value, i32 flag);
    void ReportNetError(const char* msg, i32 level);
    void ReportStatusId(UINT strId, i32 level);
    void PollSession();
    void AckDropPlayer(i32 id);

    char m_pad0[4]; // +0x000
    // The engine sub-object reached through several views: ->m_4->m_4 is the HWND
    // holder the message handlers PostMessageA through; +0x6c is the command queue
    // (CNetSubObject); and (CNetPlayerSlot*)m_4 is the base of the four per-player
    // ack-latency slots. No single clean type -> left as the +0x4 sub-object ptr.
    void* m_4; // +0x004
    char m_pad8[0x14 - 0x8];
    INetReleasable* m_releaseIface; // +0x014  the secondary COM interface Destroy releases (slot 2)
    IDirectPlay4Z* m_directPlay; // +0x018  the DirectPlay session interface (IDirectPlay4-shaped)
    i32 m_1c;                    // +0x01c  WM_COMMAND lParam value the resync handlers post
    char m_pad20[0x58 - 0x20];
    // The managed-player-object list (a by-value CObList embedded at +0x54). Its
    // 0x1c-byte body spans +0x54..+0x70; the +0x58 head node ptr below is the
    // CObList's own m_pHead (what FindPlayerById walks). Modeled as the head
    // pointer here; RemovePlayerObj reaches the embedded CObList via a +0x54 cast.
    CNetPlayerNode* m_58; // +0x58  head of the player-object list (CObList m_pHead)
    char m_pad5c[0x70 - 0x5c];
    // The three list-box selection latches (one per managed list). Each list's
    // ReadXxxSel reader writes the selected item's data here when it is in range;
    // the matching clear-loop zeroes it (along with the +0x7c/+0x80/+0x84 id below).
    i32 m_groupSel;     // +0x70  group-list selected item data (ReadGroupSel)
    i32 m_playerSel;    // +0x74  player-list selected item data (ReadPlayerSel)
    i32 m_sessionSel;   // +0x78  player-object-list selected item data
    i32 m_groupSelId;   // +0x7c  group-list selection id (zeroed with m_groupSel on clear)
    i32 m_playerSelId;  // +0x80  player-list selection id (zeroed with m_playerSel on clear)
    i32 m_sessionSelId; // +0x84  player-object-list selection id (zeroed with m_sessionSel on clear)
    char m_pad88[0x520 - 0x88];
    CNetSession* m_session; // +0x520  the DirectPlay session sub-object (command slots)
    CNetMgr* m_peer;        // +0x524  peer net-manager (owns the player list); null => no session
    i32 m_useChannelLatency; // +0x528  ack-latency source selector (set => inline m_channelLatency[])
    i32 m_sessionTerminated; // +0x52c  "the game session has been terminated"
    char m_pad530[0x534 - 0x530];
    i32 m_534;             // +0x534  host-mode flag (no use-site in matched code; left placeholder)
    i32 m_removedFromGame; // +0x538  "you have been removed from the game by the host"
    char m_pad53c[0x56c - 0x53c];
    i32 m_gameFull; // +0x56c  "this game is already full"
    i32 m_versionMismatch; // +0x570  version-mismatch latch (HandleVersionCheck sets; WaitForConnect reports)
    i32 m_outOfSyncGuard; // +0x574  OnOutOfSync per-instance reentrancy guard
    i32 m_syncGate;       // +0x578  gates the frame-sync long-frame toggle
    char m_pad57c[0x580 - 0x57c];
    i32 m_connected; // +0x580  connection established (gates resend/version report)
    i32 m_584;       // +0x584  state word cleared on each dispatch handler entry (role unproven)
    char m_pad588[0x58c - 0x588];
    i32 m_admitted; // +0x58c  set once the local player is admitted (connect-wait exit latch)
    char m_pad590[0x598 - 0x590];
    CString m_configSection; // +0x598  config section/value-name prefix ("<section>_CmdDelay" etc.)
    char m_pad59c[0x5a4 - 0x59c];
    DWORD m_cmdDelay; // +0x5a4  the "_CmdDelay" value (also the per-command sequence scale)
    DWORD m_resend;   // +0x5a8  the "_Resend" value
    i32 m_gameClosed; // +0x5ac  "this game is closed"
    char m_pad5b0[0x5bc - 0x5b0];
    i32 m_localPlayer; // +0x5bc  the local player descriptor ptr (gate in WaitForConnect)
    i32 m_5c0;         // +0x5c0  local player id (inferred from +0x5bc[+4]; no use-site here)
    char m_pad5c4[0x5e0 - 0x5c4];
    u32 m_lastFrameDelta; // +0x5e0  last frame-sync delta (ms)
    u32 m_lastFrameTime;  // +0x5e4  last frame-sync timestamp (timeGetTime)
    char m_pad5e8[0x5f0 - 0x5e8];
    DWORD m_channelLatency[4]; // +0x5f0  per-channel ack-latency values

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
    char m_pad0[4];
    void* m_4; // +0x4
};

#endif // NET_NETMGR_H
