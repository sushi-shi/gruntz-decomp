// TileTriggerContainer.cpp - Gruntz CTileTriggerContainer (C:\Proj\Gruntz).
//
// A 4-CPtrList container: a base sub-object at +0x00 (head +0x04) plus three more
// CPtrList members at +0x1c / +0x38 / +0x54 (heads +0x20 / +0x3c / +0x58).  The
// accessors find/move/remove heap command elements (CTileTriggerLogic) across the
// lists; an element is destroyed inline (stamp vtable + RezFree) before its node
// is unlinked.  The /GX ~dtor + RemoveAll empty all four lists; AddToList1/3
// allocate+append elements; SetCell flags a keyed element; SerializeApplyA/B are
// the tag-dispatched serialize helpers of the big serialize walk (0x117280).
//
// The dynamic this-tracer originally lumped these RVAs under
// CTileTriggerSwitchLogic; they are a DIFFERENT shape (verified by the EH dtor
// 0xc8640 destroying three CPtrList sub-objects at +0x1c/+0x38/+0x54 and the base
// at +0x00 - not the +0x04 child-list switch-logic layout).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
//
// wave2-F: this file is the WOVEN original obj at retail .text
// [0x115b60 .. 0x118001] (TU_MIGRATION interval 0x115b60, weave 0.43):
// CTileTriggerContainer + the CTileTriggerSwitchLogic methods the devs defined
// in this second file (their RVAs interleave with the container's throughout),
// + the tiletriggerwiring / tiletriggerfactory singletons whose RVAs sit inside
// the interval. Gate113860 (0x113860) moved OUT to TileTriggerSwitchLogic.cpp
// (its RVA is inside the 0x110430 interval). Definitions in strict ascending
// retail-RVA order (the ~dtor 0xc8640 is the lone COMDAT-at-usage outlier).
#include <Mfc.h>
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

// The *0x24556c singleton. Declared here: <Gruntz/TileGridCommand.h>'s header-level decl was
// removed so each TU picks the view/real class it needs (see the note in Play.h). Type unchanged.
extern "C" CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------
// CTileTriggerContainer::Dtor
// The /GX destructor: runs the most-derived teardown (DtorBase) then destroys the
// four CPtrList sub-objects in reverse declaration order (m_list3, m_list2,
// m_list1, m_base), each at its own EH trylevel.
// ---------------------------------------------------------------------------
RVA(0x000c8640, 0x70)
CTileTriggerContainer::~CTileTriggerContainer() {
    DtorBase();
    // m_list3 / m_list2 / m_list1 / m_base are auto-destroyed here (reverse decl
    // order) by the compiler-emitted /GX member teardown.
}

// @early-stop
// 0x115b60 (151 B) = a __thiscall predicate walking this->m_2c's chain and dispatching a
// vtable slot (+0x44/+0x68) - a tile-trigger container query. Homed from GapFunctions.cpp
// (matcher-5) by RVA neighbourhood (this TU's .text block brackets 0x115b60). Homed pending
// full reconstruction of the receiver's field/vtable model.
RVA(0x00115b60, 0x97)
i32 Gap_115b60(void) {
    return 0;
}

