// NetMgr.h - the engine CNetMgr (DirectPlay networking manager) - minimal
// reconstruction sufficient to byte-match a cluster of its state/message
// handlers and config writers. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS and code bytes are load-bearing.
//
// OWNERSHIP RESOLVED (netmgr-vs-cmulti, 2026-07-05): the CNetMgr class below is
// a CONFLATION of TWO retail objects; kept in one shape until the mass split
// (every method is byte-anchored - owner renames must go unit-by-unit against
// the delinker-packing rule):
//   1. The REAL CNetMgr (RTTI `CNetMgr : CObject`, ??_7 @0x1ea42c, ??1
//      @0x0b6000): the small DirectPlay wrapper - the +0x14/+0x18 COM
//      interfaces, the +0x1c/+0x38/+0x54 managed CObLists (exactly what
//      ~CNetMgr @0xb6000 tears down before the CObject restamp; it never
//      touches anything past +0x54), the +0x28/+0x44 counts, the +0x70..+0x84
//      selection latches, and the 0x178xxx method cluster (Init/Destroy/
//      Clear*List/Enum*/Add*Node/FindPlayerById/SetData.../Populate*List).
//   2. CMulti, the multiplayer game-state (RTTI `CMulti : CPlay : CState`,
//      <Gruntz/Multi.h>): EVERY field here >= +0x2d8 (m_session/m_peer/
//      m_localPlayer/m_configSection/m_cmdDelay/...) and the whole
//      0xb5xxx-0xbdxxx method cluster (SetupServices/PollSession/
//      DispatchRecvMsg/SendStat*/BroadcastChatLine/WaitForConnect/
//      CreateSession/VerifyCustomLevel/...) are CMulti's. Proof: CMulti::
//      CMulti (the 0x8b960 TransitionState factory) zero-inits +0x520/+0x524/
//      +0x5b0/+0x600; ~CMulti (0x8d270) destroys the +0x59c..+0x604 CString/
//      CByteArray run; CMulti::Teardown (0xb6110) reads +0x520/+0x524/+0x5bc/
//      +0x580 on the SAME `this` it passes to SendNetStat; the lobby DlgProcs
//      (0xbdc00/0xbe0a0) bind g_gameReg->m_curState (the current game-state ==
//      CMulti) and call PollSession/SendNetStat/BroadcastChatLine on it.
//   The LINK: CMulti+0x524 (m_peer below; m_netGate in <Gruntz/Multi.h>) HOLDS
//   the real CNetMgr. Every 0xbxxxx method reaches the DirectPlay wrapper
//   through it, never on its own `this`:
//     SendStatBuf 0xb91f0:        mov ecx,[ecx+0x524]; call 0x179090 (SetGroupDataFrom)
//     ResolveLocalPlayer 0xba7d0: mov ecx,[esi+0x524]; call 0x178e90 (FindPlayerById)
//     PollSession 0xb95f0:        mov eax,[esi+0x524]; mov eax,[eax+0x18]
//                                 (the peer's IDirectPlay4); call [.+0x44]
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
#include <rva.h>          // SIZE_UNKNOWN/VTBL class-metadata macros used below
#include <Wap32/Object.h> // CObject - the shared CObject-like grand-base

// <Mfc.h> brings <windows.h> USER32 (PostMessageA / Sleep / GetAsyncKeyState - the
// connect wait polls VK_ESCAPE to abort; HWND / UINT / ...) and the central WINMM
// timeGetTime decl (the frame-sync / connect-wait clock).
#include <Mfc.h>
#include <wtypes.h>   // HRESULT (Mfc.h's lean windows.h does NOT expose it - the reason
                      // ComDefs.h existed); the real header, lighter than <objbase.h>'s full
                      // OLE/RPC chain (which perturbs matched Net regalloc)
#include <basetyps.h> // STDMETHOD / STDMETHOD_ / PURE - the real COM interface macros for the
                      // hand-modeled DirectPlay (IDirectPlay4Z) + INetReleasable interfaces below
#include <Utils/RegistryHelper.h>
#include <Gruntz/ObList.h>
#include <Rez/RezMgr.h> // RezAlloc - the engine heap allocator the node factories use

// ---------------------------------------------------------------------------
// ActiveWait - the engine busy-wait (__cdecl). A plain file-scope free function
// defined in another TU (Utils/DebugTiming.cpp); here it is an external no-body decl
// so the FrameSyncWait `call rel32` reloc-masks.
void ActiveWait(u32 milliseconds); // 0x13dfe0

// ---------------------------------------------------------------------------
// CString - the minimal MFC CString model (a single char* @+0). The config
// writer builds value-name strings "m_section + _Suffix" via the engine global
// operator+ overload (AFXAPI == __stdcall) and destroys the temporaries via
// the engine CString dtor. Both are external/no-body so their `call rel32`
// displacements reloc-mask.
//   operator+(const CString&, LPCTSTR)  (AFXAPI)
//   CString::~CString()                 (__thiscall)
// ---------------------------------------------------------------------------
#include <Gruntz/String.h>

CString __stdcall operator+(const CString& lhs, const char* rhs);

// ---------------------------------------------------------------------------
// Forward declarations (defined in the TU that dereferences them).
// ---------------------------------------------------------------------------
class GruntzPlayer;     // <Gruntz/GruntzPlayer.h>  - the leaving-player slot
class CGruntzCmdMgr;    // <Gruntz/GruntzCmdMgr.h>  - the m_4 game-mgr's +0x6c command manager
class CNetMgr;          // defined below; the command slot caches one as its +0x1c owner
class CMulti;           // <Gruntz/Multi.h> - the multiplayer game-state (owns CNetSession::Init a2)
struct GruntRec;        // the lobby-sync grunt-state record (defined below CNetCmdSlot)
class CDDrawSurfaceMgr; // <Gruntz/GameRegistry.h> - the +0xc world holder (CState::m_c mirror)

// ---------------------------------------------------------------------------
// (The tiny +0x38 CGameMgr facet view is GONE - the real global-ns CGameMgr lives
// in <Wap32/Wap32.h> (which owns the ??_7 binding); the one consumer reads the
// typed g_gameReg->m_settings directly.)

// ---------------------------------------------------------------------------
// The multiplayer command dispatcher IS CMulti::RunErrorDialog (0xbc250): the
// pause/optionz/outofsync/dropplayer handlers call it as `this->RunErrorDialog(cmd,
// &callback, flag)`. Each handler passes a distinct callback (engine routines reached
// via incremental-link thunks so the `push &callback` reloc-masks); modeled as external
// no-body functions whose address is taken.
// ---------------------------------------------------------------------------

// Per-command callbacks (address-taken only; bodies are external/no-body).
extern "C" void MultiOptionzCallback();
extern "C" void MultiPauseCallback();
extern "C" void MultiOutOfSyncCallback();
extern "C" void MultiDropPlayerCallback(); // OnDropPlayer (MULTI_DROPPLAYER)

// The pending drop's player id (-999 == none). Owned + DEFINED in src/Gruntz/Multi.cpp
// (private to that TU); this is the single reference extern (canonical, one per RVA).
extern i32 g_dropPlayerId; // 0x611d88  saved dropped-player id

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
extern "C" i32 g_localVersion;  // 0x60fa70  (sibling; not TU-private)
extern i32 g_remoteVersion;     // 0x60fa74  DEFINED in src/Gruntz/Multi.cpp (owner TU)
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
SIZE_UNKNOWN(CNetVersionMsg); // host-version msg view (only +0x18/+0x1c pinned); size TBD

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
SIZE(CNetVersionPacket, 0x20); // fully-known stack packet

// ---------------------------------------------------------------------------
// CNetMgr - the DirectPlay networking manager. Only the members the matched
// methods touch are pinned:
//   +0x004 m_4              : a sub-object whose +0x4 is a window-handle holder;
//                             the message handlers do (((T*)m_4)->m_4)->m_4 to
//                             reach the HWND.
//   +0x01c m_resyncLParam   : the lParam value posted with the resync message.
//   +0x574 m_outOfSyncGuard : OnOutOfSync's per-instance reentrancy guard.
//   +0x584 m_584            : a state word the handlers clear on entry.
//   +0x598 m_configSection  : a CString - the config section/value-name prefix
//                             ("m_configSection + _CmdDelay" etc.).
//   +0x5a4 m_cmdDelay       : the _CmdDelay value persisted by ApplyCmdDelayDefaults.
//   +0x5a8 m_resend         : the _Resend value persisted by ApplyCmdDelayDefaults.
// ---------------------------------------------------------------------------
// One per-player slot viewed m_4-RELATIVE (base m_4, stride 0x238): retail's
// GetMaxAckLatency leaf addresses the same array this way (base m_4, large disps
// +0x164/+0x170/+0x37c), a distinct authentic encoding from the +0x150 channel
// base below - so this view is kept for that one leaf.
struct CNetPlayerSlot {
    char m_pad0[0x164];
    DWORD m_164; // +0x164  slot-active gate A
    char m_pad168[0x170 - 0x168];
    DWORD m_170; // +0x170  slot-active gate B
    char m_pad174[0x37c - 0x174];
    DWORD m_37c; // +0x37c  the slot's latency value
};
SIZE_UNKNOWN(CNetPlayerSlot); // m_4-relative slot view (3 gate/latency dwords pinned)

// ---------------------------------------------------------------------------
// One channel descriptor in the inline array at (CNetGameMgr + 0x150), stride
// 0x238, four entries (the 0x8e0 loop bound == 4 * 0x238) - modeled by value as
// CNetGameMgr::m_channels[4]. Same memory the CNetPlayerSlot view sees m_4-
// relative (channel+0x14 == m_4+0x164 gate A, channel+0x20 == m_4+0x170 gate B,
// channel+0x22c == m_4+0x37c latency); the serialize/parse/register run
// (0xba810..0xbb190) addresses it +0x150-relative. Only the touched fields are
// pinned. The +0x4 member is a CString (the channel's name); +0x18 a dword id;
// +0x20 the "active" gate.
struct CNetChannel {
    i32 m_id;       // +0x00  id/header dword (serialized at packet+8)
    CString m_name; // +0x04  channel name (CString)
    i32 m_slotId;   // +0x08  net-slot id (SetNetSlot key) / player word
    i32 m_c;        // +0x0c
    i32 m_10;       // +0x10
    i32 m_14;       // +0x14  (== CNetMgr::m_4+0x164 gate A)
    i32 m_playerId; // +0x18  owner player id (compared vs m_localPlayerId)
    i32 m_flag;     // +0x1c  flag (0/1)
    i32 m_active;   // +0x20  "active" gate (== CNetMgr::m_4+0x170 gate B)
    char m_pad24[0x228 - 0x24];
    i32 m_228;     // +0x228
    i32 m_latency; // +0x22c  the slot's latency value (== CNetMgr::m_4+0x37c)
    i32 m_230;     // +0x230
    char m_pad234[0x238 - 0x234];

