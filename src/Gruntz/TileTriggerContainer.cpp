// TileTriggerContainer.cpp - Gruntz CTileTriggerContainer (C:\Proj\Gruntz).
//
// A 4-CObList container: a base sub-object at +0x00 (head +0x04) plus three more
// CObList members at +0x1c / +0x38 / +0x54 (heads +0x20 / +0x3c / +0x58).  The
// accessors find/move/remove heap command elements (CTileGridCommand) across the
// lists; an element is destroyed inline (stamp vtable + RezFree) before its node
// is unlinked.  The /GX ~dtor + RemoveAll empty all four lists; AddToList1/3
// allocate+append elements; SetCell flags a keyed element; SerializeApplyA/B are
// the tag-dispatched serialize helpers of the big serialize walk (0x117280).
//
// The dynamic this-tracer originally lumped these RVAs under
// CTileTriggerSwitchLogic; they are a DIFFERENT shape (verified by the EH dtor
// 0xc8640 destroying three CObList sub-objects at +0x1c/+0x38/+0x54 and the base
// at +0x00 - not the +0x04 child-list switch-logic layout).
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + the emitted
// code bytes are load-bearing (campaign doctrine).
#include <rva.h>
#include <Gruntz/TileActionEvent.h>
#include <Gruntz/TileGridCommand.h>
#include <Mfc.h>
#include <Gruntz/TileTriggerContainer.h>

// 0x113860 - Gate113860: mode gate over a container element - validate `obj`
// against the mode (4 -> the write-check 0x4499, 7 -> the read-check 0x1893, both
// TileTriggerSwitchLogic-family helpers), passing through otherwise. __stdcall,
// ret 0x10. The __stdcall helper SerializeApplyA / CTileTriggerFactory::Build call.
// Re-homed from src/Stub/BoundaryLowerMethods.cpp (was the Gate113860 placeholder).
extern i32 __stdcall Func1893(void* p); // 0x1893 -> 0x1139a0
extern i32 __stdcall Func4499(void* p); // 0x4499 -> 0x1138b0
// @early-stop
// regalloc wall (~93%): retail keeps obj in eax (so the obj==0 return 0 is free); cl
// pins it in ecx and adds xor eax. switch(mode) recovers the case layout; the eax vs
// ecx pick is not source-steerable.
RVA(0x00113860, 0x3b)
i32 __stdcall Gate113860(void* obj, i32 mode, i32 a3, i32 a4) {
    if (obj == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!Func4499(obj)) {
                return 0;
            }
            break;
        case 7:
            if (!Func1893(obj)) {
                return 0;
            }
            break;
    }
    return 1;
}

// The list1/list2 command element: its data is compared against an arg by the
// CTileGridCommand classifier (RVA 0x112970, a __thiscall returning 0/-1/+1).
SIZE_UNKNOWN(TtcElem);

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
    TtcNode* node = m_list1.m_pNodeHead;
    if (node == 0) {
        return 0;
    }
    do {
        TtcNode* cur = node;
        node = node->m_next;
        CTileGridCommand* elem = (CTileGridCommand*)cur->m_data;
        if (elem == (CTileGridCommand*)data) {
            if (elem != 0) {
                // vptr restore compiler-managed via CTileGridCommand's real vtable; manual stamp dropped
                elem->m_1c = 0;
                RezFree(elem);
            }
            m_list1.RemoveAt(cur);
            return 1;
        }
    } while (node != 0);
    return 0;
}

