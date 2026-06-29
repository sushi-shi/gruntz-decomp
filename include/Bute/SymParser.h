// SymParser.h - CSymParser, the Remus parser/owner that builds the CSymTab scope
// tree (the object CSymTab::m_owner @+0x18 points back to). Recovered from the
// ctor (0x13aa10), the /GX scalar destructor (0x13abc0), the Clear method
// (0x13b850) and the three path-resolution thunks (0x13bff0/0x13c030/0x13c210),
// all on a single object shape that owns a heap CSymTab at +0x44, an intrusive
// object list at +0x10 (its own abstract sub-object vtable) and an engine hash
// table at +0x80. Only the OFFSETS + code bytes are load-bearing (campaign
// doctrine); unproven field roles keep m_<hex>.
//
//   +0x00  vtable      : primary vtable (0x5ef750), stamped by ctor/dtor.
//   +0x04  m_ownedBuffer : char* - an owned buffer (RezFree'd in dtor).
//   +0x08  m_08        : i32   (=1 in ctor).
//   +0x0c  m_parseArmed : void* - guard: if non-null, dtor runs Clear(0) first.
//   +0x10  m_list      : CObjList - the intrusive object list sub-object; its own
//                        abstract vtable (0x5ef760, all __purecall). { vtbl@+0x10,
//                        head@+0x14, tail@+0x18 }; m_count@+0x1c.
//   +0x44  m_root      : CSymTab* - the heap root scope (~CSymTab + RezFree in dtor).
//   +0x64  m_cachedSourceBuffer : void* - cached source buffer (RezFree'd).
//   +0x80  m_hash      : CHashBase - an engine hash table (RemoveAll in dtor).
//   +0x88  m_nodes     : CHashSlotList - a second intrusive list { head, tail };
//                        each node owns a buffer at node+0x8.
#ifndef SRC_BUTE_SYMPARSER_H
#define SRC_BUTE_SYMPARSER_H

#include <Ints.h>

#include <Bute/Hash.h>

// CSymTab (SymTab.h) and Hash.h both model the engine hash table with conflicting
// CHashEntry/CHashTable shapes, so SymParser pulls in Hash.h (for the +0x80
// CHashBase / +0x88 CHashSlotList members) and declares only the three CSymTab
// methods it calls on the heap root scope (~CSymTab + the two path-resolvers),
// reloc-masked externals. The full CSymTab layout lives in SymTab.h.
class CSymParser; // the owner the CSymTab ctor links back to (defined below)

class CSymTab {
public:
    // The 8-arg ctor (0x139de0) ParseBuffer builds the root scope with; uses the Rez
    // heap so `new CSymTab(...)` drives the operator-new + ctor-throw /GX cleanup.
    CSymTab(
        CSymParser* owner,
        void* p1,
        const char* name,
        void* p3,
        void* p4,
        void* p5,
        i32 subN,
        i32 symN
    );
    ~CSymTab(); // 0x139ee0
    void* operator new(u32 n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    void* ResolvePath(const char* path);                // 0x13bae0
    i32 ResolveQualified(const char* name, void* arg);  // 0x13be40
    i32 ApplyRecursive(i32 a0, i32 a1, i32 a2, i32 a3); // 0x13a580
    // The directory-load helpers ParseRecords drives on the current scope node (all
    // reloc-masked externals on the node CSymTab):
    void* FindSub(const char* name);                   // 0x13a230
    CSymTab* CreateSub(const char* name);              // 0x13a330
    void* FindOrAddSym(i32 key);                       // 0x13a940
    i32 Insert(const char* key, void* arg);            // 0x13a000
    i32 Method4b0(void* a, void* b, void* c, void* d); // 0x13a4b0
    i32 Method530(void* rec, void* found);             // 0x13a530
};

// The name->key map / seed builder ParseBuffer reaches: MakeSymSeed (0x13ba70, cdecl,
// the leftover-stack-args ctor trick) returns a seed left on the stack. Reloc-masked.
i32 MakeSymSeed(); // 0x13ba70

// The shared empty-string literal (?g_emptyString) the root scope is named with.
extern const char g_emptyString[]; // 0x6293f4

// The two retail vtable groups for the class (0x5ef750 primary, 0x5ef760 the
// abstract list-interface sub-object). The class is modeled with manual vtable
// stamps (its virtuals point into other, unmatched TUs), so reference the retail
// vtables by address -> reloc-masked DATA() externs.
extern void* CSymParser_vftable;      // 0x5ef750
extern void* CObjList_purecall_vftbl; // 0x5ef760

// A polymorphic list node: { vptr@+0, next@+4, prev@+8 }. Its vtable carries a
// scalar-deleting dtor at slot 1 (+0x4, a delete-flag arg) and a teardown/detach
// method at slot 5 (+0x14). Modeled as a polymorphic class (like CNetPlayerObj) so
// `n->Delete(1)` / `n->Detach()` lower to the __thiscall virtual dispatch; the
// virtuals are never defined here, so no vtable is emitted in this TU.
class CObjNode {
public:
    virtual void Slot00();                                 // +0x00
    virtual void Delete(i32 flag);                         // +0x04  slot 1 (scalar-deleting dtor)
    virtual i32 ReadRaw(i32 a, i32 b, i32 len, void* buf); // +0x08  slot 2 (binary read)
    virtual void Slot0c();                                 // +0x0c
    virtual i32 Read(void* buf, i32 a, i32 b);             // +0x10  slot 4 (parse buffer)
    virtual void* Detach();                                // +0x14  slot 5 (teardown/detach)

