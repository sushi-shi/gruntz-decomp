// Hash.h - CHash, the WAP32 engine string/int-keyed hash table embedded in
// CSymTab at +0x38 (child scopes) and +0x40 (leaf symbols). 8 bytes:
// { u32 m_count; CHashSlot* m_buckets; }.
//
// Shape recovered from the call graph (callers all in the 0x139../0x13a.. ButeMgr
// parser region): a NON-TEMPLATE base with the key-agnostic slot machinery
// (Lookup 0x184b40, Remove 0x184ab0, RemoveAll 0x184a40 - ONE physical copy each,
// called by both instances) and a TEMPLATE CHash<T> adding the key-typed lookups
// (HashStr/Walk/FindInt). CHash<T> is instantiated twice: the leaf-symbol table
// (T="_a": HashStr 0x13c240, Walk 0x13c270, HashInt 0x13c350, FindInt 0x13c360)
// and the child-scope table (T="_b": HashStr 0x13c3c0, Walk 0x13c3f0). The two
// instantiations are byte-identical code at distinct RVAs; because the pipeline
// binds each retail RVA to a concrete mangled symbol, they are modeled as the
// base CHashBase + two concrete derived classes CHash / CHashB (names are
// placeholders - campaign doctrine - so this is matching-neutral). See SymTab.h.
//
// Bucket array: m_buckets points just past a 4-byte count cookie (RezAlloc'd as
// CHashSlot[count]); each CHashSlot is 16 bytes { pad, CHashSlotList } - the
// intrusive doubly-linked chain head at slot+8/+0xc. A stored record embeds a
// CHashElement (the vtable-bearing key-hash prefix); the chain threads the
// element's link at element+4, so the chain pointer IS &element->m_link and the
// engine recovers the element via container_of (link - 4). The element carries the
// owning table @+0xc, the computed bucket @+0x10 and the (key,value) record @+0x14.
#ifndef SRC_BUTE_HASH_H
#define SRC_BUTE_HASH_H

#include <Ints.h>
#include <rva.h>
#include <string.h> // _strcmpi (0x11fdf0) / strcmp (/Oi intrinsic byte-loop) - the
                    // case-(in)sensitive compares the Hash.cpp lookups emit

// The Rez heap alloc/free (0x1b9b46 _RezAlloc = operator new / 0x1b9b82 _RezFree,
// both __cdecl); reloc-masked.
extern "C" void* RezAlloc(u32 size);
extern "C" void RezFree(void* p);

// The MSVC `'eh vector destructor iterator'` runtime (0x11f640): run `dtor` over
// `count` elements of `stride` bytes from `base`, descending, under an EH frame.
// __stdcall (callee-cleanup: retail has no `add esp,0x10` after the call); the
// reloc to it is masked. (TriggerMgrEh models the cdecl-shaped sibling; here the
// caller-side cleanup is absent, so this site's helper is __stdcall.)
void __stdcall Tm_DestroyArray(void* base, i32 stride, u32 count, void (*dtor)()); // 0x11f640

// The no-op per-element bucket-slot destructor (0x584a30, a bare `ret`); its
// address is passed to the array-delete. Modeled as a stub so the DIR32 reloc to
// it falls out.
void CHashSlot_Dtor(); // 0x584a30 (retail "empty_stub")

// The intrusive doubly-linked chain node threaded through each stored element at
// element+0x04 (next@+0, prev@+4). Chains store &element->m_link; the element is
// recovered via container_of (link - offsetof(CHashElement, m_link) == link - 4).
struct CHashLink {
    CHashLink* m_next; // +0x00 (element+0x04)
    CHashLink* m_prev; // +0x04 (element+0x08)
};
SIZE(CHashLink, 0x8);

class CHashBase;

// The intrusive {head,tail} chain-head embedded at each bucket slot's +0x08, and
// used standalone as CSymParser::m_nodes. Link splices `node` in (0x1390e0),
// Unlink removes it (0x1391e0); both __thiscall on the {head,tail} pair.
struct CHashSlotList {
    CHashLink* m_head; // +0x00  (slot+0x08)
    CHashLink* m_tail; // +0x04  (slot+0x0c)

    // The intrusive-list splice/unsplice ops (reloc-masked engine __thiscall on the
    // {head,tail} pair). Own methods so standalone m_nodes users reach them cast-free
    // (the old (DSoundList*) reinterpret was a cross-module fake view of this list).
    void Link(CHashLink* node);   // 0x1390e0  (== the InsertHead splice)
    void Unlink(CHashLink* node); // 0x1391e0
};
SIZE(CHashSlotList, 0x8);