    // The channel's name (m_name @+0x04) is fetched via the folded GetName routine
    // at 0x1f450 - identical +0x04 CString read that COMDAT-folds with
    // GruntzPlayer::GetName in retail (one address). Callers reach it as
    // ((GruntzPlayer*)ch)->GetName() (cast at the call; cannot dup the RVA).
};
SIZE(CNetChannel, 0x238); // one inline channel descriptor (array stride 0x238)

// A payload entry found through the m_58 player list. FindPlayerById matches on
// the entry's +0x4 id field.
struct CNetPlayerEntry {
    char m_pad0[4];
    i32 m_4;     // +0x4  the entry's id (the lookup key)
    CString m_8; // +0x8  display name (returned by GetName; COMDAT-shares CNetMgr::GetName's code)

    // The entry's display name (a CString at +0x8, returned by value / NRV). In the
    // retail this fetch shares CNetMgr::GetName's routine (0xba170) - same +0x8 read.
    CString GetName();
};
SIZE_UNKNOWN(CNetPlayerEntry); // payload-entry view (only +0x4 id pinned); size TBD

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
SIZE_UNKNOWN(CNetStatPacket); // 0x10-byte stat-packet header view; full record size TBD

// A singly-linked node in the m_58 player list: +0x0 next, +0x8 payload.
struct CNetPlayerNode {
    CNetPlayerNode* m_next; // +0x0
    char m_pad4[4];
    CNetPlayerEntry* m_8; // +0x8  the payload entry
};
SIZE_UNKNOWN(CNetPlayerNode); // player-list node walk-view; retail size TBD

// ---------------------------------------------------------------------------
// The per-player command-resend slot ResetPlayerCommands operates on. The
// session sub-object (CNetMgr+0x520) keeps four of these in an inline array;
// FindCmdSlot returns the one whose owning player matches. Only the fields the
// reset reads are pinned. The three engine helpers it fires (slot methods,
// external incremental-link thunks) clear the slot's command range.
// ---------------------------------------------------------------------------
// A queued player command the slot's CObList holds: +0x0 sequence number,
// +0x4 a payload word (Verify compares it against the resync entry).
struct CNetCmd {
    i32 m_seq; // +0x0  command sequence number
    i32 m_4;   // +0x4  payload word (resync compare)
};
SIZE_UNKNOWN(CNetCmd); // queued-command view (2 fields pinned); retail size TBD

// The per-slot player descriptor the slot's +0xc pointer targets (the lobby-sync
// view of the command-head target): +0x4 DirectPlay player id, +0x18 peer/target id,
// +0x2c a per-slot dirty flag (== m_cmdHead[0xb]).
struct SlotInfo {
    char pad00[4];
    i32 m_playerId; // +0x04 DirectPlay player id (SetData `a`, Recv/Read channel)
    char pad08[0x18 - 0x08];
    i32 m_netId; // +0x18 peer/target id (SetData `b`)
    char pad1c[0x2c - 0x1c];
    i32 m_dirty; // +0x2c set on slot re-init
};
SIZE_UNKNOWN(SlotInfo);

// ---------------------------------------------------------------------------
// The 0x64-byte command slot. ONE real class - the lobby-sync / lobby-channel /
// per-session ex-views (CLobbyChannel/CCluster0c/CNetSlotAux/CLobbySlot) all folded
// here: the command-session context (CNetSession command slots) and the lobby-sync
// context (per-channel state) are the SAME object, so several fields carry a
// command-view and a lobby-sync-view name via an anonymous union (byte-neutral;
// offsets unchanged). Only OFFSETS + emitted bytes are load-bearing.
// ---------------------------------------------------------------------------
struct CNetCmdSlot {
    i32 m_state; // +0x0   "armed"/slot-state flag (AckDropPlayer sets it to 1; ==3 => active)
    union {
        i32 m_resetGuard; // +0x4  command: "slot already reset" guard
        i32 m_isRemote;   //        sync:    0 = local channel
    };
    // +0x8  command: latched sequence (Touch copies m_baseSeq here); sync reuses the
    // same i32 as a counter / a CNetMgr* it casts (SendGruntRecord). One canonical name.
    i32 m_latchedSeq;
    union {
        i32* m_cmdHead; // +0xc  command: -> command-list head value (m_cmdHead[0xb] == +0x2c flag)
        SlotInfo* m_desc; //       sync:    player descriptor (same target, named view)
    };
    union {
        i32 m_latency; // +0x10  command: latency value (CheckLatency compares vs the cap)
        i32 m_timer;   //        sync:    activity timer
    };
    i32 m_baseSeq; // +0x14  base command sequence number (both views)
    union {
        i32 m_maxSeq;  // +0x18  command: high-water sequence (RaiseMax keeps the max)
        i32 m_sentSeq; //        sync:    highest sequence sent
    };
    CMulti* m_owner; // +0x1c  owning CMulti back-pointer (reaches m_session/m_4/DispatchRecvMsg;
                     //         sync cleared the same slot as m_1c). One canonical name.
    // CPtrList, not CObList: AddCmd/RemoveCmd/ClearCmds/ResetSync all call the
    // band-A list bodies (ctor 0x1b4867 / AddTail 0x1b4991 / RemoveHead 0x1b4a03),
    // whose vtable 0x1eb054 slot-0 GetRuntimeClass names "CPtrList". (CNetMgr's own
    // group/player/session lists below DO call band B = the real CObList.)
    CPtrList m_cmds;   // +0x20  queued-command list (CPtrList, 0x1c bytes)
    i32 m_ackFlags[4]; // +0x3c  per-player ack-flag array (ProcessCmd sets m_ackFlags[pid])
    i32 m_rangeA[3];   // +0x4c  command-range A (reset to -1) (sync sub-object m_4c)
    i32 m_rangeB[3];   // +0x58  command-range B (reset to -1) (sync sub-object m_58)

    CNetCmdSlot();            // bbec0  construct m_cmds (/GX EH) + reset fields
    ~CNetCmdSlot();           // b62a0  ResetAll + tear down m_cmds (CObList) [multi]
    void ResetAll();          // c0bb0  zero all fields + ranges
    void AdvanceSeq(i32 id);  // c0f10  fold an ack id into the high-water window
    void RaiseMax(i32 v);     // c0fa0  keep the high-water sequence
    void ResetTriple(i32* p); // c10a0  splat -1 over three dwords
    // The three command-id-window helpers (0xc0fd0/0xc1010/0xc1060, __thiscall). Retail
    // passes the window (m_rangeA/m_rangeB) explicitly and ignores `this`, so at every
    // call site cl loads ecx = this even though the body never reads it - modeling them as
    // members reproduces that (and binds SendBatch/SendOne's cross-view calls).
    i32 NetCmdIdFind(i32* arr, i32 v);   // c0fd0  is `v` one of the three ids in `arr`?
    void NetCmdIdAdd(i32* arr, i32 v);   // c1010  add `v` to the first free (-1) slot
    void NetCmdIdClear(i32* arr, i32 v); // c1060  clear (-1) the first slot equal to `v`
    void AddCmd(CNetCmd* cmd);           // c1170  enqueue a command (dedup by seq)
    void RemoveCmd(i32 seq);             // c11b0  drop one queued command
    void GetRange(i32* pMin, i32* pMax); // c1230  min/max queued sequence
    CNetCmd* FindCmd(i32 seq);           // c12b0  find a queued command by seq
    void ClearCmds();                    // c12e0  drain + recycle the queue
    void Touch();                        // c1390  latch the slot (sets +4, +8)
    void FullReset();                    // c0c20  zero the command fields + both ranges
    void ClearAckFlags(); // bf120  zero the +0x3c..+0x48 ack-flag dwords (sync InitSub3c)
    // Lobby-sync: emit one grunt-state record for the channel (sync SendAll's per-slot
    // send; reads m_08 as CNetMgr* + m_desc as the descriptor, ships via SetData).
    i32 SendGruntRecord(i32 seq, GruntRec* rec, i32 flag, i32 slot, i32 gruntId); // bfc70
    CString* BuildHostName(CString* out); // bc3f0  fill `out` with the slot's host name [multi]
    i32
    Init(i32 a1, i32* a2, i32 a3); // c0b10  seed a fresh slot, then ClearCmds + reset both ranges
    i32 ProcessCmd(i32 playerId, void* rec, i32 size); // c0c70  parse/dispatch a command record
    // Slot-readiness check 0xc1320 (CNetSession::Verify dispatches it on each slot cursor).
    // `this` is the slot: reads m_owner (+0x1c) -> m_session -> m_slots[], gated by this
    // slot's m_ackFlags (+0x3c). (Was a stray CNetSyncCheck view; folded back to the slot.)
    i32 Ready(); // c1320
};
SIZE(CNetCmdSlot, 0x64); // fully-laid-out inline command slot (array stride 0x64)

// One node of a slot's +0x20 command CObList (the MFC CObList CNode shape):
// +0x0 next, +0x4 prev, +0x8 the queued CNetCmd payload. NetCmdSlot.cpp walks the
// queue by casting the CObList's head POSITION to this node.
struct CNetCmdNode {
    CNetCmdNode* m_next; // +0x0
    CNetCmdNode* m_prev; // +0x4
    CNetCmd* m_data;     // +0x8  (this queue holds CNetCmd)
};
SIZE_UNKNOWN(CNetCmdNode); // CObList node walk-view; retail size TBD

// The command record's fixed header (after the opcode/parity prefix): a sequence
// number, two control words and a per-entry count byte; the payload follows.
struct CNetCmdHdr {
    i32 m_sequence;   // +0x0  sequence
    i32 m_windowBase; // +0x4  window base
    i32 m_flags;      // +0x8  flags word
    u8 m_entryCount;  // +0xc  entry count
};
SIZE_UNKNOWN(CNetCmdHdr); // record header prefix (payload follows); full record size TBD

// The recycled command packet AddCmd queues: sequence, owning slot, a flag byte,
// the payload length and the inline payload copy.
struct CNetCmdPacket {
    i32 m_sequence;       // +0x0  sequence
    CNetCmdSlot* m_owner; // +0x4  owning slot (this)
    u8 m_flags;           // +0x8  flag byte
    char m_pad9[0xc - 9];
    i32 m_payloadLength; // +0xc  payload length
    char m_payload[1];   // +0x10 payload
};
SIZE_UNKNOWN(CNetCmdPacket); // trailing-payload packet (flexible array); fixed size TBD

