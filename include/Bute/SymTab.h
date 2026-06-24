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
//   +0x04..+0x10     : value/link words (zero-init; nulled in dtor & Clear).
//   +0x14  m_rec     : void* - the hash-entry payload pointer ([entry+0x14]).
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

// The engine resource free (RezFree @0x1b9b82, __cdecl one-arg); reloc-masked.
extern "C" void RezFree(void* p);

// CRT helpers the tokenizer/resolvers emit inline or by call; reloc-masked rel32.
extern "C" char* strchr(const char* s, i32 c);           // 0x120120 (inlined word-scan)
extern "C" char* strncpy(char* d, const char* s, u32 n); // 0x120340
extern "C" u32 strlen(const char* s);                    // inlined repnz scas at /O2 /Oi

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

class CHashTable {
public:
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
    char m_pad00[0x4];
    const char* m_delims; // +0x04  delimiter set the tokenizer splits on
    char m_pad08[0x68 - 0x8];
    i32 m_68; // +0x68  flag forwarded to the +0x38 walk (passed as m_68 == 0)
};

// ---------------------------------------------------------------------------
// CSymRec - the leaf symbol record stored in m_symbols (0x139cf0/0x1397a0). Its
// teardown (Clear @0x139cf0) is a reloc-masked external; modeled minimally so the
// dtor's `mov ecx,rec; call 0x139cf0` shape falls out.
// ---------------------------------------------------------------------------
class CSymRec {
public:
    void Clear(); // 0x139cf0
};

// ---------------------------------------------------------------------------
// CSymTab - the recursive scope node.
// ---------------------------------------------------------------------------
class CSymTab {
public:
    // ~CSymTab (0x139ee0): walk both tables freeing each entry's record + node,
    // free the owned buffers, null the fields; the two CHashTable members then
    // auto-destruct at descending trylevels (the /GX frame).
    ~CSymTab();

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

    char* m_name;        // +0x00
    void* m_04;          // +0x04
    void* m_08;          // +0x08
    void* m_0c;          // +0x0c
    void* m_10;          // +0x10
    void* m_rec;         // +0x14
    CSymParser* m_owner; // +0x18
    void* m_1c;          // +0x1c
    char m_pad20[0x30 - 0x20];
    void* m_30;           // +0x30
    void* m_34;           // +0x34
    CHashTable m_subTabs; // +0x38  (destructed last: trylevel -1)
    CHashTable m_symbols; // +0x40  (destructed first: trylevel 0)
    char* m_buf48;        // +0x48
};

#endif // SRC_BUTE_SYMTAB_H
