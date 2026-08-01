#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <rva.h>
#include <new> // Rez heap throwing operator new / nothrow delete (0x1b9b46 / 0x1b9b82)

#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileGridCommand.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h> // CTileTriggerLogic + the per-id leaves AddLogic news
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerWiring.h>
#include <DDrawMgr/DDSurface.h>        // CDDSurface::m_ddSurface (the COM GetDC/ReleaseDC pair)
#include <DDrawMgr/DDrawSubMgrPages.h> // CDDrawSubMgrPages front/back pages + CDrawSubWorker
#include <DDrawMgr/DDrawSurfacePair.h> // CDDrawSurfacePair (m_backPair; needs the complete type to upcast)
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSurfaceMgr::m_drawTarget
#include <Gruntz/FontConfig.h>        // CFontConfig::Draw3DText (g_gameReg->m_chatLog)
#include <Gruntz/GameLevel.h> // CGameLevel/CDDrawWorkerHost/CTileImageSet (the id-21 board latch)

RVA(0x000c8640, 0x70)
CTileTriggerContainer::~CTileTriggerContainer() {
    DtorBase();
    // m_list3 / m_list2 / m_list1 / m_base are auto-destroyed here (reverse decl
    // order) by the compiler-emitted /GX member teardown.
}

// ---------------------------------------------------------------------------
// DrawPageDebugText (0x115b60) - the GDI "draw 3D text onto a draw-target page's
// DirectDraw surface" debug helper. Free __cdecl (retail reads arg0 at [esp+0x4]
// BEFORE any push and ends in a bare `ret`), 8 args, returns 0 on every null gate
// and 1 on success. Homed from GapFunctions.cpp by RVA neighbourhood (this TU's
// .text brackets it).
//
// IDENTITY RESOLVED 2026-07-29 (was an @identity-TODO parked on "arg0's CLASS is
// unrecoverable ... it is a ZERO-REF orphan, so there is no call site to type arg0
// from"). arg0 never needed a caller - its own field chain names it. The three
// offsets the body walks are `[arg0+0x04] -> [+0x10] / [+0x14] -> [+0x2c]`, and
// exactly one chain in the tree has that shape:
//   +0x04  CDDrawSurfaceMgr::m_drawTarget   (CDDrawSubMgrPages*)
//   +0x10  CDDrawSubMgrPages::m_frontPair   (CDDrawSurfaceChildA*)
//   +0x14  CDDrawSubMgrPages::m_backPair    (CDDrawSurfacePair*)
//   +0x2c  CDrawSubWorker::m_surface        (CDDSurface*, the shared base of both)
// so arg0 is a CDDrawSurfaceMgr* and arg4 is the front/back page selector. The +0x2c
// read is legal on EITHER arm precisely because the two page classes are siblings on
// CDrawSubWorker (see DDrawSubMgrPages.h) - no cast is needed, the upcast is implicit.
//
// The COM bracket is <ddraw.h>-exact: the surface is pushed as an explicit stack arg
// (`push &out; push pSurf; call [vtbl+0x44]`), i.e. __stdcall, and on
// IDirectDrawSurface +0x44 is slot 17 GetDC(HDC*) while +0x68 is slot 26 ReleaseDC(HDC).
// The Draw3DText binding is likewise proven, not inferred: the call is ILT 0x140b ->
// `jmp 0x22810` = CFontConfig::Draw3DText, whose ten parameters are exactly what the
// push order yields, with shadow/dx/dy the pushed literals 1/2/3, on g_gameReg->m_5c
// (CFontConfig* m_chatLog).
// @dead-code
// zero-ref (gruntz sema xref --tree): a debug helper retail compiled in and never
// called. That cost it its identity for a while; it does not any more.
RVA(0x00115b60, 0x97)
i32 DrawPageDebugText(
    CDDrawSurfaceMgr* mgr,
    const CString* text,
    RECT* dst,
    i32 fontFlag,
    i32 useFrontPage,
    i32 r,
    i32 g,
    i32 b
) {
    if (mgr == 0) {
        return 0;
    }
    CDrawSubWorker* page;
    if (useFrontPage != 0) {
        page = mgr->m_drawTarget->m_frontPair;
        if (page == 0) {
            return 0;
        }
    } else {
        page = mgr->m_drawTarget->m_backPair;
        if (page == 0) {
            return 0;
        }
    }
    CDDSurface* surf = page->m_surface;
    if (surf == 0) {
        return 0;
    }
    // MSVC reuses the now-dead arg0 slot [esp+8] for this local.
    HDC hdc = 0;
    surf->m_ddSurface->GetDC(&hdc);
    g_gameReg->m_chatLog->Draw3DText(text, hdc, dst, fontFlag, r, g, b, 1, 2, 3);
    surf->m_ddSurface->ReleaseDC(hdc);
    return 1;
}

RVA(0x00115f00, 0x13)
i32 CTileTriggerContainer::GetFlag74() {
    if (m_built != 0) {
        return 0;
    }
    m_built = 1;
    return 1;
}

RVA(0x00115f30, 0x18)
void CTileTriggerContainer::DtorBase() {
    if (m_built != 0) {
        RemoveAll();
        m_built = 0;
    }
}

