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
#include <Net/NetMgr.h>           // <Mfc.h> -> CObList / POSITION / CObject (reloc-masked)
#include <Gruntz/GruntzCmdMgr.h>  // CNetGameMgr::m_6c real command manager (EnqueueCommand)
#include <Gruntz/GruntzCommand.h> // canonical CGruntzCommand/Single/Multi (slot-7 Parse)
#include <rva.h>

#include <string.h> // memcpy (see #pragma intrinsic below)

// CNetCmdNode (the slot's +0x20 command-queue CObList node), CNetCmdHdr (the record
// header prefix) and CNetCmdPacket (the recycled queue packet) are the shared Net
// protocol types - canonical in <Net/NetMgr.h> (included above).

// Return a finished command to the engine's free pool (0xbf580, __cdecl): it
// AddTail's the node onto a global recycle list. External (reloc-masked).
extern void RecycleCmd(void* cmd);

// The three __stdcall id-set helpers are defined further down (RVA order puts
// CNetSession::ResetCmdBuffers / the slot resets above them); forward-declare so
// AdvanceSeq below can fold its window through them.
i32 __stdcall NetCmdIdFind(i32* arr, i32 v);
void __stdcall NetCmdIdAdd(i32* arr, i32 v);
void __stdcall NetCmdIdClear(i32* arr, i32 v);

// ProcessCmd's memcpy is forced intrinsic (reloc-masked / inline).
#pragma intrinsic(memcpy)
#pragma intrinsic(strcat)

// The two file-scope debug buffers NetCmdIdToString formats through: a 16-byte
// per-id wsprintfA scratch (0x64b6a0) and the comma-joined accumulator (0x64b6b0).
// BSS; address-pinned so the DIR32 loads reloc-mask.
DATA(0x0024b6a0)
extern char g_idScratch[0x10];
DATA(0x0024b6b0)
extern char g_idListBuf[0x40];

// The recycled-command-packet allocator (0xbf530, __cdecl): hands back a node from
// the global packet pool. Modeled by its delinker name so the call is named.
void* Unmatched_bf530(i32 zero); // 0xbf530

// The parsed grunt command the record's per-entry stream produces is the canonical
// command family (<Gruntz/GruntzCommand.h>): CGruntzSingleCommand/CGruntzMultiCommand
// each Allocate() from their recycle list, Parse() in place through vtable slot 7,
// then get enqueued. (Formerly a reduced local polymorphic view of the same three
// classes.)

// The per-game command manager reached through CNetMgr->m_4->m_6c is the real
// CGruntzCmdMgr (EnqueueCommand @0x23d10); ProcessCmd hands each parsed grunt
// command to it. Shared via the canonical CNetGameMgr::m_6c (folds the former
// local CGruntzCmdMgr/CNetMgrSub placeholders).

// The slot's +0x1c owner is the shared CNetMgr (NetMgr.h): ProcessCmd reaches its
// +0x4 game-mgr sub-object (CNetGameMgr; +0x6c is the grunt command manager), the
// DirectPlay session at +0x520 (FindCmdSlot), and DispatchRecvMsg (0xb9750) for the
// high-bit relay - all real typed members now, no reduced view.

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
    m_ackFlags[0] = 0;
    m_ackFlags[1] = 0;
    m_ackFlags[2] = 0;
    m_ackFlags[3] = 0;
    ResetTriple(m_rangeA);
    ResetTriple(m_rangeB);
}