// The DirectPlay session sub-object at CNetMgr+0x520. Two helpers are reached
// here (external __thiscall thunks): FindCmdSlot linear-scans the four inline
// command slots for the one whose player id matches; ResetCmdBuffers zeroes the
// head of each of those four slots.
// The per-color selection sub-object living at CNetCmdBuf+0x150. CreateSlot hands
// slot[i] a pointer to its +0x00 (the per-slot command-head target, buffer+0x150);
// NetCmdMgr.cpp's SelectColor claims/releases a color through its +0x08 owner id.
struct CColorSlot {
    i32 m_slotHead;             // +0x00 (buf+0x150)  CreateSlot's per-slot command-head target
    char m_pad04[4];            // +0x04
    i32 m_currentOwnerPlayerId; // +0x08 (buf+0x158)  current color owner player id
};
SIZE_UNKNOWN(CColorSlot); // +0x150 sub-region view; retail size TBD

// One 0x238-byte command buffer the session keeps an array of at +0x0; CreateSlot
// hands slot[i] a pointer to the +0x150 field of buffer[i].
struct CNetCmdBuf {
    char m_pad0[0x150]; // +0x000
    CColorSlot m_sel;   // +0x150
    char m_pad15c[0x238 - 0x15c];
};
SIZE(CNetCmdBuf, 0x238); // fully-known command buffer (array stride 0x238)

// One 0x410-byte resync entry the session keeps at +0x3b0; Verify compares slot
// command +0x4 against entry[(m_18-2)%128].m_4. ResetAll zeroes m_0/m_4, the
// byte at +0x8 and the dword at +0xc per entry.
struct CNetResyncEntry {
    i32 m_0;
    i32 m_4; // +0x4
    u8 m_8;  // +0x8  flag byte (zeroed by ResetAll)
    char m_pad9[0xc - 9];
    i32 m_c; // +0xc
    char m_padc[0x410 - 0x10];
};
SIZE(CNetResyncEntry, 0x410); // fully-known resync entry (array stride 0x410)

// ---------------------------------------------------------------------------
// Lobby-sync helper types (the sync-view of the session): a per-period grunt-state
// record (the +0x3b0 table, same 0x410 stride as CNetResyncEntry - the two are the
// same array, named per context), the synced game object in the +0x1b0 id-map, the
// session/game manager the sync object caches, and the sync protocol header.
// ---------------------------------------------------------------------------
// A grunt record in the +0x3b0 table (stride 0x410; overlays CNetResyncEntry).
struct GruntRec {
    i32 m_seq;             // +0x00 sequence number (== CNetResyncEntry::m_0)
    i32 m_checksum;        // +0x04 state checksum (== CNetResyncEntry::m_4, Verify compare)
    unsigned char m_count; // +0x08 grunts serialized into this record
    char pad09[3];
    i32 m_payloadLen;             // +0x0c payload length
    char m_payload[0x410 - 0x10]; // +0x10 payload
};
SIZE(GruntRec, 0x410);

// The synced game object stored in the +0x1b0 id-map IS the queued CGruntzCommand
// (<Gruntz/GruntzCommand.h>). IDENTITY RESOLVED 2026-07-19 - the unlock this note
// used to wait for ("the m_idMap-insert function") HAS landed, and the chain is
// closed end to end:
//   * the insert is CNetSession::ArmSlot (0xc03f0, NetCmdSlot.cpp; ex "GlyphTable"),
//     and its ONE retail caller is CMulti::Render (0xb6890, Multi.cpp): the object
//     it arms is popped from Mgr()->m_cmdSubMgr's queue - the COMMAND manager's
//     list - and right before the call retail writes [obj+0x06]=parity-byte and
//     [obj+0x0c]=1: exactly CGruntzCommand::m_6 (the type/key byte) and
//     CGruntzCommand::m_submitted (the "submit-context latch: net").
//   * the snapshot drain (SendSnapshot below) dispatches slot 8 @+0x20 with
//     (char* buf, i32) and adds the returned byte count - CGruntzCommand::Pack,
//     whose leaf bodies (0x24050/0x240d0) end in `ret 8` and return the count.
//   * slot 1 is the 4-arg CSerialArchive Serialize - CGruntzCommand's slot 1.
// (The ex `struct CSyncObj { v0()..v7(); Serialize; }` placeholder view and its
// invented 0-arg slot signatures are DISSOLVED onto the real class. Multi.h's
// padded CMultiLogicNode shell - m_6/m_c at the same offsets - is the SAME class
// seen from CMulti::Render's side; folding that twin is the follow-up.)
class CGruntzCommand;

// CNetSession+0x04 IS the owning CMulti - the same +0x04 field the command view holds
// as m_4 (Init's a2, the CMulti). Its "+0x564 net-busy flag" is CMulti::m_pollAbort.
// Typed CMulti* below; the deref sites (NetCmdSlot.cpp) pull <Gruntz/Multi.h>.)

// A lobby-sync control/command message header (the net protocol's 3-word header).
struct LobbyMsg {
    i32 m_type; // +0x00 message type
    i32 m_04;   // +0x04
    i32 m_08;   // +0x08
};
SIZE_UNKNOWN(LobbyMsg);

// The recycled-command-packet allocator (0xbf530, __cdecl): hands back a node from
// the global packet pool. Modeled by its delinker name so the call is named.
void* Unmatched_bf530(i32 zero); // bf530
// Return a finished command node to the global recycle pool (g_pool.AddTail).
void RecycleCmd(void* cmd); // bf580  __cdecl

// ---------------------------------------------------------------------------
// The per-game DirectPlay session (0x20bb0 bytes; new/delete via Rez). ONE real
// class - the ex-views CLobbyObjB/CNetSession2/CLobbySync all folded here: the
// command-session context and the lobby-sync context are the SAME +0x520 object
// (`new CNetSession`, dtor @0xb6220). Several fields carry a command-view and a
// lobby-sync-view name via an anonymous union (byte-neutral; offsets unchanged).
// ---------------------------------------------------------------------------
struct CNetSession {
    // per offset, so each carries a single canonical (typed/semantic) name - the
    // former hex/command-view aliases (m_00/m_4/m_8/m_c/m_10/m_14/m_18/m_1c/m_1b0)
    // are folded onto them.
    CNetCmdBuf*
        m_0; // +0x00  base of the per-slot command-buffer array (Init a1); ResetSync clears it
    CMulti*
        m_session; // +0x04  owning CMulti (Init a2, kept as an i32 handle re-passed to CreateSlot)
    CNetMgr* m_netMgr;      // +0x08  the DirectPlay CNetMgr peer (Init a3; endpoint at +0x18)
    SlotInfo* m_localDesc;  // +0x0c  local player descriptor
    i32 m_tick;             // +0x10  sub-tick counter (also the lobby slot-count-id base)
    i32 m_snapshotDone;     // +0x14  per-period snapshot-built flag
    i32 m_seq;              // +0x18  reconcile-period sequence (Verify: (m_seq-2)%128)
    i32 m_period;           // +0x1c  ticks per period / cached owner m_cmdDelay (the modulus)
    CNetCmdSlot m_slots[4]; // +0x20  four inline command slots (0x64 each)
    CGruntzCommand*
        m_idMap[0x80]; // +0x1b0  armed-command ptr table (GetSlotPtr) / 0x200-byte resync scratch
    union {
        CNetResyncEntry m_entries[0x80]; // +0x3b0  command: resync entries (signed-indexed)
        GruntRec m_records[0x80];        //        sync:    grunt-record table
    };

    CNetCmdSlot* FindCmdSlot(i32 playerId); // c00a0
    void ResetCmdBuffers();                 // c0070
    i32 AllSlotsReachedSeq(i32 seq);        // c0320  1 unless an active slot's m_maxSeq < seq
    void AdvanceAllSlots(i32 id);           // c0370  AdvanceSeq(id) over every active slot
    void RaiseAllSlotsMax(i32 v);           // c03b0  RaiseMax(v) over every active slot
    i32 CheckLatency(i32 cap);              // c04a0  any active slot with m_10 > cap?
    CNetCmdSlot* CreateSlot(i32 index, i32 owner); // bfff0  init slot[index]
    i32 Verify();                                  // c04f0  resync consistency check (0-arg)
    void ResetAll();                               // bbf80  full reset: header + 4 slots + entries
    void Reset();      // bf150  re-init header/slots/scratch/entries (session-level reset)
    i32 Verify(i32 n); // c0290  slot-window validation against sequence n
    // Scan the four inline command slots for the first active (m_state==3),
    // un-reset (m_resetGuard==0) slot whose latency exceeds key (unsigned).
    CNetCmdSlot* FindSlot(u32 key); // c0460
    // Wiring init (caches the owner pointers then Reset()s). a1=command-buffer array.
    i32
    Init(void* a1, class CMulti* a2, void* a3); // bef80  (a2 is the owning CMulti; reads a2->m_5a4)

    // --- lobby-sync methods (ex-CLobbySync, folded onto the same object) ---
    ~CNetSession();      // b6220  ResetSync + vector-destroy the 4 slots [multi]
    void ResetSync();    // bf000  clear header, recycle each slot, drain pool
    i32 Poll(i32 delta); // bf5a0  advance active channels; drain the endpoint
    i32 Dispatch(i32 a, LobbyMsg* b, i32 c); // bf700
    i32 DispatchMsg(LobbyMsg* m, i32 arg2);  // bf7c0
    i32 Tick();                              // bf9e0  snapshot -> broadcast -> flush
    i32 SendAll();                           // bfb40
    i32 SendBatch();                         // bfd40
    i32 SendOne(CNetCmdSlot* s, i32 v);      // bfeb0
    void Reconcile();                        // c00f0
    i32 Advance();                           // c01d0
    CGruntzCommand* GetSlotPtr(i32 v);    // c0430  id-map fetch (ex "GlyphTable::Get")
    void ArmSlot(void* node, i32 parity); // c03f0  id-map store (ex "GlyphTable::Set")
    // Checksum @0xc0590: the game-state signature accumulator over the 4x15
    // placed-grunt roster (defined in src/Gruntz/GameChecksum.cpp; ex the
    // "CGameSyncSig" view). Step2437 is a per-frame poke with no bound RVA,
    // kept declared so the CMulti dispatch compiles.
    i32 Checksum(); // c0590
    void Step2437(); // per-frame poke (reloc-masked)
    // 0xbf1d0 (/GX): a CRC/sync-diagnostic dump - walk the level's 4x15 placed-grunt
    // roster (m_session->Mgr()->m_cmdGrid->m_grid) and report a per-grunt CRC line
    // through m_session->ReportVersionMsg. Defined out-of-line in BuildGruntzCrcInfo.cpp.
    void BuildGruntzCrcInfo();

    // The engine routes global new/delete through RezAlloc/RezFree; model that as
    // the class allocator so `new CNetSession()` emits a direct RezAlloc call.
    void* operator new(size_t n) {
        return RezAlloc(static_cast<u32>(n));
    }
    void operator delete(void* p) {
        RezFree(p);
    }

