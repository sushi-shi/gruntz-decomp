#ifndef SRC_BUTE_SYMPARSER_H
#define SRC_BUTE_SYMPARSER_H
#include <rva.h>

#include <Ints.h>

#include <Bute/Hash.h>
#include <Bute/SymTab.h> // the single full CSymTab layout (+ the CSymParser fwd-decl)

#include <EmptyString.h> // g_emptyString (the shared "" constant)

class CRezItmBase;

#include <Bute/ObjListBase.h>

#include <Rez/RezList.h>
struct CParserObjList : public CObjList {
    virtual void V0() OVERRIDE; // [0] 0x13c4c0 (empty body; declared-only, reloc-masked)
    ~CParserObjList() {}
    i32 m_count; // +0x0c (this+0x1c)
};
SIZE(0x10); // { vptr, head, tail, count }

struct CParseSource; // the 0x3c leaf parse record (ex 'CSymLeafBuilder')

struct CSlotNode {
    DSoundLink m_link;      // +0x00  intrusive chain node { next, prev }
    CParseSource* m_buffer; // +0x08  owned parse-slot block (RezFree'd)
};
SIZE(0xc);

struct CParserHash : public CHashBase {
    // The +0x80 hash-table member's 1-arg construction (0x184960) is the canonical
    // CHashBase::Construct (folded from the former CSymList::Construct view, wave5-F1),
    // reached cast-free as m_hash.Construct(1) in SymTab.cpp.
    ~CParserHash() {
        RemoveAll();
    }
};
SIZE(0x8); // derives CHashBase (no new fields)

class CSymParser {
public:
    // The three primary slots. Retail's bodies are inert defaults (the parser's
    // "subclass me" hooks); their SIGNATURES are read off the bytes, not guessed -
    // the ret flavor gives the stack-arg count and the eax write gives the return:
    //     0x13b9f0  33 c0 c2 04 00   xor eax,eax; ret 4  -> 1 stack arg, returns 0
    //     0x13ba00        c2 04 00   ret 4               -> 1 stack arg, void
    //     0x13ba10        33 c0 c3   xor eax,eax; ret    -> no args,     returns 0
    // (The ex `void V0()/V1()/V2()` triple contradicted all three: it dropped the
    // arg that `ret 4` proves and voided the eax that `xor eax,eax` proves.)
    // Defined out-of-line in SymTab.cpp - they sit in that TU's 0x13b9e2..0x13ba20 gap.
    virtual i32 V0(i32 a);  // slot 0 (0x13b9f0)
    virtual void V1(i32 a); // slot 1 (0x13ba00)
    // slot 2 (0x13ba10) - the rez-node RETRY gate (role proven 2026-07-19): every
    // CRezItmBase/CRezFileMgr m_parent IS this parser (`new CRezDir(this, ..)` /
    // `new CRezItm(this)` in ParseBuffer), and on an I/O failure the nodes poll
    // `m_parent->Retry()` through slot 2 - the inert base body's `return 0` is the
    // default "give up". (Killed the CRezItmOwner interface view.)
    virtual i32 Retry();

    // The default ctor (0x13aa10, defined in SymParser.cpp) seeds the parse-config
    // defaults; the 3-arg buf-ctor's discarded temp `CSymParser tmp;` lowers to a
    // (reloc-masked) call to it. The 3-arg buffer ctor (0x13ab00) is also in the .cpp.
    CSymParser();                          // 0x13aa10
    CSymParser(void* buf, i32 a2, i32 a3); // 0x13ab00

    // ~CSymParser (0x13abc0): the /GX scalar destructor. Clear(0) if armed, tear
    // down the +0x10 object list, the heap root CSymTab, the owned buffers and the
    // +0x88 node list, then RemoveAll the +0x80 hash member (the trylevel-0
    // /GX member-teardown) and re-stamp the +0x10 list vtable.
    ~CSymParser();

    // Clear (0x13b850): Close() the active node (m_activeNode), drop the +0x10 object
    // list, free the heap root + the cached source buffer, then null m_parseArmed. The
    // (discarded) return is m_activeNode's Close() result, left in eax. `final` is
    // unused. RVA-keyed pairing absorbs mangling drift on the (discarded) return type.
    i32 Clear(i32 final);

    // GetRoot (0x13b900): the heap root CSymTab accessor (`mov eax,[ecx+0x44];
    // ret`). NOT inlined into the thunks - a separate __thiscall function the
    // thunks `call`; external (no body) so its rel32 is reloc-masked.
    CSymTab* GetRoot(); // 0x13b900

    // ParseBuffer (0x13ad00): the big buffer parser/loader (__thiscall, 3 args). It
    // re-caches the source buffer (m_cachedSourceBuffer = strdup(buf)), classifies the stream, then
    // builds the right reader + the root scope and folds the records into it.
    i32 ParseBuffer(void* buf, i32 a, i32 b); // 0x13ad00

