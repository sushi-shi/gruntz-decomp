// SymTab.h - CSymTab, the Remus/ButeMgr hierarchical symbol table.
//
// The Remus parser (RemusParseSource @0x139800/0x139960/0x1399d0, right next to
// these methods) builds a tree of named scopes through CSymTab. Each node is a
// two-level keyed store reached through two hash tables:
//   +0x38  m_subTabs  - child CSymTab scopes (nested namespaces); freed by
//                       recursing ~CSymTab on each entry.
//   +0x40  m_symbols  - leaf symbol records (the CSymRec class @0x139cf0/0x1397a0);
//                       freed by CSymRec::Clear (0x139cf0) on each entry.
// A dotted path "a.b.c" is resolved by tokenizing on the parser's delimiter set:
// the first segment selects a child scope (Find via the +0x38 hash), the rest
// recurses. The hash tables themselves are the engine CHashTable class
// (ClassUnknown_13: Lookup 0x184b40, First 0x184ae0, Next 0x1848b0, Remove
// 0x184ab0, RemoveAll 0x184a40; Find 0x13c360 / Walk 0x13c270/0x13c3f0).
//
// Layout recovered from the dtor (0x139ee0), the +0x38 walk wrapper (0x13a230)
// and the two recursive path-resolvers (0x13bae0/0x13be40). Only the OFFSETS +
// code bytes are load-bearing (campaign doctrine); unproven roles keep m_<hex>.
//
//   +0x00  m_name    : char* - this scope's owned name buffer (freed in dtor).
//   +0x04..+0x14     : value/link words (zero-init; only nulled in the dtor --
//                      roles unproven by the matched methods, so kept m_<hex>).
//   +0x18  m_owner   : CSymParser* - the owning parser; supplies the delimiter
//                      set ([+0x4]) the tokenizer splits on and a flag ([+0x68]
//                      != 0) passed to the +0x38 walk.
//   +0x30/+0x34      : link words (zero-init).
//   +0x38  m_subTabs : CHashTable - child-scope table (destructed at trylevel -1).
//   +0x40  m_symbols : CHashTable - leaf-symbol table   (destructed at trylevel 0).
//   +0x48  m_buf48   : char* - a second owned buffer (freed in dtor).
#ifndef SRC_BUTE_SYMTAB_H
#define SRC_BUTE_SYMTAB_H

#include <Ints.h>

// The engine resource alloc/free (RezAlloc @0x1b9b46 = operator new / RezFree
// @0x1b9b82, both __cdecl one-arg); reloc-masked. The scope-name buffer is allocated
// through the throwing global ::operator new (0x1b9b46) so the ctor's /GX state-1
// member-cleanup transition falls out; RezAlloc is the nothrow C alias used elsewhere.
void* operator new(u32 size);
extern "C" void* RezAlloc(u32 size);
extern "C" void RezFree(void* p);

// CRT helpers the tokenizer/resolvers emit inline or by call; reloc-masked rel32.
extern "C" char* strchr(const char* s, i32 c);           // 0x120120 (inlined word-scan)
extern "C" char* strncpy(char* d, const char* s, u32 n); // 0x120340
extern "C" char* strcpy(char* d, const char* s);         // inlined rep movs at /O2 /Oi
extern "C" u32 strlen(const char* s);                    // inlined repnz scas at /O2 /Oi

// CRT path helpers the keyed Find emits by call (0x18c530 _splitpath, 0x18d330
// _strupr); reloc-masked rel32.
extern "C" void __cdecl
_splitpath(const char* path, char* drive, char* dir, char* fname, char* ext);
extern "C" char* __cdecl _strupr(char* s);

// The child-scope hash-node vtable stamped at CSymTab+0x20 (the key-hash interface
// that lets a scope be inserted into its parent's m_subTabs). Modeled as a manual
// stamp (its virtuals live in other, unmatched TUs) -> reloc-masked DATA() extern.
extern void* CSymTab_node_vftable; // 0x5ef748

// A name-keyed seed builder (0x13ba70): returns time(0)-style seed, reloc-masked.
// Called as ctor arg5 so its leftover stack args double as the next ctor args.
i32 MakeSymSeed(); // 0x13ba70 (Boundary stub; cdecl, no args of its own)

// ---------------------------------------------------------------------------
// CHashTable - the engine string-keyed hash table embedded at +0x38 / +0x40
// (ClassUnknown_13). 8 bytes: { u32 m_count; Entry* m_buckets; }. Every method
// is a reloc-masked external (no body) so the __thiscall call shapes fall out.
// Entry is a 16-byte slot; a live slot's payload pointer is [slot+0]-4 (the
// engine offsets entries by 4), and the resolved record sits at [entry+0x14].
// ---------------------------------------------------------------------------
struct CHashEntry {
    char* m_key;            // +0x00  (the engine stores key+4; First/Next adjust by -4)
    char m_pad04[0x14 - 4]; // +0x04
    void* m_payload;        // +0x14  the resolved (key,node) record
};