    // Inline ctor so CNetMgr::CreateSession's `new CNetSession()` lowers to the
    // 4-slot vector-construct + ResetAll (no out-of-line ctor call), matching the
    // retail allocation shape.
    CNetSession() {
        ResetAll();
    }
};
SIZE(CNetSession, 0x20bb0); // fully-laid-out: +0x3b0 + 0x80*0x410 resync entries

// The +0x6c command-dispatch queue (its Dispatch/EnqueueCommand) is the real
// CGruntzCmdMgr the game-manager sub-object owns at CGruntzMgr+0x6c - now modeled
// as CNetGameMgr::m_6c (a CGruntzCmdMgr*, see below), consolidating the former
// per-TU CNetCmdQueue/CNetSubObject/CNetMgrSub placeholder views into one type.

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

// The DirectPlay DPNAME name blob the enum callbacks read (dwSize@+0, dwFlags@+4,
// short-name ptr@+0x8, long-name ptr@+0xc). Named locally (dplay.h's own DPNAME
// would collide in the TUs that pull it in via dplobby.h) - only the two name
// pointers are used.
struct NetDPName {
    u32 dwSize;           // +0x00
    u32 dwFlags;          // +0x04
    char* lpszShortNameA; // +0x08
    char* lpszLongNameA;  // +0x0c
};
SIZE(NetDPName, 0x10); // DirectPlay DPNAME (only the two name pointers are used)

struct IDirectPlay4Z {
    // External DirectPlay-shaped COM interface (abstract, __stdcall; no dplay.h in
    // dx/Include, so the slot layout is hand-modeled). STDMETHOD == `virtual HRESULT
    // __stdcall`, so `dp->Method(args)` lowers to `mov eax,[dp]; call [eax+slot]` -
    // the runtime COM slot the engine never link-resolves. Only the called slots
    // carry signatures; slot names follow the reconstructor's functional labels (the
    // DirectPlay method identities are not all pinned - GetMessageCount slot 17 and
    // Receive slot 25 land at their real DX indices, the rest are best-effort).
    STDMETHOD(QueryInterface)(void* riid, void* out) PURE;                 // slot 0
    STDMETHOD(v01)() PURE;                                                 // slot 1
    STDMETHOD(v02)() PURE;                                                 // slot 2
    STDMETHOD(Open)(void* a, void* b, i32 c) PURE;                         // slot 3  (+0x0c)
    STDMETHOD(v04)() PURE;                                                 // slot 4
    STDMETHOD(v05)() PURE;                                                 // slot 5
    STDMETHOD(GetSessionDesc)(void* a, void* b, i32 c, i32 d, i32 e) PURE; // slot 6 (+0x18)
    STDMETHOD(v07)() PURE;                                                 // slot 7
    STDMETHOD(v08)() PURE;                                                 // slot 8
    STDMETHOD(v09)() PURE;                                                 // slot 9
    STDMETHOD(v0a)() PURE;                                                 // slot 10
    STDMETHOD(v0b)() PURE;                                                 // slot 11
    STDMETHOD(EnumGroupsCb)(
        void* desc,
        void* callback,
        void* ctx,
        i32 flags
    ) PURE; // slot 12 (+0x30)
    STDMETHOD(EnumPlayers)(
        void* desc,
        void* a,
        void* callback,
        void* ctx,
        void* flags
    ) PURE;                                                              // slot 13 (+0x34)
    STDMETHOD(Enum2)(void* desc, void* ctx) PURE;                        // slot 14 (+0x38)
    STDMETHOD(v0f)() PURE;                                               // slot 15
    STDMETHOD(v10)() PURE;                                               // slot 16
    STDMETHOD(GetMessageCount)(i32 idPlayer, i32* lpCount) PURE;         // slot 17 (+0x44)
    STDMETHOD(v12)() PURE;                                               // slot 18
    STDMETHOD(GetGroupData)(i32 id, void* lpData, i32 flags) PURE;       // slot 19 (+0x4c)
    STDMETHOD(GetData2)(i32 id, void* lpData, u32* lpSize, u32 fl) PURE; // slot 20 (+0x50)
    STDMETHOD(v15)() PURE;                                               // slot 21
    STDMETHOD(GetPlayerData2)(void* in, void* out) PURE;                 // slot 22 (+0x58)
    STDMETHOD(v17)() PURE;                                               // slot 23
    STDMETHOD(EnumGroups)(void* desc, i32 flags) PURE;                   // slot 24 (+0x60)
    STDMETHOD(Receive)(
        i32* lpidFrom,
        i32* lpidTo,
        i32 flags,
        void* lpData,
        i32* lpSize
    ) PURE;                                                           // slot 25 (+0x64)
    STDMETHOD(SetData5)(i32 a, i32 b, i32 c, i32 d, i32 e) PURE;      // slot 26 (+0x68)
    STDMETHOD(v1b)() PURE;                                            // slot 27
    STDMETHOD(v1c)() PURE;                                            // slot 28
    STDMETHOD(GetData5)(i32 id, void* lpData, i32 size, i32 fl) PURE; // slot 29 (+0x74)
    STDMETHOD(v1e)() PURE;                                            // slot 30
    STDMETHOD(v1f)() PURE;                                            // slot 31
    STDMETHOD(v20)() PURE;                                            // slot 32
    STDMETHOD(v21)() PURE;                                            // slot 33
    STDMETHOD(v22)() PURE;                                            // slot 34
    STDMETHOD(v23)() PURE;                                            // slot 35
    STDMETHOD(v24)() PURE;                                            // slot 36
    STDMETHOD(v25)() PURE;                                            // slot 37
    STDMETHOD(v26)() PURE;                                            // slot 38
    STDMETHOD(v27)() PURE;                                            // slot 39
    STDMETHOD(v28)() PURE;                                            // slot 40
    STDMETHOD(v29)() PURE;                                            // slot 41
    STDMETHOD(v2a)() PURE;                                            // slot 42
    STDMETHOD(v2b)() PURE;                                            // slot 43
    STDMETHOD(v2c)() PURE;                                            // slot 44
    STDMETHOD(v2d)() PURE;                                            // slot 45
    STDMETHOD(v2e)() PURE;                                            // slot 46
    STDMETHOD(v2f)() PURE;                                            // slot 47
    STDMETHOD(v30)() PURE;                                            // slot 48
    STDMETHOD(SendEx)(
        i32 idFrom,
        i32 idTo,
        i32 flags,
        void* lpData,
        i32 size,
        i32 pri,
        i32 timeout,
        void* ctx,
        i32* lpMsgId
    ) PURE; // slot 49 (+0xc4)
};
SIZE_UNKNOWN(IDirectPlay4Z); // external DirectPlay COM interface (opaque object); size TBD

// ---------------------------------------------------------------------------
// A managed player object the m_54 list holds. RemovePlayerObj (0x178e20) tears
// one down: it calls the object's vtable slot 1 (a self-destruct/scalar-deleting
// dtor taking a single flag arg) and then unlinks the object from the m_54
// CObList using the cached __POSITION the object stores at +0x20. Modeled as a
// polymorphic class so `obj->SelfDestruct(1)` emits the thiscall virtual
// dispatch (slot 1 == +0x4); the virtual is never defined so no vtable is
// emitted in this TU.
//
// The concrete payloads (CNetPlayerListNode @0x5f0760, CNetSessionNode @0x5f0778,
// InterfaceObject) are all CObject-derived, so this base view's two used slots
// carry the CObject-interface identities: slot 0 is CObject::GetRuntimeClass (the
// shared grand-base slot-0 thunk 0x1bef01, undispatched here) and slot 1 is the
// per-payload scalar-deleting destructor (the flag-arg self-destruct). SelfDestruct
// stays a distinct named virtual (not ~CObject) because retail dispatches the
// deleting variant with an explicit flag=1, which the plain dtor cannot express.
// ---------------------------------------------------------------------------
class CNetPlayerObj {
public:
    virtual void GetRuntimeClass();      // +0x00  slot 0 (CObject GetRuntimeClass, 0x1bef01)
    virtual void SelfDestruct(i32 flag); // +0x04  slot 1 (scalar-deleting dtor, flag arg)

    char m_pad4[0x20 - 0x4]; // +0x04
    __POSITION* m_20;        // +0x20  cached list position
};
SIZE_UNKNOWN(CNetPlayerObj); // polymorphic-dispatch view (only +0x20 pinned); size TBD
// No VTBL: this is a slot-dispatch modeling view (virtuals undefined -> cl emits no
// vtable here); its concrete retail vtable is not confidently pinned in this TU.

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
SIZE_UNKNOWN(CNetListNode); // CObList node walk-view; retail size TBD

// ---------------------------------------------------------------------------
// Both node types derive from the shared engine grand-base CObject (RTTI
// "CObject", 5-slot interface, grand-base vtable @0x5e8cb4). vtable_hierarchy
// confirms each node's own vtable (0x5f0760 / 0x5f0778) is the CObject interface
// (slots 0/2/3/4 inherited, slot 1 the destructor override) with no new virtual.
// Real polymorphic so `new CNetPlayerListNode/CNetSessionNode` two-phase-stamps the
// base then the derived vtable (cl-emitted, reloc-masking 0x5e8cb4/0x5f0760/
// 0x5f0778) with the compiler's /GX new-cleanup frame - replacing the old manual
// g_net*NodeVtbl stamps. The emitted node vtables here are orphans (their RVAs are
// owned by NetSessionNode.cpp's VTBLs), so no VTBL is attached -> no dup-DATA.

// ---------------------------------------------------------------------------
// The 0x50-byte DPSESSIONDESC2 CNetPlayerListNode::Init deep-copies into +0x04:
// the full descriptor with dwSize@+0 (forced to 0x50 by Init) and the two name
// pointers Init strdup's in place (lpszSessionName@+0x30, lpszPassword@+0x34).
// ---------------------------------------------------------------------------
struct CNetSessionDesc {
    i32 m_dwSize; // +0x00  dwSize (forced to 0x50 by Init)
    char m_pad04[0x30 - 0x04];
    char* m_lpszName;     // +0x30  lpszSessionName
    char* m_lpszPassword; // +0x34  lpszPassword
    char m_pad38[0x50 - 0x38];
};
SIZE(CNetSessionDesc, 0x50); // the 0x50-byte DPSESSIONDESC2

// ---------------------------------------------------------------------------
// The managed-player object node the +0x38 player list holds. AddPlayerNode
// (0x1786d0) `new`-builds a 0x58-byte node (vptr 0x5f0760), whose ctor zeroes its
// body (0x14 dwords from +0x4) + m_54, then inits it from the DirectPlay player
// descriptor (the 0x1795a0 helper, which copies the 0x50-byte descriptor in and
// trims its name), and AddTail's it onto the +0x38 CObList, caching the returned
// __POSITION at +0x54.
// ---------------------------------------------------------------------------
class CNetPlayerListNode : public CObject {
public:
    CNetSessionDesc m_desc; // +0x04  the deep-copied 0x50-byte DPSESSIONDESC2
                            //        (name/password strdup'd in place at +0x34/+0x38)
    __POSITION* m_54;       // +0x54  cached AddTail position

