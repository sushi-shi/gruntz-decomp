#ifndef NET_NETMGR_H
#define NET_NETMGR_H

#include <Ints.h>
#include <rva.h>          // SIZE_UNKNOWN/VTBL class-metadata macros used below
#include <Wap32/Object.h> // CObject - the shared CObject-like grand-base

#include <Mfc.h>
#include <wtypes.h>   // HRESULT (Mfc.h's lean windows.h does NOT expose it - the reason
#include <basetyps.h> // STDMETHOD / STDMETHOD_ / PURE - the real COM interface macros for the
#include <Utils/RegistryHelper.h>
#include <Gruntz/ObList.h>
#include <Rez/RezMgr.h> // RezAlloc - the engine heap allocator the node factories use

void ActiveWait(u32 milliseconds); // 0x13dfe0

#include <Gruntz/String.h>

CString __stdcall operator+(const CString& lhs, const char* rhs);

class GruntzPlayer;     // <Gruntz/GruntzPlayer.h>  - the leaving-player slot
class CGruntzCmdMgr;    // <Gruntz/GruntzCmdMgr.h>  - the m_4 game-mgr's +0x6c command manager
class CNetMgr;          // defined below; the command slot caches one as its +0x1c owner
class CMulti;           // <Gruntz/Multi.h> - the multiplayer game-state (owns CNetSession::Init a2)
struct GruntRec;        // the lobby-sync grunt-state record (defined below CNetCmdSlot)
class CDDrawSurfaceMgr; // <Gruntz/GameRegistry.h> - the +0xc world holder (CState::m_c mirror)

extern "C" void MultiOptionzCallback();
extern "C" void MultiPauseCallback();
extern "C" void MultiOutOfSyncCallback();
extern "C" void MultiDropPlayerCallback(); // OnDropPlayer (MULTI_DROPPLAYER)

extern i32 g_dropPlayerId; // 0x611d88  saved dropped-player id

extern "C" i32 g_localVersion;  // 0x60fa70  (sibling; not TU-private)
extern i32 g_remoteVersion;     // 0x60fa74  DEFINED in src/Gruntz/Multi.cpp (owner TU)
extern "C" i32 g_cfgWord;       // 0x645550
extern "C" i32 g_buteMgrField4; // *(g_buteMgr + 4) - the CButeMgr config word

struct CNetVersionMsg {
    char m_pad0[0x18];
    i32 m_remoteVersion; // +0x18  host remote-version word
    i32 m_localVersion; // +0x1c  host local-version word
};
SIZE_UNKNOWN(CNetVersionMsg); // host-version msg view (only +0x18/+0x1c pinned); size TBD

struct CNetVersionPacket {
    u8 m_0; // +0x00  flag byte (bit7 set)
    char m_pad1[7];
    i32 m_buteConfig;  // +0x08  CButeMgr config word
    i32 m_cfgWord;  // +0x0c  g_cfgWord
    i32 m_statId; // +0x10  stat id (0x417)
    char m_pad14[4];
    i32 m_remoteVersion; // +0x18  g_remoteVersion
    i32 m_localVersion; // +0x1c  g_localVersion
};
SIZE(CNetVersionPacket, 0x20); // fully-known stack packet

struct CNetPlayerSlot {
    char m_pad0[0x164];
    DWORD m_164; // +0x164  slot-active gate A
    char m_pad168[0x170 - 0x168];
    DWORD m_170; // +0x170  slot-active gate B
    char m_pad174[0x37c - 0x174];
    DWORD m_37c; // +0x37c  the slot's latency value
};
SIZE_UNKNOWN(CNetPlayerSlot); // m_4-relative slot view (3 gate/latency dwords pinned)

#include <Gruntz/GruntzPlayer.h> // the ONE 0x238 per-player/channel record

struct CNetPlayerEntry {
    char m_pad0[4];
    i32 m_id;     // +0x4  the entry's id (the lookup key)
    CString m_name; // +0x8  display name (returned by GetName; COMDAT-shares CNetMgr::GetName's code)

    // The entry's display name (a CString at +0x8, returned by value / NRV). In the
    // retail this fetch shares CNetMgr::GetName's routine (0xba170) - same +0x8 read.
    CString GetName();
};
SIZE_UNKNOWN(CNetPlayerEntry); // payload-entry view (only +0x4 id pinned); size TBD