// The embedded table iterates RezColl (collection) / RezNode (entry): one
// physical First (0x184ae0) / Next (0x1848b0), the canonical owners of those
// RVAs. First() is __thiscall on the collection; Next() is __thiscall on the
// entry. The returned node's payload (the resolved record) sits at node+0x14.
// (The iteration accessors below add the collection/entry offset into ecx, so
// the call shapes fall out; both are reloc-masked externals.)
struct RezNode;

struct RezColl {
    RezNode* First(); // 0x184ae0
};

struct RezNode {
    RezNode* Next();   // 0x1848b0
    char m_pad0[0x14]; // +0x00
    void* m_14;        // +0x14  the resolved record
};

class CHashTable {
public:
    // Allocate the bucket array (0x184960): the ctor body lives in another TU, so the
    // C++ ctor delegates to Init -> the member ctor falls out as a reloc-masked call,
    // driving CSymTab's /GX member-construction trylevel machinery.
    CHashTable(i32 n) {
        Init(n);
    }
    void Init(i32 n); // 0x184960

    // First live entry (0x184ae0), or null.
    CHashEntry* First();
    // Entry after `e` in iteration order (0x1848b0).
    CHashEntry* Next(CHashEntry* e);
    // Remove `e` and return the next entry (0x184ab0).
    CHashEntry* Remove(CHashEntry* e);
    // Drop every entry (0x184a40) - the table's destructor.
    void RemoveAll();
    // Find the (key,node) record for `name`, or null (0x13c360).
    void* Find(const char* name);
    // Walk: apply the stored callback to each record matching `name` with `flag`
    // (0x13c3f0 / 0x13c270; __thiscall, callee-cleanup of both args).
    void* Walk(const char* name, i32 flag);
    // Insert a hash node (0x184a70): the node's vtable slot-0 hash picks the bucket,
    // then node+4 is spliced into the bucket chain. `node` points at the embedded
    // CHashInsertNode (the vtable-bearing prefix inside the stored record).
    void Insert(void* node);

    // Real destructor so ~CSymTab's /GX member-teardown frame falls out.
    ~CHashTable() {
        RemoveAll();
    }

    u32 m_count;     // +0x00
    void* m_buckets; // +0x04
};

// ---------------------------------------------------------------------------
// CSymParser - the owning Remus parser (this+0x18). Only the two fields the
// resolvers read are modeled; the rest is unmapped here.
// ---------------------------------------------------------------------------
struct CSymParser {
    // Map an upcased extension/name to its int key (0x13b910); reloc-masked __thiscall.
    void* ResolveName(const char* s); // 0x13b910

    char m_pad00[0x4];
    const char* m_delims; // +0x04  delimiter set the tokenizer splits on
    char m_pad08[0x58 - 0x8];
    i32 m_58; // +0x58  longest scope-name length seen (tracked when adding scopes)
    char m_pad5c[0x68 - 0x5c];
    i32 m_68; // +0x68  flag forwarded to the +0x38 walk (passed as m_68 == 0)
    i32 m_6c; // +0x6c  selects the leaf-record ctor variant (Init4 vs Init3)
    i32 m_70; // +0x70  leaf-record ctor arg
    i32 m_74; // +0x74  leaf-record ctor arg (Init4 only)
    i32 m_78; // +0x78  child-scope m_subTabs bucket count
    i32 m_7c; // +0x7c  child-scope m_symbols bucket count
};

// ---------------------------------------------------------------------------
// CSymRec - the leaf symbol record stored in m_symbols (0x139cf0/0x1397a0). Its
// teardown (Clear @0x139cf0) is a reloc-masked external; modeled minimally so the
// dtor's `mov ecx,rec; call 0x139cf0` shape falls out.
// ---------------------------------------------------------------------------
class CSymRec {
public:
    // The two leaf-record constructors (selected by m_owner->m_6c). Defined in
    // another TU; declared here so `new CSymRec(...)` emits the reloc-masked ctor call
    // plus the Rez operator new/delete + ctor-throw cleanup the /GX frame needs.
    CSymRec(i32 key, void* owner, i32 a, i32 b); // 0x139bf0 (m_6c != 0)
    CSymRec(i32 key, void* owner, i32 a);        // 0x139c80 (m_6c == 0)
    void Clear();                                // 0x139cf0
    void* operator new(u32 n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    char m_pad[0x30]; // sizeof == 0x30 (the leaf-record allocation size)
};

// ---------------------------------------------------------------------------
// CSymTab - the recursive scope node.
// ---------------------------------------------------------------------------
class CSymTab {
public:
    // ctor (0x139de0): stamp the +0x20 hash-node vtable + the +0x34 self-ptr, build
    // the two embedded hash tables (m_subTabs(subN), m_symbols(symN) - the /GX
    // member-construction frame), copy the scope name, then store the rest. The
    // +0x20/+0x34 stamps go in the init list so they precede the member ctors.
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

    // ~CSymTab (0x139ee0): walk both tables freeing each entry's record + node,
    // free the owned buffers, null the fields; the two CHashTable members then
    // auto-destruct at descending trylevels (the /GX frame).
    ~CSymTab();