    // Zero the 0x14-dword body + m_54 (the retail ctor sequence AddPlayerNode
    // inlines: single coalesced vptr stamp 0x5f0760 then the zero loop).
    CNetPlayerListNode() {
        i32* body = reinterpret_cast<i32*>(&m_desc);
        for (i32 i = 0; i < 0x14; i++) {
            body[i] = 0;
        }
        m_54 = 0;
    }
    virtual ~CNetPlayerListNode() OVERRIDE; // 0x1793b0 (NetSessionNode.cpp)
    i32 Init(CNetSessionDesc* desc);        // 0x1795a0  copy + trim the descriptor
    // Free the two strdup'd descriptor names (+0x34/+0x38) and clear the dwSize
    // marker (0x179680, NetMgr.cpp). The dtor's only helper - retail's SINGLE
    // caller is ??1 @0x1793db, and the dtor chain has exactly two vptr stamps
    // (own 0x5f0760 -> CObject 0x5e8cb4), so the ex-"CWapNodeB" that carried it
    // was a duplicate view of THIS class (its m_type/m_buf34/m_buf38 were
    // m_desc.m_dwSize/m_lpszName/m_lpszPassword), not a base - DISSOLVED.
    void FreeStrings(); // 0x179680
};
SIZE(CNetPlayerListNode, 0x58);       // AddPlayerNode (NetMgr.cpp 0x1786d0) RezAlloc(0x58)
VTBL(CNetPlayerListNode, 0x001f0760); // ??_7CNetPlayerListNode@@6B@ (5-slot CObject-derived)

// ---------------------------------------------------------------------------
// The session-list node the +0x54 list holds: AddSessionNode (0x178b30,
// /GX EH) `new`-builds a 0x24-byte node (base-dtor vptr 0x5e8cb4 during the two
// CString member ctors, final vptr 0x5f0778), whose ctor zeroes +0x4/+0x14/+0x18/
// +0x20, then GetData5's (slot 0x74) the session blob and AddTail's the node onto
// the +0x54 list.
// ---------------------------------------------------------------------------
class CNetSessionNode : public CObject {
public:
    i32 m_sessionId;      // +0x04
    CString m_8;          // +0x08  name CString
    CString m_c;          // +0x0c  second CString
    i32 m_10;             // +0x10
    char* m_ownedBufferB; // +0x14  owned buffer (freed second)
    char* m_ownedBufferA; // +0x18  owned buffer (freed first)
    i32 m_1c;             // +0x1c
    i32 m_listPosition;   // +0x20  cached AddTail position

    // The retail ctor AddSessionNode inlines: base stamp 0x5e8cb4, the two CString
    // members' default ctors, final stamp 0x5f0778, then zero the 4 scalar fields.
    CNetSessionNode() {
        m_sessionId = 0;
        m_listPosition = 0;
        m_ownedBufferA = 0;
        m_ownedBufferB = 0;
    }
    virtual ~CNetSessionNode() OVERRIDE; // 0x179420 (NetSessionNode.cpp)

    // The 4-arg init the session-node ctor runs (0x1796c0): store the dword id
    // (+0x4) and the second dword (+0x10), assign the two CStrings (+0x8/+0xc), and
    // zero +0x14/+0x18/+0x1c. Returns TRUE.
    i32 InitSession(i32 id, const char* nameA, const char* nameB, i32 d); // 0x1796c0

    // The session name fetched by value (NRV into the caller's slot); thiscall
    // engine routine reached through an incremental-link thunk (no body here).
    CString GetName();
};
SIZE(CNetSessionNode, 0x24); // AddSessionNode (NetMgr.cpp 0x178b30) RezAlloc(0x24)

// (The shared CObject grand-base dtor vptr @0x5e8cb4 that ~CNetMgr re-installs is no
// longer a manual stamp: CNetMgr derives from CObject, whose inline dtor cl folds
// into ~CNetMgr as the grand-base restamp - see Wap32/Object.h.)
// PTR_LAB_005f0588 - the QueryInterface riid pointer the Init wrapper (0x178170)
// hands to slot 0 (a static GUID blob; DIR32 reloc-masked). 0x5f0588.
extern "C" void* g_netDirectPlayRiid; // 0x5f0588

// ---------------------------------------------------------------------------
// The per-session enumeration callback EnumPlayersInto hands to the DirectPlay
// EnumSessions slot (a 4-arg EnumSessionsCallback2: lpThisSD, lpdwTimeout,
// dwFlags, lpContext). Skips the DPESC_TIMEDOUT (flag&1) sweep and, for each live
// session descriptor, forwards it to CNetMgr::AddPlayerNode. Its address is taken
// (=> DIR32 reloc-masked). 0x005786a0.
// ---------------------------------------------------------------------------
extern "C" BOOL __stdcall
NetEnumPlayerCb(void* lpThisSD, void* lpdwTimeout, DWORD dwFlags, CNetMgr* ctx);

// The group enumeration callback the EnumGroups wrappers (0x178a40/0x178a80) hand
// to the DirectPlay EnumGroups slot (a 5-arg EnumGroupsCallback2: dpId, dwType,
// lpName, dwFlags, lpContext). Forwards the id + the DPNAME short/long name to
// CNetMgr::AddSessionNode. Its address is taken (=> DIR32 reloc-masked). 0x00578b00.
extern "C" BOOL __stdcall
NetEnumCb(u32 dpId, DWORD dwType, NetDPName* lpName, DWORD dwFlags, CNetMgr* ctx);

// ---------------------------------------------------------------------------
// A COM interface CNetMgr keeps at +0x14 alongside the IDirectPlay4 at +0x18.
// Destroy releases both: the +0x14 interface via its vtable slot 2 (the
// IUnknown::Release form), the +0x18 via slot 4 then slot 2. Only those slots
// are pinned; everything else is opaque padding. __stdcall (COM convention).
// ---------------------------------------------------------------------------
struct INetReleasable {
    // External COM interface (IUnknown-shaped, abstract, __stdcall). STDMETHOD form
    // (== `virtual HRESULT __stdcall`); only Release (slot 2) and the slot-4 teardown
    // hook the Destroy path calls are pinned, everything else is placeholder.
    STDMETHOD(QueryInterface)(void* riid, void* out) PURE; // slot 0
    STDMETHOD(v01)() PURE;                                 // slot 1
    STDMETHOD(Release)() PURE;                             // slot 2  (+0x08)
    STDMETHOD(v03)() PURE;                                 // slot 3
    STDMETHOD(Slot10)() PURE;                              // slot 4  (+0x10)
};
SIZE_UNKNOWN(INetReleasable); // external COM interface (opaque object); size TBD

// ---------------------------------------------------------------------------
// The control message HandleControlMsg (0xba1a0) dispatches on: a packed record
// whose +0x0 is the message code (the switch tag), +0x4 a sub-code, +0x8 a
// payload word (a player id for the player-left case).
// ---------------------------------------------------------------------------
struct CNetCtrlMsg {
    i32 m_0; // +0x0  message code (switch tag)
    i32 m_4; // +0x4  sub-code
    i32 m_8; // +0x8  payload (player id on the player-left path)
};
SIZE_UNKNOWN(CNetCtrlMsg); // packed control-record view (3 dwords pinned); size TBD

// The menu-select control-message variant CMulti::LoadMenuSelectSprite (0xba620) reads.
// PROVEN the same record as CNetCtrlMsg: HandleControlMsg (0xba1a0) dispatches on
// msg->m_0 and one arm does `push eax(=msg); call LoadMenuSelectSprite` (0xba1ed), so
// the void* it takes IS this control message. Its +0x4 gate (== 1) is CNetCtrlMsg's
// m_4; +0x8 the target player/slot id; +0x20/+0x24 the two AddSessionNode name args.
struct MenuSelectEvent {
    char m_pad0[0x4];
    i32 m_armed;   // +0x4  armed gate (== 1; the CNetCtrlMsg m_4 sub-code)
    i32 m_id;      // +0x8  player/slot id
    char m_pad0c[0x20 - 0xc];
    char* m_nameA; // +0x20  AddSessionNode name arg A
    char* m_nameB; // +0x24  AddSessionNode name arg B
};
SIZE_UNKNOWN(MenuSelectEvent); // menu-select control-message view (touched offsets pinned)

// (CNetChatLog is GONE - 2026-07-13. It was an EMPTY placeholder view of the +0x5c
// chat/text display, and this very comment already said what it is: "a CFontConfig the
// message text is appended to". Same slot, same object: CNetGameMgr IS the *0x24556c
// game-manager singleton, and <Gruntz/GruntzMgr.h> types ITS +0x5c as
// `CFontConfig* m_chatLog`. Every use here was already casting through -
// `((CFontConfig*)NetGameMgr()->m_5c)->FreeNodes()`, `...->AddItem(text, 0x20, 0x11)`
// (0x21c60) - so the slot is now typed CFontConfig* and the casts fall out.)
class CFontConfig; // <Gruntz/FontConfig.h> (the deref TUs include the real header)

// The window object at CNetGameMgr+0x4: its own +0x4 holds the engine HWND. The
// message handlers post through m_4->m_4->m_4 (game-mgr -> window -> HWND); this
// folds the former generic CNetHwndHolder "+0x4 holder" placeholder.
struct CNetGameWnd {
    char m_pad0[4];
    HWND m_hwnd; // +0x4  the engine HWND
};
SIZE_UNKNOWN(CNetGameWnd); // window view (only +0x4 HWND pinned); retail size TBD

