// SymParser.cpp - CSymParser, the Remus parser/owner that builds + owns the
// CSymTab scope tree (the object CSymTab::m_owner @+0x18 points back to). The five
// methods (trace group "ClassUnknown_11") all operate on one object shape that
// owns a heap root CSymTab at +0x44, an intrusive polymorphic-object list at +0x10
// (its own abstract sub-object vtable 0x5ef760) and an engine hash table at +0x80.
// See include/Bute/SymParser.h for the layout + the call-graph evidence.
#include <rva.h>

#include <Bute/SymParser.h>

// The two retail vtable groups (manual-stamp model; their virtuals live in other,
// unmatched TUs). Reloc-masked DATA() externs.
DATA(0x005ef750)
void* CSymParser_vftable;
DATA(0x005ef760)
void* CObjList_purecall_vftbl;

// ~CSymParser (0x13abc0): the /GX scalar destructor. Re-stamp the primary vtable,
// run Clear(0) if armed (m_0c), drain the +0x10 object list, free the heap root
// CSymTab + the owned buffers, drain the +0x88 node list, then RemoveAll the +0x80
// hash member (the trylevel-0 /GX member-teardown) and re-stamp the +0x10 list
// sub-object vtable. The +0x80 CHashBase auto-destructs after the body.
// @early-stop
// ~98%, all teardown bytes exact. Residual is the /GX EH-state machine + reloc
// naming: retail entry trylevel 2 + vptr-stamp scheduled into state 0 (early), vs
// recompile trylevel 1 + stamp after the trylevel write (eh-dtor-vptr-stamp-vs-
// trylevel-order wall), plus the differently-named Unwind/__except_list/PTR_*
// reloc operands (scoring artifact). Logic complete; parked for the final sweep.
RVA(0x0013abc0, 0x13f)
CSymParser::~CSymParser() {
    m_vtbl = &CSymParser_vftable;
    if (m_0c) {
        Clear(0);
    }
    CObjNode* p;
    for (p = m_list.m_head; p != 0; p = m_list.m_head) {
        m_list.Remove(p);
        m_list.m_count--;
        if (p) {
            p->Delete(1);
        }
    }
    CSymTab* root = m_root;
    if (root) {
        root->~CSymTab();
        RezFree(root);
        m_root = 0;
    }
    if (m_buf64) {
        RezFree(m_buf64);
        m_buf64 = 0;
    }
    if (m_buf04) {
        RezFree(m_buf04);
        m_buf04 = 0;
    }
    CSlotNode* node = (CSlotNode*)m_nodes.m_head;
    m_0c = 0;
    m_activeNode = 0;
    m_30 = 0;
    m_34 = 0;
    m_38 = 0;
    m_3c = 0;
    m_40 = 1;
    m_root = 0;
    m_48 = 0;
    m_4c = 0;
    m_50 = 1;
    m_54 = 0;
    m_58 = 0;
    m_5c = 0;
    m_60 = 0;
    m_08 = 1;
    m_buf64 = 0;
    if (node) {
        do {
            RezFree(node->m_08);
            m_nodes.Unlink(node);
            RezFree(node);
            node = (CSlotNode*)m_nodes.m_head;
        } while (node);
    }
    // m_hash (RemoveAll) then m_list (vptr restore to 0x5ef760) auto-destruct here,
    // in reverse declaration order, under the /GX member-teardown trylevels.
}

// Clear (0x13b850): drop the active node (m_activeNode) + the +0x10 object list,
// free the heap root CSymTab + the +0x64 buffer, then null m_0c. The arg is unused;
// the return is the active node's slot[5] (Detach) result, left in eax.
// @early-stop
// regalloc wall: retail pins `this`->edi + the walked node->esi; recompile swaps
// them (this->esi, node->edi) - same instruction stream, opposite callee-saved
// assignment. Body byte-exact modulo the register-naming; logic complete. ~91%,
// parked for the final sweep.
RVA(0x0013b850, 0xa8)
void* CSymParser::Clear(i32 final) {
    (void) final;
    void* r = m_activeNode->Detach();
    m_list.Remove(m_activeNode);
    m_list.m_count--;
    if (m_activeNode) {
        m_activeNode->Delete(1);
    }
    m_activeNode = 0;
    CObjNode* p;
    for (p = m_list.m_head; p != 0; p = m_list.m_head) {
        p->Detach();
        m_list.Remove(p);
        m_list.m_count--;
        if (p) {
            p->Delete(1);
        }
    }
    if (m_root) {
        m_root->~CSymTab();
        RezFree(m_root);
        m_root = 0;
    }
    if (m_buf64) {
        RezFree(m_buf64);
        m_buf64 = 0;
    }
    m_0c = 0;
    return r;
}

