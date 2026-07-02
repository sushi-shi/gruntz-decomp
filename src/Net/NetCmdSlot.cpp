// NetCmdSlot.cpp - CNetMgr per-player command-slot helpers (0x0c0fa0..0x0c1390).
//
// A CNetCmdSlot is one of the four inline 0x64-byte slots the DirectPlay session
// (CNetMgr+0x520) keeps; FindCmdSlot returns the one whose player matches. Each
// slot owns a CObList of queued commands at +0x20 (a CObject vptr + head/tail/
// count + free-list). The resend path (CNetMgr::ResetPlayerCommands) latches the
// slot (Touch), redispatches the windowed commands and drops each (RemoveCmd),
// then clears both command ranges (ResetTriple). These are all self-contained
// (no DIR32 relocs) - the only external references are the CObList mutators (a
// shared collection TU at 0x1b49xx) and the command-recycle helper (0xbf580),
// all reloc-masked rel32 calls.
//
// Also here (retail emits them in this address run):
//   - three __stdcall helpers over a 3-int player-id set (find/add/clear), and
//   - a session-readiness check on an as-yet-unidentified owner object that
//     holds a CNetMgr* at +0x1c and a per-slot ack-flag array at +0x3c.
#include <Net/NetMgr.h> // <Mfc.h> -> CObList / POSITION / CObject (reloc-masked)
#include <rva.h>

// The CObList node shape (CObject list): +0x0 next, +0x4 prev, +0x8 payload.
// Used to walk the queue and to name the position the removal targets.
struct CObListNode {
    CObListNode* m_next; // +0x0
    CObListNode* m_prev; // +0x4
    void* m_data;        // +0x8
};
SIZE_UNKNOWN(CObListNode); // CObList node walk-view; retail size TBD

// Return a finished command to the engine's free pool (0xbf580, __cdecl): it
// AddTail's the node onto a global recycle list. External (reloc-masked).
extern void RecycleCmd(void* cmd);

// The three __stdcall id-set helpers are defined further down (RVA order puts
// CNetSession::ResetCmdBuffers / the slot resets above them); forward-declare so
// AdvanceSeq below can fold its window through them.
i32 __stdcall NetCmdIdFind(i32* arr, i32 v);
void __stdcall NetCmdIdAdd(i32* arr, i32 v);
void __stdcall NetCmdIdClear(i32* arr, i32 v);

// ProcessCmd's externals (all reloc-masked rel32 callees / inline intrinsics).
extern "C" void* memcpy(void* d, const void* s, u32 n);
#pragma intrinsic(memcpy)

// The recycled-command-packet allocator (0xbf530, __cdecl): hands back a node from
// the global packet pool. Modeled by its delinker name so the call is named.
void* Unmatched_bf530(i32 zero); // 0xbf530

// A parsed grunt command the record's per-entry stream produces. Allocated by one
// of two static factories, parsed in place through vtable slot 7, then enqueued.
// Modeled polymorphically so `obj->Parse()` emits the slot-7 thiscall dispatch; no
// virtual is defined here so cl emits no vtable.
class CGruntzCommand {
public:
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual i32 Parse(void* data, i32 len); // +0x1c (slot 7)

    char m_pad4[0xc - 0x4];
    i32 m_submitted; // +0x0c  "submitted" flag
};
class CGruntzSingleCommand : public CGruntzCommand {
public:
    static CGruntzSingleCommand* Allocate(); // 0x24220
};
class CGruntzMultiCommand : public CGruntzCommand {
public:
    static CGruntzMultiCommand* Allocate(); // 0x24360
};