// ---------------------------------------------------------------------------
// CNetGameMgr - the ONE canonical view of CNetMgr's +0x4 game-manager sub-object
// (the CGruntzMgr the net layer drives; modeled minimally here to avoid a
// cross-module dependency on GruntzMgr.h). It consolidates the former per-TU
// placeholder views of the same object: CNetHwndHolder (+0x4 window/HWND chain),
// CNetSubObject / CNetMgrSub (+0x6c command manager), and OptionsHost
// (CountActiveChannels @0x492e30). The +0x4 sub-object is ALSO the base of the
// inline per-channel slot array at +0x150 (CNetGameMgr::m_channels, see CNetChannel).
// ---------------------------------------------------------------------------
// The +0x38 registry/config store (SetValueDword/SetValueString/GetValueDword);
// Utils::RegistryHelper (from <Utils/RegistryHelper.h>, included above) is the real
// class, exposed as a named typed member instead of a raw m_4+0x38 cast.
// CNetGameMgr IS *g_gameReg, the same object <Gruntz/GruntzMgr.h> models as CGruntzMgr
// (Net-side named view; the MFC-wall "required split" is dead - see NetMgrMenuSelect).
// These are that object's own CGruntzMgr methods, declared here (reloc-masked to the
// same RVAs) so the Net code calls them DIRECTLY instead of cross-casting m_4 to an
// unrelated CGruntzMgr* - the no-sane-dev cross-cast is gone. (FindPlayer IS
// CNetMgr::ResolveLocalPlayer; cast at that call. The full type-identity merge of the
// sub-object fields into CGruntzMgr is a separate reconciliation - see the report.)
struct CNetGameMgr {
    // (METHOD-FREE VIEW: every method once declared here was a phantom alias of a real
    // CGruntzMgr method, resolved by reading each call site's rel32 through its ILT:
    //   CountReadyOptionsSlots @0x92e30 -> CGruntzMgr::CountReadyOptionsSlots
    //   ResetClockGlobals      @0x8f4f0 -> CGruntzMgr::ResetClockGlobals
    //   ClearOptionsSlots      @0x92ec0 -> CGruntzMgr::ClearOptionsSlots
    //   InitializeLobbyConnectionSettings @0x8eca0 -> CGruntzMgr's (same name)
    //   GetWorldFileName       @0x928c0 -> CGruntzMgr::GetWorldFileName
    //   BuildRezPath           @0x93d40 -> CGruntzMgr::BuildLevelRezPath
    //   ShowModal              @0x8ef10 -> CGruntzMgr::EnterModalUI
    //   FindPlayer  (ILT 0x2e00) @0x92e80 -> CGruntzMgr::FindOptionsSlot(i32) - and the
    //     no-arg FindPlayer decl had DROPPED the i32 arg retail pushes at EVERY site.
    // Callers (Multi.cpp / MultiStartDlgRoster.cpp) now call the canonical methods
    // through CMulti::Mgr() / a CGruntzMgr cast of the singleton. Only the FIELD view
    // remains; folding these fields onto CGruntzMgr (m_channels == m_options, the
    // 5-name 0x238 record knot) is the record-merge TODO - see <Gruntz/Multi.h>.)
    char m_pad0[4];     // +0x00
    CNetGameWnd* m_wnd; // +0x04  the window (its +0x4 is the engine HWND)
    char m_pad8[0x38 - 8];
    Utils::RegistryHelper* m_configStore; // +0x38  registry/config store (Service/Player_Name/...)
    char m_pad3c[0x48 - 0x3c];
    // +0x48: the sound/bank object - the SAME slot <Gruntz/GruntzMgr.h> types
    // CGruntzSoundZ* m_sound (CNetGameMgr IS *g_gameReg). CMulti's join wait plays the
    // AMBIENT%d cue through it; it was reached via a `WaitLogic` +0x48 view.
    class CGruntzSoundZ* m_sound; // +0x48
    char m_pad4c[0x5c - 0x4c];
    CFontConfig* m_5c; // +0x5c  the chat/text display (the real class; ex-CNetChatLog view)
    char m_pad60[0x6c - 0x60];
    CGruntzCmdMgr* m_6c; // +0x6c  the grunt command manager (Dispatch/EnqueueCommand)
    char m_pad70[0xac - 0x70];
    i32 m_ac; // +0xac  connect-in-progress guard (the CMulti slot-1 driver toggles 1/0 per phase)
    char m_padb0[0x110 - 0xb0];
    i32 m_110; // +0x110  session-armed gate (Setup saves the old value into CMulti::m_590)
    i32 m_114; // +0x114
    char m_pad118[0x12c - 0x118];
    i32 m_12c; // +0x12c  custom-vs-stock level flag (Setup sets 0 custom / 1 stock)
    char m_pad130[0x150 - 0x130];
    CNetChannel m_channels[4]; // +0x150  the inline per-channel slot array (stride 0x238)
};
SIZE_UNKNOWN(CNetGameMgr); // game-mgr view (+0x4/+0x5c/+0x6c/+0x150 pinned); retail size TBD

// The DirectPlay player-descriptor node the +0x38 player CObList holds (the
// payload PopulatePlayerList lists into the Win32 list box). Only its +0x34
// name-string pointer is touched (LB_ADDSTRING). This is a DISTINCT object from
// the GruntzPlayer slot CNetGameMgr::FindPlayer returns (whose name is the +0x4
// CString / GetName @0x1f450) - the chat broadcaster uses GruntzPlayer directly.
struct CNetPlayerDesc {
    char m_pad0[0x34];
    char* m_profile; // +0x34  keyed player-name/profile text (LB_ADDSTRING source;
                     //         NetFormatKeyed reads the NAME key out of it)
};
SIZE_UNKNOWN(CNetPlayerDesc); // descriptor-node view (only +0x34 name pinned); size TBD

// FUN_004db2b0 (__cdecl): g_netSlotTable[idx] = value (a global flag array at
// 0x64c3f0). External, no body -> the call reloc-masks.

// FUN_004db1d0 (__cdecl): zero the whole 0x11-dword net-slot flag table at
// 0x64c3f0 (ParseChannelTable resets it when not in channel-latency mode).
// External, no body -> the call reloc-masks.
void ResetNetSlots(); // 0x004db1d0

// The shared 0x800-byte DirectPlay receive scratch buffer (DAT_006467d8); the
// poll loop receives each message into it. DIR32 reloc-masked.
extern "C" char g_recvBuffer[]; // 0x6467d8

// Two file-scope static stat packets the channel-stat senders stamp + ship.
// Retail emits each field as its OWN .data symbol (disp-0 DIR32 relocs at the
// three consecutive addresses), so they are modeled as three separate globals
// per packet (a single struct would emit base+disp and mis-encode the
// displacement bytes). External; DIR32 reloc-masked.
// DEFINED in src/Gruntz/Multi.cpp (owner TU); reference externs only here.
extern u8 g_chanStat422_flag; // 0x646fd8
extern i32 g_chanStat422_id;  // 0x646fdc
extern i32 g_chanStat422_val; // 0x646fe0
extern u8 g_chanStat423_flag; // 0x646378
extern i32 g_chanStat423_id;  // 0x64637c
extern i32 g_chanStat423_val; // 0x646380

// The file-scope static chat-broadcast packet BroadcastChatLine assembles. Like
// the channel-stat packets, retail emits each field as its own .data symbol
// (disp-0 DIR32 relocs): a flag byte (0x6473e0), the stat id 0x3f0 (0x6473e4), a
// value dword (0x6473e8), then the text buffer (0x6473ec). External; reloc-masked.
extern "C" u8 g_chatPacket_flag;  // 0x6473e0
extern "C" i32 g_chatPacket_id;   // 0x6473e4
extern "C" i32 g_chatPacket_val;  // 0x6473e8
extern "C" char g_chatPacket_buf; // 0x6473ec  (strcpy dest)

// The shared "a player-left was processed this frame" flag (DAT_00648ce4) and
// the active-player refcount the leave path decrements (DAT_00648cec). External
// engine globals; DIR32 reloc-masked.
extern "C" i32 g_playerLeftFlag; // 0x648ce4
// CANONICAL name (shared with CMulti.cpp / Globals.cpp): one symbol per RVA.
extern "C" i32 g_activePlayerCount; // 0x648cec  active-player refcount

// The multiplayer-create context singleton (DAT_00648cf4): CreateSession reads
// its +0x74 group-enumeration record and hands it to the peer's EnumGroupsRange.
// External engine global pointer; DIR32 reloc-masked.
struct InterfaceObject; // the DirectPlay service-provider node (IsInterface2 probe)
struct CNetCreateCtx {
    char m_pad0[0x70];
    InterfaceObject*
        m_serviceProvider; // +0x70  selected service-provider (IsInterface2 -> slow-link timeout)
    u8* m_74;              // +0x74  the group-enumeration record blob
};
SIZE_UNKNOWN(CNetCreateCtx); // create-context view (only +0x74 pinned); retail size TBD
// (g_netCreateCtx moved to its only user, Multi.cpp, so it can carry the DATA() binding a
//  header cannot - declared here it was bound to no retail address at all.)

// One node of the +0x1c service-provider group CObList (MFC CNode shape: next@+0,
// prev@+4, payload@+8). Find() walks this chain, caching the running POSITION at
// CNetMgr+0x7c. Sibling to CNetListNode/CNetCmdNode/CNetPlayerNode.
struct CGroupNode {
    CGroupNode* m_next;      // +0x00  CObList CNode pNext
    CGroupNode* m_prev;      // +0x04  CObList CNode pPrev (not walked here)
    InterfaceObject* m_data; // +0x08  payload service-provider node
};
SIZE_UNKNOWN(CGroupNode); // traversal view of the +0x1c group list node

// CNetMgr derives from the shared CObject grand-base (Wap32/Object.h): its own
// vtable (??_7CNetMgr@@6B@, 0x1ea42c) overrides only slot 1 (the dtor); slots 0/2/3/4
// come from inheritance. cl auto-emits the own vptr stamp at ~CNetMgr entry AND folds
// the CObject grand-base restamp (masks 0x5e8cb4) at the dtor tail - no manual
// stamp.
class CNetMgr : public CObject {
public:
    virtual ~CNetMgr() OVERRIDE; // slot 1  (dtor; ??1 @0xb6000, ??_G @0x260d thunk)

    void OnMultiOptions();
    void OnMultiPause();
    void OnOutOfSync();
    void ApplyCmdDelayDefaults();
    u32 GetMaxAckLatency();
    void ReportAckLatency();
    CNetPlayerEntry* FindPlayerById(i32 id);

    // The menu-select event handler (0xba620, defined in NetMgrMenuSelect.cpp). Its
    // event arg is that TU's local MenuSelectEvent view -> typed void* here so the
    // shared class needs no menu-only type. It reaches the +0x524 sub-object (the
    // REAL CNetMgr, see the header verdict - its GetPlayerData/AddSessionNode are
    // genuine CNetMgr methods) through a cast of m_peer to that TU's PlayerMgr view.
    i32 LoadMenuSelectSprite(void* ev); // 0xba620

    // Find (0x179270): walk the +0x1c group CObList (m_pHead @+0x20, running
    // POSITION cached @+0x7c) and return the first InterfaceObject payload whose
    // GUID matches the service-provider class selected by `kind` (1/2/5).
    struct InterfaceObject* Find(i32 kind);

