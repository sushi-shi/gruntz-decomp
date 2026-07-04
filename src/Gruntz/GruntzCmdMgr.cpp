// GruntzCmdMgr.cpp - CGruntzCmdMgr, the per-game command/target queue the game
// manager owns (CGruntzMgr+0x6c; BroadcastCmd 0x093460 dispatches into it). See
// include/Gruntz/GruntzCmdMgr.h for the layout. The class owns a primary CObList
// queue (base, +0x00) of target objects, a nested CObList (+0x1c), and a state-
// providing manager pointer (+0x38). Names are placeholders; offsets + code bytes
// are load-bearing.
//
// The destructor (0x085bd0) carries a /GX EH frame (the inline CObList teardown
// is the destructible sub-object); this TU is built flags="eh".
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommand.h>
#include <rva.h>

// The g_mgrSettings singleton (0x64556c). Minimal local view: IsActive folds
// reg->m_30 into a boolean. extern "C" so the DATA load reloc-masks against the
// canonical _g_mgrSettings symbol (single view; was a mis-named g_gameReg alias).
SIZE_UNKNOWN(MgrSettings30);
struct MgrSettings30 {
    char m_pad0[0x30];
    i32 m_world; // +0x30
};
DATA(0x0024556c)
extern "C" MgrSettings30* g_mgrSettings;

// the registry-active predicate the read pass gates on (an out-of-line
// twin of IsActive at 0x024a90; same body). External to this TU, reloc-masked.
i32 IsActive2(i32 enable);

// ---------------------------------------------------------------------------
// SetMgr: install the state-providing manager pointer; return 1.
// ---------------------------------------------------------------------------
RVA(0x000239d0, 0xf)
i32 CGruntzCmdMgr::SetMgr(GzMgr* mgr) {
    m_38 = mgr;
    return 1;
}

// ---------------------------------------------------------------------------
// ClearAndReset: null the manager pointer, then drain everything
// (tail-call Clear). MSVC emits the tail-call as a jmp to Clear's body.
// ---------------------------------------------------------------------------
RVA(0x000239f0, 0xc)
void CGruntzCmdMgr::ClearAndReset() {
    m_38 = 0;
    Clear();
}