// The per-game command manager reached through CNetMgr->m_4->m_6c; ProcessCmd
// hands each parsed grunt command to it.
struct CGruntzCmdMgr {
    void EnqueueCommand(i32 a, void* cmd); // 0x23d10
};
// The game-manager sub-object hanging off CNetMgr+0x4 (its +0x6c is the grunt
// command manager). The shared CNetMgr models m_4 as an untyped sub-object ptr (it
// has "no single clean type" - reached through several per-TU views, see NetMgr.h);
// this is the command-manager view of it.
struct CNetMgrSub {
    char m_pad0[0x6c];
    CGruntzCmdMgr* m_cmdMgr; // +0x6c
};
SIZE_UNKNOWN(CNetMgrSub); // CNetMgr+0x4 game-sub, command-manager view (only +0x6c pinned)

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
    i32 m_sequence; // +0x0  sequence
    void* m_owner;  // +0x4  owning slot (this)
    u8 m_flags;     // +0x8  flag byte
    char m_pad9[0xc - 9];
    i32 m_payloadLength; // +0xc  payload length
    char m_payload[1];   // +0x10 payload
};
SIZE_UNKNOWN(CNetCmdPacket); // trailing-payload packet (flexible array); fixed size TBD

// ---------------------------------------------------------------------------
// CNetSession::ResetCmdBuffers (0x0c0070, __thiscall) - zero the +0x10 head of
// each of the four inline command slots.
// ---------------------------------------------------------------------------
RVA(0x000c0070, 0x15)
void CNetSession::ResetCmdBuffers() {
    for (i32 i = 0; i < 4; i++) {
        m_slots[i].m_latency = 0;
    }
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::ResetAll (0x0c0bb0, __thiscall) - full wipe: zero every scalar
// field (incl. m_state/m_cmdHead/m_owner), drain the queue, then splat both ranges.
// ---------------------------------------------------------------------------
// @early-stop
// zero-register-pinning wall (73.7%): logic byte-exact. Retail re-materializes
// the splat constant (`xor eax,eax` twice, around ClearCmds) and saves only esi;
// cl instead pins 0 in callee-saved edi across the call (one xor + an extra
// push/pop edi). A regalloc coin-flip identical to ResetTriple/AllSlotsReady in
// this TU (docs/patterns/zero-register-pinning.md); not source-steerable. Final sweep.
RVA(0x000c0bb0, 0x47)
void CNetCmdSlot::ResetAll() {
    m_state = 0;
    m_resetGuard = 0;
    m_latchedSeq = 0;
    m_cmdHead = 0;
    m_latency = 0;
    m_baseSeq = 0;
    m_maxSeq = 0;
    m_owner = 0;
    ClearCmds();
    m_3c = 0;
    m_40 = 0;
    m_44 = 0;
    m_48 = 0;
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::ProcessCmd (0x0c0c70, __thiscall) - parse one incoming command
// record (opcode + parity prefix, a fixed header, then a per-entry payload).
// Bit 7 relays the record straight to the net manager. Even/odd records gate on
// the slot's reset guard; matched records bump the high-water windows, queue a
// recycled copy of the payload, and unpack each entry into a grunt command that is
// submitted to the per-game command manager.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc + this-residency wall (~75%): the full control flow is byte-faithful -
// the opcode/parity dispatch (recovered as two independent `if` guards), the bit-7
// relay, the FindCmdSlot ack-flag set, RaiseMax/NetCmdIdAdd/NetCmdIdClear/
// NetCmdIdFind/AdvanceSeq window updates, the Unmatched_bf530 packet alloc + inline
// payload memcpy + AddCmd, and the per-entry Single/MultiCommand parse loop with the
// vtable-slot-7 dispatch + EnqueueCommand. Three unsteerable MSVC5 /O2 choices
// remain: (a) cl pins `this` in ebp where retail keeps it in esi + a [esp+0x18]
// reload (a callee-saved-register coin-flip that cascades); (b) the redundant
// `mov ecx,esi` retail refreshes before each __stdcall NetCmdId* call (the same
// this-residency idiom that parks this TU's AdvanceSeq at 86.6%); (c) retail walks
// the header through an advancing cursor with grouped byte-counter decrements
// (-8/-4/-1) where cl folds the offsets (p+13, rem-13). No source lever; final sweep.
RVA(0x000c0c70, 0x20f)
i32 CNetCmdSlot::ProcessCmd(i32 playerId, void* rec, i32 size) {
    if (rec == 0) {
        return 0;
    }
    u8 opcode = *(u8*)rec;
    i32 odd = opcode & 1;
    char* p = (char*)rec + 1;
    if (m_state != 3) {
        return 1;
    }
    if (opcode & 0x80) {
        return ((CNetMgr*)m_owner)->DispatchRecvMsg(m_cmdHead[6], (char*)rec, size);
    }
    if (odd == 0) {
        if (m_resetGuard != 0) {
            return 1;
        }
    }
    if (odd) {
        if (m_resetGuard == 0) {
            return 1;
        }
    }

    i32 rem = size - 1;
    if (odd) {
        p++;
        rem--;
    }
    CNetCmdHdr* h = (CNetCmdHdr*)p;
    i32 seq = h->m_sequence;
    i32 base = h->m_windowBase;
    i32 flags = h->m_flags;
    u8 count = h->m_entryCount;
    char* cursor = p + 13;
    rem -= 13;

    if (m_resetGuard != 0 && odd) {
        CNetCmdSlot* slot = ((CNetMgr*)m_owner)->m_session->FindCmdSlot(playerId);
        if (slot == 0) {
            return 0;
        }
        if (opcode & 2) {
            i32 pid = slot->m_cmdHead[0] & 0xff;
            (&m_3c)[pid] = 1;
            if (seq > m_latchedSeq) {
                m_latchedSeq = seq;
            }
        }
    }

    RaiseMax(base);
    if (opcode & 0x10) {
        NetCmdIdAdd(m_rangeB, base + 2);
    } else if (opcode & 0x20) {
        NetCmdIdAdd(m_rangeB, base + 3);
    }
    NetCmdIdClear(m_rangeB, base + 1);

    if (m_baseSeq >= seq) {
        return 1;
    }
    if (NetCmdIdFind(m_rangeA, seq)) {
        return 1;
    }
    AdvanceSeq(seq);

    CNetCmdPacket* pkt = (CNetCmdPacket*)Unmatched_bf530(0);
    pkt->m_sequence = seq;
    pkt->m_owner = this;
    pkt->m_flags = (u8)flags;
    pkt->m_payloadLength = rem;
    memcpy(pkt->m_payload, cursor, rem);
    AddCmd((CNetCmd*)pkt);

    for (i32 i = count & 0xff; i > 0; i--) {
        u8 b = *(u8*)cursor;
        CGruntzCommand* obj;
        if (b & 1) {
            obj = CGruntzSingleCommand::Allocate();
        } else if (b & 2) {
            obj = CGruntzMultiCommand::Allocate();
        } else {
            continue;
        }
        i32 consumed = obj->Parse(cursor, rem);
        obj->m_submitted = 1;
        ((CNetMgrSub*)((CNetMgr*)m_owner)->m_4)->m_cmdMgr->EnqueueCommand(0, obj);
        rem -= consumed;
        cursor += consumed;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::AdvanceSeq (0x0c0f10, __thiscall) - fold an acknowledged id into
// the high-water window: if it is exactly m_14+1, retire it (and every
// already-present successor) out of range A; otherwise just record it.
// ---------------------------------------------------------------------------
// @early-stop
// this-in-ecx residency wall (86.6%): logic + control flow byte-exact. Retail
// refreshes `mov ecx,esi` (this) before each __stdcall NetCmdId* call even though
// the convention ignores ecx (4 redundant moves cl omits); the optimizer keeps
// `this` resident in ecx as well as esi. Same flavor as the framed-module
// this-residency idiom; not source-steerable here. Final sweep.
RVA(0x000c0f10, 0x6e)
void CNetCmdSlot::AdvanceSeq(i32 id) {
    if (m_baseSeq + 1 == id) {
        NetCmdIdClear(m_rangeA, m_baseSeq);
        m_baseSeq++;
        while (NetCmdIdFind(m_rangeA, m_baseSeq + 1)) {
            m_baseSeq++;
            NetCmdIdClear(m_rangeA, m_baseSeq);
        }
    } else {
        NetCmdIdAdd(m_rangeA, id);
    }
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::RaiseMax (0x0c0fa0, __thiscall) - keep the high-water sequence.
// ---------------------------------------------------------------------------
RVA(0x000c0fa0, 0x11)
void CNetCmdSlot::RaiseMax(i32 v) {
    if (v > m_maxSeq) {
        m_maxSeq = v;
    }
}

// ---------------------------------------------------------------------------
// NetCmdIdFind (0x0c0fd0, __stdcall) - is `v` one of the three ids in `arr`?
// ---------------------------------------------------------------------------
RVA(0x000c0fd0, 0x24)
i32 __stdcall NetCmdIdFind(i32* arr, i32 v) {
    for (i32 i = 0; i < 3; i++) {
        if (v == arr[i]) {
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// NetCmdIdAdd (0x0c1010, __stdcall) - add `v` to the first free (-1) slot of
// `arr`, unless it is already present.
// ---------------------------------------------------------------------------
RVA(0x000c1010, 0x32)
void __stdcall NetCmdIdAdd(i32* arr, i32 v) {
    if (NetCmdIdFind(arr, v)) {
        return;
    }
    for (i32 i = 0; i < 3; i++) {
        if (arr[i] == -1) {
            arr[i] = v;
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// NetCmdIdClear (0x0c1060, __stdcall) - clear (-1) the first slot of `arr`
// equal to `v`.
// ---------------------------------------------------------------------------
RVA(0x000c1060, 0x29)
void __stdcall NetCmdIdClear(i32* arr, i32 v) {
    for (i32 i = 0; i < 3; i++) {
        if (v == arr[i]) {
            arr[i] = -1;
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::ResetTriple (0x0c10a0, __thiscall) - splat -1 over three dwords.
// (The slot pointer is unused; only the explicit array argument is touched.)
// ---------------------------------------------------------------------------
// @early-stop
// regalloc swap (93.33%): logic byte-exact, but retail loads p into ecx (reusing
// the dead `this`) and materializes -1 into eax (`mov ecx,[esp+4]; or eax,-1`),
// while cl picks eax for p and ecx for -1. A pure eax/ecx coin-flip on a thiscall
// member that ignores `this` (see const-materialize-into-reg-vs-immediate.md /
// zero-register-pinning.md); not source-steerable. Final sweep.
RVA(0x000c10a0, 0x12)
void CNetCmdSlot::ResetTriple(i32* p) {
    p[0] = -1;
    p[1] = -1;
    p[2] = -1;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::AddCmd (0x0c1170, __thiscall) - enqueue a command, deduping on
// its sequence number.
// ---------------------------------------------------------------------------
RVA(0x000c1170, 0x26)
void CNetCmdSlot::AddCmd(CNetCmd* cmd) {
    if (cmd != 0 && FindCmd(cmd->m_seq) == 0) {
        m_cmds.AddTail((CObject*)cmd);
    }
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::RemoveCmd (0x0c11b0, __thiscall) - drop the queued command with
// the given sequence number and recycle it.
// ---------------------------------------------------------------------------
RVA(0x000c11b0, 0x55)
void CNetCmdSlot::RemoveCmd(i32 seq) {
    CObListNode* node = (CObListNode*)m_cmds.GetHeadPosition();
    while (node != 0) {
        CObListNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = (CNetCmd*)cur->m_data;
        if (seq == cmd->m_seq) {
            if (node != 0) {
                m_cmds.RemoveAt((POSITION)node->m_prev);
            } else {
                m_cmds.RemoveTail();
            }
            RecycleCmd(cmd);
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::GetRange (0x0c1230, __thiscall) - min/max queued sequence into
// (*pMin, *pMax); zero both when the queue is empty.
// ---------------------------------------------------------------------------
RVA(0x000c1230, 0x55)
void CNetCmdSlot::GetRange(i32* pMin, i32* pMax) {
    if (pMin == 0) {
        return;
    }
    if (pMax == 0) {
        return;
    }
    *pMax = 0x80000001;
    *pMin = 0x7fffffff;
    CObListNode* node = (CObListNode*)m_cmds.GetHeadPosition();
    if (node == 0) {
        *pMax = 0;
        *pMin = 0;
        return;
    }
    do {
        CObListNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = (CNetCmd*)cur->m_data;
        if (cmd->m_seq > *pMax) {
            *pMax = cmd->m_seq;
        }
        if (cmd->m_seq < *pMin) {
            *pMin = cmd->m_seq;
        }
    } while (node != 0);
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::FindCmd (0x0c12b0, __thiscall) - the queued command with the
// given sequence number, or null.
// ---------------------------------------------------------------------------
RVA(0x000c12b0, 0x1f)
CNetCmd* CNetCmdSlot::FindCmd(i32 seq) {
    CObListNode* node = (CObListNode*)m_cmds.GetHeadPosition();
    while (node != 0) {
        CObListNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = (CNetCmd*)cur->m_data;
        if (seq == cmd->m_seq) {
            return cmd;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::ClearCmds (0x0c12e0, __thiscall) - drain the queue, recycling
// each command.
// ---------------------------------------------------------------------------
RVA(0x000c12e0, 0x2c)
void CNetCmdSlot::ClearCmds() {
    while (m_cmds.GetCount() != 0) {
        CNetCmd* cmd = (CNetCmd*)m_cmds.RemoveHead();
        if (cmd != 0) {
            RecycleCmd(cmd);
        }
    }
}

// The owner of the session-readiness check below: an object that caches a
// CNetMgr* at +0x1c and a per-slot local-ack flag array at +0x3c. Its real
// class is not yet pinned (self-contained, so matching is name-independent);
// modeled minimally here.
struct CNetSyncCheck {
    char m_pad0[0x1c]; // +0x00
    CNetMgr* m_netMgr; // +0x1c  the owning net manager
    char m_pad20[0x3c - 0x20];
    i32 m_localAckFlags[4]; // +0x3c  per-slot local-ack flags

    i32 AllSlotsReady(); // c1320
};
SIZE_UNKNOWN(CNetSyncCheck); // minimal view (only +0x1c/+0x3c pinned); retail size TBD

// ---------------------------------------------------------------------------
// CNetSyncCheck::AllSlotsReady (0x0c1320, __thiscall) - false (0) if any active
// (m_state==3), unreset (m_resetGuard==0) command slot has not yet been acked locally
// (m_3c[i]==0); true (1) otherwise.
// ---------------------------------------------------------------------------
// @early-stop
// induction-var/regalloc wall (~79%): logic byte-exact. Retail keeps `sess` in
// esi and recomputes the slot via `lea ecx,[esi+eax+0x20]` each iteration with
// the byte offset eax (compared to 0x190) as the loop variable, spending a 5th
// register (push edi for the slot->m_4 temp); cl folds sess+offset into one
// running slot pointer and counts i<4, needing only 4 registers (one push). The
// offset-based source form regresses it (72%). An IV-selection coin-flip
// (cf. const-materialize-into-reg-vs-immediate.md); not steerable. Final sweep.
RVA(0x000c1320, 0x4a)
i32 CNetSyncCheck::AllSlotsReady() {
    CNetMgr* mgr = m_netMgr;
    if (mgr == 0) {
        return 0;
    }
    CNetSession* sess = mgr->m_session;
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &sess->m_slots[i];
        if (slot != 0 && slot->m_state == 3 && slot->m_resetGuard == 0 && m_localAckFlags[i] == 0) {
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CNetCmdSlot::Touch (0x0c1390, __thiscall) - latch the slot the first time it
// is reset (sets the guard and copies the base sequence).
// ---------------------------------------------------------------------------
RVA(0x000c1390, 0x15)
void CNetCmdSlot::Touch() {
    if (m_resetGuard == 0) {
        m_resetGuard = 1;
        m_latchedSeq = m_baseSeq;
    }
}