struct CNetStatPacket {
    u8 m_0; // +0x0  flag byte (bit7 set)
    char m_pad1[3];
    i32 m_statId;        // +0x4  stat id
    i32 m_value;        // +0x8  value / player id
    char m_padc[4]; // +0xc  (0x10 total)
};
SIZE_UNKNOWN(CNetStatPacket); // 0x10-byte stat-packet header view; full record size TBD

struct CNetPlayerNode {
    CNetPlayerNode* m_next; // +0x0
    char m_pad4[4];
    CNetPlayerEntry* m_8; // +0x8  the payload entry
};
SIZE_UNKNOWN(CNetPlayerNode); // player-list node walk-view; retail size TBD

struct CNetCmd {
    i32 m_seq; // +0x0  command sequence number
    i32 m_4;   // +0x4  payload word (resync compare)
};
SIZE_UNKNOWN(CNetCmd); // queued-command view (2 fields pinned); retail size TBD

struct SlotInfo {
    char pad00[4];
    i32 m_playerId; // +0x04 DirectPlay player id (SetData `a`, Recv/Read channel)
    char pad08[0x18 - 0x08];
    i32 m_netId; // +0x18 peer/target id (SetData `b`)
    char pad1c[0x2c - 0x1c];
    i32 m_dirty; // +0x2c set on slot re-init
};
SIZE_UNKNOWN(SlotInfo);

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
    CString BuildHostName(); // bc3f0  the slot's host name (by-value NRVO, fwds m_desc->GetName) [multi]
    i32
    Init(i32 a1, i32* a2, i32 a3); // c0b10  seed a fresh slot, then ClearCmds + reset both ranges
    i32 ProcessCmd(i32 playerId, void* rec, i32 size); // c0c70  parse/dispatch a command record
    // Slot-readiness check 0xc1320 (CNetSession::Verify dispatches it on each slot cursor).
    // `this` is the slot: reads m_owner (+0x1c) -> m_session -> m_slots[], gated by this
    // slot's m_ackFlags (+0x3c). (Was a stray CNetSyncCheck view; folded back to the slot.)
    i32 Ready(); // c1320
};
SIZE(CNetCmdSlot, 0x64); // fully-laid-out inline command slot (array stride 0x64)

struct CNetCmdNode {
    CNetCmdNode* m_next; // +0x0
    CNetCmdNode* m_prev; // +0x4
    CNetCmd* m_data;     // +0x8  (this queue holds CNetCmd)
};
SIZE_UNKNOWN(CNetCmdNode); // CObList node walk-view; retail size TBD

struct CNetCmdHdr {
    i32 m_sequence;   // +0x0  sequence
    i32 m_windowBase; // +0x4  window base
    i32 m_flags;      // +0x8  flags word
    u8 m_entryCount;  // +0xc  entry count
};
SIZE_UNKNOWN(CNetCmdHdr); // record header prefix (payload follows); full record size TBD

struct CNetCmdPacket {
    i32 m_sequence;       // +0x0  sequence
    CNetCmdSlot* m_owner; // +0x4  owning slot (this)
    u8 m_flags;           // +0x8  flag byte
    char m_pad9[0xc - 9];
    i32 m_payloadLength; // +0xc  payload length
    char m_payload[1];   // +0x10 payload
};
SIZE_UNKNOWN(CNetCmdPacket); // trailing-payload packet (flexible array); fixed size TBD

struct CColorSlot {
    i32 m_slotHead;             // +0x00 (buf+0x150)  CreateSlot's per-slot command-head target
    char m_pad04[4];            // +0x04
    i32 m_currentOwnerPlayerId; // +0x08 (buf+0x158)  current color owner player id
};
SIZE_UNKNOWN(CColorSlot); // +0x150 sub-region view; retail size TBD

struct CNetCmdBuf {
    char m_pad0[0x150]; // +0x000
    CColorSlot m_sel;   // +0x150
    char m_pad15c[0x238 - 0x15c];
};
SIZE(CNetCmdBuf, 0x238); // fully-known command buffer (array stride 0x238)