// ===========================================================================
// CTileTriggerContainer::AddSwitchLogic (0x115f60) - the switch-logic factory, the
// 0x8c-family twin of AddLogic above. Switch on the tag (the retail jump table at
// 0x516240 covers tag-1 in [0,7], i.e. tags 1..8), `new` the matching
// CTileTriggerSwitchLogic subclass, copy the six by-value blocks into one
// contiguous local, hand its address to the object's slot-1 BuildSmall, and on
// success AddTail it into m_base. /GX: one trylevel per `new` (states 0..5, in the
// arm order below), and the failure path inlines `delete obj` as the vptr restamp +
// m_initGate=0 + operator delete.
// @confidence: high
// @source: full-disasm-decode (jump table 0x516240 read out of .rdata; all six ctor
//   ILT thunks resolved; the 9-arg vtable +0x04 call matches BuildSmall exactly)
// @early-stop
// BYTE-EXACT, scored 95.05%. Verified instruction-for-instruction with
// `llvm-objdump -dr` on build/objdiff/base/tiletriggercontainer.obj against the
// retail 0x115f60 bytes: every opcode, every displacement and every EH-state store
// matches; the only base-side differences are unresolved reloc placeholders
// (??2/??3, the six ctors, ??_7CTileTriggerSwitchLogic, __except_list). The residual
// is the known delinker jump-table artifact - the `jmp [eax*4+<table>]` DIR32 points
// at a size-0 duplicate symbol for the .rdata table, so objdiff cannot pair those
// bytes (docs: delinker-jumptable-dup-symbol-undercount). Nothing to fix in source.
RVA(0x00115f60, 0x300)
CTileTriggerSwitchLogic* CTileTriggerContainer::AddSwitchLogic(
    i32 tag,
    i32 col,
    i32 row,
    i32 key,
    // the same six rect blocks AddLogic takes (see its declaration for the offsets)
    RECT extent,
    RECT area,
    RECT switchRect,
    RECT clip,
    RECT switchRectA,
    RECT switchRectB,
    i32 isMatch,
    i32 m120,
    i32 zero
) {
    CTileTriggerSwitchLogic* obj = 0;
    switch (tag) {
        case TRIGID_SWITCH_1:
        case TRIGID_SWITCH_2:
        case TRIGID_SWITCH_5:
            obj = new CTileTriggerSwitchLogic;
            break;
        case TRIGID_MULTI_SWITCH_3:
            obj = new CTileMultiTriggerSwitchLogic;
            break;
        case TRIGID_EXCLUSIVE_SWITCH_4:
            obj = new CTileExclusiveTriggerSwitchLogic;
            break;
        case TRIGID_SECRET_SWITCH_6:
            obj = new CTileSecretTriggerSwitchLogic;
            break;
        case TRIGID_TIME_SWITCH_7:
            obj = new CTileTimeTriggerSwitchLogic;
            break;
        case TRIGID_CHECKPOINT_SWITCH_8:
            obj = new CCheckpointTriggerSwitchLogic;
            break;
    }
    if (obj == 0) {
        return 0;
    }

    RECT local[6];
    local[0] = extent;
    local[1] = area;
    local[2] = switchRect;
    local[3] = clip;
    local[4] = switchRectA;
    local[5] = switchRectB;

    if (obj->BuildSmall(this, tag, col, row, key, local, isMatch, m120, zero) == 0) {
        // inline ~CTileTriggerSwitchLogic (vptr restamp + m_initGate = 0) + ??3
        delete obj;
        return 0;
    }
    m_base.AddTail(obj);
    return obj;
}

RVA(0x00116320, 0x66)
i32 CTileTriggerContainer::RemoveByKeys(i32 k1, i32 k2) {
    POSITION pos = m_base.GetHeadPosition();
    while (pos != 0) {
        POSITION cur = pos;
        CTileTriggerSwitchLogic* data = static_cast<CTileTriggerSwitchLogic*>(m_base.GetNext(pos));
        if (data->m_typeId == k2 && data->m_key1 == k1) {
            // ~CTileTriggerSwitchLogic is non-virtual + inline: the dtor restamps the vptr
            // (`mov [data],offset ??_7`) + clears m_initGate, then ??3 frees it.
            delete data;
            m_base.RemoveAt(cur);
            return 1;
        }
    }
    return 0;
}

// ===========================================================================
// CTileTriggerWiring::AddLogicDefaults  (0x1163b0)
// ===========================================================================
// @early-stop
// Register-allocation wall (topic:regalloc). The forwarder structure is faithful -
// push the four trailing ids, build six in-place zeroed 16-byte param blocks, push
// the five leading ids, tail into AddLogic - and the zeroing-ctor temps reproduce
// retail's shared-zero-register stores (MSVC5 will NOT value-init `CTrigParam()`,
// so the explicit ctor is required: a no-ctor POD copied garbage, 27%->76%). The
// residual is purely which registers cl picks: retail loads the forwarded ids
// THROUGH ebx (reused as the block pointer) and keeps exactly FOUR zero regs
// (eax/edx/esi/edi) live across all six blocks; cl loads through ebp and spends a
// FIFTH zero reg (ebx), so every arg-load/zero-store operand shifts. A pure /O2
// regalloc coin-flip with no source lever. ~75.9%, logic complete; deferred to the
// final sweep.
RVA(0x001163b0, 0xb2)
void CTileTriggerContainer::AddLogicDefaults(
    i32 tileType,
    i32 logicType,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 tileToken,
    i32 dutyOnSpan,
    i32 leadInSpan,
    i32 dutyOffSpan
) {
    AddLogic(
        tileType,
        logicType,
        tileX,
        tileY,
        cellKey,
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        tileToken,
        dutyOnSpan,
        leadInSpan,
        dutyOffSpan
    );
}

