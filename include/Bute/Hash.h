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
#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

// (The `'eh vector destructor iterator'` (0x11f640 = ??_M) used to be declared here as a
// hand-rolled `Tm_DestroyArray`, on the belief that ??_M is "un-spellable". It is not: it
// is what cl emits for a plain `delete[]` of an object array, and ??_M@YGXPAXIHP6EX0@Z@Z
// is in the CRT libs. ?Tm_DestroyArray@@YGXPAXHIP6AXXZ@Z, by contrast, was defined nowhere
// - a guaranteed unresolved external. CHashBase::RemoveAll now says `delete[] m_buckets`
// and CWwdGrid::FreeBuckets `delete[] m_buckets`; both lower to retail's exact bytes.)

// (The `'eh vector constructor iterator'` (0x11f5a0 = ??_P) was likewise hand-declared as
// `Tm_ConstructArray`, with the CHashSlot ctor/dtor passed to it as bare `void()` fn-ptrs
// named CHashSlot_Ctor / CHashSlot_Dtor. All three were fabricated symbols. CHashBase::
// Construct is simply `m_buckets = new CHashSlot[count]`: cl emits the ??_P call, both
// element ctor/dtor addresses and the /GX EH frame by itself. 0x184a20 is
// CHashSlot::CHashSlot and 0x184a30 is CHashSlot::~CHashSlot - see the class below.)

// The intrusive doubly-linked chain node threaded through each stored element at
// element+0x04 (next@+0, prev@+4), and the {head,tail} chain-head embedded at
// each bucket slot's +0x08 (also standalone as CSymParser::m_nodes), are the ONE
// shared WAP32 intrusive list - DSoundLink / DSoundList
// (<Dsndmgr/SoundVoiceList.h>, which owns the RVA claims: InsertHead 0x1390e0,
// Unlink 0x1391e0). The former Bute-local CHashLink/CHashSlotList twins were
// fake views of it (their Link/Unlink decls resolved to nothing while every
// retail call lands on the DSoundList bodies). Chains store &element->m_link;
// the element is recovered via container_of (link - 4).
#include <Dsndmgr/SoundVoiceList.h>

class CHashBase;

// A 16-byte bucket slot: two opaque words then the intrusive chain head. Its
// per-element destructor (0x584a30) is a bare `ret` (the slot owns nothing).
struct CHashSlot {
    // ctor (0x184a20): zero the intrusive {head,tail} chain head; the two opaque
    // words at +0x00 are left uninitialised (RezAlloc'd array).
    CHashSlot(); // 0x184a20
    // The empty dtor (0x184a30 - a bare 1-byte `ret`). DECLARED here, DEFINED out-of-line
    // in RezColl.cpp, and that is load-bearing: given an inline `~CHashSlot() {}` cl SEES
    // the teardown is a no-op and elides the whole vector-dtor loop from `delete[]`, so no
    // ??_M call is emitted at all. Retail's RemoveAll DOES call ??_M, which means the dtor
    // was opaque to it - i.e. out-of-line, exactly as here. (It used to be modelled as a
    // free function CHashSlot_Dtor whose address was passed by hand.)
    ~CHashSlot();

    char m_pad00[0x8];     // +0x00
    DSoundList m_chain; // +0x08  { head, tail }
};
SIZE(CHashSlot, 0x10);

// ---------------------------------------------------------------------------
// CHashBase - the key-agnostic slot machinery (one physical copy of each method,
// shared by both CHash<T> instantiations).
// ---------------------------------------------------------------------------

