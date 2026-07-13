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
#include <rva.h>
#include <new> // Rez heap throwing operator new / nothrow delete (0x1b9b46 / 0x1b9b82)

#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileGridCommand.h>
#include <Gruntz/TileTriggerContainer.h>
#include <Gruntz/TileTriggerLogic.h> // CTileTriggerLogic + the per-id leaves AddLogic news
#include <Gruntz/TileTriggerSwitchLogic.h>
#include <Gruntz/TileTriggerWiring.h>

// The *0x24556c singleton. Declared here: <Gruntz/TileGridCommand.h>'s header-level decl was
// removed so each TU picks the view/real class it needs (see the note in Play.h). Type unchanged.
extern "C" CGameRegistry* g_gameReg;

// The mode gate over a container element (defined in TileTriggerSwitchLogic.cpp,
// its RVA 0x113860 sits in that TU's interval); reloc-masked rel32 callee of
// SerializeApplyA/B + CTileTriggerFactory::Build.
i32 __stdcall Gate113860(void* obj, i32 mode, i32 a3, i32 a4);

// The list1/list2 command element: its data is compared against an arg by the
// CTileTriggerLogic classifier (RVA 0x112970, a __thiscall returning 0/-1/+1).
SIZE_UNKNOWN(TtcElem);

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