    // LoadEntry (0x13b0c0): mount one archive entry named `name`. Ghidra-mislabeled
    // CRezDir::Stub_13b0c0; the real owner is CSymParser (it sits between ParseBuffer
    // and ParseRecords and drives ParseRecords/ApplyRecursive on `this`). A directory
    // builds a CRezDir node + recurses ParseRecords; a file builds a CRezItm node,
    // reads its 0xa8-byte header and runs the root scope's ApplyRecursive.
    i32 LoadEntry(char* name, i32 flag); // 0x13b0c0

    // ParseRecords (0x13b300): the recursive directory/record loader ParseBuffer drives
    // (__thiscall, 4 args). Enumerates `path` via _findfirst/_findnext, building child
    // scopes (CreateSub) per subdir and leaf records per file, recursing into each dir.
    i32 ParseRecords(void* reader, CSymTab* node, char* path, i32 flag); // 0x13b300

    // Classify (0x13c080): inspect the cached buffer, returning the stream flavor
    // (non-zero = text/structured, 0 = binary). Reloc-masked extern.
    i32 Classify(char* buf); // 0x13c080

    // (0x13b910 "ResolveName" was a mislabel: it is the free __stdcall ::PackTag - the
    // ext->key mapper declared at file scope above. ParseRecords calls PackTag directly.)

    // ReParse (0x13c050): if armed (m_parseArmed), Clear(0) then re-parse the cached
    // +0x64 buffer. Returns 0 if not armed, else ParseBuffer's result.
    i32 ReParse(); // 0x13c050

    // PopParseSlot (0x13c0c0): pop the first parse-slot record out of m_hash. If the
    // table is empty, Rez-alloc a fresh block of m_parseSlotBlockCount parse slots, init each, stamp
    // their self-ptrs + register them into m_hash, link the block into m_nodes, then
    // pop the first. Returns the popped record (or 0 on allocation failure).
    CParseSource* PopParseSlot(); // 0x13c0c0  (pops a leaf parse record from the pool)

    // MakeSeed: the name-keyed clock-seed builder (0x13ba70), __thiscall on the parser
    // (ecx = this; body ignores it, returning time(&t)). The real method - every caller
    // loads the parser into ecx before the call (AddNodeEntry's byte-exact `mov ecx,
    // m_owner; call` proves the thiscall convention). Defined in SymTab.cpp.
    i32 MakeSeed(); // 0x13ba70 (defined in SymTab.cpp)

    // CheckNodes (0x13ba20): walk m_list, calling each node's slot-7 probe; return 1
    // iff every node returned nonzero (no early exit). Orphan copy (no caller).
    i32 CheckNodes(); // 0x13ba20
    // SetDelims (0x13ba80): free the current m_delims buffer, then own a fresh
    // strdup of `s`. Orphan copy (no caller).
    void SetDelims(char* s); // 0x13ba80

    // The three path-resolution thunks: forward into GetRoot()'s CSymTab.
    struct CParseSource*
    ResolveQualified(const char* name, void* arg); // 0x13bff0 -> root->ResolveQualified
    void* ResolvePath(const char* path);           // 0x13c030 -> root->ResolvePath
    void AddNode(void* rec);                       // 0x13c210 -> m_hash insert

    // vptr implicit @ +0x00 (??_7CSymParser@@6B@)
    // +0x04  owned delimiter-set buffer: the tokenizer split set the CSymTab path
    //         resolvers read (SymTab.cpp m_owner->m_delims), RezFree'd in the dtor.
    char* m_delims;
    i32 m_08;                   // +0x08  (=1)
    i32 m_parseArmed;           // +0x0c  Clear guard (0/1 flag)
    CParserObjList m_list;      // +0x10  (+0x10..+0x1c)
    CRezItmBase* m_activeNode;  // +0x20  Close()'d+removed+deleted first in Clear
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
    i32 m_longestScopeNameLen;  // +0x58  longest scope-name length seen (SymTab.cpp)
    i32 m_longestLeafNameLen;   // +0x5c  longest leaf-name length seen (SymTab.cpp AddNodeEntry)
    i32 m_60;                   // +0x60
    char* m_cachedSourceBuffer; // +0x64  strdup'd source buffer (RezFree'd)
    i32 m_68;                   // +0x68  flag forwarded to the +0x38 walk (m_68 == 0)
    i32 m_6c;                   // +0x6c  selects the leaf-record ctor variant (Init4 vs Init3)
    i32 m_70;                   // +0x70  leaf-record ctor arg
    i32 m_74;                   // +0x74  leaf-record ctor arg (Init4 only)
    i32 m_subTabBucketCount;    // +0x78  child-scope m_subTabs bucket count (CSymTab ctor subN)
    i32 m_symbolBucketCount;    // +0x7c  child-scope m_symbols bucket count (CSymTab ctor symN)
    CParserHash m_hash;         // +0x80
    DSoundList m_nodes;         // +0x88  { head, tail }
    i32 m_parseSlotBlockCount;  // +0x90  number of parse-slot records per allocated block
};
SIZE(0x94); // fields through m_parseSlotBlockCount @0x90

#endif // SRC_BUTE_SYMPARSER_H