// GetFlag74 (0x115f00): test-and-set the container's m_74 latch (1 on first call,
// then 0). Out-of-line (retail emits it standalone; the inline member folded away).
// CONTAINER method (was misfiled on CTileTriggerSwitchLogic, where +0x74 fell
// inside m_block): ModeObjInit builds the receiver as this 4-CPtrList container
// and zeroes exactly this +0x74 slot before the first call.
RVA(0x00115f00, 0x13)
i32 CTileTriggerContainer::GetFlag74() {
    if (m_74 != 0) {
        return 0;
    }
    m_74 = 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::DtorBase
// When +0x74 is set, runs RemoveAll (RVA 0x116fa0) and clears +0x74.
// Reloc-masked rel32 callee invoked first by the EH dtor.
// ---------------------------------------------------------------------------
RVA(0x00115f30, 0x18)
void CTileTriggerContainer::DtorBase() {
    if (m_74 != 0) {
        RemoveAll();
        m_74 = 0;
    }
}

// Engine-label backlog stubs (moved from src/Stub/CTileTriggerSwitchLogic.cpp).

// @confidence: high
// @source: xref
// @stub
// The AddSwitchLogic factory: news a 0x8c CTileTriggerSwitchLogic (`push 0x8c;
// call ??2; call ??0CTileTriggerSwitchLogic` @0x115f96 - the rtti-vptr signal was
// the BUILT object's ctor stamp, not the receiver's). The receiver is this
// container (same m_2e4 object every sibling here runs on).
RVA(0x00115f60, 0x2de)
void CTileTriggerContainer::AddSwitchLogic_115f60() {}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::RemoveByKeys
// Walks m_base (head @ +0x04; node next@0, prev@4, data@8); on the first 0x8c
// switch-logic element whose m_04 == k2 and m_key1 == k1, deletes it (inlined
// dtor vptr-restamp + RezFree), unlinks the node via m_base.RemoveAt, returns 1.
// Returns 0 if no match. CONTAINER method: with the receiver settled as the
// container, the old `(CPtrList*)this` reinterpret dissolves - m_base IS the
// CPtrList at +0x00. The loop is the inlined CPtrList::GetNext idiom: a saved
// position (cur, esi) + the GetNext local (pn, ecx) are two distinct node copies;
// pn advances node then derefs data, so retail materializes the extra `mov ecx,eax`
// copy and defers the data load past the advance.
// ---------------------------------------------------------------------------
RVA(0x00116320, 0x66)
i32 CTileTriggerContainer::RemoveByKeys(i32 k1, i32 k2) {
    TtcNode* node = TtcHead(m_base);
    while (node) {
        TtcNode* cur = node; // savePos (esi)
        TtcNode* pn = node;  // GetNext local (ecx)
        node = node->m_next;
        CTileTriggerSwitchLogic* data = (CTileTriggerSwitchLogic*)pn->m_data;
        if (data->m_04 == k2 && data->m_key1 == k1) {
            if (data) {
                // The inlined `delete data`: the dtor restamps the vptr
                // (`mov [data],offset ??_7`) + clears m_20, then RezFree frees it.
                data->~CTileTriggerSwitchLogic();
                ::operator delete(data);
            }
            m_base.RemoveAt((POSITION)cur);
            return 1;
        }
    }
    return 0;
}

// TileTriggerWiring.cpp - CTileTriggerWiring::AddLogicDefaults (0x1163b0): forward
// to the full AddLogic factory, supplying six zeroed 16-byte parameter blocks.

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

// ===========================================================================
// CTileTriggerWiring::AddLogicFromRecord  (0x1164a0)
// ===========================================================================
// Same forward as AddLogicDefaults, but the five ids (a3/a4/a5) and the six
// CTrigParam blocks come from a source tile record instead of being zeroed:
// a3/a4/a5 = rec->m_164/m_168/m_4; p1..p6 = rec->m_134/m_144/m_154/m_64 and the
// +0x7c sub-object's m_f0/m_100. __thiscall, ret 0xc.
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
// Then (when the leaf's m_1c init-gate is clear) copy the six CTrigParam blocks into
// the leaf's m_block, fill the id/owner/clock fields, append it to m_list1 (m_list2
// for id 0x17), and latch id-0x15 board tiles 0x67/0x68 into m_70.  The failure path
// deletes the leaf (inline ~CTileTriggerLogic: ??_7 stamp + m_1c=0 + RezFree).
// @early-stop  (~70%)
// /GX jump-table + per-`new` trylevel regalloc wall.  Full body + all four ctor / new /
// delete / AddTail / ??_7CTileTriggerLogic / g_645588 relocs bind - this is what binds
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
        case 0x15:
        case 0x18:
            obj = new CTileTriggerLogic;
            break;
        case 0x19:
            obj = new CTileSecretTriggerLogic;
            break;
        case 0x1a:
            obj = new CCoveredPowerupLogic;
            break;
        case 0x17:
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
    if (obj->m_1c == 0) {
        memcpy(obj->m_block, local, sizeof(local));
        if (obj->m_1c == 0) {
            obj->m_0c = a4;
            obj->m_08 = a3;
            obj->m_20 = this;
            obj->m_typeTag = logicType;
            obj->m_10 = a5;
            obj->m_1c = 1;
            obj->m_34 = a6;
            obj->m_24 = g_645588;
            obj->m_2c = a8;
            obj->m_dutyOn = 0;
            obj->m_28 = a7;
            obj->m_30 = a9;
            if (logicType != 0x1a && a9 == 0) {
                obj->m_30 = a7;
            }
            obj->m_24 = g_645588;
            ok = 1;
        }
    }

    if (ok == 0) {
        delete obj;
        return 0;
    }

    TtcObList* list = logicType == 0x17 ? &m_list2 : &m_list1;
    list->AddTail(obj);
    if (logicType == 0x15 && (a1 == 0x67 || a1 == 0x68)) {
        m_70 = obj;
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
// new; field-fill + rep-movs + AddTail identical (arg->field mapping approximate).
RVA(0x00116cf0, 0x111)
CGiantRockLogic*
CTileTriggerContainer::AddToList1(i32 a1, i32 a2, i32* block9, i32 a4, i32 a5, i32 a6, i32 a7) {
    CGiantRockLogic* e = new CGiantRockLogic;
    if (e == 0) {
        return 0;
    }
    if (e->m_1c != 0) {
        // The inlined failure-path delete runs the BASE dtor (retail stamps
        // ??_7CTileTriggerLogic @0x5eaea4, not the rock's own vtable): the devs
        // deleted through a CTileTriggerLogic* with the non-virtual dtor.
        CTileTriggerLogic* dead = e;
        dead->~CTileTriggerLogic();
        ::operator delete(dead);
        return 0;
    }
    for (i32 i = 0; i < 9; i++) {
        e->m_matrix[i] = block9[i];
    }
    e->m_c0 = a4;
    e->m_c4 = a6;
    e->m_0c = a2;
    e->m_typeTag = TRIGID_GIANT_ROCK_22;
    e->m_08 = a1;
    e->m_10 = (i32)block9;
    e->m_20 = this;
    e->m_1c = 1;
    e->m_dutyOn = 0;
    e->m_24 = g_645588;
    e->m_28 = 0;
    e->m_34 = 0;
    e->m_2c = 0;
    e->m_30 = 0;
    e->m_30 = a7;
    e->m_24 = g_645588;
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
        CTileTriggerLogic* elem = (CTileTriggerLogic*)cur->m_data;
        if (elem == (CTileTriggerLogic*)data) {
            if (elem != 0) {
                // the inlined `delete elem`: ~CTileTriggerLogic restamps the vptr
                // (??_7CTileTriggerLogic @0x5eaea4) + clears m_1c, then RezFree.
                elem->~CTileTriggerLogic();
                ::operator delete(elem);
            }
            m_list1.RemoveAt((POSITION)cur);
            return 1;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::FindChild
// Walks m_base (head @ +0x04; node next@+0x00, data@+0x08); returns the first
// switch-logic element whose m_key1 == k1 and (k2==0 || its type id m_04 == k2),
// else NULL. CONTAINER method: VerifyBlockLinks reaches it as m_owner->FindChild
// where m_owner is the container LoadElement stamps.
// ---------------------------------------------------------------------------
RVA(0x00116ee0, 0x2f)
CTileTriggerSwitchLogic* CTileTriggerContainer::FindChild(i32 k1, i32 k2) {
    TtcNode* node = TtcHead(m_base);
    while (node) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerSwitchLogic* data = (CTileTriggerSwitchLogic*)cur->m_data;
        if (data->m_key1 == k1) {
            if (k2 == 0 || data->m_04 == k2) {
                return data;
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::FindInLists12
// Scans list1 (head @ +0x20) then list2 (head @ +0x3c) for the first element
// whose +0x10 == a and (b == 0 || +0x04 == b); returns the element, else NULL.
// ---------------------------------------------------------------------------
RVA(0x00116f20, 0x51)
CTileTriggerLogic* CTileTriggerContainer::FindInLists12(i32 a, i32 b) {
    TtcNode* node = TtcHead(m_list1);
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            CTileTriggerLogic* elem = (CTileTriggerLogic*)cur->m_data;
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
            CTileTriggerLogic* elem = (CTileTriggerLogic*)cur->m_data;
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

// ---------------------------------------------------------------------------
// CTileTriggerContainer::RemoveAll
// Empties all four lists, inline-destroying every element (stamp its vtable +
// clear a field + RezFree) before RemoveAll'ing the list, then clears m_70.  The
// element type/cleared-field differs per list: m_list1 / m_list2 hold grid
// commands (vtable 0x5eaea4, clear +0x1c); m_base holds switch siblings (vtable
// 0x5eae8c, clear +0x20); m_list3 holds plain records (clear +0x10, no stamp).
// ---------------------------------------------------------------------------
RVA(0x00116fa0, 0xc7)
void CTileTriggerContainer::RemoveAll() {
    TtcNode* node = TtcHead(m_list1);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerLogic* elem = (CTileTriggerLogic*)cur->m_data;
        if (elem != 0) {
            elem->~CTileTriggerLogic(); // vptr 0x5eaea4 restamp + m_1c = 0
            ::operator delete(elem);
        }
    }
    m_list1.RemoveAll();
    node = TtcHead(m_base);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerSwitchLogic* elem = (CTileTriggerSwitchLogic*)cur->m_data;
        if (elem != 0) {
            elem->~CTileTriggerSwitchLogic(); // vptr 0x5eae8c restamp + m_20 = 0
            ::operator delete(elem);
        }
    }
    m_base.RemoveAll();
    node = TtcHead(m_list2);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileTriggerLogic* elem = (CTileTriggerLogic*)cur->m_data;
        if (elem != 0) {
            elem->~CTileTriggerLogic(); // vptr 0x5eaea4 restamp + m_1c = 0
            ::operator delete(elem);
        }
    }
    m_list2.RemoveAll();
    node = TtcHead(m_list3);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileActionEvent* elem = (CTileActionEvent*)cur->m_data;
        if (elem != 0) {
            elem->~CTileActionEvent(); // m_10 = 0 (no vtable -> no stamp)
            ::operator delete(elem);
        }
    }
    m_list3.RemoveAll();
    m_70 = 0;
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
            CTileTriggerLogic* elem = (CTileTriggerLogic*)cur->m_data;
            i32 r = elem->Classify((i32)arg);
            if (r == 0) {
                m_list2.RemoveAt((POSITION)cur);
                if (elem != 0) {
                    elem->~CTileTriggerLogic(); // vptr 0x5eaea4 restamp + m_1c = 0
                    ::operator delete(elem);
                }
            } else if (r == -1) {
                m_list2.RemoveAt((POSITION)cur);
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
            m_list1.RemoveAt((POSITION)cur);
            m_list2.AddTail(elem);
            *((i32*)elem + 14) = 0; // elem+0x38
            return 1;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::FindByField0C
// Walks m_list3 (head @ +0x58); returns the first CTileActionEvent whose cell
// key m_c == key, else NULL. CONTAINER method (on the switch-logic view +0x58
// fell inside m_block[11] - an overrun tell); this is SetCell's "FindByKey"
// (ILT 0x2838 jmps here).
// ---------------------------------------------------------------------------
RVA(0x001171d0, 0x20)
CTileActionEvent* CTileTriggerContainer::FindByField0C(i32 key) {
    TtcNode* node = TtcHead(m_list3);
    while (node) {
        TtcNode* cur = node;
        node = node->m_next;
        CTileActionEvent* data = (CTileActionEvent*)cur->m_data;
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
        CTileActionEvent* elem = (CTileActionEvent*)node->m_data;
        if (elem == (CTileActionEvent*)data) {
            if (elem != 0) {
                elem->~CTileActionEvent(); // m_10 = 0 (no vtable -> no stamp)
                ::operator delete(elem);
            }
            m_list3.RemoveAt((POSITION)node);
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
            if (SerializeApplyA(s, 4, a3, a4, (CTileTriggerSwitchLogic*)node->m_data) == 0) {
                return 0;
            }
        }
        cnt = m_list1.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_list1); node != 0; node = node->m_next) {
            if (SerializeApplyB(s, 4, a3, a4, (CTileTriggerLogic*)node->m_data) == 0) {
                return 0;
            }
        }
        cnt = m_list2.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_list2); node != 0; node = node->m_next) {
            if (SerializeApplyB(s, 4, a3, a4, (CTileTriggerLogic*)node->m_data) == 0) {
                return 0;
            }
        }
        cnt = m_list3.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_list3); node != 0; node = node->m_next) {
            if (((CTileActionEvent*)node->m_data)->Serialize(s, 4, a3, a4) == 0) {
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
    i32 tag = o->m_04;
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
            return ((CGiantRockLogic*)o)->ApplyByType(s, a2, a3, a4) != 0;
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

// TileTriggerFactory.cpp - the tile-trigger logic factory @0x117800 (proximity
// CTileTriggerContainer / CTileTriggerSwitchLogic). A __thiscall(reader, 7, a2, a3)
// ret 0x10 that reads a 4-byte type id off the serialized `reader` (vtable slot 11 =
// +0x2c), then dispatches a dense `switch(id)` (ids 1..26, the MSVC compact byte-index
// + jump-table idiom: index table @0x517cbc, jump table @0x517c80) to build the matching
// trigger-logic object. Each case Rez-allocates the object (0x8c / 0x9c / 0xc8 bytes),
// runs its 0-arg ctor thunk, then a 4-arg register thunk (0x277f for ids 1..8, 0x1abe
// for ids 23..26 + 21, 0x1d39 for id 22), stamps the owner + id, and returns it. id 21
// additionally resolves the board tile under the object and, on a 0x67/0x68 tile, latches
// the object into this->m_70.
//
// Field names are placeholders; only OFFSETS + code bytes are load-bearing. The ctor /
// register thunks are reloc-masked externals (no body); the type id map (id-1):
//   0..7 -> cases 0..7   8..19 -> default(0)   20..25 -> cases 8..13

// The serialized type id is read off the shared CSerialArchive stream (Read @
// vtable slot 11, +0x2c); `mov eax,[r]; call [eax+0x2c]` falls out with no cast.

// DE-VIEW (2026-07-13): the `CTileTriggerFactory` view was CTileTriggerContainer - this
// very class. Proof, all already in the tree: (1) <Gruntz/TileTriggerContainer.h> ALREADY
// declares `void* LoadElement(CSerialArchive*, i32, i32, i32); // 0x117800` - the exact RVA
// the view's Build defined, so 0x117800 was double-claimed and the container's own decl was
// a phantom; (2) the view's only field, +0x70, is the container's `CTileTriggerLogic* m_70`
// (same offset, same id-21 latch role); (3) the object it builds back-stamps the factory
// into its owner slot, and CTileTriggerLogic::m_20 is typed `CTileTriggerContainer*`.
//
// DE-VIEW (2026-07-13, Fable lane): CTrigLogic + its 11 ctor tags + the 3 size buckets
// (CTrigLogic8c/9c/C8) are GONE. The bucket premise was refuted: every per-id leaf is a
// real class at exactly the bucketed size (the 0x8c switch family in
// TileTriggerSwitchLogic.h, the 0x9c logic family + the 0xc8 CGiantRockLogic in
// TileTriggerLogic.h), so each case `new`s the real class. The three "register thunks"
// resolve to three REAL methods (retail ILT jmps): 0x277f -> 0x113860
// CTileTriggerSwitchLogic::ValidateByType, 0x1abe -> 0x113a90
// CTileTriggerLogic::ValidateByType, 0x1d39 -> 0x113d40 CGiantRockLogic::ApplyByType -
// all __thiscall on the built ELEMENT (retail `mov ecx,esi` before each), not the
// receiver-dropping `__stdcall Gate113860` the old model called. The +0x24-vs-+0x20
// owner-stamp split is the two FAMILIES' owner fields: CTileTriggerSwitchLogic::m_owner
// @+0x24 vs CTileTriggerLogic::m_20 @+0x20 - and the +0x24 attribution conflict is
// SETTLED: the owner is the CONTAINER (ModeObjInit constructs the receiver as this
// 4-CPtrList container; VerifyBlockLinks' m_owner->+0x20 walk reads m_list1's head).

// Build tail for the 0x8c switch-logic family (ids 1..8): register, stamp owner+id.
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
    obj->m_04 = id;
    return obj;
}

// Build tail for the 0x9c logic family (ids 23..26): register, stamp owner+id.
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
    obj->m_20 = self;
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
            obj->m_20 = this;
            obj->m_typeTag = id;
            // resolve the board tile under the object; latch on a 0x67/0x68 tile.
            CGameLevel* level = g_gameReg->m_world->m_24;
            i32 x = obj->m_08;
            i32 y = obj->m_0c;
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
            if (tile == (i32)0xeeeeeeee || tile == -1) {
                type = 0;
            } else {
                // m_imageSets' CObArray payload -> the CTileImageSet collision record;
                // retail pushes two zeros: GetCollisionAt(0, 0) (the 0-arg "TypeId"
                // view mis-modeled this slot).
                CTileImageSet* rec = (CTileImageSet*)level->m_imageSets.GetData()[tile & 0xffff];
                type = rec->GetCollisionAt(0, 0);
            }
            if (type == 0x67 || type == 0x68) {
                this->m_70 = obj;
            }
            return obj;
        }
        case 22: {
            CGiantRockLogic* obj = new CGiantRockLogic;
            if (obj->ApplyByType(reader, 7, a2, a3) == 0) {
                return 0;
            }
            obj->m_20 = this;
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

// ---------------------------------------------------------------------------
// CTileTriggerContainer::TransferFlag74
// If the stream is null returns 0; if the active game-manager (g_gameReg+0x30)
// is null returns 0; otherwise writes the 4-byte m_74 latch through the
// stream's write slot (+0x30) and returns 1. CONTAINER method (its own
// Serialize op-4 close; +0x74 is the container's m_74, not m_block[18]).
// ---------------------------------------------------------------------------
RVA(0x00117e20, 0x36)
i32 CTileTriggerContainer::TransferFlag74(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_74, 4);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::LoadFlag74
// The read counterpart of TransferFlag74: same null/game-manager gates, but
// reads m_74 through the stream's read slot (+0x2c). Returns 1 on success.
// ---------------------------------------------------------------------------
RVA(0x00117e70, 0x36)
i32 CTileTriggerContainer::LoadFlag74(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_74, 4);
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
            CGiantRockLogic* r = (CGiantRockLogic*)FindInLists12(py + base, TRIGID_GIANT_ROCK_22);
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