// GetFlag74 (0x115f00): test-and-set the m_block[18] latch (1 on first call, then
// 0). Out-of-line (retail emits it standalone; the inline member folded away).
RVA(0x00115f00, 0x13)
i32 CTileTriggerSwitchLogic::GetFlag74() {
    if (m_block[18] != 0) {
        return 0;
    }
    m_block[18] = 1;
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
// @source: rtti-vptr
// @stub
RVA(0x00115f60, 0x2de)
void CTileTriggerSwitchLogic::CTileTriggerSwitchLogic_115f60() {}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::RemoveByKeys
// Walks the child list (head @ +0x04, an MFC CPtrList: node next@0, prev@4, data@8);
// on the first child whose +0x04 == k2 and +0x10 == k1, deletes the child (inlined
// dtor vptr-restamp + RezFree), unlinks the node via CPtrList::RemoveAt, returns 1.
// Returns 0 if no match. The loop is the inlined CPtrList::GetNext idiom: a saved
// position (cur, esi) + the GetNext local (pn, ecx) are two distinct node copies;
// pn advances node then derefs data, so retail materializes the extra `mov ecx,eax`
// copy and defers the data load past the advance.
// ---------------------------------------------------------------------------
RVA(0x00116320, 0x66)
i32 CTileTriggerSwitchLogic::RemoveByKeys(i32 k1, i32 k2) {
    ListNode* node = (ListNode*)m_04;
    while (node) {
        ListNode* cur = node; // savePos (esi)
        ListNode* pn = node;  // GetNext local (ecx)
        node = node->m_next;
        CTileTriggerSwitchLogic* data = pn->m_data;
        if (data->m_04 == k2 && data->m_key1 == k1) {
            if (data) {
                // The inlined `delete data`: the dtor restamps the vptr
                // (`mov [data],offset ??_7`) + clears m_20, then RezFree frees it.
                data->~CTileTriggerSwitchLogic();
                ::operator delete(data);
            }
            // this's leading {vptr,head,tail,count} overlay a CPtrList (own vtable
            // 0x1eae8c differs, so no inheritance) - retail reuses CPtrList::RemoveAt
            // (0x1b4ac7) on it directly; cur is the node position.
            ((CPtrList*)this)->RemoveAt((POSITION)cur);
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
TtcMark*
CTileTriggerContainer::AddToList3(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8) {
    TtcMark* raw = (TtcMark*)RezAlloc(0x28);
    TtcMark* m = raw != 0 ? TtcMarkCtor(raw) : 0;
    if (m == 0) {
        return 0;
    }
    if (m->m_10 != 0) {
        m->m_10 = 0;
        ::operator delete(m);
        return 0;
    }
    m->m_04 = a2;
    m->m_08 = a3;
    m->m_0c = a4;
    m->m_18 = a5;
    m->m_1c = a6;
    m->m_24 = a8;
    m->m_00 = a1;
    m->m_14 = this;
    m->m_10 = 1;
    m->m_20 = a7;
    ((CTileActionEvent*)m)->SetActionCode((i32)a1);
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
TtcMark* CTileTriggerContainer::AddToList3Switch(i32 a1, i32 a2, i32 a3, i32 a4, i32 type) {
    TtcMark* raw = (TtcMark*)RezAlloc(0x28);
    TtcMark* m = raw != 0 ? TtcMarkCtor(raw) : 0;
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
    m->m_04 = a2;
    m->m_08 = a3;
    m->m_0c = a4;
    m->m_20 = b;
    m->m_00 = a1;
    m->m_14 = this;
    m->m_10 = 1;
    m->m_18 = d;
    m->m_1c = c;
    m->m_24 = a;
    ((CTileActionEvent*)m)->SetActionCode((i32)a1);
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
TtcBaseElem*
CTileTriggerContainer::AddToList1(i32 a1, i32 a2, i32* block9, i32 a4, i32 a5, i32 a6, i32 a7) {
    TtcBaseElem* raw = (TtcBaseElem*)RezAlloc(0xc8);
    TtcBaseElem* e = raw != 0 ? TtcBaseElemCtor(raw) : 0;
    if (e == 0) {
        return 0;
    }
    if (e->m_1c != 0) {
        // inline-dtor base-vptr restore dropped (compiler-managed; % ok)
        e->m_1c = 0;
        ::operator delete(e);
        return 0;
    }
    for (i32 i = 0; i < 9; i++) {
        e->m_9c[i] = block9[i];
    }
    e->m_c0 = a4;
    e->m_c4 = a6;
    e->m_0c = a2;
    e->m_04 = 0x16;
    e->m_08 = a1;
    e->m_10 = block9;
    e->m_20 = this;
    e->m_1c = 1;
    e->m_38 = 0;
    e->m_24 = (i32)g_645588;
    e->m_28 = 0;
    e->m_34 = 0;
    e->m_2c = 0;
    e->m_30 = 0;
    e->m_30 = a7;
    e->m_24 = (i32)g_645588;
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
                // vptr restore compiler-managed via CTileTriggerLogic's real vtable; manual stamp dropped
                elem->m_1c = 0;
                ::operator delete(elem);
            }
            m_list1.RemoveAt((POSITION)cur);
            return 1;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::FindChild
// Walks the child list (head @ +0x04; node next@+0x00, data@+0x08); returns the
// first child whose +0x10 == k1 and (k2==0 || +0x04 == k2), else NULL.
// ---------------------------------------------------------------------------
RVA(0x00116ee0, 0x2f)
CTileTriggerSwitchLogic* CTileTriggerSwitchLogic::FindChild(i32 k1, i32 k2) {
    ListNode* node = (ListNode*)m_04;
    while (node) {
        ListNode* cur = node;
        node = node->m_next;
        CTileTriggerSwitchLogic* data = cur->m_data;
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
void* CTileTriggerContainer::FindInLists12(i32 a, i32 b) {
    TtcNode* node = TtcHead(m_list1);
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            i32* elem = (i32*)cur->m_data;
            if (elem[4] == a) {
                if (b == 0) {
                    return elem;
                }
                if (elem[1] == b) {
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
            i32* elem = (i32*)cur->m_data;
            if (elem[4] == a) {
                if (b == 0) {
                    return elem;
                }
                if (elem[1] == b) {
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
        i32* elem = (i32*)cur->m_data;
        if (elem != 0) {
            // vptr restore compiler-managed via CTileTriggerLogic's real vtable; manual stamp dropped
            elem[7] = 0; // +0x1c
            ::operator delete(elem);
        }
    }
    m_list1.RemoveAll();
    node = TtcHead(m_base);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        i32* elem = (i32*)cur->m_data;
        if (elem != 0) {
            // inline-dtor base-vptr restore dropped (compiler-managed; % ok)
            elem[8] = 0; // +0x20
            ::operator delete(elem);
        }
    }
    m_base.RemoveAll();
    node = TtcHead(m_list2);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        i32* elem = (i32*)cur->m_data;
        if (elem != 0) {
            // vptr restore compiler-managed via CTileTriggerLogic's real vtable; manual stamp dropped
            elem[7] = 0; // +0x1c
            ::operator delete(elem);
        }
    }
    m_list2.RemoveAll();
    node = TtcHead(m_list3);
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        i32* elem = (i32*)cur->m_data;
        if (elem != 0) {
            elem[4] = 0; // +0x10
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
                    // vptr restore compiler-managed via CTileTriggerLogic's real vtable; manual stamp dropped
                    elem->m_1c = 0;
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
// CTileTriggerSwitchLogic::FindByField0C
// Walks the list at +0x58; returns the first child whose +0x0c == key, else NULL.
// ---------------------------------------------------------------------------
RVA(0x001171d0, 0x20)
CTileTriggerSwitchLogic* CTileTriggerSwitchLogic::FindByField0C(i32 key) {
    ListNode* node = (ListNode*)m_block[11];
    while (node) {
        ListNode* cur = node;
        node = node->m_next;
        CTileTriggerSwitchLogic* data = cur->m_data;
        if (data->m_key0c == key) {
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
        i32* elem = (i32*)node->m_data;
        if (elem == (i32*)data) {
            if (elem != 0) {
                elem[4] = 0; // elem+0x10
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
            if (SerializeApplyA(s, 4, a3, a4, (TtcTrigElem*)node->m_data) == 0) {
                return 0;
            }
        }
        cnt = m_list1.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_list1); node != 0; node = node->m_next) {
            if (SerializeApplyB(s, 4, a3, a4, (TtcTrigElem*)node->m_data) == 0) {
                return 0;
            }
        }
        cnt = m_list2.GetCount();
        s->Write(&cnt, 4);
        for (node = TtcHead(m_list2); node != 0; node = node->m_next) {
            if (SerializeApplyB(s, 4, a3, a4, (TtcTrigElem*)node->m_data) == 0) {
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
        TtcMark* raw = (TtcMark*)RezAlloc(0x28);
        TtcMark* m = raw != 0 ? TtcMarkCtor(raw) : 0;
        if (((CTileActionEvent*)m)->Serialize(s, 7, a3, a4) == 0) {
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
// Streams the object's type tag, then for tags 1..7 applies operation A and for
// tag 8 applies it again as the trailing case; returns whether the apply
// succeeded (0 for the null object or an out-of-range tag).  __stdcall helper of
// the container's serialize walk (117280).
// ---------------------------------------------------------------------------
// @early-stop
// switch jump-table-vs-cmp-tree wall (~57%): logic identical; retail lowers the
// 8-tag switch to a jmp[tbl+(tag-1)*4] table, the recompile to a range-check tree
// (the two identical case bodies collapse).  See docs/patterns/switch-cmpje-tree-vs-jumptable.md
RVA(0x00117630, 0x82)
i32 __stdcall SerializeApplyA(CSerialArchive* s, i32 a2, i32 a3, i32 a4, TtcTrigElem* o) {
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
            return Gate113860((void*)s, a2, a3, a4) != 0;
        case 8:
            return Gate113860((void*)s, a2, a3, a4) != 0;
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// SerializeApplyB  (0x117710)
// Streams the object's type tag, then dispatches tags 0x15..0x1a: 0x16 applies
// operation B, the rest apply operation C; returns success.  __stdcall helper of
// the container's serialize walk (117280).
// ---------------------------------------------------------------------------
// @early-stop
// switch jump-table-vs-cmp-tree wall (~63%): logic identical; retail lowers the
// 6-tag switch to a jmp[tbl+(tag-0x15)*4] table, the recompile to a cmp tree.
// See docs/patterns/switch-cmpje-tree-vs-jumptable.md
RVA(0x00117710, 0xa6)
i32 __stdcall SerializeApplyB(CSerialArchive* s, i32 a2, i32 a3, i32 a4, TtcTrigElem* o) {
    if (o == 0) {
        return 0;
    }
    i32 tag = o->m_04;
    s->Write(&tag, 4);
    switch (tag) {
        case 0x16:
            return Gate113860((void*)s, a2, a3, a4) != 0;
        case 0x15:
        case 0x17:
        case 0x18:
        case 0x19:
            return Gate113860((void*)s, a2, a3, a4) != 0;
        case 0x1a:
            return Gate113860((void*)s, a2, a3, a4) != 0;
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

i32 __stdcall Gate113860(void* a, i32 b, i32 c, i32 d); // 0x113860 (TtcTrigElem::Reg* view)

// The serialized type id is read off the shared CSerialArchive stream (Read @
// vtable slot 11, +0x2c); `mov eax,[r]; call [eax+0x2c]` falls out with no cast.

// A board tile-object reached via g_gameReg->m_world->m_24->m_4c[tile]; slot 8 (+0x20)
// returns the tile's gameplay type id. Reloc-masked virtual.
struct CTileObj {
    virtual void s0();
    virtual void s1();
    virtual void s2();
    virtual void s3();
    virtual void s4();
    virtual void s5();
    virtual void s6();
    virtual void s7();
    virtual i32 TypeId(); // slot 8 (+0x20)
};
SIZE_UNKNOWN(CTileObj);

// The board geometry (g_gameReg->m_world->m_24): m_5c->m_28 / m_5c->m_2c are the x/y
// bounds, m_5c->m_24 the row base, m_5c->m_20 the cell->tile map, m_4c the tile-object
// table. Reached by raw offset (engine struct, modeled minimally).
struct CTrigBoardGeo {
    char m_pad00[0x24];
    i32* m_24row;                     // +0x24  row base (cell index = m_24row[y] + x)
    char m_pad28[0x20 - 0x28 + 0x20]; // pad to +0x20-relative kept raw below
};
SIZE_UNKNOWN(CTrigBoardGeo);
struct CTrigBoard {
    char m_pad00[0x4c];
    CTileObj** m_4c; // +0x4c  tile-object table (indexed by the resolved tile id & 0xffff)
    char m_pad50[0x5c - 0x50];
    i32* m_5c; // +0x5c  board geometry block (bounds @0x28/0x2c, row-base @0x24, cell-map @0x20)
};
SIZE_UNKNOWN(CTrigBoard);
struct CTrigMgrInner {
    char m_pad00[0x24];
    CTrigBoard* m_24; // +0x24
};
SIZE_UNKNOWN(CTrigMgrInner);
struct CTrigMgr {
    char m_pad00[0x30];
    CTrigMgrInner* m_world; // +0x30
};
SIZE_UNKNOWN(CTrigMgr);

// The built trigger-logic object. The 14 distinct ctor thunks (0-arg __thiscall,
// returning the object) and the 3 register thunks (4-arg __thiscall, returning success)
// are reloc-masked externals reached through the incremental-link thunk table.
//
// DISPOSITION (CTrigLogic8c/9c/C8): these are NOT single classes - each is a SIZE
// BUCKET (0x8c/0x9c/0xc8 bytes) that the switch allocates for SEVERAL distinct real
// leaf classes (e.g. CTrigLogic8c covers 6 different trigger-logic leaves). A single
// real class name can't stand for a size bucket, so the buckets stay size-tagged;
// but each per-id ctor selector IS one real RTTI class, resolved by ILT-jmp target
// and now named for it (the tag/New* pairs below -> CTileTriggerSwitchLogic /
// CTileMultiTriggerSwitchLogic / ... / CGiantRockLogic, defs in
// TileTriggerDerivedCtors.cpp + the two base ctors 0x110430/0x1107f0).
struct CTileTriggerFactory; // the owning factory (this); back-stamped into m_20/m_24

struct CTrigLogic {
    struct TileTriggerSwitchLogicTag {};
    struct TileMultiTriggerSwitchLogicTag {};
    struct TileExclusiveTriggerSwitchLogicTag {};
    struct TileSecretTriggerSwitchLogicTag {};
    struct TileTimeTriggerSwitchLogicTag {};
    struct CheckpointTriggerSwitchLogicTag {};
    struct TileTriggerLogicTag {};
    struct GiantRockLogicTag {};
    struct TileTimeTriggerLogicTag {};
    struct TileSecretTriggerLogicTag {};
    struct CoveredPowerupLogicTag {};

    i32 m_00; // +0x00
    i32 m_04; // +0x04  type id (the switch id 1..26)
    i32 m_08; // +0x08  (id 21: board x)
    i32 m_0c; // +0x0c  (id 21: board y)
    char m_pad10[0x20 - 0x10];
    CTileTriggerFactory* m_20; // +0x20  owner (1abe / 1d39 group)
    CTileTriggerFactory* m_24; // +0x24  owner (277f group)
    char m_pad28[0x74 - 0x28]; // +0x28..+0x73 (folds the unused +0x70 owner slot)

    // Each thunk (ILT 0xNNNN -> the real leaf ctor RVA) constructs one RTTI class;
    // resolved from the ILT jmp target (TileTriggerDerivedCtors.cpp / the base ctors).
    CTrigLogic*
    NewTileTriggerSwitchLogic(); // ILT 0x3206 -> ??0CTileTriggerSwitchLogic 0x110430 (ids 1,2,5)
    CTrigLogic*
    NewTileMultiTriggerSwitchLogic(); // ILT 0x3eb3 -> ??0CTileMultiTriggerSwitchLogic 0x111f10 (id 3)
    CTrigLogic*
    NewTileExclusiveTriggerSwitchLogic(); // ILT 0x4192 -> ??0CTileExclusiveTriggerSwitchLogic 0x112050 (id 4)
    CTrigLogic*
    NewTileSecretTriggerSwitchLogic(); // ILT 0x2db5 -> ??0CTileSecretTriggerSwitchLogic 0x112790 (id 6)
    CTrigLogic*
    NewTileTimeTriggerSwitchLogic(); // ILT 0x332d -> ??0CTileTimeTriggerSwitchLogic 0x1127c0 (id 7)
    CTrigLogic*
    NewCheckpointTriggerSwitchLogic(); // ILT 0x2f72 -> ??0CCheckpointTriggerSwitchLogic 0x1127f0 (id 8)
    CTrigLogic* NewTileTriggerLogic(); // ILT 0x43b3 -> ??0CTileTriggerLogic 0x1107f0 (ids 21,24)
    CTrigLogic* NewGiantRockLogic();   // ILT 0x2c3e -> ??0CGiantRockLogic 0x112210 (id 22)
    CTrigLogic*
    NewTileTimeTriggerLogic(); // ILT 0x18de -> ??0CTileTimeTriggerLogic 0x112270 (id 23)
    CTrigLogic*
    NewTileSecretTriggerLogic(); // ILT 0x310c -> ??0CTileSecretTriggerLogic 0x112760 (id 25)
    CTrigLogic* NewCoveredPowerupLogic(); // ILT 0x2a4f -> ??0CCoveredPowerupLogic 0x112240 (id 26)
    i32 Reg277f(void* r, i32 k, i32 a2, i32 a3); // 0x277f (ids 1..8)
    i32 Reg1abe(void* r, i32 k, i32 a2, i32 a3); // 0x1abe (ids 21,23..26)
    i32 Reg1d39(void* r, i32 k, i32 a2, i32 a3); // 0x1d39 (id 22)
};
SIZE_UNKNOWN(CTrigLogic);

struct CTrigLogic8c : public CTrigLogic {
    inline CTrigLogic8c(CTrigLogic::TileTriggerSwitchLogicTag) {
        NewTileTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::TileMultiTriggerSwitchLogicTag) {
        NewTileMultiTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::TileExclusiveTriggerSwitchLogicTag) {
        NewTileExclusiveTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::TileSecretTriggerSwitchLogicTag) {
        NewTileSecretTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::TileTimeTriggerSwitchLogicTag) {
        NewTileTimeTriggerSwitchLogic();
    }
    inline CTrigLogic8c(CTrigLogic::CheckpointTriggerSwitchLogicTag) {
        NewCheckpointTriggerSwitchLogic();
    }

    char m_sizePad[0x8c - 0x74];
};
SIZE_UNKNOWN(CTrigLogic8c);

struct CTrigLogic9c : public CTrigLogic {
    inline CTrigLogic9c(CTrigLogic::TileTriggerLogicTag) {
        NewTileTriggerLogic();
    }
    inline CTrigLogic9c(CTrigLogic::TileTimeTriggerLogicTag) {
        NewTileTimeTriggerLogic();
    }
    inline CTrigLogic9c(CTrigLogic::TileSecretTriggerLogicTag) {
        NewTileSecretTriggerLogic();
    }
    inline CTrigLogic9c(CTrigLogic::CoveredPowerupLogicTag) {
        NewCoveredPowerupLogic();
    }

    char m_sizePad[0x9c - 0x74];
};
SIZE_UNKNOWN(CTrigLogic9c);

struct CTrigLogicC8 : public CTrigLogic {
    inline CTrigLogicC8(CTrigLogic::GiantRockLogicTag) {
        NewGiantRockLogic();
    }

    char m_sizePad[0xc8 - 0x74];
};
SIZE_UNKNOWN(CTrigLogicC8);

// The factory container (this): the built object's owner; id 21 latches the object
// into this->m_70.
struct CTileTriggerFactory {
    char m_pad00[0x70];
    CTrigLogic* m_70; // +0x70  id-21 latches the built object here

    void* Build(CSerialArchive* reader, i32 kind, i32 a2, i32 a3); // 0x117800
};
SIZE_UNKNOWN(CTileTriggerFactory);

// Build the 277f-group object: alloc 0x8c, run `ctor`, register, stamp owner+id.
static void* Reg277fTail(
    CTileTriggerFactory* self,
    CTrigLogic* obj,
    CSerialArchive* reader,
    i32 a2,
    i32 a3,
    i32 id
) {
    if (Gate113860((void*)reader, 7, a2, a3) == 0) {
        return 0;
    }
    obj->m_24 = self;
    obj->m_04 = id;
    return obj;
}

// Build the 1abe-group object tail: register, stamp owner+id.
static void* Reg1abeTail(
    CTileTriggerFactory* self,
    CTrigLogic* obj,
    CSerialArchive* reader,
    i32 a2,
    i32 a3,
    i32 id
) {
    if (Gate113860((void*)reader, 7, a2, a3) == 0) {
        return 0;
    }
    obj->m_20 = self;
    obj->m_04 = id;
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
void* CTileTriggerFactory::Build(CSerialArchive* reader, i32 kind, i32 a2, i32 a3) {
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
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 3: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileMultiTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 4: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileExclusiveTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 6: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileSecretTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 7: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::TileTimeTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 8: {
            CTrigLogic* obj = new CTrigLogic8c(CTrigLogic::CheckpointTriggerSwitchLogicTag());
            return Reg277fTail(this, obj, reader, a2, a3, id);
        }
        case 21: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::TileTriggerLogicTag());
            if (Gate113860((void*)reader, 7, a2, a3) == 0) {
                return 0;
            }
            obj->m_20 = this;
            obj->m_04 = id;
            // resolve the board tile under the object; latch on a 0x67/0x68 tile.
            CTrigBoard* board = (CTrigBoard*)g_gameReg->m_world->m_24;
            i32 x = obj->m_08;
            i32 y = obj->m_0c;
            i32* geo = board->m_5c;
            if (x < 0) {
                x = 0;
            } else if (x >= geo[0x28 / 4]) {
                x = geo[0x28 / 4] - 1;
            }
            if (y < 0) {
                y = 0;
            } else if (y >= geo[0x2c / 4]) {
                y = geo[0x2c / 4] - 1;
            }
            i32* rowbase = (i32*)geo[0x24 / 4];
            i32 cell = rowbase[y] + x;
            i32 tile = ((i32*)geo[0x20 / 4])[cell];
            i32 type;
            if (tile == (i32)0xeeeeeeee || tile == -1) {
                type = 0;
            } else {
                type = board->m_4c[tile & 0xffff]->TypeId();
            }
            if (type == 0x67 || type == 0x68) {
                this->m_70 = obj;
            }
            return obj;
        }
        case 22: {
            CTrigLogic* obj = new CTrigLogicC8(CTrigLogic::GiantRockLogicTag());
            if (Gate113860((void*)reader, 7, a2, a3) == 0) {
                return 0;
            }
            obj->m_20 = this;
            obj->m_04 = id;
            return obj;
        }
        case 23: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::TileTimeTriggerLogicTag());
            return Reg1abeTail(this, obj, reader, a2, a3, id);
        }
        case 24: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::TileTriggerLogicTag());
            return Reg1abeTail(this, obj, reader, a2, a3, id);
        }
        case 25: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::TileSecretTriggerLogicTag());
            return Reg1abeTail(this, obj, reader, a2, a3, id);
        }
        case 26: {
            CTrigLogic* obj = new CTrigLogic9c(CTrigLogic::CoveredPowerupLogicTag());
            return Reg1abeTail(this, obj, reader, a2, a3, id);
        }
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::TransferFlag74
// If the stream is null returns 0; if the active game-manager (g_gameReg+0x30)
// is null returns 0; otherwise transfers the 4-byte +0x74 flag through the
// stream's Transfer (vtable slot 12) and returns 1.
// ---------------------------------------------------------------------------
RVA(0x00117e20, 0x36)
i32 CTileTriggerSwitchLogic::TransferFlag74(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Write(&m_block[18], 4);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::LoadFlag74
// The read counterpart of TransferFlag74: same null/game-manager gates, but
// reads the 4-byte +0x74 flag (== m_block[18]) through the stream's read slot
// (vtable +0x2c) instead of the write slot (+0x30). Returns 1 on success.
// ---------------------------------------------------------------------------
RVA(0x00117e70, 0x36)
i32 CTileTriggerSwitchLogic::LoadFlag74(CSerialArchive* s) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    s->Read(&m_block[18], 4);
    return 1;
}

// ---------------------------------------------------------------------------
// CTileTriggerSwitchLogic::ScanNeighborhood
// Scans the 3x3 cell neighborhood centered on (x, y): for px in [x-1, x+2) and
// py in [y-1, y+2), probes cell (py + (px << 8)) with kind 0x16; returns the
// first nonzero probe result, else 0.
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
i32 CTileTriggerSwitchLogic::ScanNeighborhood(i32 x, i32 y) {
    for (i32 px = x - 1; px < x + 2; px++) {
        i32 base = px << 8;
        for (i32 py = y - 1; py < y + 2; py++) {
            i32 r = ProbeCell(py + base, 0x16);
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
    TtcKeyedElem* elem = FindByKey(key);
    if (elem != 0) {
        if (verb == 5) {
            elem->m_flags[0] = 1;
            elem->m_flags[1] = 1;
            elem->m_flags[2] = 1;
            elem->m_flags[3] = 1;
        } else {
            elem->m_flags[verb] = 1;
        }
        ((CTileActionEvent*)elem)->SetActionCode((i32) * (void**)elem);
        return 1;
    }
    if (AddMark(key, 0x1a) != 0) {
        return 1;
    }
    return RunFallback(a, b) != 0;
}
