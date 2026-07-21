#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <rva.h>
#include <new> // Rez heap throwing operator new / nothrow delete (0x1b9b46 / 0x1b9b82)

#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileGridCommand.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h> // CTileTriggerLogic + the per-id leaves AddLogic news
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerWiring.h>
#include <Gruntz/GameLevel.h> // CGameLevel/CLevelPlane/CTileImageSet (the id-21 board latch)

RVA(0x000c8640, 0x70)
CTileTriggerContainer::~CTileTriggerContainer() {
    DtorBase();
    // m_list3 / m_list2 / m_list1 / m_base are auto-destroyed here (reverse decl
    // order) by the compiler-emitted /GX member teardown.
}

// @identity-TODO (arg0's class only - see BLOCKER; everything else below is PROVEN)
// 0x115b60 (151 B) - a GDI "draw 3D text onto a plane's DirectDraw surface" helper. Homed
// from GapFunctions.cpp (matcher-5) by RVA neighbourhood (this TU's .text brackets it).
//
// The old note here was WRONG on two counts (GO1, corrected from retail bytes):
//   * NOT a "__thiscall predicate": retail reads arg0 at [esp+0x4] BEFORE any push and ends
//     in a bare `ret` (no `ret N`, no ecx receiver) -> a FREE __cdecl fn.
//   * The "+0x44/+0x68 vtable slots" are not a game class's: the object is pushed as an
//     explicit stack arg (`push &out; push pSurf; call [vtbl+0x44]`) = COM __stdcall. On
//     IDirectDrawSurface, +0x44 is slot 17 GetDC(HDC*) and +0x68 is slot 26 ReleaseDC(HDC)
//     (exact <ddraw.h> offsets) - i.e. this is a GetDC/draw/ReleaseDC bracket.
//
// DECODED (byte-exact; 8 args, returns 0 on every null-gate, 1 on success):
//   f(obj, const CString* text, RECT* dst, i32 fontFlag, i32 useFront, i32 r, i32 g, i32 b)
//     if (!obj) return 0;
//     p = useFront ? obj->m_4->m_10 : obj->m_4->m_14;   if (!p) return 0;
//     CDDSurface* s = p->m_2c;                          if (!s) return 0;
//     HDC hdc = 0;                       // MSVC reuses the DEAD arg0 slot [esp+8] as this local
//     s->m_8->GetDC(&hdc);               // m_8 is the IDirectDrawSurface* (DDSurface.h)
//     g_gameReg->m_chatLog->Draw3DText(text, hdc, dst, fontFlag, r, g, b, 1, 2, 3);
//     s->m_8->ReleaseDC(hdc);
//     return 1;
//   The Draw3DText binding is PROVEN, not inferred: the call is ILT 0x140b -> `jmp 0x22810`,
//   and CFontConfig::Draw3DText @0x22810 (FontConfig.h) takes exactly the 10 args the push
//   order yields - (strSrc, hdc, dst, fontFlag, r, g, b, shadow, dx, dy) with shadow/dx/dy
//   the pushed literals 1/2/3. `g_gameReg->m_5c` IS CFontConfig* m_chatLog (GruntzMgr.h).
//
// BLOCKER (why this is not written out as code): arg0's CLASS is unrecoverable. Full chase
// run and dead-ended - `sema xref 0x115b60` AND `--tree`: "(no direct call/jmp rel32 caller
// in .text)"; it is a ZERO-REF orphan (dead debug helper, compiled in but never called), so
// there is no call site to type arg0 from, and no vtable/data-ref either. Writing the
// obj->m_4->m_10 chain would require FABRICATING a view of an un-xref-able receiver
// (no-fake-view rule). Everything except arg0's type is settled; the moment any caller of
// 0x115b60 is reconstructed, this is a ~20-line write-out.
RVA(0x00115b60, 0x97)
i32 Gap_115b60(void) {
    return 0;
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

// @confidence: high
// @source: xref
// @stub
// The AddSwitchLogic factory: news a 0x8c CTileTriggerSwitchLogic (`push 0x8c;
// call ??2; call ??0CTileTriggerSwitchLogic` @0x115f96 - the rtti-vptr signal was
// the BUILT object's ctor stamp, not the receiver's). The receiver is this
// container (same m_2e4 object every sibling here runs on). Full 13-arg signature
// from the seven CPlay::ValidateLevelTiles call sites (ret 0x7c; ex the
// TriggerRegistrar::RegisterSwitchLogic view).
RVA(0x00115f60, 0x2de)
i32 CTileTriggerContainer::AddSwitchLogic(
    i32 tag,
    i32 col,
    i32 row,
    i32 key,
    RECT r134,
    RECT r144,
    RECT r154,
    RECT r64,
    RECT rF0,
    RECT r100,
    i32 isMatch,
    i32 m120,
    i32 zero
) {
    return 0;
}

RVA(0x00116320, 0x66)
i32 CTileTriggerContainer::RemoveByKeys(i32 k1, i32 k2) {
    TtcNode* node = TtcHead(m_base);
    while (node) {
        TtcNode* cur = node; // savePos (esi)
        TtcNode* pn = node;  // GetNext local (ecx)
        node = node->m_next;
        CTileTriggerSwitchLogic* data = static_cast<CTileTriggerSwitchLogic*>(pn->m_data);
        if (data->m_typeId == k2 && data->m_key1 == k1) {
            // ~CTileTriggerSwitchLogic is non-virtual + inline: the dtor restamps the vptr
            // (`mov [data],offset ??_7`) + clears m_initGate, then ??3 frees it.
            delete data;
            m_base.RemoveAt(reinterpret_cast<POSITION>(cur));
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
void CTileTriggerContainer::
    AddLogicDefaults(i32 type, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9) {
    AddLogic(
        type,
        a2,
        a3,
        a4,
        a5,
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        CTrigParam(),
        a6,
        a7,
        a8,
        a9
    );
}

RVA(0x001164a0, 0x116)
void CTileTriggerContainer::AddLogicFromRecord(i32 type, i32 a2, CTrigSourceRecord* rec) {
    AddLogic(
        type,
        a2,
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
    i32 a1,
    i32 logicType,
    i32 a3,
    i32 a4,
    i32 a5,
    CTrigParam p1,
    CTrigParam p2,
    CTrigParam p3,
    CTrigParam p4,
    CTrigParam p5,
    CTrigParam p6,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
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
    local[0] = p1;
    local[1] = p2;
    local[2] = p3;
    local[3] = p4;
    local[4] = p5;
    local[5] = p6;

    i32 ok = 0;
    if (obj->m_initGate == 0) {
        memcpy(obj->m_block, local, sizeof(local));
        if (obj->m_initGate == 0) {
            obj->m_tileY = a4;
            obj->m_tileX = a3;
            obj->m_owner = this;
            obj->m_typeTag = logicType;
            obj->m_10 = a5;
            obj->m_initGate = 1;
            obj->m_tileToken = a6;
            obj->m_startClock = g_frameTime;
            obj->m_leadInSpan = a8;
            obj->m_dutyOn = 0;
            obj->m_dutyOnSpan = a7;
            obj->m_dutyOffSpan = a9;
            if (logicType != TRIGID_COVERED_POWERUP_26 && a9 == 0) {
                obj->m_dutyOffSpan = a7;
            }
            obj->m_startClock = g_frameTime;
            ok = 1;
        }
    }

    if (ok == 0) {
        delete obj;
        return 0;
    }

    TtcObList* list = logicType == TRIGID_TIME_TRIGGER_23 ? &m_list2 : &m_list1;
    list->AddTail(obj);
    if (logicType == TRIGID_TILE_TRIGGER_21 && (a1 == 0x67 || a1 == 0x68)) {
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
CTileActionEvent*
CTileTriggerContainer::AddToList3(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8) {
    CTileActionEvent* m = new CTileActionEvent;
    if (m == 0) {
        return 0;
    }
    if (m->m_10 != 0) {
        m->m_10 = 0;
        ::operator delete(m);
        return 0;
    }
    m->m_tileX = a2;
    m->m_tileY = a3;
    m->m_c = a4;
    m->m_playerFlags[0] = a5;
    m->m_playerFlags[1] = a6;
    m->m_playerFlags[3] = a8;
    m->m_actionCode = a1;
    m->m_14 = this;
    m->m_10 = 1;
    m->m_playerFlags[2] = a7;
    m->SetActionCode(a1);
    m_list3.AddTail(m);
    return m;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::AddToList3Switch  (0x116b80)
// Twin of AddToList3: allocates+constructs a 0x28-byte mark, and (when its init
// flag is clear) fills its fields from the args, computes four state flags from a
// switch on `type` (cases 0..5, default = all clear), notifies it, and appends it
// to m_list3.  Returns the mark, or 0 on alloc/double-init failure.
// ---------------------------------------------------------------------------
// @early-stop
// RezAlloc + placement-ctor /GX wall (~49%): twin of AddToList3 - retail carries the
// ctor-in-flight EH frame (push -1/fs:0 + trylevel + shared jmp epilogue) that MSVC5
// won't emit for the RezAlloc+ctor pair; switch flag-fill + Notify + AddTail are
// byte-identical (scores above the no-switch twin AddToList3 at 43%).
// See docs/patterns/rezalloc-placement-new-no-eh-frame.md
RVA(0x00116b80, 0x105)
CTileActionEvent*
CTileTriggerContainer::AddToList3Switch(i32 a1, i32 a2, i32 a3, i32 a4, i32 type) {
    CTileActionEvent* m = new CTileActionEvent;
    if (m == 0) {
        return 0;
    }
    i32 a = 0, b = 0, c = 0, d = 0;
    switch (type) {
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
        case 5:
            a = 1;
            b = 1;
            c = 1;
            d = 1;
            break;
    }
    if (m->m_10 != 0) {
        m->m_10 = 0;
        ::operator delete(m);
        return 0;
    }
    m->m_tileX = a2;
    m->m_tileY = a3;
    m->m_c = a4;
    m->m_playerFlags[2] = b;
    m->m_actionCode = a1;
    m->m_14 = this;
    m->m_10 = 1;
    m->m_playerFlags[0] = d;
    m->m_playerFlags[1] = c;
    m->m_playerFlags[3] = a;
    m->SetActionCode(a1);
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
CGiantRockLogic*
CTileTriggerContainer::AddToList1(i32 a1, i32 a2, i32 a3, i32* block9, i32 a5, i32 a6, i32 a7) {
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
    e->m_powerupType = a5;
    e->m_textId = a6;
    e->m_tileY = a2;
    e->m_typeTag = TRIGID_GIANT_ROCK_22;
    e->m_tileX = a1;
    e->m_10 = a3;
    e->m_owner = this;
    e->m_initGate = 1;
    e->m_dutyOn = 0;
    e->m_startClock = g_frameTime;
    e->m_dutyOnSpan = 0;
    e->m_tileToken = 0;
    e->m_leadInSpan = 0;
    e->m_dutyOffSpan = 0;
    e->m_dutyOffSpan = a7;
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
// @early-stop
// list-walk regalloc wall (~69%): logic + body identical; retail rotates the node
// through eax with twin ecx/esi copies and hoists arg to edx, vs our single
// callee-saved esi direct-deref.  See docs/patterns/linked-list-walk-node-eax-rotation.md
RVA(0x00116e60, 0x59)
i32 CTileTriggerContainer::DelFromList1(void* data) {
    TtcNode* node = TtcHead(m_list1);
    if (node == 0) {
        return 0;
    }
    do {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(cur->m_data);
        if (elem == static_cast<CTileTriggerLogic*>(data)) {
            // ~CTileTriggerLogic (non-virtual, inline) restamps the vptr
            // (??_7CTileTriggerLogic @0x5eaea4) + clears m_initGate, then ??3.
            delete elem;
            m_list1.RemoveAt(reinterpret_cast<POSITION>(cur));
            return 1;
        }
    } while (node != 0);
    return 0;
}

RVA(0x00116ee0, 0x2f)
CTileTriggerSwitchLogic* CTileTriggerContainer::FindChild(i32 k1, i32 k2) {
    TtcNode* node = TtcHead(m_base);
    while (node) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerSwitchLogic* data = static_cast<CTileTriggerSwitchLogic*>(cur->m_data);
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
    TtcNode* node = TtcHead(m_list1);
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(cur->m_data);
            if (elem->m_10 == a) {
                if (b == 0) {
                    return elem;
                }
                if (elem->m_typeTag == b) {
                    return elem;
                }
            }
        } while (node != 0);
    }
    node = TtcHead(m_list2);
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(cur->m_data);
            if (elem->m_10 == a) {
                if (b == 0) {
                    return elem;
                }
                if (elem->m_typeTag == b) {
                    return elem;
                }
            }
        } while (node != 0);
    }
    return 0;
}

RVA(0x00116fa0, 0xc7)
void CTileTriggerContainer::RemoveAll() {
    TtcNode* node = TtcHead(m_list1);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(cur->m_data);
        delete elem; // vptr 0x5eaea4 restamp + m_initGate = 0, then ??3
    }
    m_list1.RemoveAll();
    node = TtcHead(m_base);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerSwitchLogic* elem = static_cast<CTileTriggerSwitchLogic*>(cur->m_data);
        delete elem; // vptr 0x5eae8c restamp + m_initGate = 0, then ??3
    }
    m_base.RemoveAll();
    node = TtcHead(m_list2);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(cur->m_data);
        delete elem; // vptr 0x5eaea4 restamp + m_initGate = 0, then ??3
    }
    m_list2.RemoveAll();
    node = TtcHead(m_list3);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileActionEvent* elem = static_cast<CTileActionEvent*>(cur->m_data);
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
// @early-stop
// list-walk regalloc wall (~84%): logic + Classify/RemoveAt/AddTail branches
// identical; same node-eax-rotation vs callee-saved-esi shape as the siblings.
// See docs/patterns/linked-list-walk-node-eax-rotation.md
RVA(0x001170b0, 0x72)
i32 CTileTriggerContainer::FilterList2(void* arg) {
    TtcNode* node = TtcHead(m_list2);
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            CTileTriggerLogic* elem = static_cast<CTileTriggerLogic*>(cur->m_data);
            i32 r = elem->Classify(reinterpret_cast<i32>(arg));
            if (r == 0) {
                m_list2.RemoveAt(reinterpret_cast<POSITION>(cur));
                delete elem; // vptr 0x5eaea4 restamp + m_initGate = 0, then ??3
            } else if (r == -1) {
                m_list2.RemoveAt(reinterpret_cast<POSITION>(cur));
                m_list1.AddTail(elem);
            }
        } while (node != 0);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::MoveList1ToList2
// Scans list1 (head @ +0x20) for the node whose data == arg; removes it from
// list1 and appends the element to list2, then clears element+0x38.  Returns 1.
// ---------------------------------------------------------------------------
// @early-stop
// list-walk regalloc wall (~51%): logic + RemoveAt/AddTail/clear identical; same
// node-eax-rotation vs callee-saved-esi shape as the siblings.
// See docs/patterns/linked-list-walk-node-eax-rotation.md
RVA(0x00117150, 0x53)
i32 CTileTriggerContainer::MoveList1ToList2(void* data) {
    TtcNode* node = TtcHead(m_list1);
    if (node == 0) {
        return 0;
    }
    do {
        TtcNode* cur = node;
        node = node->m_next;
        void* elem = cur->m_data;
        if (elem == data) {
            m_list1.RemoveAt(reinterpret_cast<POSITION>(cur));
            m_list2.AddTail(elem);
            *(reinterpret_cast<i32*>(elem) + 14) = 0; // elem+0x38
            return 1;
        }
    } while (node != 0);
    return 0;
}

RVA(0x001171d0, 0x20)
CTileActionEvent* CTileTriggerContainer::FindByField0C(i32 key) {
    TtcNode* node = TtcHead(m_list3);
    while (node) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileActionEvent* data = static_cast<CTileActionEvent*>(cur->m_data);
        if (data->m_c == key) {
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
// @early-stop
// list-walk regalloc wall (~82%): logic + delete/RemoveAt identical; same
// node-eax-rotation vs callee-saved-esi shape as the siblings.
// See docs/patterns/linked-list-walk-node-eax-rotation.md
RVA(0x00117200, 0x53)
i32 CTileTriggerContainer::DelFromList3(void* data) {
    for (TtcNode* node = TtcHead(m_list3); node != 0; node = node->m_next) {
        CTileActionEvent* elem = static_cast<CTileActionEvent*>(node->m_data);
        if (elem == static_cast<CTileActionEvent*>(data)) {
            delete elem; // m_10 = 0 (no vtable -> no stamp), then ??3
            m_list3.RemoveAt(reinterpret_cast<POSITION>(node));
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
i32 CTileTriggerContainer::Serialize(CSerialArchive* s, i32 op, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    if (op == 4) {
        // SAVE
        TtcNode* node;
        i32 cnt = m_base.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_base); node != 0; node = node->m_next) {
            if (SerializeApplyA(s, 4, a3, a4, static_cast<CTileTriggerSwitchLogic*>(node->m_data)) == 0) {
                return 0;
            }
        }
        cnt = m_list1.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_list1); node != 0; node = node->m_next) {
            if (SerializeApplyB(s, 4, a3, a4, static_cast<CTileTriggerLogic*>(node->m_data)) == 0) {
                return 0;
            }
        }
        cnt = m_list2.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_list2); node != 0; node = node->m_next) {
            if (SerializeApplyB(s, 4, a3, a4, static_cast<CTileTriggerLogic*>(node->m_data)) == 0) {
                return 0;
            }
        }
        cnt = m_list3.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_list3); node != 0; node = node->m_next) {
            if ((static_cast<CTileActionEvent*>(node->m_data))->Serialize(s, 4, a3, a4) == 0) {
                return 0;
            }
        }
        if (TransferFlag74(s) == 0) {
            return 0;
        }
        return 1;
    }
    if (op != 7) {
        return 1;
    }
    // LOAD
    RemoveAll();
    i32 n;
    i32 i;
    void* e;
    s->Read(&n, 4);
    for (i = 0; i < n; i++) {
        e = LoadElement(s, 7, a3, a4);
        if (e == 0) {
            return 0;
        }
        m_base.AddTail(e);
    }
    s->Read(&n, 4);
    for (i = 0; i < n; i++) {
        e = LoadElement(s, 7, a3, a4);
        if (e == 0) {
            return 0;
        }
        m_list1.AddTail(e);
    }
    s->Read(&n, 4);
    for (i = 0; i < n; i++) {
        e = LoadElement(s, 7, a3, a4);
        if (e == 0) {
            return 0;
        }
        m_list2.AddTail(e);
    }
    s->Read(&n, 4);
    for (i = 0; i < n; i++) {
        CTileActionEvent* m = new CTileActionEvent;
        if (m->Serialize(s, 7, a3, a4) == 0) {
            return 0;
        }
        m->m_14 = this;
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
SerializeApplyA(CSerialArchive* s, i32 a2, i32 a3, i32 a4, CTileTriggerSwitchLogic* o) {
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
            return o->ValidateByType(s, a2, a3, a4) != 0;
        case 8:
            return o->ValidateByType(s, a2, a3, a4) != 0;
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
i32 __stdcall SerializeApplyB(CSerialArchive* s, i32 a2, i32 a3, i32 a4, CTileTriggerLogic* o) {
    if (o == 0) {
        return 0;
    }
    i32 tag = o->m_typeTag;
    s->Write(&tag, 4);
    switch (tag) {
        case 0x16:
            return (static_cast<CGiantRockLogic*>(o))->ApplyByType(s, a2, a3, a4) != 0;
        case 0x15:
        case 0x17:
        case 0x18:
        case 0x19:
            return o->ValidateByType(s, a2, a3, a4) != 0;
        case 0x1a:
            return o->ValidateByType(s, a2, a3, a4) != 0;
        default:
            return 0;
    }
}

static void* RegSwitchTail(
    CTileTriggerContainer* self,
    CTileTriggerSwitchLogic* obj,
    CSerialArchive* reader,
    i32 a2,
    i32 a3,
    i32 id
) {
    if (obj->ValidateByType(reader, 7, a2, a3) == 0) {
        return 0;
    }
    obj->m_owner = self;
    obj->m_typeId = id;
    return obj;
}

static void* RegLogicTail(
    CTileTriggerContainer* self,
    CTileTriggerLogic* obj,
    CSerialArchive* reader,
    i32 a2,
    i32 a3,
    i32 id
) {
    if (obj->ValidateByType(reader, 7, a2, a3) == 0) {
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
void* CTileTriggerContainer::LoadElement(CSerialArchive* reader, i32 kind, i32 a2, i32 a3) {
    if (reader == 0) {
        return 0;
    }
    if (kind != 7) {
        return 0;
    }
    i32 id;
    reader->Read(&id, 4);
    switch (id) {
        case 1:
        case 2:
        case 5: {
            CTileTriggerSwitchLogic* obj = new CTileTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, a2, a3, id);
        }
        case 3: {
            CTileTriggerSwitchLogic* obj = new CTileMultiTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, a2, a3, id);
        }
        case 4: {
            CTileTriggerSwitchLogic* obj = new CTileExclusiveTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, a2, a3, id);
        }
        case 6: {
            CTileTriggerSwitchLogic* obj = new CTileSecretTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, a2, a3, id);
        }
        case 7: {
            CTileTriggerSwitchLogic* obj = new CTileTimeTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, a2, a3, id);
        }
        case 8: {
            CTileTriggerSwitchLogic* obj = new CCheckpointTriggerSwitchLogic;
            return RegSwitchTail(this, obj, reader, a2, a3, id);
        }
        case 21: {
            CTileTriggerLogic* obj = new CTileTriggerLogic;
            if (obj->ValidateByType(reader, 7, a2, a3) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            // resolve the board tile under the object; latch on a 0x67/0x68 tile.
            CGameLevel* level = g_gameReg->m_world->m_level;
            i32 x = obj->m_tileX;
            i32 y = obj->m_tileY;
            CLevelPlane* geo = level->m_mainPlane;
            if (x < 0) {
                x = 0;
            } else if (x >= geo->m_width) {
                x = geo->m_width - 1;
            }
            if (y < 0) {
                y = 0;
            } else if (y >= geo->m_height) {
                y = geo->m_height - 1;
            }
            i32 cell = geo->m_colOffsets[y] + x;
            i32 tile = geo->m_tileGrid[cell];
            i32 type;
            if (tile == static_cast<i32>(0xeeeeeeee) || tile == -1) {
                type = 0;
            } else {
                // m_imageSets' CObArray payload -> the CTileImageSet collision record;
                // retail pushes two zeros: GetCollisionAt(0, 0) (the 0-arg "TypeId"
                // view mis-modeled this slot).
                CTileImageSet* rec = static_cast<CTileImageSet*>(level->m_imageSets.GetData()[tile & 0xffff]);
                type = rec->GetCollisionAt(0, 0);
            }
            if (type == 0x67 || type == 0x68) {
                this->m_latchedLeaf = obj;
            }
            return obj;
        }
        case 22: {
            CGiantRockLogic* obj = new CGiantRockLogic;
            if (obj->ApplyByType(reader, 7, a2, a3) == 0) {
                return 0;
            }
            obj->m_owner = this;
            obj->m_typeTag = id;
            return obj;
        }
        case 23: {
            CTileTriggerLogic* obj = new CTileTimeTriggerLogic;
            return RegLogicTail(this, obj, reader, a2, a3, id);
        }
        case 24: {
            CTileTriggerLogic* obj = new CTileTriggerLogic;
            return RegLogicTail(this, obj, reader, a2, a3, id);
        }
        case 25: {
            CTileTriggerLogic* obj = new CTileSecretTriggerLogic;
            return RegLogicTail(this, obj, reader, a2, a3, id);
        }
        case 26: {
            CTileTriggerLogic* obj = new CCoveredPowerupLogic;
            return RegLogicTail(this, obj, reader, a2, a3, id);
        }
        default:
            return 0;
    }
}

RVA(0x00117e20, 0x36)
i32 CTileTriggerContainer::TransferFlag74(CSerialArchive* s) {
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
i32 CTileTriggerContainer::LoadFlag74(CSerialArchive* s) {
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
            CGiantRockLogic* r = static_cast<CGiantRockLogic*>(FindInLists12(py + base, TRIGID_GIANT_ROCK_22));
            if (r != 0) {
                return r;
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::SetCell
// Looks up the keyed element for cell (a,b) (key = (a<<8)|b).  If it exists, a
// verb of 5 flags all four state words [+0x18..+0x24], otherwise just word
// [+0x18 + verb*4]; either way the element is notified.  If absent, a new mark
// is registered (AddMark key,0x1a); on failure the fallback (RunFallback a,b)
// decides the result.  Returns 1 on success, 0 only from a failed fallback.
// ---------------------------------------------------------------------------
// @early-stop
// regalloc wall (~78%): logic identical; retail pins key in edi / this in esi,
// the recompile swaps them (esi<->edi), propagating through the body.
RVA(0x00117f60, 0xa1)
i32 CTileTriggerContainer::SetCell(i32 a, i32 b, i32 verb) {
    i32 key = (a << 8) + b;
    CTileActionEvent* elem = FindByField0C(key);
    if (elem != 0) {
        if (verb == 5) {
            elem->m_playerFlags[0] = 1;
            elem->m_playerFlags[1] = 1;
            elem->m_playerFlags[2] = 1;
            elem->m_playerFlags[3] = 1;
        } else {
            elem->m_playerFlags[verb] = 1;
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
    return ScanNeighborhood(a, b) != 0;
}
