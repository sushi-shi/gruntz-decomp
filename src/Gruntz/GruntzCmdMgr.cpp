// GruntzCmdMgr.cpp - the command TU (C:\Proj\Gruntz), interval 0x0239d0-0x024ae0
// (+ two out-of-band strays). ONE original TU per docs/exe-map/interval-dossiers.md
// #1: our gruntzcmdmgr + gruntzcommand units were slices of this single file - the
// block order is a cmdmgr | command | cmdmgr SANDWICH (A-B-A is impossible for two
// objs at first link), the init-fragment runs bracket the interval, and the manager
// enqueues/serializes exactly these command objects.
//
// CGruntzCmdMgr is the per-game command/target queue the game manager owns
// (CGruntzMgr+0x6c; BroadcastCmd 0x093460 dispatches into it) - see
// include/Gruntz/GruntzCmdMgr.h for the layout. CGruntzCommand +
// CGruntzSingleCommand / CGruntzMultiCommand are the command-pattern family it
// queues - see include/Gruntz/GruntzCommand.h. Names are placeholders; offsets +
// code bytes are load-bearing.
//
// The manager destructor (0x085bd0) carries a /GX EH frame (the inline CPtrList
// teardown is the destructible sub-object); this TU is built flags="eh".
#include <Mfc.h>        // afx-first umbrella (windows.h for the 0x92ab0 DialogProc)
#include <Gruntz/WwdGameRegPtr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/GruntzCmdMgr.h>
#include <Gruntz/GruntzCommand.h>
#include <Gruntz/State.h>         // CState::Update (slot 4) - the live state's id tag
#include <Gruntz/Play.h>          // CPlay::ExecCommand - the ApplyOne/ApplyMask target
#include <Gruntz/SerialArchive.h> // the shared archive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/WwdGameReg.h>    // the canonical WwdGameReg singleton (g_gameReg)
#include <Gruntz/GruntzMgr.h>     // the m_38 manager back-ptr (CGruntzMgr) + m_world chain
#include <Gruntz/GameLevel.h>     // CGameLevel (m_world->m_level: m_planeCtx + m_mainPlane)
#include <rva.h>

// The game registry singleton (canonical <Gruntz/WwdGameReg.h>). The command
// Save/Load paths and the manager's IsActive/IsActive2 predicates all gate on its
// +0x30 active-game slot (m_world) being non-null; this TU only null-tests it, so
// the m_world facet type is irrelevant here. (The former separate MgrSettings30
// view of the same singleton is dissolved onto this one canonical extern.)

// the registry-active predicate the read pass gates on (0x024ac0, an out-of-line
// free __stdcall twin of the IsActive member at 0x024a90; same body). Defined below
// (re-homed from src/Stub/BoundaryMisc.cpp, where it was the HasMgrSlot30 placeholder).
i32 __stdcall IsActive2(void* enable);

// The slot-0 `??_G` scalar-deleting destructor (0x24330) is reconstructed as
// CGruntzCommand::ScalarDtor below (in ascending-RVA order, after 0x24220).

// Out-of-line vtable anchors (the slots NOT reconstructed here) so the
// CGruntzCommand vftable is emitted in this TU (the ctor/dtor reference it).
// Bodies are placeholders.
i32 CGruntzCommand::Serialize(CSerialArchive*, i32, i32, i32) {
    return 1;
}
i32 CGruntzCommand::Save(CSerialArchive*) {
    return 0;
}
i32 CGruntzCommand::Load(CSerialArchive*) {
    return 0;
}
i32 CGruntzCommand::GetTag() {
    return 0;
}
i32 CGruntzCommand::Parse(void*, i32) {
    return 0;
}

// NOT matched (see report): the two 7-byte void-thiscall vtable-set helpers at
// 0x0242f0 / 0x024430 are reconstructed below as the out-of-line base-dtor
// materializations (CGruntzCommand_0242f0 / _024430).

// 0x0239d0 - install the manager pointer; returns 1. Homed out-of-line (matcher-5).
RVA(0x000239d0, 0xf)
i32 CGruntzCmdMgr::SetMgr(CGruntzMgr* mgr) {
    m_38 = mgr;
    return 1;
}

