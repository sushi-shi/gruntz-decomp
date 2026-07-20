#ifndef SRC_BUTE_HASH_H
#define SRC_BUTE_HASH_H

#include <Ints.h>
#include <rva.h>
#include <string.h> // _strcmpi (0x11fdf0) / strcmp (/Oi intrinsic byte-loop) - the

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

#include <Dsndmgr/SoundVoiceList.h>

class CHashBase;

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
        return link ? reinterpret_cast<CHashElement*>((reinterpret_cast<char*>(link) - 4)) : 0;
    }

    u32 m_count;          // +0x00
    CHashSlot* m_buckets; // +0x04
};
SIZE(CHashBase, 0x8);

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

#endif // SRC_BUTE_HASH_H