    CObjNode* m_next; // +0x04
    CObjNode* m_prev; // +0x08
};

// The CObjList sub-object embedded at +0x10: an intrusive doubly-linked list of
// polymorphic nodes. Its Remove (0x1852e0, __thiscall on the list head) unlinks a
// node from the {head@+4, tail@+8} pair.
struct CObjList {
    void* m_vtbl;     // +0x00 (this+0x10): abstract list-interface vtable
    CObjNode* m_head; // +0x04 (this+0x14)
    CObjNode* m_tail; // +0x08 (this+0x18)
    i32 m_count;      // +0x0c (this+0x1c)

    // Remove(node): unlink a node from the chain (0x1852e0).
    void Remove(CObjNode* node);
    // Link(node): splice a freshly-built reader node onto the list (0x1851e0).
    void Link(void* node);

    // The list sub-object's teardown restores its vptr to the abstract base
    // (0x5ef760) - the `[this+0x10]=0x5ef760` the owning /GX dtor emits as the
    // last member-destruct, after the +0x80 hash's RemoveAll.
    ~CObjList() {
        m_vtbl = &CObjList_purecall_vftbl;
    }
};

// A node owned by the +0x88 CHashSlotList: it owns a buffer at +0x08; the list
// uses the CHashSlotList::Unlink (0x1391e0) machinery from Hash.h.
struct CSlotNode {
    char m_pad00[0x8];
    void* m_buffer; // +0x08  owned buffer (RezFree'd)
};

// The +0x80 hash member. CHashBase carries no destructor (a standalone CHashBase
// value member in the shared hash/symtab TUs must stay trivially-destructible to
// preserve their codegen), so the destructible flavor used as a /GX member here is
// a thin local subclass whose teardown is RemoveAll. Same 8-byte layout (no new
// fields); only this TU sees the destructor, so Hash.h's siblings are unaffected.
struct CParserHash : public CHashBase {
    // 0x184960: the +0x80 hash-table member's 1-arg constructor. Modeled as a method
    // (the call shape is `lea ecx,[this+0x80]; push n; call`) so the 3-arg CSymParser
    // ctor's member-init lowers without re-touching Hash.h's shared CHashBase.
    void Init(i32 n); // 0x184960
    ~CParserHash() {
        RemoveAll();
    }
};

// The +0x10 list sub-object vtable the CONSTRUCTORS stamp (0x5ef75c) - 4 bytes ahead
// of the 0x5ef760 the destructor restores (a distinct MI sub-vtable slot). Reloc-
// masked DATA() extern.
extern void* CObjList_ctor_vftbl; // 0x5ef75c

// ---------------------------------------------------------------------------
// CSymParser - the Remus parser/owner.
// ---------------------------------------------------------------------------
class CSymParser {
public:
    // The default ctor (0x13aa10) lives in another (unmatched) TU - declared (no body)
    // so the 3-arg ctor's discarded temp `CSymParser tmp;` lowers to a reloc-masked
    // call; and the 3-arg buffer ctor (0x13ab00) defined in SymParser.cpp.
    CSymParser();                          // 0x13aa10 (external)
    CSymParser(void* buf, i32 a2, i32 a3); // 0x13ab00

    // ~CSymParser (0x13abc0): the /GX scalar destructor. Clear(0) if armed, tear
    // down the +0x10 object list, the heap root CSymTab, the owned buffers and the
    // +0x88 node list, then RemoveAll the +0x80 hash member (the trylevel-0
    // /GX member-teardown) and re-stamp the +0x10 list vtable.
    ~CSymParser();

