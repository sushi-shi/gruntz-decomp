// Hash.h - CHash, the WAP32 engine string/int-keyed hash table embedded in
// CSymTab at +0x38 (child scopes) and +0x40 (leaf symbols). 8 bytes:
// { u32 m_count; CHashSlot* m_buckets; }.
//
// Shape recovered from the call graph (callers all in the 0x139../0x13a.. Remus
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
// CHashSlot[count]); each CHashSlot is 16 bytes { ?, ?, head, tail } - an
// intrusive doubly-linked list head at slot+8/+0xc. A live entry is reached
// through the "biased +4" convention: the chain threads node=entry+4, so the
// chain pointer is entry+4 and the engine subtracts 4 to recover the entry. An
// entry is { ? ; node(next@+4, prev@+8) ; u32 m_bucket@+0x10 ; void* m_rec@+0x14 }.
// The record at +0x14 carries the key first ([rec+0] = char* key for the string
// table, or the raw int key for the int table).
#ifndef SRC_BUTE_HASH_H
#define SRC_BUTE_HASH_H

#include <Ints.h>

// The Rez heap alloc/free (0x1b9b46 _RezAlloc = operator new / 0x1b9b82 _RezFree,
// both __cdecl); reloc-masked.
extern "C" void* RezAlloc(u32 size);
extern "C" void RezFree(void* p);

// The MSVC `'eh vector destructor iterator'` runtime (0x11f640): run `dtor` over
// `count` elements of `stride` bytes from `base`, descending, under an EH frame.
// __stdcall (callee-cleanup: retail has no `add esp,0x10` after the call); the
// reloc to it is masked. (TriggerMgrEh models the cdecl-shaped sibling; here the
// caller-side cleanup is absent, so this site's helper is __stdcall.)
void __stdcall Tm_DestroyArray(void* base, i32 stride, i32 count, void* dtor); // 0x11f640

// The no-op per-element bucket-slot destructor (0x584a30, a bare `ret`); its
// address is passed to the array-delete. Modeled as a stub so the DIR32 reloc to
// it falls out.
void CHashSlot_Dtor(); // 0x584a30 (retail "empty_stub")

// Case-(in)sensitive compares the lookups emit (CRT, reloc-masked rel32).
extern "C" i32 __cdecl _strcmpi(const char* a, const char* b); // 0x11fdf0
extern "C" i32 strcmp(const char* a, const char* b);           // inline byte loop

// The intrusive doubly-linked-list helpers (__thiscall on the slot's {head,tail}
// pair): Link splices `node` in (0x1390e0), Unlink removes it (0x1391e0). The
// biased node is entry+4. Modeled on a tiny head struct so `mov ecx,&slot.head;
// call` falls out reloc-masked.
struct CHashSlotList {
    void* m_head;            // slot+8
    void* m_tail;            // slot+0xc
    void Link(void* node);   // 0x1390e0
    void Unlink(void* node); // 0x1391e0
};

// The entry as seen by Insert: its first dword is a vtable whose slot 0 is the
// virtual hash (returns the bucket index for this entry's key). Insert stamps the
// owning table at +0xc and the computed bucket at +0x10, then links entry+4 into
// the bucket chain. Modeled as a polymorphic class so the `mov eax,[node]; call
// [eax]` dispatch falls out (no cast); the vtable is owned elsewhere (this TU
// never emits it - only declares the pure virtual to drive the dispatch shape).
class CHashInsertNode {
public:
    virtual u32 Hash() = 0; // slot 0 (the key-typed bucket hash)
    char m_pad04[0x0c - 0x04];
    void* m_ownerTable; // +0x0c  owning table back-ptr (Insert stamps this)
    u32 m_bucket;       // +0x10  computed bucket (Insert stamps this)
};

// A 16-byte bucket slot; its per-element destructor (0x584a30, a bare `ret`) is
// empty (the slot owns nothing). { ?, ?, head, tail }.
struct CHashSlot {
    void* m_00;
    void* m_04;
    void* m_head; // +0x08
    void* m_tail; // +0x0c
};

// A live hash entry. The chain threads node=entry+4 (next@+4, prev@+8 of the
// entry); the helpers bias by +/-4. m_bucket@+0x10 records the owning slot;
// m_rec@+0x14 is the (key,value) record, key first.
struct CHashEntry {
    void* m_00;   // +0x00
    void* m_next; // +0x04  node.next (biased; chain stores entry+4)
    void* m_prev; // +0x08  node.prev
    char m_pad0c[0x10 - 0x0c];
    u32 m_bucket; // +0x10
    void* m_rec;  // +0x14
};

// ---------------------------------------------------------------------------
// CHashBase - the key-agnostic slot machinery (one physical copy of each method,
// shared by both CHash<T> instantiations).
// ---------------------------------------------------------------------------
class CHashBase {
public:
    // First live entry in iteration order (0x184ae0), or 0.
    CHashEntry* First(); // 0x184ae0
    // Chain head for bucket `idx`, biased back to the entry (head - 4), or 0.
    CHashEntry* Lookup(u32 idx); // 0x184b40
    // Unlink `entry` (its biased node = entry+4) from its owning slot's chain.
    void Remove(CHashEntry* entry); // 0x184ab0
    // Drop every entry: array-delete the bucket array (no-op per-slot dtor + free
    // the count-cookie). The table's destructor.
    void RemoveAll(); // 0x184a40
    // Insert `node` into the table (0x184a70): ask the entry for its bucket (the
    // slot-0 virtual hash), stamp the owning table + bucket into the entry, then
    // splice entry+4 into the bucket chain.
    void Insert(CHashInsertNode* node); // 0x184a70

    u32 m_count;          // +0x00
    CHashSlot* m_buckets; // +0x04
};

// ---------------------------------------------------------------------------
// CHash - the leaf-symbol instantiation ("_a").
// ---------------------------------------------------------------------------
class CHash : public CHashBase {
public:
    u32 HashStr(const char* s);           // 0x13c240
    void* Walk(const char* name, i32 ci); // 0x13c270
    u32 HashInt(u32 key);                 // 0x13c350
    void* FindInt(u32 key);               // 0x13c360
};

// ---------------------------------------------------------------------------
// CHashB - the child-scope instantiation ("_b"): identical HashStr/Walk source,
// distinct RVAs (only internal rel32s differ).
// ---------------------------------------------------------------------------
class CHashB : public CHashBase {
public:
    u32 HashStr(const char* s);           // 0x13c3c0
    void* Walk(const char* name, i32 ci); // 0x13c3f0
};

#endif // SRC_BUTE_HASH_H