// ---------------------------------------------------------------------------
// CTileTriggerContainer::FindInLists12
// Scans list1 (head @ +0x20) then list2 (head @ +0x3c) for the first element
// whose +0x10 == a and (b == 0 || +0x04 == b); returns the element, else NULL.
// ---------------------------------------------------------------------------
RVA(0x00116f20, 0x51)
void* CTileTriggerContainer::FindInLists12(i32 a, i32 b) {
    TtcNode* node = m_list1.m_pNodeHead;
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
    node = m_list2.m_pNodeHead;
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
// CTileTriggerContainer::FilterList2
// Walks list2 (head @ +0x3c); classifies each element via CTileGridCommand
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
    TtcNode* node = m_list2.m_pNodeHead;
    if (node != 0) {
        do {
            TtcNode* cur = node;
            node = node->m_next;
            CTileGridCommand* elem = (CTileGridCommand*)cur->m_data;
            i32 r = elem->Classify((i32)arg);
            if (r == 0) {
                m_list2.RemoveAt(cur);
                if (elem != 0) {
                    // vptr restore compiler-managed via CTileGridCommand's real vtable; manual stamp dropped
                    elem->m_1c = 0;
                    RezFree(elem);
                }
            } else if (r == -1) {
                m_list2.RemoveAt(cur);
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
    TtcNode* node = m_list1.m_pNodeHead;
    if (node == 0) {
        return 0;
    }
    do {
        TtcNode* cur = node;
        node = node->m_next;
        void* elem = cur->m_data;
        if (elem == data) {
            m_list1.RemoveAt(cur);
            m_list2.AddTail(elem);
            *((i32*)elem + 14) = 0; // elem+0x38
            return 1;
        }
    } while (node != 0);
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
    for (TtcNode* node = m_list3.m_pNodeHead; node != 0; node = node->m_next) {
        i32* elem = (i32*)node->m_data;
        if (elem == (i32*)data) {
            if (elem != 0) {
                elem[4] = 0; // elem+0x10
                RezFree(elem);
            }
            m_list3.RemoveAt(node);
            return 1;
        }
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
    TtcNode* node = m_list1.m_pNodeHead;
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        i32* elem = (i32*)cur->m_data;
        if (elem != 0) {
            // vptr restore compiler-managed via CTileGridCommand's real vtable; manual stamp dropped
            elem[7] = 0; // +0x1c
            RezFree(elem);
        }
    }
    m_list1.RemoveAll();
    node = m_base.m_pNodeHead;
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        i32* elem = (i32*)cur->m_data;
        if (elem != 0) {
            // inline-dtor base-vptr restore dropped (compiler-managed; % ok)
            elem[8] = 0; // +0x20
            RezFree(elem);
        }
    }
    m_base.RemoveAll();
    node = m_list2.m_pNodeHead;
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        i32* elem = (i32*)cur->m_data;
        if (elem != 0) {
            // vptr restore compiler-managed via CTileGridCommand's real vtable; manual stamp dropped
            elem[7] = 0; // +0x1c
            RezFree(elem);
        }
    }
    m_list2.RemoveAll();
    node = m_list3.m_pNodeHead;
    while (node != 0) {
        TtcNode* cur = node;
        node = node->m_next;
        i32* elem = (i32*)cur->m_data;
        if (elem != 0) {
            elem[4] = 0; // +0x10
            RezFree(elem);
        }
    }
    m_list3.RemoveAll();
    m_70 = 0;
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

// ---------------------------------------------------------------------------
// CTileTriggerContainer::Dtor
// The /GX destructor: runs the most-derived teardown (DtorBase) then destroys the
// four CObList sub-objects in reverse declaration order (m_list3, m_list2,
// m_list1, m_base), each at its own EH trylevel.
// ---------------------------------------------------------------------------
RVA(0x000c8640, 0x70)
CTileTriggerContainer::~CTileTriggerContainer() {
    DtorBase();
    // m_list3 / m_list2 / m_list1 / m_base are auto-destroyed here (reverse decl
    // order) by the compiler-emitted /GX member teardown.
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
        RezFree(m);
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
        RezFree(e);
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
        RezFree(m);
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
        i32 cnt = m_base.m_0c;
        s->Write(&cnt, 4);
        for (node = m_base.m_pNodeHead; node != 0; node = node->m_next) {
            if (SerializeApplyA(s, 4, a3, a4, (TtcTrigElem*)node->m_data) == 0) {
                return 0;
            }
        }
        cnt = m_list1.m_0c;
        s->Write(&cnt, 4);
        for (node = m_list1.m_pNodeHead; node != 0; node = node->m_next) {
            if (SerializeApplyB(s, 4, a3, a4, (TtcTrigElem*)node->m_data) == 0) {
                return 0;
            }
        }
        cnt = m_list2.m_0c;
        s->Write(&cnt, 4);
        for (node = m_list2.m_pNodeHead; node != 0; node = node->m_next) {
            if (SerializeApplyB(s, 4, a3, a4, (TtcTrigElem*)node->m_data) == 0) {
                return 0;
            }
        }
        cnt = m_list3.m_0c;
        s->Write(&cnt, 4);
        for (node = m_list3.m_pNodeHead; node != 0; node = node->m_next) {
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

// @early-stop
// 0x115b60 (151 B) = a __thiscall predicate walking this->m_2c's chain and dispatching a
// vtable slot (+0x44/+0x68) - a tile-trigger container query. Homed from GapFunctions.cpp
// (matcher-5) by RVA neighbourhood (this TU's .text block brackets 0x115b60). Homed pending
// full reconstruction of the receiver's field/vtable model.
RVA(0x00115b60, 0x97)
i32 Gap_115b60(void) {
    return 0;
}