// ResolveQualified (0x13bff0): forward (name, arg) into GetRoot()'s CSymTab.
RVA(0x0013bff0, 0x19)
i32 CSymParser::ResolveQualified(const char* name, void* arg) {
    return GetRoot()->ResolveQualified(name, arg);
}

// ResolvePath (0x13c030): forward `path` into GetRoot()'s CSymTab.
RVA(0x0013c030, 0x14)
void* CSymParser::ResolvePath(const char* path) {
    return GetRoot()->ResolvePath(path);
}

// ReParse (0x13c050): if the parser is armed (m_0c non-null), drop the current
// parse state (Clear(0)) and re-parse the cached +0x64 source buffer; return that
// parse's result. Not armed -> return 0. (ParseBuffer @0x13ad00 is the big buffer
// parser, modeled as a reloc-masked extern.)
RVA(0x0013c050, 0x28)
i32 CSymParser::ReParse() {
    if (m_0c == 0) {
        return 0;
    }
    Clear(0);
    return ParseBuffer(m_buf64, 1, 0);
}

// AddNode (0x13c210): splice a record's intrusive node (rec+0x1c) into the +0x80
// hash table, when rec is non-null.
RVA(0x0013c210, 0x1a)
void CSymParser::AddNode(void* rec) {
    if (rec) {
        m_hash.Insert((CHashInsertNode*)((char*)rec + 0x1c));
    }
}

// A parse-slot record (0x3c bytes): a CRemusReadStream whose hash-node prefix is at
// +0x1c (stamped by Init) and whose self-ptr lives at +0x30. Init (0x1396f0) stamps
// the node vtable + nulls the body; reloc-masked __thiscall, no body here.
struct CParseSlot {
    char m_pad00[0x1c];
    void* m_node1c; // +0x1c  hash-node vtable prefix (the insert handle)
    char m_pad20[0x30 - 0x20];
    void* m_30; // +0x30  self-ptr
    char m_pad34[0x3c - 0x34];
    void Init(); // 0x1396f0
};

// PopParseSlot (0x13c0c0): see SymParser.h. The /GX frame guards the freshly Rez-
// alloc'd slot block while its elements are being initialized + registered.
// @early-stop
// EH-state + regalloc wall (~77%): logic complete. The node/array allocations land in
// a swapped callee-saved register (ebx vs ebp), the operator-new trylevel transitions
// + the slot-block down-counter init loop idiom diverge, and the hash-method reloc
// operands are differently named. Banked for the final sweep.
RVA(0x0013c0c0, 0x14b)
void* CSymParser::PopParseSlot() {
    CHashEntry* e = m_hash.First();
    void* rec = e ? e->m_rec : 0;
    if (rec == 0) {
        CSlotNode* node = (CSlotNode*)RezAlloc(0xc);
        if (node == 0) {
            return 0;
        }
        i32 n = m_90;
        CParseSlot* arr = (CParseSlot*)RezAlloc(n * 0x3c);
        if (arr) {
            CParseSlot* p = arr;
            i32 i = n;
            i--;
            if (i >= 0) {
                i++;
                do {
                    p->Init();
                    p++;
                    i--;
                } while (i);
            }
        }
        node->m_08 = arr;
        if (arr == 0) {
            RezFree(node);
            return 0;
        }
        for (i32 j = 0; (u32)j < (u32)m_90; j++) {
            CParseSlot* el = (CParseSlot*)((char*)node->m_08 + j * 0x3c);
            el->m_30 = el;
            m_hash.Insert((CHashInsertNode*)((char*)el + 0x1c));
        }
        m_nodes.Link(node);
        e = m_hash.First();
        rec = e->m_rec;
    }
    if (rec) {
        m_hash.Remove((CHashEntry*)((char*)rec + 0x1c));
    }
    return rec;
}

// CObjList::Remove (0x1852e0): unlink `node` from the intrusive {head@+4,tail@+8}
// chain (m_list at CSymParser+0x10). The node's links are m_next@+4 / m_prev@+8; a
// null prev/next means `node` was the head/tail. __thiscall on the list head,
// callee-cleanup of the single arg. (Trace-discovered; was the ClassUnknown_47 stub.)
RVA(0x001852e0, 0x35)
void CObjList::Remove(CObjNode* node) {
    if (node->m_prev) {
        node->m_prev->m_next = node->m_next;
    } else {
        m_head = node->m_next;
    }
    if (node->m_next) {
        node->m_next->m_prev = node->m_prev;
    } else {
        m_tail = node->m_prev;
    }
}