RVA(0x001164a0, 0x116)
void CTileTriggerContainer::AddLogicFromRecord(
    i32 tileType,
    i32 logicType,
    CTrigSourceRecord* rec
) {
    AddLogic(
        tileType,
        logicType,
        rec->m_164,
        rec->m_168,
        rec->m_4,
        rec->m_134,
        rec->m_144,
        rec->m_154,
        rec->m_64,
        rec->m_7c->m_f0,
        rec->m_7c->m_100,
        rec->m_124,
        rec->m_120,
        rec->m_118,
        rec->m_128
    );
}

// ===========================================================================
// CTileTriggerContainer::AddLogic  (0x116610)
// ===========================================================================
// The per-id logic-leaf factory the two forwarders above call.  Switch on the
// SECOND arg (logicType, the retail switch reads [esp+0x88]; ids 0x15..0x1a = 21..26,
// the same id space the serialize Build factory 0x117800 uses):
//   0x15/0x18 -> CTileTriggerLogic     (ILT 0x43b3 -> ??0 0x1107f0)  trylevel 0
//   0x19      -> CTileSecretTriggerLogic(ILT 0x310c -> ??0 0x112760)  trylevel 1
//   0x1a      -> CCoveredPowerupLogic  (ILT 0x2a4f -> ??0 0x112240)  trylevel 2
//   0x17      -> CTileTimeTriggerLogic (ILT 0x18de -> ??0 0x112270)  trylevel 3
//   0x16/other-> 0
// Then (when the leaf's m_initGate is clear) copy the six CTrigParam blocks into
// the leaf's m_block, fill the id/owner/clock fields, append it to m_list1 (m_list2
// for id 0x17), and latch id-0x15 board tiles 0x67/0x68 into m_70.  The failure path
// deletes the leaf (inline ~CTileTriggerLogic: ??_7 stamp + m_initGate=0 + RezFree).
// @early-stop  (~70%)
// /GX jump-table + per-`new` trylevel regalloc wall.  Full body + all four ctor / new /
// delete / AddTail / ??_7CTileTriggerLogic / g_frameTime relocs bind - this is what binds
// both forwarders' CALLs (reloc_fidelity tiletriggercontainer -> 0 UNBOUND).  The
// residual is the documented /O2 register-allocation coin-flip (same lever as the
// AddLogicDefaults @early-stop just above): retail keeps exactly THREE callee-saved regs
// (ebx/esi/edi) and reads the switch arg straight through edi (`lea eax,[edi-0x15]`);
// cl picks up ebp as a FIFTH live register, reshapes the frame, and reads the switch
// value from a shifted spill slot (`mov eax,[esp+0x7c]; add eax,-0x15`), so every
// prologue/arg-load operand shifts.  Compounded by MSVC5 tail-merging the four
// independent operator-new EH trylevel state machines differently.  Logic complete;
// byte-match parked for the final sweep.
RVA(0x00116610, 0x32c)
CTileTriggerLogic* CTileTriggerContainer::AddLogic(
    i32 tileType,
    i32 logicType,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    CTrigParam extent,
    CTrigParam area,
    CTrigParam switchRect,
    CTrigParam clip,
    CTrigParam switchRectA,
    CTrigParam switchRectB,
    i32 tileToken,
    i32 dutyOnSpan,
    i32 leadInSpan,
    i32 dutyOffSpan
) {
    CTileTriggerLogic* obj = 0;
    switch (logicType) {
        case TRIGID_TILE_TRIGGER_21:
        case TRIGID_TILE_TRIGGER_24:
            obj = new CTileTriggerLogic;
            break;
        case TRIGID_SECRET_TRIGGER_25:
            obj = new CTileSecretTriggerLogic;
            break;
        case TRIGID_COVERED_POWERUP_26:
            obj = new CCoveredPowerupLogic;
            break;
        case TRIGID_TIME_TRIGGER_23:
            obj = new CTileTimeTriggerLogic;
            break;
    }
    if (obj == 0) {
        return 0;
    }

    CTrigParam local[6];
    local[0] = extent;
    local[1] = area;
    local[2] = switchRect;
    local[3] = clip;
    local[4] = switchRectA;
    local[5] = switchRectB;

    i32 ok = 0;
    if (obj->m_initGate == 0) {
        memcpy(obj->m_block, local, sizeof(local));
        if (obj->m_initGate == 0) {
            obj->m_tileY = tileY;
            obj->m_tileX = tileX;
            obj->m_owner = this;
            obj->m_typeTag = logicType;
            obj->m_10 = cellKey;
            obj->m_initGate = 1;
            obj->m_tileToken = tileToken;
            obj->m_startClock = g_frameTime;
            obj->m_leadInSpan = leadInSpan;
            obj->m_dutyOn = 0;
            obj->m_dutyOnSpan = dutyOnSpan;
            obj->m_dutyOffSpan = dutyOffSpan;
            if (logicType != TRIGID_COVERED_POWERUP_26 && dutyOffSpan == 0) {
                obj->m_dutyOffSpan = dutyOnSpan;
            }
            obj->m_startClock = g_frameTime;
            ok = 1;
        }
    }

    if (ok == 0) {
        delete obj;
        return 0;
    }

    CPtrList* list = logicType == TRIGID_TIME_TRIGGER_23 ? &m_list2 : &m_list1;
    list->AddTail(obj);
    if (logicType == TRIGID_TILE_TRIGGER_21
        && (tileType == TILEKIND_PYRAMID_LATCH_A || tileType == TILEKIND_PYRAMID_LATCH_B)) {
        m_latchedLeaf = obj;
    }
    return obj;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::AddToList3
// Allocates a 0x28-byte mark, constructs it, and (when its init flag is clear)
// fills its fields from the args, back-links it to this container, notifies it,
// and appends it to m_list3.  Returns the mark, or 0 on alloc/double-init failure.
// ---------------------------------------------------------------------------
// @early-stop
// /GX operator-new wall (~43%): the RezAlloc + placement-ctor + exception-cleanup
// trylevel guard around the partially-constructed heap element is not reproducible
// with a plain new (distinct from the member-teardown dtor frame, which IS
// steerable - see eh-dtor-model-members-as-destructible.md); field-fill + Notify +
// AddTail identical.
RVA(0x00116a40, 0xf5)
CTileActionEvent* CTileTriggerContainer::AddToList3(
    i32 actionCode,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 player0,
    i32 player1,
    i32 player2,
    i32 player3
) {
    CTileActionEvent* m = new CTileActionEvent;
    if (m == 0) {
        return 0;
    }
    if (m->m_live != 0) {
        m->m_live = 0;
        ::operator delete(m);
        return 0;
    }
    m->m_tileX = tileX;
    m->m_tileY = tileY;
    m->m_cellKey = cellKey;
    m->m_playerFlags[0] = player0;
    m->m_playerFlags[1] = player1;
    m->m_playerFlags[3] = player3;
    m->m_actionCode = actionCode;
    m->m_owner = this;
    m->m_live = 1;
    m->m_playerFlags[2] = player2;
    m->SetActionCode(actionCode);
    m_list3.AddTail(m);
    return m;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::AddToList3Switch  (0x116b80)
// Twin of AddToList3: allocates+constructs a 0x28-byte mark, and (when its init
// flag is clear) fills its fields from the args, computes the four m_playerFlags words
// from a switch on `playerSlot` (0..3 = that one player, PLAYERSLOT_ALL = all four,
// default = all clear - the same slot space SetCell/MorphByTool use), notifies it, and
// appends it to m_list3.  Returns the mark, or 0 on alloc/double-init failure.
// ---------------------------------------------------------------------------
// @early-stop
// RezAlloc + placement-ctor /GX wall (~49%): twin of AddToList3 - retail carries the
// ctor-in-flight EH frame (push -1/fs:0 + trylevel + shared jmp epilogue) that MSVC5
// won't emit for the RezAlloc+ctor pair; switch flag-fill + Notify + AddTail are
// byte-identical (scores above the no-switch twin AddToList3 at 43%).
// See docs/patterns/rezalloc-placement-new-no-eh-frame.md
RVA(0x00116b80, 0x105)
CTileActionEvent* CTileTriggerContainer::AddToList3Switch(
    i32 actionCode,
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32 playerSlot
) {
    CTileActionEvent* m = new CTileActionEvent;
    if (m == 0) {
        return 0;
    }
    i32 a = 0, b = 0, c = 0, d = 0;
    switch (playerSlot) {
        case 0:
            d = 1;
            break;
        case 1:
            c = 1;
            break;
        case 2:
            b = 1;
            break;
        case 3:
            a = 1;
            break;
        case PLAYERSLOT_ALL:
            a = 1;
            b = 1;
            c = 1;
            d = 1;
            break;
    }
    if (m->m_live != 0) {
        m->m_live = 0;
        ::operator delete(m);
        return 0;
    }
    m->m_tileX = tileX;
    m->m_tileY = tileY;
    m->m_cellKey = cellKey;
    m->m_playerFlags[2] = b;
    m->m_actionCode = actionCode;
    m->m_owner = this;
    m->m_live = 1;
    m->m_playerFlags[0] = d;
    m->m_playerFlags[1] = c;
    m->m_playerFlags[3] = a;
    m->SetActionCode(actionCode);
    m_list3.AddTail(m);
    return m;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::AddToList1
// Allocates a 0xc8-byte command element, constructs it, copies the 9-dword block
// into +0x9c, fills the rest from the args + two game-clock snapshots, back-links
// this container, and appends it to m_list1.  Returns the element, or 0 on
// alloc/double-init failure (vtable-stamped + freed).
// ---------------------------------------------------------------------------
// @early-stop
// /GX operator-new wall (~29%): the RezAlloc + placement-ctor + exception-cleanup
// trylevel guard around the partial heap element is not reproducible with a plain
// new; field-fill + rep-movs + AddTail identical.
// ARG ORDER FIXED (2026-07-14, retail stack reads): the 9-dword matrix source is
// the FOURTH arg (rep movs esi=[esp+0x34]=arg4), matching the byte-proven caller
// CPlay::ScanBuildTiles (m_164, m_168, m_4, &buf, m_11c, m_118, m_130) and the
// sibling AddLogic mapping (m_08<-m_164, m_0c<-m_168, m_10<-m_4). The old def had
// block9 third and folded two args into one.
RVA(0x00116cf0, 0x111)
CGiantRockLogic* CTileTriggerContainer::AddToList1(
    i32 tileX,
    i32 tileY,
    i32 cellKey,
    i32* block9,
    i32 powerupType,
    i32 textId,
    i32 dutyOffSpan
) {
    CGiantRockLogic* e = new CGiantRockLogic;
    if (e == 0) {
        return 0;
    }
    if (e->m_initGate != 0) {
        // The failure-path delete runs the BASE dtor (retail stamps ??_7CTileTriggerLogic
        // @0x5eaea4, not the rock's own vtable): the devs deleted through a
        // CTileTriggerLogic* whose non-virtual dtor makes the static type load-bearing.
        CTileTriggerLogic* dead = e;
        delete dead;
        return 0;
    }
    for (i32 i = 0; i < 9; i++) {
        e->m_matrix[i] = block9[i];
    }
    e->m_powerupType = powerupType;
    e->m_textId = textId;
    e->m_tileY = tileY;
    e->m_typeTag = TRIGID_GIANT_ROCK_22;
    e->m_tileX = tileX;
    e->m_10 = cellKey;
    e->m_owner = this;
    e->m_initGate = 1;
    e->m_dutyOn = 0;
    e->m_startClock = g_frameTime;
    e->m_dutyOnSpan = 0;
    e->m_tileToken = 0;
    e->m_leadInSpan = 0;
    e->m_dutyOffSpan = 0;
    e->m_dutyOffSpan = dutyOffSpan;
    e->m_startClock = g_frameTime;
    m_list1.AddTail(e);
    return e;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::DelFromList1
// Scans list1 (head @ +0x20) for the node whose data == arg; deletes that
// element inline (vtable 0x5eaea4 + [elem+0x1c]=0 + RezFree) and unlinks the
// node via list1.RemoveAt.  Returns 1 on a hit, 0 otherwise.
// ---------------------------------------------------------------------------
RVA(0x00116e60, 0x59)
i32 CTileTriggerContainer::DelFromList1(CTileTriggerLogic* want) {
    POSITION pos = m_list1.GetHeadPosition();
    while (pos != 0) {
        POSITION cur = pos;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list1.GetNext(pos));
        if (elem == want) {
            // ~CTileTriggerLogic (non-virtual, inline) restamps the vptr
            // (??_7CTileTriggerLogic @0x5eaea4) + clears m_initGate, then ??3.
            delete elem;
            m_list1.RemoveAt(cur);
            return 1;
        }
    }
    return 0;
}

RVA(0x00116ee0, 0x2f)
CTileTriggerSwitchLogic* CTileTriggerContainer::FindChild(i32 k1, i32 k2) {
    POSITION pos = m_base.GetHeadPosition();
    while (pos != 0) {
        CTileTriggerSwitchLogic* data = static_cast<CTileTriggerSwitchLogic*>(m_base.GetNext(pos));
        if (data->m_key1 == k1) {
            if (k2 == 0 || data->m_typeId == k2) {
                return data;
            }
        }
    }
    return 0;
}

RVA(0x00116f20, 0x51)
CTileTriggerLogic* CTileTriggerContainer::FindInLists12(i32 a, i32 b) {
    POSITION pos = m_list1.GetHeadPosition();
    while (pos != 0) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list1.GetNext(pos));
        if (elem->m_10 == a) {
            if (b == 0) {
                return elem;
            }
            if (elem->m_typeTag == b) {
                return elem;
            }
        }
    }
    pos = m_list2.GetHeadPosition();
    while (pos != 0) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list2.GetNext(pos));
        if (elem->m_10 == a) {
            if (b == 0) {
                return elem;
            }
            if (elem->m_typeTag == b) {
                return elem;
            }
        }
    }
    return 0;
}