// A stored record's hash-element prefix: the vtable-bearing key-hash node the
// engine splices into a bucket chain. Records EMBED one as a member (CSymRec::
// m_symNode @+0x04, CSymLeafBuilder::m_node @+0x1c). Its slot-0 virtual
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

    // The next live element in iteration order (0x1848b0): follow this element's
    // chain link (container_of -4), else scan the owning table's later buckets for
    // the next occupied chain head. The canonical intrusive-chain node walk; the
    // former RezNode::Next view folded onto this (wave5-F1). Body in RezColl.cpp.
    CHashElement* Next(); // 0x1848b0

    DSoundLink m_link;   // +0x04  intrusive chain node { next, prev }
    CHashBase* m_owner; // +0x0c  owning table back-ptr (Insert stamps this)
    u32 m_bucket;       // +0x10  computed bucket (Insert stamps this)
    void* m_record;     // +0x14  the stored (key,value) payload (key first);
                        //        genuinely heterogeneous (CSymRec / CSymTab / ...)
};
SIZE(CHashElement, 0x18);

class CHashBase {
public:
    // Allocate the bucket array (0x184960): store `count` at +0x00, RezAlloc a
    // (count<<4)+4-byte block (an int cookie + `count` 16-byte CHashSlot buckets),
    // ehvec-construct the slots, store the array at +0x04. Returns this. This is the
    // real sized "constructor" (a plain method, not a C++ ctor - retail mangles it
    // ?Construct@... returning `this`); the derived tables' sized ctors delegate to
    // it. Was the CSymList::Construct view (wave5-F1). Body in RezColl.cpp.
    CHashBase* Construct(i32 count); // 0x184960

    // First live entry in iteration order (0x184ae0), or 0. Was RezColl::First
    // (folded wave5-F1); body in RezColl.cpp.
    CHashElement* First(); // 0x184ae0
    // Last live entry in reverse iteration order (0x184b10): scan the bucket array
    // from the highest index down, return the tail element of the first non-empty
    // bucket (FromLink(slot.m_chain.m_tail)), or 0.
    CHashElement* Last(); // 0x184b10
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
    static CHashElement* FromLink(DSoundLink* link) {
        return link ? (CHashElement*)(reinterpret_cast<char*>(link) - 4) : 0;
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
    // The default (empty-table) ctor (0x184950): zero count+buckets. OUT-OF-LINE
    // (retail `call`s it from CSymRec's 3-arg ctor for m_keyTable - it is not
    // inlined), so declared here + defined in RezColl.cpp. Was unhomed (wave5-F1).
    CHash(); // 0x184950
    // The sized ctor: delegate to CHashBase::Construct (a direct `call 0x184960`
    // once /O2 folds this trivial wrapper - matches the member-init reloc).
    CHash(i32 n) {
        Construct(n);
    }
    // Member-teardown: the two CHashTable members of CSymRec/CSymTab auto-destruct
    // via RemoveAll (inline, so it folds into the enclosing /GX dtor frame).
    ~CHash() {
        RemoveAll();
    }

    // HashStr/HashInt are genuine OUT-OF-LINE functions in retail (called via
    // `call` from Walk/FindInt at their own RVAs) - defining them inline here let
    // /O2 fold them into their callers, so they never emitted. Out-of-line in Hash.cpp.
    u32 HashStr(const char* s);           // 0x13c240
    void* Walk(const char* name, i32 ci); // 0x13c270
    u32 HashInt(u32 key);                 // 0x13c350
    void* FindInt(u32 key);               // 0x13c360
};
SIZE(CHash, 0x8);

// ---------------------------------------------------------------------------
// CHashB - the child-scope instantiation ("_b"): identical HashStr/Walk source,
// distinct RVAs (only internal rel32s differ).
// ---------------------------------------------------------------------------
class CHashB : public CHashBase {
public:
    // The child-scope table is always sized (m_subTabs(subN)); its sized ctor
    // delegates to CHashBase::Construct (direct `call 0x184960`), and its member
    // teardown is RemoveAll (folds into ~CSymTab's /GX frame).
    CHashB(i32 n) {
        Construct(n);
    }
    ~CHashB() {
        RemoveAll();
    }

    u32 HashStr(const char* s);           // 0x13c3c0 (out-of-line; see CHash above)
    void* Walk(const char* name, i32 ci); // 0x13c3f0
};
SIZE(CHashB, 0x8);

// --- vtable catalog ---

#endif // SRC_BUTE_HASH_H