struct CNetResyncEntry {
    i32 m_0;
    i32 m_4; // +0x4
    u8 m_8;  // +0x8  flag byte (zeroed by ResetAll)
    char m_pad9[0xc - 9];
    i32 m_c; // +0xc
    char m_padc[0x410 - 0x10];
};
SIZE(CNetResyncEntry, 0x410); // fully-known resync entry (array stride 0x410)

struct GruntRec {
    i32 m_seq;             // +0x00 sequence number (== CNetResyncEntry::m_0)
    i32 m_checksum;        // +0x04 state checksum (== CNetResyncEntry::m_4, Verify compare)
    unsigned char m_count; // +0x08 grunts serialized into this record
    char pad09[3];
    i32 m_payloadLen;             // +0x0c payload length
    char m_payload[0x410 - 0x10]; // +0x10 payload
};
SIZE(GruntRec, 0x410);

class CGruntzCommand;

struct LobbyMsg {
    i32 m_type; // +0x00 message type
    i32 m_04;   // +0x04
    i32 m_08;   // +0x08
};
SIZE_UNKNOWN(LobbyMsg);

void* Unmatched_bf530(i32 zero); // bf530
void RecycleCmd(void* cmd); // bf580  __cdecl

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

class CNetPlayerObj {
public:
    virtual void GetRuntimeClass();      // +0x00  slot 0 (CObject GetRuntimeClass, 0x1bef01)
    virtual void SelfDestruct(i32 flag); // +0x04  slot 1 (scalar-deleting dtor, flag arg)

    char m_pad4[0x20 - 0x4]; // +0x04
    __POSITION* m_20;        // +0x20  cached list position
    char m_pad24[0x34 - 0x24];
    char* m_profile; // +0x34  keyed player-name/profile text (LB_ADDSTRING source;
                     //        NetFormatKeyed reads the NAME key out of it - the ex
                     //        CNetPlayerDesc facet, merged)
};
SIZE_UNKNOWN(CNetPlayerObj); // the payload node (+0x20 position + +0x34 profile pinned)

struct CNetListNode {
    CNetListNode* m_next;  // +0x00  next node
    char m_pad4[4];        // +0x04  prev node (unused)
    CNetPlayerObj* m_data; // +0x08  payload sub-object (polymorphic; slot1 self-destruct)
};
SIZE_UNKNOWN(CNetListNode); // CObList node walk-view; retail size TBD

struct CNetSessionDesc {
    i32 m_dwSize; // +0x00  dwSize (forced to 0x50 by Init)
    char m_pad04[0x30 - 0x04];
    char* m_lpszName;     // +0x30  lpszSessionName
    char* m_lpszPassword; // +0x34  lpszPassword
    char m_pad38[0x50 - 0x38];
};
SIZE(CNetSessionDesc, 0x50); // the 0x50-byte DPSESSIONDESC2

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
    // Return the deep-copied session/group name (m_desc.m_lpszName @+0x34). Body is
    // out-of-lined into the Multi TU (its lone caller, CMulti::StartTitle @0xb72c0).
    char* GroupName(); // 0xb76a0
};
SIZE(CNetPlayerListNode, 0x58);       // AddPlayerNode (NetMgr.cpp 0x1786d0) RezAlloc(0x58)
VTBL(CNetPlayerListNode, 0x001f0760); // ??_7CNetPlayerListNode@@6B@ (5-slot CObject-derived)

