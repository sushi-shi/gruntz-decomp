// SymParser.h - CSymParser, the ButeMgr parser/owner that builds the CSymTab scope
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
#include <rva.h>

#include <Ints.h>

#include <Bute/Hash.h>
#include <Bute/SymTab.h> // the single full CSymTab layout (+ the CSymParser fwd-decl)

// The name->key map / seed builder ParseBuffer reaches: MakeSymSeed (0x13ba70, cdecl,
// the leftover-stack-args ctor trick) returns a seed left on the stack. Reloc-masked.
i32 MakeSymSeed(); // 0x13ba70

// The shared empty-string literal the root scope is named with (0x6293f4; homed in
// NetMgrReportError.cpp as extern "C" - the majority convention across the tree; the
// DATA reference reloc-masks so the C linkage is matching-neutral).
extern "C" char g_emptyString[]; // 0x6293f4

// CSymParser's own primary vtable (0x5ef750) is REAL POLYMORPHIC (??_7CSymParser@@6B@,
// 3-slot, non-virtual dtor) - see the class below. The +0x10 CObjList sub-object is
// also real polymorphic now (??_7CObjList @0x5ef760); the ctor/dtor vptr stamps
// reloc-mask against it.

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
SIZE(CObjNode, 0xc); // base subobject { vptr, next, prev }

// CObjListBase - the abstract list-interface base whose vtable (0x1ef760, one
// __purecall slot) is the DESTRUCTION vtable of the +0x10 CObjList sub-object: the
// CSymParser /GX dtor's last member-destruct stamps `mov [esi+0x10],0x5ef760`. Kept
// as a standalone abstract class (NOT wired as CObjList's C++ base) so the delicate
// CSymParser ctor/dtor codegen is untouched; the pair-of-vtables collapse to
// CObjList's own single ??_7 in our model (the dtor stamp reloc-masks).
SIZE(CObjListBase, 0x4);
VTBL(CObjListBase, 0x001ef760);
struct CObjListBase {
    virtual void V0() = 0; // slot 0 (__purecall)
};

// The CObjList sub-object embedded at +0x10: an intrusive doubly-linked list of
// polymorphic nodes. Its Remove (0x1852e0, __thiscall on the list head) unlinks a
// node from the {head@+4, tail@+8} pair. REAL POLYMORPHIC (ALL-VTABLES): the +0x00
// list-interface vtable is the implicit vptr (virtual dtor). The enclosing CSymParser
// ctor auto-stamps the vptr `mov [esi+0x10],0x5ef75c` (CObjList's OWN vtable, slot0
// sub_13c4c0); its /GX dtor's last member-destruct stamps the abstract base vtable
// 0x5ef760 (see CObjListBase) - both reloc-mask against ??_7CObjList.
VTBL(CObjList, 0x001ef75c);
struct CObjList {
    virtual ~CObjList() {} // +0x00 (this+0x10): list-interface vptr (slot0 sub_13c4c0)
    CObjNode* m_head;      // +0x04 (this+0x14)
    CObjNode* m_tail;      // +0x08 (this+0x18)
    i32 m_count;           // +0x0c (this+0x1c)

    // Remove(node): unlink a node from the chain (0x1852e0).
    void Remove(CObjNode* node);
    // Link(node): splice a freshly-built reader node onto the list (0x1851e0).
    void Link(void* node);
};
SIZE(CObjList, 0x10); // { vptr, head, tail, count }

// The parse-slot record block CSlotNode owns (an array of n*0x3c-byte slots);
// full definition in SymParser.cpp.
struct CParseSlot;

// A node owned by the +0x88 CHashSlotList: its intrusive chain link is at +0x00
// (so the list head points straight at it) and it owns a buffer at +0x08; the
// list uses the CHashSlotList::Link/Unlink (0x1390e0/0x1391e0) machinery from Hash.h.
struct CSlotNode {
    CHashLink m_link;     // +0x00  intrusive chain node { next, prev }
    CParseSlot* m_buffer; // +0x08  owned parse-slot block (RezFree'd)
};
SIZE(CSlotNode, 0xc);

// The +0x80 hash member. CHashBase carries no destructor (a standalone CHashBase
// value member in the shared hash/symtab TUs must stay trivially-destructible to
// preserve their codegen), so the destructible flavor used as a /GX member here is
// a thin local subclass whose teardown is RemoveAll. Same 8-byte layout (no new
// fields); only this TU sees the destructor, so Hash.h's siblings are unaffected.
struct CParserHash : public CHashBase {
    // The +0x80 hash-table member's 1-arg construction (0x184960) IS CSymList::Construct;
    // reached via a CSymList cast at the member-init call in SymParser.cpp.
    ~CParserHash() {
        RemoveAll();
    }
};
SIZE(CParserHash, 0x8); // derives CHashBase (no new fields)

// ---------------------------------------------------------------------------
// CSymParser - the ButeMgr parser/owner. REAL POLYMORPHIC (ALL-VTABLES phase):
// primary vtable ??_7CSymParser@@6B@ @0x5ef750 (3 non-dtor slots; the dtor is
// non-virtual - not in the vtable). cl auto-stamps the vptr @+0 at the start of
// the ctor AND the (non-virtual, but polymorphic-class) dtor - the manual
// CSymParser_vftable stamps are gone. The +0x10 CObjList member keeps its own
// manual vptr stamps (embedded-member "incorrect load into struct").
// ---------------------------------------------------------------------------
VTBL(CSymParser, 0x001ef750); // primary vtable (3 slots V0/V1/V2); ctor/dtor stamp
                              // `mov [esi],0x5ef750` at +0. Rehomed from AnalysisVtables.
class CSymParser {
public:
    virtual void V0(); // slot 0 (sub_13b9f0)
    virtual void V1(); // slot 1 (sub_13ba00)
    virtual void V2(); // slot 2 (sub_13ba10)

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

    // MakeSeed: the name-keyed seed builder (0x13ba70) dispatched __thiscall on the
    // parser (ecx = this) - the same physical seed builder MakeSymSeed calls as a
    // free cdecl, reached here with a `this` (SymTab.cpp AddNodeEntry). Bare decl (no
    // RVA - the RVA is carried by MakeSymSeed); the rel32 call reloc-masks.
    i32 MakeSeed(); // 0x13ba70 (__thiscall view of the seed builder)

    // The three path-resolution thunks: forward into GetRoot()'s CSymTab.
    i32 ResolveQualified(const char* name, void* arg); // 0x13bff0 -> root->ResolveQualified
    void* ResolvePath(const char* path);               // 0x13c030 -> root->ResolvePath
    void AddNode(void* rec);                           // 0x13c210 -> m_hash insert

    // vptr implicit @ +0x00 (??_7CSymParser@@6B@)
    // +0x04  owned delimiter-set buffer: the tokenizer split set the CSymTab path
    //         resolvers read (SymTab.cpp m_owner->m_delims), RezFree'd in the dtor.
    char* m_delims;
    i32 m_08;                   // +0x08  (=1)
    i32 m_parseArmed;           // +0x0c  Clear guard (0/1 flag)
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
    CHashSlotList m_nodes;      // +0x88  { head, tail }
    i32 m_parseSlotBlockCount;  // +0x90  number of parse-slot records per allocated block
};
SIZE(CSymParser, 0x94); // fields through m_parseSlotBlockCount @0x90

#endif // SRC_BUTE_SYMPARSER_H