    // The DirectPlay session-management wrapper run (engine CNetMgr base;
    // ~0x178xxx). Each thin wrapper calls one IDirectPlay4 vtable slot on the
    // m_18 interface and, on a nonzero HRESULT, routes it through the static
    // ReportError diagnostic with this TU's __FILE__/__LINE__.
    i32 RemovePlayerObj(CNetPlayerObj* obj); // 0x178e20
    i32 RemovePlayerById(i32 id);            // 0x178e60  GetPlayerData(id) -> RemovePlayerObj
    i32 RemovePlayerNode(CNetPlayerListNode* node); // 0x1790e0  drop one +0x38 player node
    i32 EnumSessions2(void* ctx); // 0x179240  enum into a 0x28 desc, return desc+0x18
    void* GetPlayerData(i32 id);  // 0x178eb0
    i32 SetGroupData2(CNetPlayerEntry* a, CNetPlayerEntry* b, i32 c, i32 d, i32 e); // 0x178ef0
    i32 SendEx(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f, i32 g, i32 h, i32 i);      // 0x178f50
    i32 SetData(i32 a, i32 b, i32 c, i32 d, i32 e);                                 // 0x178fc0
    i32 Receive(
        CNetPlayerEntry* from,
        CNetPlayerEntry* to,
        i32 flags,
        void* lpData,
        i32* lpSize
    );                                                             // 0x179010
    i32 SetGroupDataFrom(CNetPlayerEntry* a, i32 c, i32 d, i32 e); // 0x179090
    i32 GetGroupInfo(CNetPlayerEntry* a, void* desc, i32 flags);   // 0x179190
    i32 EnumSessions(void* desc, void* ctx);                       // 0x179130

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

    // The 0x178xxx session-management wrapper run (continued). Each thin wrapper
    // fires one IDirectPlay4 slot on the m_18/+0x18 interface and, on a nonzero
    // HRESULT, routes it through ReportError; the node factories operator-new a
    // list node and AddTail it onto one of the managed CObLists.
    i32
    Init(void* a, i32 c, i32 d, i32 e, i32 f); // 0x178170  Open + QueryInterface + reset selections
    i32 AddPlayerNode(void* playerDesc);       // 0x1786d0  new player node -> +0x38 list
    void PopulatePlayerList(void* hList);      // 0x178790  fill a Win32 player list box
    i32 EnumPlayersCb(
        void* a,
        i32 b,
        i32 c,
        i32 d
    );                                         // 0x1789e0  EnumPlayers slot wrapper -> CreatePlayer
    i32 EnumGroupsAll();                       // 0x178a40  EnumGroups (slot 0xc) wrapper
    i32 EnumGroupsRange(void* rec, i32 flags); // 0x178a80  EnumGroups (slot 0xc) over a record
    i32 AddSessionNode(
        i32 id,
        const char* nameA,
        const char* nameB,
        i32 d
    ); // 0x178b30  (/GX) new session node -> InitSession + GetData5 -> +0x54 list
    i32 CreatePlayer(void* a, i32 b, i32 c); // 0x178cb0  GetSessionDesc + AddSessionNode
    void PopulateSessionList(void* hList);   // 0x178d40  (/GX) fill a Win32 session list box

    // The 0xbbxxx / 0xbcxxx connect/config helpers reconstructed in this TU.
    i32 DropChannelPlayer(i32 idx); // 0xbb510  drop the player on channel[idx]
    i32 LoadConfig(void* cfg);      // 0xbce80  copy the command-timing config in
    void AutoTuneCmdDelay();        // 0xbcc10  derive m_cmdDelay/m_resend from the ping

    // AutoTuneCmdDelay's external probes (incremental-link thunks; no body here so
    // the call rel32 reloc-masks). MeasurePing samples the round-trip; ProbeLatency
    // returns a secondary latency class; WriteCmdDelay persists the tuned pair.
    i32 MeasurePing();            // 0x... (thunked) round-trip sample
    i32 ProbeLatency(i32 flag);   // 0x... secondary latency probe
    void WriteCmdDelay(i32 flag); // 0x... persist m_cmdDelay/m_resend

    // The provider/group cluster (0x178xxx). InitFromProvider DirectPlayCreate's a
    // fresh DP object for a selected service-provider GUID + queries IDirectPlay4;
    // EnumServiceProviders refills the +0x1c group list via DirectPlayEnumerate;
    // AddGroupNode operator-new's a 0x10 InterfaceObject node onto the +0x1c list;
    // EnumGroupsInto probes a group's players + adds a player node (0x1786d0).
    // a = the provider/session descriptor (its +0x4 is the DirectPlay GUID pointer);
    // appGuid = the application GUID passed BY VALUE (its 4 dwords are recorded into the
    // m_4 setup block). Proven by the NetSessionOpen.cpp caller (0xb77dd: sub esp,0x10 +
    // 4 GUID stores + push descriptor) - the by-value struct IS the 4 trailing dwords.
    i32 InitFromProvider(void* a, GUID appGuid);        // 0x1780b0
    i32 EnumServiceProviders(i32 validated);            // 0x178280
    i32 AddGroupNode(void* guid, void* name);           // 0x178360
    i32 EnumGroupsInto(void* a, void* b, i32 c, i32 d); // 0x1788a0

    // The diagnostic error reporter (lives in the netmgrerror TU; static
    // __cdecl). Declared here so the wrappers can route HRESULTs through it.
    static void ReportError(char* file, i32 line, i32 hr, void* hWnd);
    // Set the four reporting-mode globals ReportError consults. (0x177670, static
    // __cdecl; lives in the netmgrerror TU next to ReportError.)
    static void SetReportMode(i32 log, i32 msgBox, i32 beep, i32 third);

    // Version-check cluster (engine CNetMgr base; ~0xbd0xx). HandleVersionCheck
    // compares the host packet's version pair against the two locals and, on a
    // mismatch, reports + announces; AnnounceVersion ships the version packet as
    // stat 0x417 through the engine dispatcher.
    void HandleVersionCheck(CNetVersionMsg* msg);
    void AnnounceVersion(i32 param);

    // CreateSession (0xbbc90, /GX EH): enumerate the host group, resolve the
    // local player, allocate + construct the DirectPlay command-session
    // (new CNetSession -> 4-slot vector-ctor + ResetAll), wire it (Init), derive
    // the resync tick (m_resyncTick), and seed one command slot per active channel.
    i32 CreateSession(); // 0xbbc90

    // VerifyCustomLevel (0xb8fc0, /GX EH): build the level-name rez path from the
    // config name CStrings, run it past the active session (Poll), and pop the
    // appropriate g_gameReg error modal on failure / level mismatch.
    i32 VerifyCustomLevel(i32 a1, i32 a2); // 0xb8fc0

    // Poll the active session for the verify response (0xbba10, reloc-masked).
    i32 Poll(i32 token); // 0xbba10

    // The join-failure re-arm helper Poll fires on the resend deadline (0-arg
    // __thiscall; external incremental-link thunk -> no body here so the call
    // reloc-masks). Retail thunk 0x35e4 -> 0xbc420.
    void AckJoinFailure(); // 0xbc420
    void DropTimeout();    // 0xbc2d0 (drop a timed-out player)
    i32 GetAmbientId();    // 0xda200 (current ambient-track index for the "AMBIENT%d" cue)

    // (WaitForOtherPlayers @0xbb700 moved to CMulti in the netmgr-vs-cmulti split.)

    // CreateLocalPlayer (0xbc750, /GX EH): register the local player with the peer
    // under the local name, latch its id (m_localPlayerId), wait for the host to admit it,
    // then announce the join (stat 0x3f9 packet carrying the name). Returns 1.
    i32 CreateLocalPlayer();       // 0xbc750
    CString GetString5a0();        // 0xb7ad0  the local player-name CString
    void ReportConnectFailed(i32); // 0xb7f60  connect-failed diagnostic (1-arg)

    // SaveConfig (0xbccd0, /GX EH): pack the command-timing config (m_5b0, the two
    // config-name strings, m_cmdDelay/m_resend/m_600/m_2d8) into a 0x11c-byte stat
    // 0x416 blob and ship it - to one recipient when given, else broadcast.
    i32 SaveConfig(CNetPlayerEntry* recipient); // 0xbccd0

    // The stat-send family (matched in NetMgr.cpp). All ship a 0x10-byte
    // CNetStatPacket (or a caller packet) to the local player's peer group
    // through the DirectPlay set-data wrappers; SendStatFlag/SendNetStat build
    // the header then funnel through SendStatBuf.
    //   SendStatBuf   (b91f0) the core: SetGroupDataFrom(localPlayer, flag, pkt, 0x10)
    //   SendStatFlag  (b9240) build {id, localPlayer.id} then SendStatBuf
    //   SendNetStat   (b9290) build {id, value} then SendStatBuf
    //   SendStatFrom  (b92e0) SetGroupDataFrom wrapper, null-guarded packet
    //   SendStatPair  (b9330) SetGroupData2 wrapper, null-guarded packet
    i32 SendStatBuf(CNetStatPacket* pkt, i32 flag);                           // 0xb91f0
    void SendStatFlag(i32 id, i32 flag);                                      // 0xb9240
    void SendNetStat(i32 id, u32 value, i32 flag);                            // 0xb9290
    i32 SendStatFrom(CNetStatPacket* pkt, i32 b, i32 c);                      // 0xb92e0
    i32 SendStatPair(CNetPlayerEntry* recipient, CNetStatPacket* pkt, i32 c); // 0xb9330
    // Three more stat-send variants in this cluster: each builds a 0x10-byte stat
    // header on the stack (or forwards a caller packet) and ships it through one
    // of the DirectPlay set-data wrappers.
    i32 SendStatTo(CNetPlayerEntry* recipient, i32 id, i32 c); // 0xb93a0
    // SendStatPair sibling: builds {m_4=id, m_8=value} to a specific recipient (the
    // explicit-value form of SendStatTo, which uses localPlayer.id for m_8). // 0xb9490
    i32 SendNetStatTo(CNetPlayerEntry* recipient, i32 id, u32 value, i32 c);
    i32 SendStatPairRaw(CNetPlayerEntry* recipient, void* pkt, i32 size, i32 c); // 0xb9500
    i32 SendStatValue(i32 id, i32 statId, i32 value, i32 flag);                  // 0xb9570
    // Session-ready gate (0xb9180): with both args set, polls the session once if the
    // done-latch (m_534) is clear, then reports whether it is now set.
    i32 PollSessionGated(i32 a1, i32 a2); // 0xb9180
    // (GetConfigNameA @0xb6090 / GetConfigNameB @0xb60d0 moved to CMulti in the
    // netmgr-vs-cmulti split - they return CMulti's m_5b4/m_5b8 config-name CStrings.)
    // The control-message dispatch + the player-left handler.
    RVA(0x000ba170, 0x20)
    CString GetName() {
        return m_8;
    }
    i32 HandleControlMsg(CNetCtrlMsg* msg, i32 arg2); // 0xba1a0  switch on msg->m_0 (arg2 unused)
    i32 OnPlayerLeft(i32 playerId); // 0xba3b0  (/GX) report + tear down a leaving player
    // The sprite/menu-message handler (case 3 of HandleControlMsg); its body lives
    // in a sibling stub TU (ApiCallers.cpp), declared here no-body so the call
    // reloc-masks.
    void HandleSpriteMsg(CNetCtrlMsg* msg); // 0xba620
    // The "rejoin/reconnect" finalizer fired from OnPlayerLeft when the channel
    // selector is set; external (the 0xba810 backlog method), no body here.
    void RejoinIfNeeded(i32 flag); // 0xba810
    // The version-report diagnostic (logs a message string + zero) and the
    // 4-arg stat-packet dispatcher AnnounceVersion fires. Both __thiscall engine
    // routines reached through incremental-link thunks; no body here.
    void ReportVersionMsg(const char* msg, i32 zero);
    void SendStatPacket(i32 param, const void* packet, i32 size, i32 flag);