class CNetSessionNode : public CObject {
public:
    i32 m_sessionId;      // +0x04
    CString m_shortName;          // +0x08  name CString
    CString m_longName;          // +0x0c  second CString
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

extern "C" void* g_netDirectPlayRiid; // 0x5f0588

extern "C" BOOL __stdcall
NetEnumPlayerCb(void* lpThisSD, void* lpdwTimeout, DWORD dwFlags, CNetMgr* ctx);

extern "C" BOOL __stdcall
NetEnumCb(u32 dpId, DWORD dwType, NetDPName* lpName, DWORD dwFlags, CNetMgr* ctx);

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

struct CNetCtrlMsg {
    i32 m_code; // +0x0  message code (switch tag)
    i32 m_subCode; // +0x4  sub-code
    i32 m_playerId; // +0x8  payload (player id on the player-left path)
};
SIZE_UNKNOWN(CNetCtrlMsg); // packed control-record view (3 dwords pinned); size TBD

struct MenuSelectEvent {
    char m_pad0[0x4];
    i32 m_armed;   // +0x4  armed gate (== 1; the CNetCtrlMsg m_4 sub-code)
    i32 m_id;      // +0x8  player/slot id
    char m_pad0c[0x20 - 0xc];
    char* m_nameA; // +0x20  AddSessionNode name arg A
    char* m_nameB; // +0x24  AddSessionNode name arg B
};
SIZE_UNKNOWN(MenuSelectEvent); // menu-select control-message view (touched offsets pinned)

class CFontConfig; // <Gruntz/FontConfig.h> (the deref TUs include the real header)

struct CNetGameWnd {
    char m_pad0[4];
    HWND m_hwnd; // +0x4  the engine HWND
};
SIZE_UNKNOWN(CNetGameWnd); // window view (only +0x4 HWND pinned); retail size TBD

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
    CFontConfig* m_chatDisplay; // +0x5c  the chat/text display (the real class; ex-CNetChatLog view)
    char m_pad60[0x6c - 0x60];
    CGruntzCmdMgr* m_cmdMgr; // +0x6c  the grunt command manager (Dispatch/EnqueueCommand)
    char m_pad70[0xac - 0x70];
    i32 m_connectGuard; // +0xac  connect-in-progress guard (the CMulti slot-1 driver toggles 1/0 per phase)
    char m_padb0[0x110 - 0xb0];
    i32 m_sessionArmed; // +0x110  session-armed gate (Setup saves the old value into CMulti::m_590)
    i32 m_114; // +0x114
    char m_pad118[0x12c - 0x118];
    i32 m_customLevel; // +0x12c  custom-vs-stock level flag (Setup sets 0 custom / 1 stock)
    char m_pad130[0x150 - 0x130];
    GruntzPlayer m_channels[4]; // +0x150  the inline per-player slot array (== CGruntzMgr::m_options)
};
SIZE_UNKNOWN(CNetGameMgr); // game-mgr view (+0x4/+0x5c/+0x6c/+0x150 pinned); retail size TBD

void ResetNetSlots(); // 0x004db1d0

extern "C" char g_recvBuffer[]; // 0x6467d8

extern u8 g_chanStat422_flag; // 0x646fd8
extern i32 g_chanStat422_id;  // 0x646fdc
extern i32 g_chanStat422_val; // 0x646fe0
extern u8 g_chanStat423_flag; // 0x646378
extern i32 g_chanStat423_id;  // 0x64637c
extern i32 g_chanStat423_val; // 0x646380

extern "C" u8 g_chatPacket_flag;  // 0x6473e0
extern "C" i32 g_chatPacket_id;   // 0x6473e4
extern "C" i32 g_chatPacket_val;  // 0x6473e8
extern "C" char g_chatPacket_buf; // 0x6473ec  (strcpy dest)

extern "C" i32 g_playerLeftFlag; // 0x648ce4
extern "C" i32 g_activePlayerCount; // 0x648cec  active-player refcount

struct InterfaceObject; // the DirectPlay service-provider node (IsInterface2 probe)
struct CNetCreateCtx {
    char m_pad0[0x70];
    InterfaceObject*
        m_serviceProvider; // +0x70  selected service-provider (IsInterface2 -> slow-link timeout)
    u8* m_74;              // +0x74  the group-enumeration record blob
};
SIZE_UNKNOWN(CNetCreateCtx); // create-context view (only +0x74 pinned); retail size TBD

struct CGroupNode {
    CGroupNode* m_next;      // +0x00  CObList CNode pNext
    CGroupNode* m_prev;      // +0x04  CObList CNode pPrev (not walked here)
    InterfaceObject* m_data; // +0x08  payload service-provider node
};
SIZE_UNKNOWN(CGroupNode); // traversal view of the +0x1c group list node

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
        return reinterpret_cast<CString&>(m_8); // +0x8 raw payload viewed as a CString
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
    i32 BroadcastOneChannel(GruntzPlayer* ch); // 0xbaf00  serialize one channel -> 0x2c packet
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
    // +0x008  a name CString's raw payload. Retail's ~CNetMgr (0xb6000) destroys ONLY
    // the 3 CObLists (+0x1c/+0x38/+0x54) and leaks this - so it is NOT a destructible
    // CString member here; GetName reads it via a CString& view (copy-ctor at 0xba170).
    char* m_8;
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

#endif // NET_NETMGR_H