// A 16-byte bucket slot: two opaque words then the intrusive chain head. Its
// per-element destructor (0x584a30) is a bare `ret` (the slot owns nothing).
struct CHashSlot {
    char m_pad00[0x8];     // +0x00
    CHashSlotList m_chain; // +0x08  { head, tail }
};
SIZE(CHashSlot, 0x10);

// ---------------------------------------------------------------------------
// CHashBase - the key-agnostic slot machinery (one physical copy of each method,
// shared by both CHash<T> instantiations).
// ---------------------------------------------------------------------------

// A stored record's hash-element prefix: the vtable-bearing key-hash node the
// engine splices into a bucket chain. Records EMBED one as a member (CSymRec::
// m_symNode @+0x04, CSymLeafBuilder/CParseSlot::m_node @+0x1c). Its slot-0 virtual
// returns the bucket index for the element's key; Insert dispatches it and stamps
// the owning table (+0xc) and computed bucket (+0x10); the payload sits at +0x14.
// Hash() is left DECLARED-but-undefined (not pure) so the class is embeddable AND
// polymorphic - the real vtable is stamped by the record's own (external) engine
// ctor, and cl never constructs a CHashElement here (the containing records are
// RezAlloc'd, never C++-default-constructed), so no spurious ??_7CHashElement
// vtable is emitted.
class CHashElement {
public:
    virtual u32 Hash(); // +0x00  slot 0 (the key-typed bucket hash; stamped ctor)
    CHashLink m_link;   // +0x04  intrusive chain node { next, prev }
    CHashBase* m_owner; // +0x0c  owning table back-ptr (Insert stamps this)
    u32 m_bucket;       // +0x10  computed bucket (Insert stamps this)
    void* m_record;     // +0x14  the stored (key,value) payload (key first);
                        //        genuinely heterogeneous (CSymRec / CSymTab / ...)
};
SIZE(CHashElement, 0x18);

class CHashBase {
public:
    // First live entry in iteration order (0x184ae0), or 0.
    CHashElement* First(); // 0x184ae0
    // Chain head for bucket `idx`, biased back to the element (head - 4), or 0.
    CHashElement* Lookup(u32 idx); // 0x184b40
    // Unlink `entry` (its chain node = &entry->m_link) from its owning slot's chain.
    void Remove(CHashElement* entry); // 0x184ab0
    // Drop every entry: array-delete the bucket array (no-op per-slot dtor + free
    // the count-cookie). The table's destructor.
    void RemoveAll(); // 0x184a40
    // Insert `node` into the table (0x184a70): ask the element for its bucket (the
    // slot-0 virtual hash), stamp the owning table + bucket into it, then splice
    // its m_link (element+4) into the bucket chain.
    void Insert(CHashElement* node); // 0x184a70

    // Recover the containing element from a chain link (container_of, -4). Genuine
    // intrusive-list back-cast: the chain stores &element->m_link.
    static CHashElement* FromLink(CHashLink* link) {
        return link ? (CHashElement*)((char*)link - 4) : 0;
    }

    u32 m_count;          // +0x00
    CHashSlot* m_buckets; // +0x04
};
SIZE(CHashBase, 0x8);

// ---------------------------------------------------------------------------
// CHash - the leaf-symbol instantiation ("_a").
// ---------------------------------------------------------------------------
class CHash : public CHashBase {
public:
    RVA(0x0013c240, 0x29)
    u32 HashStr(const char* s) {
        if (!s) {
        return 0;
        }
        u32 len = 0;
        while (*s) {
        ++len;
        ++s;
        }
        return len % m_count;
    }
    void* Walk(const char* name, i32 ci); // 0x13c270
    RVA(0x0013c350, 0xd)
    u32 HashInt(u32 key) {
        return key % m_count;
    }
    void* FindInt(u32 key);               // 0x13c360
};
SIZE(CHash, 0x8);

// ---------------------------------------------------------------------------
// CHashB - the child-scope instantiation ("_b"): identical HashStr/Walk source,
// distinct RVAs (only internal rel32s differ).
// ---------------------------------------------------------------------------
class CHashB : public CHashBase {
public:
    RVA(0x0013c3c0, 0x29)
    u32 HashStr(const char* s) {
        if (!s) {
        return 0;
        }
        u32 len = 0;
        while (*s) {
        ++len;
        ++s;
        }
        return len % m_count;
    }
    void* Walk(const char* name, i32 ci); // 0x13c3f0
};
SIZE(CHashB, 0x8);

// --- vtable catalog ---

#endif // SRC_BUTE_HASH_H