// ---------------------------------------------------------------------------
// ScanTargets: walk the base queue (by index, removing each as it is
// processed) filtering for objects that are either selected (flag 0x2) or
// active+keyed (flag 0x1 and the type byte matches `param`). When the game is in
// the play state (id 0x11), latch each match into a 4-slot per-team table (keyed
// by the object's index byte) for a deferred ordered select pass; otherwise
// select+deselect it immediately. After the walk, in play state, run the deferred
// select+deselect over the table. Returns 1.
// @early-stop
// regalloc/scheduling wall - the state-filter scan pins the 4-slot stack table +
// the index counter the way only the original source spelling reproduces; logic
// is byte-for-byte, the residual is register-coloring + the signed-mod-by-4
// table index ordering. Deferred to the final sweep.
RVA(0x00023a10, 0xe7)
i32 CGruntzCmdMgr::ScanTargets(i32 param) {
    GzStateProvider* sp = m_38->m_2c;
    i32 isPlay = (sp->GetStateId() == 0x11);
    GzTargetObj* table[4];
    table[0] = 0;
    table[1] = 0;
    table[2] = 0;
    table[3] = 0;
    i32 i;
    for (i = 0; i < m_base.m_c; i++) {
        void* pos = m_base.FindIndex(i);
        GzTargetObj* obj = *(GzTargetObj**)((char*)pos + 8);
        i32 flags = obj->m_c;
        if (!(flags & 2)) {
            if (!(flags & 1)) {
                continue;
            }
            if ((u8)obj->m_6 != (u32)param) {
                continue;
            }
        }
        if (isPlay) {
            table[*(u8*)((char*)obj + 4)] = obj;
        } else {
            obj->Select(sp);
            obj->Deselect();
        }
        m_base.RemoveAt(pos);
        i--;
    }
    if (isPlay) {
        for (i = 0; i < 4; i++) {
            GzTargetObj* obj = table[i % 4];
            if (obj) {
                obj->Select(sp);
                obj->Deselect();
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// RemoveMatchingTarget: walk the base queue by index and remove the
// FIRST object that matches both the type byte (m_6 == typeByte) and the index
// byte (m_4 == indexByte): RemoveAt(pos) + deselect (vtable slot +0x28). Stops at
// the first hit. typeByte is hoisted out of the loop; indexByte is re-read per
// iteration (the natural codegen for the two-key compare).
// ---------------------------------------------------------------------------
RVA(0x00023b40, 0x53)
void CGruntzCmdMgr::RemoveMatchingTarget(char indexByte, char typeByte) {
    for (i32 i = 0; i < m_base.m_c; i++) {
        void* pos = m_base.FindIndex(i);
        GzTargetObj* obj = *(GzTargetObj**)((char*)pos + 8);
        if (obj->m_6 == (u8)typeByte && obj->m_4 == (u8)indexByte) {
            m_base.RemoveAt(pos);
            obj->Deselect();
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// DrainBase: while the base queue is non-empty, RemoveTail() a node
// and (if non-null) deselect it via its vtable slot +0x28.
// ---------------------------------------------------------------------------
RVA(0x00023bc0, 0x25)
void CGruntzCmdMgr::DrainBase() {
    while (m_base.m_c) {
        GzTargetObj* obj = (GzTargetObj*)m_base.RemoveTail();
        if (obj) {
            obj->Deselect();
        }
    }
}

// ---------------------------------------------------------------------------
// Clear: full reset. Drain the base queue (deselecting each), then
// RemoveAll the nested +0x1c list, then free both command recycle lists.
// ---------------------------------------------------------------------------
RVA(0x00023c00, 0x1c)
void CGruntzCmdMgr::Clear() {
    DrainBase();
    m_1c.RemoveAll();
    CGruntzSingleCommand::FreeAll();
    CGruntzMultiCommand::FreeAll();
}

// ---------------------------------------------------------------------------
// EnqueueSingle: allocate a single-target command, set its params,
// and enqueue it. The argument permutation in the SetParamsEx call is the
// compiler forwarding the caller's argument block.
// ---------------------------------------------------------------------------
RVA(0x00023c30, 0x47)
void CGruntzCmdMgr::EnqueueSingle(
    i32 p1,
    char p2,
    char p3,
    char p4,
    i16 p5,
    i16 p6,
    char p7,
    char p8
) {
    CGruntzSingleCommand* cmd = CGruntzSingleCommand::Allocate();
    cmd->SetParamsEx(p2, p4, p8, p5, p6, p3, p7);
    EnqueueCommand(p1, cmd);
}

// ---------------------------------------------------------------------------
// EnqueueMulti: allocate a multi-target (mask) command, build its
// flag mask from the index list, and enqueue it.
// ---------------------------------------------------------------------------
RVA(0x00023ca0, 0x47)
void CGruntzCmdMgr::EnqueueMulti(
    i32 p1,
    char p2,
    i32 p3,
    u8* p4,
    char p5,
    i16 p6,
    i16 p7,
    char p8
) {
    CGruntzMultiCommand* cmd = CGruntzMultiCommand::Allocate();
    cmd->SetMaskFromList(p2, p5, p8, p6, p7, p3, p4);
    EnqueueCommand(p1, cmd);
}

// ---------------------------------------------------------------------------
// EnqueueCommand: tag the command's phase field from the current
// game state (state 3 -> phase 2, state 0x11 -> phase 4) and AddTail it onto
// the nested +0x1c list, then always AddTail onto the base queue.
// ---------------------------------------------------------------------------
RVA(0x00023d10, 0x5a)
void CGruntzCmdMgr::EnqueueCommand(i32 flag, void* cmd) {
    if (!cmd) {
        return;
    }
    if (flag) {
        if (m_38->m_2c->GetStateId() == 3) {
            ((CGruntzCommand*)cmd)->m_c = 2;
        } else if (m_38->m_2c->GetStateId() == 0x11) {
            ((CGruntzCommand*)cmd)->m_c = 4;
        }
        m_1c.AddTail(cmd);
    }
    m_base.AddTail(cmd);
}

// ---------------------------------------------------------------------------
// Serialize: (de)serialize the command queue through a stream.
//   mode 4 (write): gate on IsActive(stream); write the node count, then walk the
//       base queue writing each command's tag byte followed by its Serialize(4)
//       payload. Returns 1 (0 on a failed element / inactive).
//   mode 7 (read): gate on IsActive2(stream); Clear() the queue; read the node
//       count, then read that many (tag, payload) pairs, allocating a Single
//       (tag 1) / Multi (tag 2) command, deserializing it, and AddTail'ing it onto
//       the base queue. Returns 1 (0 on a bad tag / failed element / inactive).
// @early-stop
// this-register-spill regalloc wall (see docs/patterns/reread-member-view-pointer.md
// + loop-preheader-vs-exit-block-order.md "this-register spill"): retail keeps the
// heavily-used `stream` arg in esi and SPILLS `this` to a stack local ([esp+0x10],
// the leading `push ecx`), reloading it for the read-path AddTail; the recompile
// keeps both `stream` and `this` in callee-saved regs (no spill), so esi/edi/ebx
// are swapped pervasively (~8% fuzzy, but logic byte-for-byte: the mode 4/7 + tag
// 1/2 branch tree and every call match). Not source-steerable; final sweep.
RVA(0x00024890, 0x18d)
i32 CGruntzCmdMgr::Serialize(CSerialArchive* stream, i32 mode, i32 a3, i32 a4) {
    if (!stream) {
        return 0;
    }
    if (mode == 4) {
        // write
        if (!IsActive((i32)stream)) {
            return 0;
        }
        i32 count = m_base.m_c;
        stream->Write(&count, 4);
        GzCmdNode* node = (GzCmdNode*)m_base.m_4;
        while (node) {
            GzSerCmd* cmd = node->m_8;
            node = node->m_0;
            i32 tag = cmd->GetTag() & 0xff;
            stream->Write(&tag, 4);
            if (!cmd->Serialize(stream, 4, a3, a4)) {
                return 0;
            }
        }
        return 1;
    }
    if (mode != 7) {
        return 1;
    }
    // read
    if (!IsActive2((i32)stream)) {
        return 0;
    }
    Clear();
    i32 count;
    stream->Read(&count, 4);
    u32 idx = 0;
    if ((u32)count == 0) {
        return 1;
    }
    do {
        i32 tag;
        stream->Read(&tag, 4);
        GzSerCmd* cmd;
        if (tag == 1) {
            cmd = (GzSerCmd*)CGruntzSingleCommand::Allocate();
        } else if (tag == 2) {
            cmd = (GzSerCmd*)CGruntzMultiCommand::Allocate();
        } else {
            return 0;
        }
        if (!cmd->Serialize(stream, 7, a3, a4)) {
            return 0;
        }
        m_base.AddTail(cmd);
        idx++;
    } while (idx < (u32)count);
    return 1;
}

// ---------------------------------------------------------------------------
// IsActive: predicate. If the enable flag is clear, return 0;
// otherwise return whether the registry's multiplayer slot (+0x30) is set.
// ---------------------------------------------------------------------------
RVA(0x00024a90, 0x20)
i32 CGruntzCmdMgr::IsActive(i32 enable) {
    if (!enable) {
        return 0;
    }
    return g_mgrSettings->m_world != 0;
}

// ---------------------------------------------------------------------------
// the destructor (/GX EH frame). Body: ClearAndReset() (null the
// manager + drain everything); the compiler then runs the implicit member
// teardown (~m_1c then ~m_base, reverse declaration order), each an inline
// ~CObList. The EH frame tracks which sub-object is live across the teardown.
RVA(0x00085bd0, 0x56)
CGruntzCmdMgr::~CGruntzCmdMgr() {
    ClearAndReset();
}

SIZE_UNKNOWN(GzGameReg);
