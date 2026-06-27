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

// One queued command: only the +0x0 sequence number is inspected here.
struct CNetCmd {
    i32 m_seq; // +0x0  command sequence number
};

// The CObList node shape (CObject list): +0x0 next, +0x4 prev, +0x8 payload.
// Used to walk the queue and to name the position the removal targets.
struct CObListNode {
    CObListNode* m_next; // +0x0
    CObListNode* m_prev; // +0x4
    void* m_data;        // +0x8
};

// Return a finished command to the engine's free pool (0xbf580, __cdecl): it
// AddTail's the node onto a global recycle list. External (reloc-masked).
extern void RecycleCmd(void* cmd);

// ---------------------------------------------------------------------------
// CNetCmdSlot::RaiseMax (0x0c0fa0, __thiscall) - keep the high-water sequence.
// ---------------------------------------------------------------------------
RVA(0x000c0fa0, 0x11)
void CNetCmdSlot::RaiseMax(i32 v) {
    if (v > m_18) {
        m_18 = v;
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
    CNetMgr* m_1c;     // +0x1c  the owning net manager
    char m_pad20[0x3c - 0x20];
    i32 m_3c[4]; // +0x3c  per-slot local-ack flags

    i32 AllSlotsReady(); // c1320
};

// ---------------------------------------------------------------------------
// CNetSyncCheck::AllSlotsReady (0x0c1320, __thiscall) - false (0) if any active
// (m_0==3), unreset (m_4==0) command slot has not yet been acked locally
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
    CNetMgr* mgr = m_1c;
    if (mgr == 0) {
        return 0;
    }
    CNetSession* sess = mgr->m_session;
    for (i32 i = 0; i < 4; i++) {
        CNetCmdSlot* slot = &sess->m_slots[i];
        if (slot != 0 && slot->m_0 == 3 && slot->m_4 == 0 && m_3c[i] == 0) {
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
    if (m_4 == 0) {
        m_4 = 1;
        m_8 = m_14;
    }
}