RVA(0x00116fa0, 0xc7)
void CTileTriggerContainer::RemoveAll() {
    POSITION pos = m_list1.GetHeadPosition();
    while (pos != 0) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list1.GetNext(pos));
        delete elem; // vptr 0x5eaea4 restamp + m_initGate = 0, then ??3
    }
    m_list1.RemoveAll();
    pos = m_base.GetHeadPosition();
    while (pos != 0) {
        CTileTriggerSwitchLogic* elem = static_cast<CTileTriggerSwitchLogic*>(m_base.GetNext(pos));
        delete elem; // vptr 0x5eae8c restamp + m_initGate = 0, then ??3
    }
    m_base.RemoveAll();
    pos = m_list2.GetHeadPosition();
    while (pos != 0) {
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list2.GetNext(pos));
        delete elem; // vptr 0x5eaea4 restamp + m_initGate = 0, then ??3
    }
    m_list2.RemoveAll();
    pos = m_list3.GetHeadPosition();
    while (pos != 0) {
        CTileActionEvent* elem = static_cast<CTileActionEvent*>(m_list3.GetNext(pos));
        delete elem; // m_10 = 0 (no vtable -> no stamp), then ??3
    }
    m_list3.RemoveAll();
    m_latchedLeaf = 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::FilterList2