    // Clear (0x13b850): detach the active node (m_activeNode), drop the +0x10 object
    // list, free the heap root + the cached source buffer, then null m_parseArmed. The (discarded)
    // return is m_activeNode's Detach() result, left in eax. `final` is unused. RVA-keyed pairing
    // absorbs the void(QAEXH)-vs-void*(QAEPAXH) mangling mismatch.
    void* Clear(i32 final);

    // GetRoot (0x13b900): the heap root CSymTab accessor (`mov eax,[ecx+0x44];
    // ret`). NOT inlined into the thunks - a separate __thiscall function the
    // thunks `call`; external (no body) so its rel32 is reloc-masked.
    CSymTab* GetRoot(); // 0x13b900

    // ParseBuffer (0x13ad00): the big buffer parser/loader (__thiscall, 3 args). It
    // re-caches the source buffer (m_cachedSourceBuffer = strdup(buf)), classifies the stream, then
    // builds the right reader + the root scope and folds the records into it.
    i32 ParseBuffer(void* buf, i32 a, i32 b); // 0x13ad00

    // ParseRecords (0x13b300): the recursive directory/record loader ParseBuffer drives
    // (__thiscall, 4 args). Enumerates `path` via _findfirst/_findnext, building child
    // scopes (CreateSub) per subdir and leaf records per file, recursing into each dir.
    i32 ParseRecords(void* reader, CSymTab* node, char* path, i32 flag); // 0x13b300

    // Classify (0x13c080): inspect the cached buffer, returning the stream flavor
    // (non-zero = text/structured, 0 = binary). Reloc-masked extern.
    i32 Classify(char* buf); // 0x13c080

    // ResolveName (0x13b910): map an upcased name to its int key. Reloc-masked extern.
    void* ResolveName(const char* s); // 0x13b910

    // ReParse (0x13c050): if armed (m_parseArmed), Clear(0) then re-parse the cached
    // +0x64 buffer. Returns 0 if not armed, else ParseBuffer's result.
    i32 ReParse(); // 0x13c050

    // PopParseSlot (0x13c0c0): pop the first parse-slot record out of m_hash. If the
    // table is empty, Rez-alloc a fresh block of m_parseSlotBlockCount parse slots, init each, stamp
    // their self-ptrs + register them into m_hash, link the block into m_nodes, then
    // pop the first. Returns the popped record (or 0 on allocation failure).
    void* PopParseSlot(); // 0x13c0c0

    // The three path-resolution thunks: forward into GetRoot()'s CSymTab.
    i32 ResolveQualified(const char* name, void* arg); // 0x13bff0 -> root->ResolveQualified
    void* ResolvePath(const char* path);               // 0x13c030 -> root->ResolvePath
    void AddNode(void* rec);                           // 0x13c210 -> m_hash insert

    void* m_vtbl;               // +0x00
    char* m_ownedBuffer;        // +0x04
    i32 m_08;                   // +0x08  (=1)
    void* m_parseArmed;         // +0x0c  Clear guard
    CObjList m_list;            // +0x10  (+0x10..+0x1c)
    CObjNode* m_activeNode;     // +0x20  detached+removed+deleted first in Clear
    i32 m_24;                   // +0x24
    i32 m_nextGeneratedFileKey; // +0x28
    i32 m_2c;                   // +0x2c
    i32 m_30;                   // +0x30
    i32 m_34;                   // +0x34
    i32 m_38;                   // +0x38
    i32 m_3c;                   // +0x3c
    i32 m_40;                   // +0x40  (=1)
    CSymTab* m_root;            // +0x44
    i32 m_48;                   // +0x48
    i32 m_4c;                   // +0x4c
    i32 m_50;                   // +0x50  (=1)
    i32 m_54;                   // +0x54
    i32 m_58;                   // +0x58
    i32 m_5c;                   // +0x5c
    i32 m_60;                   // +0x60
    void* m_cachedSourceBuffer; // +0x64
    char m_pad68[0x78 - 0x68];
    i32 m_subTabBucketCount;   // +0x78  child-scope m_subTabs bucket count (CSymTab ctor subN)
    i32 m_symbolBucketCount;   // +0x7c  child-scope m_symbols bucket count (CSymTab ctor symN)
    CParserHash m_hash;        // +0x80
    CHashSlotList m_nodes;     // +0x88  { head, tail }
    i32 m_parseSlotBlockCount; // +0x90  number of parse-slot records per allocated block
};

#endif // SRC_BUTE_SYMPARSER_H