// 0x0239f0 - null the manager then drain everything (tail-calls Clear). Out-of-line.
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
    CState* sp = m_38->m_curState;
    // slot 4 (+0x10): the state reports its own id tag. 0x11 IS GAMESTATE_NONE (the
    // PerFrameTick sentinel), not the PLAY id - the local's historical name is kept.
    i32 isPlay = (sp->Update() == GAMESTATE_NONE);
    GzTargetObj* table[4];
    table[0] = 0;
    table[1] = 0;
    table[2] = 0;
    table[3] = 0;
    i32 i;
    for (i = 0; i < m_base.GetCount(); i++) {
        POSITION pos = m_base.FindIndex(i);
        GzTargetObj* obj = *(GzTargetObj**)((char*)pos + 8);
        i32 flags = obj->m_submitted; // +0x0c submit-context latch
        if (!(flags & 2)) {
            if (!(flags & 1)) {
                continue;
            }
            if (static_cast<u8>(obj->m_6) != static_cast<u32>(param)) {
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
    for (i32 i = 0; i < m_base.GetCount(); i++) {
        POSITION pos = m_base.FindIndex(i);
        GzTargetObj* obj = *(GzTargetObj**)((char*)pos + 8);
        if (obj->m_6 == static_cast<u8>(typeByte) && obj->m_4 == static_cast<u8>(indexByte)) {
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
    while (m_base.GetCount()) {
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
        if (m_38->m_curState->Update() == GAMESTATE_PLAY) {
            ((CGruntzCommand*)cmd)->m_submitted = 2; // submit-context = playing
        } else if (m_38->m_curState->Update() == GAMESTATE_NONE) {
            ((CGruntzCommand*)cmd)->m_submitted = 4; // submit-context = ready
        }
        m_1c.AddTail(cmd);
    }
    m_base.AddTail(cmd);
}

// ---------------------------------------------------------------------------
// 0x23d90 (dossier seam: gamekeyhandler -> this TU; sits inside the sandwich):
// snap a draw rectangle to the 0x20 tile grid and dispatch a blit - the command-
// target tile marker. `this` IS
// this TU's CGruntzCmdMgr - CPlay::DispatchKey dispatches it on [CGruntzMgr+0x6c]
// == m_cmdSubMgr, and the view's m_38->m_30->m_24 chain IS m_38 (the manager
// back-ptr EnqueueCommand already walks) ->m_world->m_level - the same
// m_planeCtx/m_mainPlane walk as the DispatchKey P/x cheat keys. The blit
// primitive reached through ILT thunk 0x2095 (__stdcall, callee-clean).
void __stdcall Func2095(i32, i32, i32, i32, i32, i32, i32, i32);
// @early-stop
// scheduling wall (~50%): logic exact, but retail interleaves the sx/sy compute
// sharing the R/P loads and snaps sx with a byte `and al,0xe0` (proven high bits 0)
// vs our full `and eax,~0x1f`; our /O2 evaluates sy fully then sx and pushes args
// eagerly. Pure x86 instruction scheduling/regalloc.
RVA(0x00023d90, 0x64)
void CGruntzCmdMgr::BlitTileMarker(i32 a1, i32 a2, i32 x, i32 y, i32 a5) {
    CGameLevel* p = m_38->m_world->m_level;
    CLevelPlane* r = p->m_mainPlane;
    i32 sx = ((r->m_originX - p->m_planeCtx.minX + (x & 0xffff)) & ~0x1f) + 0x10;
    i32 sy = ((r->m_originY - p->m_planeCtx.minY + (y & 0xffff)) & ~0x1f) + 0x10;
    Func2095(a1, a2, 0, 0, sx, sy, 0, a5);
}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetParams() - 0x023e20 (vtable slot 4). The base "set the
// five scalar params" implementation: store kind/sub bytes + two words, return
// 1. Inherited unchanged by both leaves (slot 4 = this in all three vtables).
// ---------------------------------------------------------------------------
RVA(0x00023e20, 0x2f)
i32 CGruntzCommand::SetParams(char a0, char a1, char a2, i16 a3, i16 a4) {
    m_4 = a0;
    m_5 = a1;
    m_6 = a2;
    m_8 = a3;
    m_a = a4;
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetParamsEx() - 0x023e60. Delegate the five scalar params to
// the base SetParams (a direct, devirtualized call), then store the +0x10/+0x11
// byte pair; return 1 (or 0 if SetParams failed).
// ---------------------------------------------------------------------------
RVA(0x00023e60, 0x42)
i32 CGruntzCommand::SetParamsEx(char a0, char a1, char a2, i16 a3, i16 a4, char a5, char a6) {
    if (!CGruntzCommand::SetParams(a0, a1, a2, a3, a4)) {
        return 0;
    }
    m_10 = a5;
    m_11 = a6;
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::SetMaskFromList() - 0x023ed0. Validate (buf!=0, count<=0x10),
// delegate the five scalar params to SetParams, then OR a 16-bit flag mask at
// +0x10 from `count` indices in `buf` (m_10word |= 1<<buf[i]). Returns 1/0.
// ---------------------------------------------------------------------------
RVA(0x00023ed0, 0x83)
i32 CGruntzCommand::SetMaskFromList(char a0, char a1, char a2, i16 a3, i16 a4, i32 count, u8* buf) {
    if (!buf) {
        return 0;
    }
    if (static_cast<u8>(count) > 0x10) {
        return 0;
    }
    if (!CGruntzCommand::SetParams(a0, a1, a2, a3, a4)) {
        return 0;
    }
    *(u16*)&m_10 = 0;
    for (i32 i = 0; i < (count & 0xff); i++) {
        *(u16*)&m_10 |= g_cmdBitTable[buf[i]];
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Parse() - 0x023f90 (vtable slot 7). The inverse of Pack:
// deserialize the flat byte buffer back into the five scalar params + m_10, plus the
// conditional m_11 byte (when m_5 >= 8). The tag byte is skipped. Returns the byte
// count consumed. The `len` arg is unused (the record is self-delimiting).
// @early-stop
// walker-register coin-flip (~86.4%, docs/patterns/zero-register-pinning.md family):
// the running-pointer deserialize is byte-faithful instruction-for-instruction (the
// tag skip, the m_4/m_5/m_6 byte reads, the m_8/m_a word reads, m_10, the m_5>=8
// m_11 tail, and the buf-start byte count), verified vs target with sema disasm
// --diff. The sole residual is which register holds the running buffer pointer:
// retail loads `data` into eax BEFORE `push esi` and walks eax (esi=start copy); cl
// pins the walker in esi and derives eax, cascading the modrm register field through
// every read. Not source-steerable (three spellings + the permuter: no change).
// ---------------------------------------------------------------------------
RVA(0x00023f90, 0x48)
i32 CGruntzSingleCommand::Parse(void* data, i32 /*len*/) {
    char* buf = (char*)data + 1; // skip the tag byte
    m_4 = *buf++;
    m_5 = *buf++;
    m_6 = *buf++;
    m_8 = *(i16*)buf;
    buf += 2;
    m_a = *(i16*)buf;
    buf += 2;
    m_10 = *buf++;
    // read m_5 as an unsigned byte (retail `cmp byte,8; jb`) via the &-address form so
    // the codegen is identical to an unsigned member conversion without a member cast.
    if (*(u8*)&m_5 >= 8) {
        m_11 = *buf++;
    }
    return buf - (char*)data;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Parse() - 0x024000 (vtable slot 7). The multi-target twin: the
// +0x10 field is read as a full 16-bit flag mask (a WORD) rather than the conditional
// byte pair. Returns the byte count consumed.
// @early-stop
// walker-register coin-flip (~83.2%): same wall as CGruntzSingleCommand::Parse above -
// the running-pointer deserialize is byte-faithful except the walker register (retail
// eax, cl esi). Not source-steerable.
// ---------------------------------------------------------------------------
RVA(0x00024000, 0x3e)
i32 CGruntzMultiCommand::Parse(void* data, i32 /*len*/) {
    char* buf = (char*)data + 1; // skip the tag byte
    m_4 = *buf++;
    m_5 = *buf++;
    m_6 = *buf++;
    m_8 = *(i16*)buf;
    buf += 2;
    m_a = *(i16*)buf;
    buf += 2;
    *(i16*)&m_10 = *(i16*)buf;
    buf += 2;
    return buf - (char*)data;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Pack() - 0x024050. Serialize the command into a flat
// byte buffer: the tag byte (vtable slot 6), the kind/sub/aux byte triple
// (m_4/m_5/m_6), the two words (m_8/m_a), the m_10 byte, and - only when the kind
// byte m_5 is >= 8 - the m_11 byte. Returns the byte count written. The buffer
// pointer walks with single-byte post-increments then word stores (the natural
// running-pointer codegen).
// ---------------------------------------------------------------------------
RVA(0x00024050, 0x57)
i32 CGruntzSingleCommand::Pack(char* buf, i32 /*unused*/) {
    char* start = buf;
    *buf = static_cast<char>(GetTag());
    *++buf = m_4;
    *++buf = m_5;
    *++buf = m_6;
    char* w = buf + 1;
    *(i16*)w = m_8;
    w += 2;
    *(i16*)w = m_a;
    w += 2;
    *w = m_10;
    w++;
    if (static_cast<u8>(m_5) >= 8) {
        *w = m_11;
        w++;
    }
    return w - start;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Pack() - 0x0240d0. The multi-target twin: same tag + the
// five scalar params, but the +0x10 field is written as a full 16-bit flag mask
// (a WORD) rather than the conditional byte pair. Returns the byte count written.
// ---------------------------------------------------------------------------
RVA(0x000240d0, 0x4d)
i32 CGruntzMultiCommand::Pack(char* buf, i32 /*unused*/) {
    char* start = buf;
    *buf = static_cast<char>(GetTag());
    *++buf = m_4;
    *++buf = m_5;
    *++buf = m_6;
    char* w = buf + 1;
    *(i16*)w = m_8;
    w += 2;
    *(i16*)w = m_a;
    w += 2;
    *(i16*)w = *(i16*)&m_10;
    w += 2;
    return w - start;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::ApplyOne() - 0x024140. If p!=0, unpack the params and call the
// CPlay executor once with index slot = m_10. Returns its result (0 if p==0).
// ---------------------------------------------------------------------------
RVA(0x00024140, 0x35)
i32 CGruntzCommand::ApplyOne(CPlay* p) {
    if (!p) {
        return 0;
    }
    // ExecCommand's narrow (char/i16) params reproduce retail's mov al/mov dx
    // member loads (the ex-CGruntzCmdTarget shim is dissolved; one signature).
    return p->ExecCommand(m_4, m_10, m_5, m_8, m_a, m_11, m_6);
}

// ---------------------------------------------------------------------------
// CGruntzCommand::ApplyMask() - 0x024190. For each of the 16 bit positions set
// in the +0x10 flag mask, call the executor with that index; AND the results
// (return 1 only if every call succeeded; 0 if p==0).
// ---------------------------------------------------------------------------
RVA(0x00024190, 0x6c)
i32 CGruntzCommand::ApplyMask(CPlay* p) {
    if (!p) {
        return 0;
    }
    i32 ok = 1;
    for (i32 i = 0; i < 16; i++) {
        if (g_cmdBitTable[i] & *(u16*)&m_10) {
            if (!p->ExecCommand(m_4, static_cast<char>(i), m_5, m_8, m_a, 0, m_6)) {
                ok = 0;
            }
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Allocate() - 0x024220.
//
// new-or-recycle allocator: if the per-class recycle list is non-empty
// (g_singleCmdCount != 0), tail-call g_singleCmdList.RemoveTail() to reuse a
// node; otherwise operator new(0x14) a fresh object and (if non-null) run the
// inline ctor, which only stamps the leaf vftable. Returns the object (or 0).
// ---------------------------------------------------------------------------
RVA(0x00024220, 0x2b)
CGruntzSingleCommand* CGruntzSingleCommand::Allocate() {
    if (g_singleCmdCount) {
        return (CGruntzSingleCommand*)g_singleCmdList.RemoveTail();
    }
    return new CGruntzSingleCommand;
}

// CGruntzSingleCommand::Vslot05 (0x24260) - the single-target slot-5 override; returns 1.
RVA(0x00024260, 0x6)
i32 CGruntzSingleCommand::Vslot05() {
    return 1;
}

// The CGruntzCommand base vftable (0x5e9674; == ??_7CGruntzCommand@@6B@ this TU
// emits). The two out-of-line stamps store it into [this]; the operand reloc-
// masks against the base vftable via this DATA-named extern (re-homed from
// src/Stub/CGruntzCommand.cpp).

// CGruntzCommand::CGruntzCommand_0242f0 (0x000242f0) - out-of-line base-vtable
// restore called by CGruntzSingleCommand's scalar-deleting dtor (??_G @0x242c0).
// The explicit inline base-dtor call lowers to `mov [ecx],&??_7CGruntzCommand; ret`.
RVA(0x000242f0, 0x7)
void CGruntzCommand::CGruntzCommand_0242f0() {
    this->CGruntzCommand::~CGruntzCommand();
}

// CGruntzCommand::Vslot05 (0x24310) - base slot-5 default; returns 1 (role unrecovered).
RVA(0x00024310, 0x6)
i32 CGruntzCommand::Vslot05() {
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCommand::ScalarDtor - the slot-0 `??_G` scalar-deleting destructor
// (0x24330). Run the trivial ~CGruntzCommand (inlines to the vptr re-stamp), then -
// when the low bit of the hidden flags arg is set - RezFree(this); return this.
// Hand-written non-virtual + RVA pin (the CFileImageSurface::ScalarDelete pattern)
// so the body emits and matches (MSVC's own synthesized ??_G is a separate symbol).
// The scalar-deleting-dtor free is the global ::operator delete (0x1b9b82 =
// ??3@YAXPAX@Z, NAFXCW); the `::` forces the global over any CObject member delete.
RVA(0x00024330, 0x20)
void* CGruntzCommand::ScalarDtor(u32 flags) {
    this->CGruntzCommand::~CGruntzCommand();
    if (flags & 1) {
        ::operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Allocate() - 0x024360. Same shape, Multi list/vftable.
// ---------------------------------------------------------------------------
RVA(0x00024360, 0x2b)
CGruntzMultiCommand* CGruntzMultiCommand::Allocate() {
    if (g_multiCmdCount) {
        return (CGruntzMultiCommand*)g_multiCmdList.RemoveTail();
    }
    return new CGruntzMultiCommand;
}

// CGruntzMultiCommand::Vslot05 (0x243a0) - the multi-target slot-5 override; returns 1.
RVA(0x000243a0, 0x6)
i32 CGruntzMultiCommand::Vslot05() {
    return 1;
}

// CGruntzCommand::CGruntzCommand_024430 (0x00024430) - out-of-line base-vtable
// restore called by CGruntzMultiCommand's scalar-deleting dtor (??_G @0x24400).
RVA(0x00024430, 0x7)
void CGruntzCommand::CGruntzCommand_024430() {
    this->CGruntzCommand::~CGruntzCommand();
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::FreeAll() - 0x024450. Drain the per-class recycle list
// (g_singleCmdList): while non-empty, RemoveTail() a node and `delete` it (the
// virtual scalar-deleting dtor with flag 1). A static method (no this). (Retail
// FLIRT-misattributed this RVA as NetUnattributed_024450; it is the single-cmd
// twin of CGruntzMultiCommand::FreeAll below.)
// ---------------------------------------------------------------------------
RVA(0x00024450, 0x29)
void CGruntzSingleCommand::FreeAll() {
    while (g_singleCmdCount) {
        CGruntzCommand* node = (CGruntzCommand*)g_singleCmdList.RemoveTail();
        if (node) {
            delete node;
        }
    }
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::FreeAll() - 0x024490. Drain the per-class recycle list:
// while it is non-empty, RemoveTail() a node and `delete` it (the virtual
// scalar-deleting dtor with flag 1). A static method (no this).
// ---------------------------------------------------------------------------
RVA(0x00024490, 0x29)
void CGruntzMultiCommand::FreeAll() {
    while (g_multiCmdCount) {
        CGruntzCommand* node = (CGruntzCommand*)g_multiCmdList.RemoveTail();
        if (node) {
            delete node;
        }
    }
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Serialize() - 0x0244d0 (vtable slot 1). The (de)serialize
// dispatcher: bail on a null stream; on mode 4 drive Save (slot 2), on mode 7
// drive Load (slot 3), both through the vtable; return 1 (or the sub-call's 0 on
// gate failure). Multi's identical twin is 0x0246c0.
// ---------------------------------------------------------------------------
RVA(0x000244d0, 0x3b)
i32 CGruntzSingleCommand::Serialize(CSerialArchive* s, i32 mode, i32, i32) {
    if (!s) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!Save(s)) {
                return 0;
            }
            break;
        case 7:
            if (!Load(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Save() - 0x024520 (vtable slot 2). Network-serialize the 8
// scalar fields out through the stream's Write (vtable slot +0x30). No-op (return 0)
// unless the stream is non-null AND the registry's active-game gate (g_gameReg->
// m_world) is set. The base declares Save pure; this is the single-target override.
// ---------------------------------------------------------------------------
RVA(0x00024520, 0x98)
i32 CGruntzSingleCommand::Save(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
        return 0;
    }
    s->Write(&m_4, 1);
    s->Write(&m_5, 1);
    s->Write(&m_6, 1);
    s->Write(&m_8, 2);
    s->Write(&m_a, 2);
    s->Write(&m_submitted, 4);
    s->Write(&m_10, 1);
    s->Write(&m_11, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzSingleCommand::Load() - 0x0245f0 (vtable slot 3). The read twin: same
// gate, the 8 scalar fields read back through the stream's Read (vtable slot +0x2c).
// ---------------------------------------------------------------------------
RVA(0x000245f0, 0x98)
i32 CGruntzSingleCommand::Load(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
        return 0;
    }
    s->Read(&m_4, 1);
    s->Read(&m_5, 1);
    s->Read(&m_6, 1);
    s->Read(&m_8, 2);
    s->Read(&m_a, 2);
    s->Read(&m_submitted, 4);
    s->Read(&m_10, 1);
    s->Read(&m_11, 1);
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Serialize() - 0x0246c0 (vtable slot 1). The multi-target
// twin of CGruntzSingleCommand::Serialize: identical dispatcher body, but drives
// this class's own overridden Save/Load (slots 2/3).
// ---------------------------------------------------------------------------
RVA(0x000246c0, 0x3b)
i32 CGruntzMultiCommand::Serialize(CSerialArchive* s, i32 mode, i32, i32) {
    if (!s) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!Save(s)) {
                return 0;
            }
            break;
        case 7:
            if (!Load(s)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Save() - 0x024710 (vtable slot 2). The multi-target write
// twin: same gate/shape as the base Save, but writes the +0x10 field as one
// 16-bit value (the multi-command's flag mask) instead of the m_10/m_11 byte pair.
// ---------------------------------------------------------------------------
RVA(0x00024710, 0x8b)
i32 CGruntzMultiCommand::Save(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
        return 0;
    }
    s->Write(&m_4, 1);
    s->Write(&m_5, 1);
    s->Write(&m_6, 1);
    s->Write(&m_8, 2);
    s->Write(&m_a, 2);
    s->Write(&m_submitted, 4);
    s->Write(&m_10, 2);
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzMultiCommand::Load() - 0x0247d0 (vtable slot 3). Same gate/shape as the
// base Load, but reads the +0x10 field as one 16-bit value (the multi-command's
// flag mask) instead of the m_10/m_11 byte pair.
// ---------------------------------------------------------------------------
RVA(0x000247d0, 0x8b)
i32 CGruntzMultiCommand::Load(CSerialArchive* s) {
    if (!s) {
        return 0;
    }
    if (!g_gameReg->m_world) {
        return 0;
    }
    s->Read(&m_4, 1);
    s->Read(&m_5, 1);
    s->Read(&m_6, 1);
    s->Read(&m_8, 2);
    s->Read(&m_a, 2);
    s->Read(&m_submitted, 4);
    s->Read(&m_10, 2);
    return 1;
}

// ---------------------------------------------------------------------------
// CGruntzCmdMgr::Serialize - 0x024890: (de)serialize the command queue through a
// stream.
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
        i32 count = m_base.GetCount();
        stream->Write(&count, 4);
        GzCmdNode* node =
            (GzCmdNode*)
                m_base.GetHeadPosition(); // MFC-protected m_pNodeHead via the inline accessor
        while (node) {
            CGruntzCommand* cmd = node->m_8;
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
    if (!IsActive2(stream)) {
        return 0;
    }
    Clear();
    i32 count;
    stream->Read(&count, 4);
    u32 idx = 0;
    if (static_cast<u32>(count) == 0) {
        return 1;
    }
    do {
        i32 tag;
        stream->Read(&tag, 4);
        CGruntzCommand* cmd;
        if (tag == 1) {
            cmd = CGruntzSingleCommand::Allocate();
        } else if (tag == 2) {
            cmd = CGruntzMultiCommand::Allocate();
        } else {
            return 0;
        }
        if (!cmd->Serialize(stream, 7, a3, a4)) {
            return 0;
        }
        m_base.AddTail(cmd);
        idx++;
    } while (idx < static_cast<u32>(count));
    return 1;
}

// ---------------------------------------------------------------------------
// IsActive: predicate. If the enable flag is clear, return 0;
// otherwise return whether the registry's active-game slot (+0x30) is set.
// ---------------------------------------------------------------------------
RVA(0x00024a90, 0x20)
i32 CGruntzCmdMgr::IsActive(i32 enable) {
    if (!enable) {
        return 0;
    }
    return g_gameReg->m_world != 0;
}

// 0x24ac0 (re-homed from src/Stub/BoundaryMisc.cpp): IsActive2 - the free __stdcall
// twin of IsActive above, called from Serialize's mode-7 read gate. Null arg -> 0,
// else the same g_gameReg->m_world predicate. __stdcall, ret 4.
RVA(0x00024ac0, 0x20)
i32 __stdcall IsActive2(void* enable) {
    if (enable == 0) {
        return 0;
    }
    return g_gameReg->m_world != 0;
}

// ---------------------------------------------------------------------------
// the destructor (0x085bd0, out-of-band stray; /GX EH frame). Body:
// ClearAndReset() (null the manager + drain everything); the compiler then runs
// the implicit member teardown (~m_1c then ~m_base, reverse declaration order),
// each an inline ~CPtrList. The EH frame tracks which sub-object is live across
// the teardown.
RVA(0x00085bd0, 0x56)
CGruntzCmdMgr::~CGruntzCmdMgr() {
    ClearAndReset();
}

// The 1<<i bit table (0x5e9608 = RVA 0x1e9608) the mask builder/scanner indexes.
// Bound via @data-symbol (not DATA): this const u16[] is DEFINED here, and clang
// mangles a const array with the `Q` storage class while cl5's reloc/definition
// uses `?g_cmdBitTable@@3PBGB` (PB) - @data-symbol names the exact cl5 symbol so the
// three DIR32 mask-loop operands reloc-pair.
// @data-symbol: ?g_cmdBitTable@@3PBGB 0x001e9608
const u16 g_cmdBitTable[16] = {
    1,
    2,
    4,
    8,
    0x10,
    0x20,
    0x40,
    0x80,
    0x100,
    0x200,
    0x400,
    0x800,
    0x1000,
    0x2000,
    0x4000,
    0x8000
};

// The two command recycle-list globals + their non-empty counts (0x22b5d0 region,
// .bss). Bound to their real RVAs here (a DATA() in GruntzCommand.h is invisible to
// labels.py's per-.cpp text scan). Allocate/FreeAll gate on the count and RemoveTail
// a node off the list to reuse it.
// The two count globals are owned here (.bss); real definitions, canonical externs
// in <Gruntz/GruntzCommand.h>. (The g_*CmdList recycle heads stay extern - shared.)
DATA(0x0022b5dc)
i32 g_singleCmdCount;
DATA(0x0022b5d0)
extern CGruntzCmdList g_singleCmdList;
DATA(0x0022b64c)
i32 g_multiCmdCount;
DATA(0x0022b640)
extern CGruntzCmdList g_multiCmdList;

// ---------------------------------------------------------------------------
// The numeric settings DialogProc (0x092ab0, out-of-band stray; re-homed from the
// ApiCaller stubs): CGruntzCommand::ApplyOne / ApplyMask (0x024140/0x024190)
// drive the command dialog it backs. WM_INITDIALOG loads the 12 cached numeric
// edit fields into controls 0x4db..0x4e6; IDOK reads them back; IDCANCEL closes.
// The 12 field caches are cluster-local globals (moved here with the DlgProc).
// ---------------------------------------------------------------------------
// DEFINED HERE (storage, .bss zero-init), RVA-ascending. Nothing in the tree defined
// these twelve: this TU extern-bound them while CRezSync::Init reset the same addresses
// under its own C++-linkage hex spellings (g_6451a4/g_645268/...), so BOTH names were
// unresolved externals - one variable, two symbols. RezSync now references these names.
//
// OWNER: this TU. The dialog is the only code that gives each cell an individual
// identity (one dedicated edit control apiece, 0x4da..0x4e9); RezSync only bulk-resets
// the band. No .bute key names them, so the role name (`dlgVal`) is all the binary
// proves - the numeric value cached behind edit control N. Not invented, but not a
// recovered dev name either.
DATA(0x002451a4)
i32 g_dlgVal_6451a4;
DATA(0x00245268)
i32 g_dlgVal_645268;
DATA(0x0024526c)
i32 g_dlgVal_64526c;
DATA(0x002452a8)
i32 g_dlgVal_6452a8;
DATA(0x002452d0)
i32 g_dlgVal_6452d0;
DATA(0x002452d4)
i32 g_dlgVal_6452d4;
DATA(0x00245538)
i32 g_dlgVal_645538;
DATA(0x00245558)
i32 g_dlgVal_645558;
DATA(0x0024555c)
i32 g_dlgVal_64555c;
DATA(0x00245560)
i32 g_dlgVal_645560;
DATA(0x00245564)
i32 g_dlgVal_645564;
DATA(0x00245568)
i32 g_dlgVal_645568;
RVA(0x00092ab0, 0x20d)
i32 CALLBACK winapi_092ab0_EndDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case 0x110:
            SetDlgItemInt(hDlg, 0x4db, g_dlgVal_64526c, 0);
            SetDlgItemInt(hDlg, 0x4da, g_dlgVal_6452d0, 0);
            SetDlgItemInt(hDlg, 0x4dc, g_dlgVal_645268, 0);
            SetDlgItemInt(hDlg, 0x4dd, g_dlgVal_645568, 0);
            SetDlgItemInt(hDlg, 0x4de, g_dlgVal_645538, 0);
            SetDlgItemInt(hDlg, 0x4df, g_dlgVal_6451a4, 0);
            SetDlgItemInt(hDlg, 0x4e0, g_dlgVal_6452d4, 0);
            SetDlgItemInt(hDlg, 0x4e9, g_dlgVal_6452a8, 0);
            SetDlgItemInt(hDlg, 0x4e3, g_dlgVal_645558, 0);
            SetDlgItemInt(hDlg, 0x4e4, g_dlgVal_645560, 0);
            SetDlgItemInt(hDlg, 0x4e5, g_dlgVal_64555c, 0);
            SetDlgItemInt(hDlg, 0x4e6, g_dlgVal_645564, 0);
            return 1;
        case 0x111:
            if (wParam == 2) {
                EndDialog(hDlg, 0);
                return 1;
            }
            if (wParam == 1) {
                g_dlgVal_64526c = GetDlgItemInt(hDlg, 0x4db, 0, 0);
                g_dlgVal_6452d0 = GetDlgItemInt(hDlg, 0x4da, 0, 0);
                g_dlgVal_645268 = GetDlgItemInt(hDlg, 0x4dc, 0, 0);
                g_dlgVal_645568 = GetDlgItemInt(hDlg, 0x4dd, 0, 0);
                g_dlgVal_645538 = GetDlgItemInt(hDlg, 0x4de, 0, 0);
                g_dlgVal_6451a4 = GetDlgItemInt(hDlg, 0x4df, 0, 0);
                g_dlgVal_6452d4 = GetDlgItemInt(hDlg, 0x4e0, 0, 0);
                g_dlgVal_6452a8 = GetDlgItemInt(hDlg, 0x4e9, 0, 0);
                g_dlgVal_645558 = GetDlgItemInt(hDlg, 0x4e3, 0, 0);
                g_dlgVal_645560 = GetDlgItemInt(hDlg, 0x4e4, 0, 0);
                g_dlgVal_64555c = GetDlgItemInt(hDlg, 0x4e5, 0, 0);
                g_dlgVal_645564 = GetDlgItemInt(hDlg, 0x4e6, 0, 0);
                EndDialog(hDlg, 1);
                return 1;
            }
            break;
    }
    return 0;
}