// Walks list2 (head @ +0x3c); classifies each element via CTileTriggerLogic
// 0x112970.  result 0  -> remove from list2 + delete element (0x5eaea4 + RezFree);
//           result -1 -> move element from list2 to list1 (RemoveAt + AddTail).
// Returns 1.
// ---------------------------------------------------------------------------
RVA(0x001170b0, 0x72)
i32 CTileTriggerContainer::FilterList2(i32 arg) {
    POSITION pos = m_list2.GetHeadPosition();
    while (pos != 0) {
        POSITION cur = pos;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(m_list2.GetNext(pos));
        i32 r = elem->Classify(arg);
        if (r == 0) {
            m_list2.RemoveAt(cur);
            delete elem; // vptr 0x5eaea4 restamp + m_initGate = 0, then ??3
        } else if (r == -1) {
            m_list2.RemoveAt(cur);
            m_list1.AddTail(elem);
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::MoveList1ToList2
// Scans list1 (head @ +0x20) for the node whose data == arg; removes it from
// list1 and appends the element to list2, then clears element+0x38.  Returns 1.
// ---------------------------------------------------------------------------
RVA(0x00117150, 0x53)
i32 CTileTriggerContainer::MoveList1ToList2(void* data) {
    POSITION pos = m_list1.GetHeadPosition();
    while (pos != 0) {
        POSITION cur = pos;
        void* elem = m_list1.GetNext(pos);
        if (elem == data) {
            m_list1.RemoveAt(cur);
            m_list2.AddTail(elem);
            *(static_cast<i32*>(elem) + 14) = 0; // elem+0x38
            return 1;
        }
    }
    return 0;
}

RVA(0x001171d0, 0x20)
CTileActionEvent* CTileTriggerContainer::FindByField0C(i32 key) {
    POSITION pos = m_list3.GetHeadPosition();
    while (pos != 0) {
        CTileActionEvent* data = static_cast<CTileActionEvent*>(m_list3.GetNext(pos));
        if (data->m_cellKey == key) {
            return data;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::DelFromList3
// Scans list3 (head @ +0x58) for the node whose data == arg; deletes that
// element inline ([elem+0x10]=0 + RezFree) and unlinks the node via
// list3.RemoveAt.  Returns 1 on a hit, 0 otherwise.
// ---------------------------------------------------------------------------
RVA(0x00117200, 0x53)
i32 CTileTriggerContainer::DelFromList3(CTileActionEvent* want) {
    POSITION pos = m_list3.GetHeadPosition();
    while (pos != 0) {
        POSITION cur_node = pos;
        CTileActionEvent* elem = static_cast<CTileActionEvent*>(m_list3.GetNext(pos));
        if (elem == want) {
            delete elem; // m_10 = 0 (no vtable -> no stamp), then ??3
            m_list3.RemoveAt(cur_node);
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::Serialize  (0x117280)
// The big save/load serialize walk.  Returns 0 if the stream is null or for any
// op other than 4/7 returns 1 (no-op).
//   op 4 (SAVE): for each of the four lists, write its count to the stream then
//                serialize-apply every element via SerializeApplyA (m_base, m_list3
//                marks via TtcMark::Serialize) / SerializeApplyB (m_list1, m_list2);
//                close with Method117e20.
//   op 7 (LOAD): RemoveAll, then for each list read a count and LoadElement that
//                many elements, AddTail'd into the list (m_list3 marks alloc'd
//                inline + TtcMark::Serialize); close with Method117e70.
// ---------------------------------------------------------------------------
// @early-stop
// /GX serialize-walk wall (~30%): 748-byte EH function; the inline RezAlloc + ctor for
// the m_list3 mark (op 7) hits the same RezAlloc+placement-ctor /GX wall as AddToList3
// (no ctor-in-flight EH frame on MSVC5), and the four near-identical list-walk loops +
// vtable serialize calls schedule their node cursors differently from retail.  Logic +
// list/helper dispatch + count read/write identical.
// See docs/patterns/rezalloc-placement-new-no-eh-frame.md
RVA(0x00117280, 0x2ec)
i32 CTileTriggerContainer::Serialize(CFileMemBase* s, i32 op, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }
    if (op == SERIAL_SAVE) {
        // SAVE
        POSITION pos;
        i32 cnt = m_base.GetCount();
        s->Write(&cnt, 4);
        pos = m_base.GetHeadPosition();
        while (pos != 0) {
            CTileTriggerSwitchLogic* e0 =
                static_cast<CTileTriggerSwitchLogic*>(m_base.GetNext(pos));
            if (SerializeApplyA(s, SERIAL_SAVE, typeId, pObj, e0) == 0) {
                return 0;
            }
        }
        cnt = m_list1.GetCount();
        s->Write(&cnt, 4);
        pos = m_list1.GetHeadPosition();
        while (pos != 0) {
            CTileTriggerLogic* e1 = static_cast<CTileTriggerLogic*>(m_list1.GetNext(pos));
            if (SerializeApplyB(s, SERIAL_SAVE, typeId, pObj, e1) == 0) {
                return 0;
            }
        }
        cnt = m_list2.GetCount();
        s->Write(&cnt, 4);
        pos = m_list2.GetHeadPosition();
        while (pos != 0) {
            CTileTriggerLogic* e2 = static_cast<CTileTriggerLogic*>(m_list2.GetNext(pos));
            if (SerializeApplyB(s, SERIAL_SAVE, typeId, pObj, e2) == 0) {
                return 0;
            }
        }
        cnt = m_list3.GetCount();
        s->Write(&cnt, 4);
        pos = m_list3.GetHeadPosition();
        while (pos != 0) {
            CTileActionEvent* e3 = static_cast<CTileActionEvent*>(m_list3.GetNext(pos));
            if (e3->Serialize(s, SERIAL_SAVE, typeId, pObj) == 0) {
                return 0;
            }
        }
        if (TransferFlag74(s) == 0) {
            return 0;
        }
        return 1;
    }
    if (op != SERIAL_LOAD) {
        return 1;
    }
    // LOAD
    RemoveAll();
    i32 n;
    i32 i;
    void* e;
    s->Read(&n, 4);
    for (i = 0; i < n; i++) {
        e = LoadElement(s, SERIAL_LOAD, typeId, pObj);
        if (e == 0) {
            return 0;
        }
        m_base.AddTail(e);
    }
    s->Read(&n, 4);
    for (i = 0; i < n; i++) {
        e = LoadElement(s, SERIAL_LOAD, typeId, pObj);
        if (e == 0) {
            return 0;
        }
        m_list1.AddTail(e);
    }
    s->Read(&n, 4);
    for (i = 0; i < n; i++) {
        e = LoadElement(s, SERIAL_LOAD, typeId, pObj);
        if (e == 0) {
            return 0;
        }
        m_list2.AddTail(e);
    }
    s->Read(&n, 4);
    for (i = 0; i < n; i++) {
        CTileActionEvent* m = new CTileActionEvent;
        if (m->Serialize(s, SERIAL_LOAD, typeId, pObj) == 0) {
            return 0;
        }
        m->m_owner = this;
        m_list3.AddTail(m);
    }
    if (LoadFlag74(s) == 0) {
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// SerializeApplyA  (0x117630)
// Streams the switch-logic element's type id, then for ids 1..7 (and 8 as the
// trailing case) runs the element's own serialize dispatcher (0x113860, __thiscall
// on the ELEMENT - retail: `mov ecx,edi; call 0x277f`); returns whether the apply
// succeeded (0 for the null object or an out-of-range tag).  __stdcall helper of
// the container's serialize walk (117280).
// ---------------------------------------------------------------------------
// @early-stop
// switch jump-table-vs-cmp-tree wall (~57%): logic identical; retail lowers the
// 8-tag switch to a jmp[tbl+(tag-1)*4] table, the recompile to a range-check tree
// (the two identical case bodies collapse).  See docs/patterns/switch-cmpje-tree-vs-jumptable.md
RVA(0x00117630, 0x82)
i32 __stdcall
SerializeApplyA(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj, CTileTriggerSwitchLogic* o) {
    if (o == 0) {
        return 0;
    }
    i32 tag = o->m_typeId;
    s->Write(&tag, 4);
    switch (tag) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            return o->ValidateByType(s, mode, typeId, pObj) != 0;
        case 8:
            return o->ValidateByType(s, mode, typeId, pObj) != 0;
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// SerializeApplyB  (0x117710)
// Streams the logic element's type tag, then dispatches tags 0x15..0x1a: 0x16
// (the giant rock) runs CGiantRockLogic::ApplyByType (retail `mov ecx,edi; call
// 0x1d39` -> 0x113d40), the rest CTileTriggerLogic::ValidateByType (0x1abe ->
// 0x113a90); returns success.  __stdcall helper of the serialize walk (117280).
// ---------------------------------------------------------------------------
// @early-stop
// switch jump-table-vs-cmp-tree wall (~63%): logic identical; retail lowers the
// 6-tag switch to a jmp[tbl+(tag-0x15)*4] table, the recompile to a cmp tree.
// See docs/patterns/switch-cmpje-tree-vs-jumptable.md
RVA(0x00117710, 0xa6)
i32 __stdcall
SerializeApplyB(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj, CTileTriggerLogic* o) {
    if (o == 0) {
        return 0;
    }
    i32 tag = o->m_typeTag;
    s->Write(&tag, 4);
    switch (tag) {
        case 0x16:
            return (static_cast<CGiantRockLogic*>(o))->ApplyByType(s, mode, typeId, pObj) != 0;
        case 0x15:
        case 0x17:
        case 0x18:
        case 0x19:
            return o->ValidateByType(s, mode, typeId, pObj) != 0;
        case 0x1a:
            return o->ValidateByType(s, mode, typeId, pObj) != 0;
        default:
            return 0;
    }
}

static void* RegSwitchTail(
    CTileTriggerContainer* self,
    CTileTriggerSwitchLogic* obj,
    CFileMemBase* reader,
    i32 typeId,
    i32 pObj,
    i32 id
) {
    if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
        return 0;
    }
    obj->m_owner = self;
    obj->m_typeId = id;
    return obj;
}

static void* RegLogicTail(
    CTileTriggerContainer* self,
    CTileTriggerLogic* obj,
    CFileMemBase* reader,
    i32 typeId,
    i32 pObj,
    i32 id
) {
    if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
        return 0;
    }
    obj->m_owner = self;
    obj->m_typeTag = id;
    return obj;
}

// @early-stop
// 0x47f (1151 B) /GX compact-switch factory. The body reproduces the reader read, the
// dense id 1..26 switch (the documented MSVC byte-index + jump-table wall: id->case map
// recovered from 0x517cbc/0x517c80), every per-case Rez-alloc + ctor + register + owner
// stamp, and the id 21 board-tile gate. The plateau is the jump-table/reloc-typing wall
// + the per-`new` /GX trylevel state machine (each case carries its own EH state, which
// MSVC tail-merges differently from the helper-factored spelling here) + the differently
// -named ctor/register reloc operands. Logic complete; byte-match parked for the final sweep.
RVA(0x00117800, 0x47f)
void* CTileTriggerContainer::LoadElement(CFileMemBase* reader, i32 kind, i32 typeId, i32 pObj) {
    if (reader == 0) {
        return 0;
    }
    if (kind != SERIAL_LOAD) {
        return 0;
    }
    i32 id;
    reader->Read(&id, 4);
    switch (id) {
        case 1:
        case 2:
        case 5: {
            CTileTriggerSwitchLogic* obj = new CTileTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, typeId, pObj, id);
        }
        case 3: {
            CTileTriggerSwitchLogic* obj = new CTileMultiTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, typeId, pObj, id);
        }
        case 4: {
            CTileTriggerSwitchLogic* obj = new CTileExclusiveTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, typeId, pObj, id);
        }
        case 6: {
            CTileTriggerSwitchLogic* obj = new CTileSecretTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, typeId, pObj, id);
        }
        case 7: {
            CTileTriggerSwitchLogic* obj = new CTileTimeTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, typeId, pObj, id);
        }
        case 8: {
            CTileTriggerSwitchLogic* obj = new CCheckpointTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, typeId, pObj, id);
        }
        case 21: {
            CTileTriggerLogic* obj = new CTileTriggerLogic;
            if (obj->ValidateByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            // resolve the board tile under the object; latch on a pyramid-band tile.
            CGameLevel* level = g_gameReg->m_world->m_level;
            i32 x = obj->m_tileX;
            i32 y = obj->m_tileY;
            CDDrawWorkerHost* geo = level->m_mainPlane;
            if (x < 0) {
                x = 0;
            } else if (x >= geo->m_gridW) {
                x = geo->m_gridW - 1;
            }
            if (y < 0) {
                y = 0;
            } else if (y >= geo->m_gridH) {
                y = geo->m_gridH - 1;
            }
            i32 cell = geo->m_colOffsets[y] + x;
            i32 tile = geo->m_tileGrid[cell];
            i32 tileKind;
            if (tile == static_cast<i32>(0xeeeeeeee) || tile == -1) {
                tileKind = 0;
            } else {
                // m_imageSets' CObArray payload -> the CTileImageSet collision record;
                // retail pushes two zeros: GetCollisionAt(0, 0) (the 0-arg "TypeId"
                // view mis-modeled this slot).
                CTileImageSet* rec =
                    static_cast<CTileImageSet*>(level->m_imageSets.GetData()[tile & 0xffff]);
                tileKind = rec->GetCollisionAt(0, 0);
            }
            if (tileKind == TILEKIND_PYRAMID_LATCH_A || tileKind == TILEKIND_PYRAMID_LATCH_B) {
                this->m_latchedLeaf = obj;
            }
            return obj;
        }
        case 22: {
            CGiantRockLogic* obj = new CGiantRockLogic;
            if (obj->ApplyByType(reader, SERIAL_LOAD, typeId, pObj) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case 23: {
            CTileTriggerLogic* obj = new CTileTimeTriggerLogic;
            return RegLogicTail(this, obj, reader, typeId, pObj, id);
        }
        case 24: {
            CTileTriggerLogic* obj = new CTileTriggerLogic;
            return RegLogicTail(this, obj, reader, typeId, pObj, id);
        }
        case 25: {
            CTileTriggerLogic* obj = new CTileSecretTriggerLogic;
            return RegLogicTail(this, obj, reader, typeId, pObj, id);
        }
        case 26: {
            CTileTriggerLogic* obj = new CCoveredPowerupLogic;
            return RegLogicTail(this, obj, reader, typeId, pObj, id);
        }
        default:
            return 0;
    }
}

RVA(0x00117e20, 0x36)
i32 CTileTriggerContainer::TransferFlag74(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_built, 4);
    return 1;
}

RVA(0x00117e70, 0x36)
i32 CTileTriggerContainer::LoadFlag74(CFileMemBase* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_built, 4);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::ScanNeighborhood
// Scans the 3x3 cell neighborhood centered on (x, y): for px in [x-1, x+2) and
// py in [y-1, y+2), probes cell (py + (px << 8)) with tag 0x16 via
// FindInLists12 (retail: `call 0x21df`, that method's own ILT thunk); returns
// the first hit, else 0. CONTAINER method (TriggerMgr drives it on m_2e4).
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~76%): logic exact, inner loop + ProbeCell call/regs match.
// Retail reserves an 8B frame (sub esp,8), keeps the inner bound py_end in a
// callee-saved REGISTER (ebp, hoisted once) and spills px/base to stack, whereas
// MSVC5 keeps px/base in registers and reloads py_end from the frame each inner
// step. The strength-reduced form (base += 0x100 accumulator) reproduces retail's
// outer tail but flips the same 2 callee-saved registers the other way (72%), so
// the plain double-for is the closest. Which of {px,base,py_end} wins the two
// callee-saved regs is a non-steerable regalloc pick. Final-sweep.
RVA(0x00117ec0, 0x7f)
CGiantRockLogic* CTileTriggerContainer::ScanNeighborhood(i32 x, i32 y) {
    for (i32 px = x - 1; px < x + 2; px++) {
        i32 base = px << 8;
        for (i32 py = y - 1; py < y + 2; py++) {
            // tag 0x16 (== factory id 22) IS the CGiantRockLogic discriminant, so
            // the hit is a rock element - the ONE checked downcast lives here so
            // every caller is cast-free.
            CGiantRockLogic* r =
                static_cast<CGiantRockLogic*>(FindInLists12(py + base, TRIGID_GIANT_ROCK_22));
            if (r != 0) {
                return r;
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::SetCell
// Looks up the keyed element for cell (tileX,tileY) (key = (x<<8)|y).  If it
// exists, playerSlot 5 (PLAYERSLOT_ALL) flags all four m_playerFlags words
// [+0x18..+0x24], otherwise just word [+0x18 + playerSlot*4]; either way the
// element is notified.  If absent, a covered-powerup command is probed, then the
// 3x3 neighborhood.  Returns 1 on success, 0 only from a failed fallback.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~78%): logic identical; retail pins key in edi / this in esi,
// the recompile swaps them (esi<->edi), propagating through the body.
RVA(0x00117f60, 0xa1)
i32 CTileTriggerContainer::SetCell(i32 tileX, i32 tileY, i32 playerSlot) {
    i32 key = (tileX << 8) + tileY;
    CTileActionEvent* elem = FindByField0C(key);
    if (elem != 0) {
        if (playerSlot == PLAYERSLOT_ALL) {
            elem->m_playerFlags[0] = 1;
            elem->m_playerFlags[1] = 1;
            elem->m_playerFlags[2] = 1;
            elem->m_playerFlags[3] = 1;
        } else {
            elem->m_playerFlags[playerSlot] = 1;
        }
        elem->SetActionCode(elem->m_actionCode);
        return 1;
    }
    // "AddMark @0x21df" / "RunFallback @0x377e" were FindInLists12 / ScanNeighborhood
    // all along (the ILT thunks jmp straight to them). Tag 0x1a == the
    // covered-powerup command (factory id 26).
    if (FindInLists12(key, TRIGID_COVERED_POWERUP_26) != 0) {
        return 1;
    }
    return ScanNeighborhood(tileX, tileY) != 0;
}