// ---------------------------------------------------------------------------
// 0x0c0c20 (spatially re-homed from src/Stub/Cluster0c.cpp). Field-init of an
// unidentified per-session net object (called by CNetMgr::AckDropPlayer /
// CNetSession::Reset / CLobbySync::Reconcile): zero a span of members, then
// construct the two embedded sub-objects at +0x4c/+0x58. @orphan (class identity
// unrecovered; its Cleanup sibling homes to MultiStartDlgRoster.cpp).
// @early-stop
// regalloc tie-break wall (~71%): logic byte-identical, but retail rematerializes
// the zero constant in eax while cl hoists 0 into a callee-saved edi across the
// Init12e0 call; a cl-build heuristic delta, not a source shape.
struct CNetThing; // +0x60 owned child (dtor 0xc5280, in src/Net/NetThingDtor.cpp)
struct CCluster0c {
    char pad00[4];
    int m_04; // +0x04
    int m_08; // +0x08
    char pad0c[0x10 - 0x0c];
    int m_10; // +0x10
    int m_14; // +0x14
    int m_18; // +0x18
    char pad1c[0x3c - 0x1c];
    int m_3c;               // +0x3c
    int m_40;               // +0x40
    int m_44;               // +0x44
    int m_48;               // +0x48
    char m_4c[0x58 - 0x4c]; // +0x4c sub-object
    char m_58[0x60 - 0x58]; // +0x58 sub-object
    CNetThing* m_60;        // +0x60 owned child

    void Init12e0();        // 0xc12e0
    void Init10a0(void* p); // 0xc10a0

    void Init(); // 0xc0c20
};
RVA(0x000c0c20, 0x3f)
void CCluster0c::Init() {
    m_04 = 0;
    m_08 = 0;
    m_10 = 0;
    m_14 = 0;
    m_18 = 0;
    Init12e0();
    m_3c = 0;
    m_40 = 0;
    m_44 = 0;
    m_48 = 0;
    Init10a0(&m_4c);
    Init10a0(&m_58);
}
SIZE_UNKNOWN(CCluster0c);

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
        return m_owner->DispatchRecvMsg(m_cmdHead[6], (char*)rec, size);
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
        CNetCmdSlot* slot = m_owner->m_session->FindCmdSlot(playerId);
        if (slot == 0) {
            return 0;
        }
        if (opcode & 2) {
            i32 pid = slot->m_cmdHead[0] & 0xff;
            m_ackFlags[pid] = 1;
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
        obj->m_submitted = 1; // "submitted" latch
        m_owner->m_4->m_6c->EnqueueCommand(0, obj);
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
// NetCmdIdToString (0x0c10d0, __stdcall) - format the three-int player-id set as a
// "%d,"-joined debug string in the shared g_idListBuf accumulator (skipping -1 free
// slots); each id is rendered through the g_idScratch scratch and strcat'd on.
// ---------------------------------------------------------------------------
// @early-stop
// frame-slot-selection wall (95.36%): the whole body is byte-faithful (the IAT-ptr
// hoist into ebx, the "%d," wsprintfA + inline strcat, the walking-pointer countdown
// loop). The only real residue is where the spilled 3->0 counter lives: retail
// REUSES the dead arg1 home slot ([esp+0x14]) with a zero-extra frame, while cl
// allocates a fresh local (`push ecx`/`pop ecx`, counter at [esp+0x10]) - a
// build-8034 frame-allocator coin-flip on a fn whose 4 callee-saved regs are all
// live across the calls. Not source-steerable (do-while/for/permute identical).
RVA(0x000c10d0, 0x7c)
char* __stdcall NetCmdIdToString(i32* arr) {
    g_idListBuf[0] = 0;
    i32 n = 3;
    do {
        if (*arr != -1) {
            wsprintfA(g_idScratch, "%d,", *arr);
            strcat(g_idListBuf, g_idScratch);
        }
        arr++;
    } while (--n != 0);
    return g_idListBuf;
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
    CNetCmdNode* node = (CNetCmdNode*)m_cmds.GetHeadPosition();
    while (node != 0) {
        CNetCmdNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = cur->m_data;
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
    CNetCmdNode* node = (CNetCmdNode*)m_cmds.GetHeadPosition();
    if (node == 0) {
        *pMax = 0;
        *pMin = 0;
        return;
    }
    do {
        CNetCmdNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = cur->m_data;
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
    CNetCmdNode* node = (CNetCmdNode*)m_cmds.GetHeadPosition();
    while (node != 0) {
        CNetCmdNode* cur = node;
        node = node->m_next;
        CNetCmd* cmd = cur->m_data;
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