    // CSymTab uses the Rez heap (nothrow): operator new = RezAlloc(0x4c), delete =
    // RezFree, so `new CSymTab(...)` drives the alloc + ctor-throw cleanup EH pattern.
    void* operator new(u32 n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }

    // CreateSub (0x13a330): if `name` is not already a child scope, heap-alloc a new
    // CSymTab child (owner inherited, parent=this), insert it into m_subTabs, and
    // bump the parser's longest-name counter. Returns the child (or 0).
    CSymTab* CreateSub(const char* name);

    // FindOrAddSym (0x13a940): find the int-keyed leaf record, or build one (the ctor
    // variant chosen by m_owner->m_6c) and insert it into m_symbols. Returns it.
    void* FindOrAddSym(i32 key);

    // ApplyRecursive (0x13a580): clear each child's m_04, run the range operation
    // (0x13a640) over this scope, then recurse into children whose m_04 was set.
    // Returns 1 unless a recursion failed. (a2 == 0 is a no-op returning 1.)
    i32 ApplyRecursive(i32 a0, i32 a1, i32 a2, i32 a3);

    // The big range operation 0x13a640 invoked by ApplyRecursive: reads a block of
    // (a0-stream) records into a temp buffer and folds each into this scope's tables
    // (sub-scope records -> m_subTabs; leaf records -> the leaf builder + m_symbols).
    i32 ApplyRange(i32 a0, i32 a1, i32 a2, i32 a3); // 0x13a640

    // The leaf-merge helper ApplyRange calls when a leaf record's +0x24 sub-table walk
    // already has the key (0x13a530, __thiscall(rec, found)). Reloc-masked extern.
    i32 Method530(void* rec, void* found); // 0x13a530

    // Walk m_subTabs (+0x38) for `name`, forwarding m_owner->m_68 == 0 (0x13a230).
    void* FindSub(const char* name);

    // Resolve/insert a dotted path under this scope (0x13bae0): tokenize the first
    // segment, recurse on the remainder.
    void* ResolvePath(const char* path);

    // Resolve a fully-qualified name by its last delimiter (0x13be40): split off
    // the trailing key, resolve the leading scope, then dispatch.
    i32 ResolveQualified(const char* name, void* arg);

    // Insert/resolve `key` directly into this scope's leaf table (+0x40), passing
    // m_owner->m_68 == 0 (0x13a000; the ResolveQualified tail). __thiscall extern,
    // no body -> reloc-masked. (Currently labeled ClassUnknown_14; a CSymTab method.)
    i32 Insert(const char* key, void* arg);

    // Look `key` up directly in this scope's leaf table (the +0x40 path); the read
    // counterpart of Insert (0x13a040, one-arg __thiscall). Reloc-masked extern.
    void* Find(const char* key);

    // Find a fully-qualified name by its last delimiter (0x13bca0): split off the
    // trailing key, resolve the leading scope (ResolvePath), then Find. The read
    // counterpart of ResolveQualified.
    void* FindQualified(const char* name);

    // Drop this scope's cached parse state (0x13a190): free the owned +0x48 buffer
    // if live, else end each leaf record's parse stream; when `recurse`, descend into
    // every child scope. Returns 1.
    i32 ReleaseParseBuffers(i32 recurse);

    // Iteration accessors over the two embedded tables. First* are __thiscall on
    // the scope; Next* take a previously-returned record and advance via the node
    // the engine embeds inside it at a fixed offset (each record IS an intrusive
    // list node). All return the entry's payload ([entry+0x14]), or 0 at the end.
    void* FirstSub();          // 0x13a260  m_subTabs.First()->payload
    void* NextSub(void* rec);  // 0x13a280  (rec+0x20)->Next()->payload
    void* FirstSym();          // 0x13a2b0  m_symbols.First()->payload
    void* NextSym(void* rec);  // 0x13a2d0  (rec+0x04)->Next()->payload
    void* NextSym2(void* rec); // 0x13a2f0  (rec+0x24)->Next()->payload
    void* NextSym3(void* rec); // 0x13a310  (rec+0x1c)->Next()->payload

    char* m_name;        // +0x00
    void* m_04;          // +0x04
    void* m_08;          // +0x08
    void* m_0c;          // +0x0c
    void* m_10;          // +0x10
    void* m_14;          // +0x14  (role unproven; only nulled in dtor)
    CSymParser* m_owner; // +0x18
    void* m_1c;          // +0x1c
    void* m_node20;      // +0x20  hash-node vtable (the scope's parent-table interface)
    char m_pad24[0x30 - 0x24];
    void* m_30;           // +0x30
    void* m_34;           // +0x34  self-ptr (the scope's own record back-pointer)
    CHashTable m_subTabs; // +0x38  (destructed last: trylevel -1)
    CHashTable m_symbols; // +0x40  (destructed first: trylevel 0)
    char* m_buf48;        // +0x48
};

#endif // SRC_BUTE_SYMTAB_H