    // ---- 0xbaxxx channel-table cluster -------------------------------------
    // The per-channel ack/state table at (m_4 + 0x150) is serialized to / parsed
    // from a 0x88-byte (whole-table) or 0x2c-byte (single) stat packet, and a
    // pair of register/remove helpers create / tear down one channel slot.
    // AckDropPlayer (0xba590) is declared with the bc0xx helper run below.
    i32 ResolveLocalPlayer(); // 0xba7d0  m_localPlayer = peer->FindPlayerById(m_localPlayerId)
    i32 BroadcastChannelTable(
        CNetPlayerEntry* recipient
    );                                   // 0xba810  serialize all channels -> 0x88 packet
    i32 ParseChannelTable(void* packet); // 0xba980  parse a 0x88 packet -> channels
    i32 RegisterChannelFrom(
        const char* name,
        i32 b,
        i32 e,
        i32 f
    ); // 0xbaa90 tail-wrap into RegisterChannel
    i32 RegisterChannel(const char* name, i32 id, i32 c, i32 d, i32 idx, i32 e); // 0xbaac0 (/GX)
    i32 RegisterChannelRec(void* rec);        // 0xbac40  unpack rec -> RegisterChannel
    i32 RemoveChannel(i32 idx);               // 0xbac90  free channel[idx]
    i32 OnPauseChannel();                     // 0xbad00  m_580 ? SendStatFlag+OnMultiPause
    i32 BroadcastOneChannel(CNetChannel* ch); // 0xbaf00  serialize one channel -> 0x2c packet
    i32 ParseOneChannel(void* rec);           // 0xbaff0  parse one record -> channel[rec.idx]
    i32 SendChannelStat422();                 // 0xbb0b0  build {0x422} -> SetGroupDataFrom
    i32 SendChannelStat423();                 // 0xbb120  build {0x423} -> SetGroupDataFrom
    i32 BroadcastChatLine(char* text, i32 toChat, i32 showWnd, void* hWnd); // 0xbb190

    // AckDropPlayer's record helper is RecordDropPlayer2 (0xbb5e0, declared above);
    // the former duplicate declared-only RecordDropPlayer alias was folded onto it.

    // The chat-window dispatcher BroadcastChatLine fires when the show-window flag
    // is set: posts the assembled line to a Win32 chat control (SendMessageA-based
    // helper). __thiscall; external (sibling 0xbb3e0), no body here.
    void ShowChatLine(void* hWnd, const char* text); // 0xbb3e0

    // ---- 0xbc0xx cluster ---------------------------------------------------
    // The cluster's matched methods (defined in NetMgr.cpp).
    u32 FrameSyncWait();             // 0xbc070
    void OnDropPlayer();             // 0xbc110
    i32 WaitForConnect();            // 0xbca50
    i32 ResetPlayerCommands(i32 id); // 0xbcf20
    void WriteTag(const char* tag);  // 0xbd4a0  reloc-masked (no-op stub)

    // External engine helpers the cluster fires through incremental-link thunks
    // (__thiscall on `this` unless noted; bodies external/no-body so the
    // `call rel32` reloc-masks).
    //   SendStat3      (b9410) the 3-arg stat sender (id, value, flag)
    //   ReportNetError (b7e30) status-bar diagnostic (string, level)
    //   ReportStatusId (b7ec0) status-bar diagnostic by string-resource id
    //   AckDropPlayer  (ba590) finalize a dropped player (id)
    //   ResetCmdBuffers(c0070) zero the four per-slot command buffers, no args
    void ReportNetError(const char* msg, i32 level);
    void ReportStatusId(UINT strId, i32 level);
    void AckDropPlayer(i32 id);
    // SendStat3 (b9410) + PollSession (b95f0) are matched in this TU. PollSession
    // hands each received message to the engine dispatcher (Stub_0b9750), reached
    // here through an incremental-link thunk; modeled no-body (reloc-masked).
    i32 SendStat3(i32 id, u32 value, i32 flag);           // 0xb9410
    i32 PollSession();                                    // 0xb95f0
    i32 DispatchRecvMsg(i32 sender, char* buf, i32 size); // 0xb9750 (ret used by ProcessCmd)

    // === CNetMgr layout: the REAL 0x8c-byte DirectPlay wrapper ================
    // netmgr-vs-cmulti split DONE: the former +0x2d8..+0x60c multiplayer block and
    // the 0xb5xxx-0xbdxxx method cluster belong to CMulti (<Gruntz/Multi.h>); this
    // class is only the DirectPlay session wrapper CMulti holds at CMulti+0x524
    // (CMulti::m_netGate, reached via Peer()). Constructed inline at
    // CMulti::LoadGameAssetNamespaces (slot 1) @0xb560e: global `operator new(0x8c)`, the
    // CObject base vptr stamp (0x5e8cb4), the three CObList ctors (nBlockSize 10),
    // the derived vptr stamp (0x5ea42c), then zero +0x14/+0x18.
    // (vptr implicit at +0x000)
    CNetGameMgr* m_4;      // +0x004  game-manager sub-object (window/HWND, +0x6c cmd mgr)
    CString m_8;           // +0x008  a name CString (GetName returns a copy by value)
    CDDrawSurfaceMgr* m_c; // +0x00c  the world holder (CState::m_c mirror)
    char m_pad10[0x14 - 0x10];
    INetReleasable* m_releaseIface; // +0x014  secondary COM interface Destroy releases (slot 2)
    IDirectPlay4Z* m_directPlay; // +0x018  the DirectPlay session interface (IDirectPlay4-shaped)
    // The three managed collections (by-value MFC CObList, 0x1c bytes each; head at
    // +0x4, count at +0xc within each). ~CNetMgr @0xb6000 tears them down in reverse
    // (+0x54,+0x38,+0x1c) after Destroy(); the ctor default-constructs each. The
    // Clear/Add/Populate methods reach them by name now; the named head/count reads
    // use the assert-free MFC GetHeadPosition()/GetCount() inlines (byte-identical to
    // the old m_3c/+0x20/+0x58 head and m_groupCount/m_playerCount +0x28/+0x44 reads).
    CObList m_groups;   // +0x01c  service-provider group list  (head +0x20, count +0x28)
    CObList m_players;  // +0x038  player-descriptor list        (head +0x3c, count +0x44)
    CObList m_sessions; // +0x054  session / player-object list  (head +0x58, count +0x60)
    // The three list-box selection latches + their walk-cursor ids. Each ReadXxxSel
    // reader writes the selected item's data here in range; the clear-loops zero them.
    i32 m_groupSel;              // +0x070  group-list selected item data (ReadGroupSel)
    i32 m_playerSel;             // +0x074  player-list selected item data (ReadPlayerSel)
    i32 m_sessionSel;            // +0x078  session-list selected item data
    CGroupNode* m_groupSelId;    // +0x07c  group-list walk cursor (Find/PopulateGroupList)
    CNetListNode* m_playerSelId; // +0x080  player-list walk cursor / selection id
    CNetListNode* m_sessionSelId; // +0x084  session-list walk cursor / selection id
    i32 m_88; // +0x088  (rounds the object to the observed RezAlloc/operator-new 0x8c size)

    // Inline ctor: the CObject base + three CObList members are auto-constructed by
    // cl (base+member+derived vptr stamps), then this body zeroes +0x14/+0x18 -
    // reproducing the peer construction inlined at the CMulti slot-1 driver @0xb560e.
    CNetMgr() {
        m_releaseIface = 0;
        m_directPlay = 0;
    }

    // The managed-list teardown run of the destructor (~CNetMgr, 0x0b6000) is
    // declared as `virtual ~CNetMgr()` in the vtable block above.

    // SetupServices (0xb78b0, /GX): enumerate the peer's service providers and, on
    // success, dispatch MULTI_HOST/JOINSERVICES and write the selected service /
    // player-name / game-name into the engine config store; returns the selected
    // provider. DispatchServices/GetGameName are its external helpers.
    i32 SetupServices();                                       // 0xb78b0
    i32 DispatchServices(const char* cmd, i32 flag, void* cb); // 0xbc250
    CString GetGameName();                                     // 0xb7a90

    // JoinAndRegisterChannel (0xb8b10, /GX): build the command-timing config string,
    // enumerate the host group into it, create the local player, and register the
    // local channel; returns the enum result iff the channel registered.
    i32 JoinAndRegisterChannel(); // 0xb8b10

    // The connection-config family (all /GX). DetectConnectionConfig resolves the
    // connection class from the selected provider + loads its "<section>_CmdDelay/
    // _Resend" timing then joins; SetupTcpIpConfig is the TcpIp-specific variant that
    // loads config + creates the local player + registers the channel inline;
    // OnJoinConfirm reads the join dialog, resolves the player + config, and ships
    // the "player joined" packet.
    i32 DetectConnectionConfig();  // 0xb82e0
    i32 SetupTcpIpConfig();        // 0xbc460
    i32 OnJoinConfirm(void* hDlg); // 0xb8cf0
    // The "dyn command-delay" config setter OnJoinConfirm applies (0xb76c0,
    // __thiscall, CString by value); external/no-body so the call reloc-masks.
    void ApplyDynSetting(CString s); // 0xb76c0

    // The setup dialog's helpers: PopulateGroupList (0x178470, defined in NetMgr.cpp)
    // fills the service-provider combo from the +0x1c group list, filtering by the
    // caller's flag (bit1 -> drop IsInterface2, bit2 -> drop IsInterface1); SetServiceName
    // records the entered service name (0xb7730, CString by value, reloc-masked no-body).
    void PopulateGroupList(HWND hList, i32 flag); // 0x178470
    void SetServiceName(CString s);               // 0xb7730

    // (The multiplayer connect/init driver @0xb5460, ex "SetupMultiplayerSession",
    // is NOT a CNetMgr method: it is CMulti's slot-1 LoadGameAssetNamespaces
    // override (<Gruntz/Multi.h>; retail ??_7CMulti slot 1 = ILT 0x3fb2 -> 0xb5460),
    // run on this=g_curMulti. The orphan alias decl that sat here is gone.)
};
SIZE(CNetMgr, 0x8c);       // the real DirectPlay wrapper (RezAlloc/operator new 0x8c @0xb560e)
VTBL(CNetMgr, 0x001ea42c); // ??_7CNetMgr@@6B@ (config/vtable_names.csv); cl-emitted

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // NET_NETMGR_H
