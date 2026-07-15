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
// the clock-seed builder - returns time(&t)); the PackTag/UnpackTag ext<->key helpers.
// All declared in <Bute/SymTab.h> (included below), defined in SymParser.cpp.

// The shared empty-string literal the root scope is named with (0x6293f4; homed in
// NetMgrReportError.cpp as extern "C" - the majority convention across the tree; the
// DATA reference reloc-masks so the C linkage is matching-neutral).
#include <EmptyString.h> // g_emptyString (the shared "" constant)

// CSymParser's own primary vtable (0x5ef750) is REAL POLYMORPHIC (??_7CSymParser@@6B@,
// 3-slot, non-virtual dtor) - see the class below. The +0x10 CObjList sub-object is
// also real polymorphic now (??_7CObjList @0x5ef760); the ctor/dtor vptr stamps
// reloc-mask against it.

// DISSOLVED (Fable A2, 2026-07-14): the former "CSymObjNode" 8-slot reader-node
// view WAS the canonical CRezItmBase (<Rez/RezMgr.h>) - PROVEN by the reader
// ctors ParseBuffer calls: the text reader's ctor 0x13c940 stamps ??_7CRezDir
// (0x5ef7a8) and the binary reader's 0x13c540 stamps ??_7CRezItm (0x5ef788),
// both `: CRezItmBase` ({vptr,next,prev} @+0..+0xb, the same 8-slot table).
// Slot-for-slot: "Delete(1)" = the slot-1 scalar-deleting dtor (`delete p`);
// "ReadRaw" = Read [2]; "Slot0c" = Write [3]; the view's "Read(buf,a,b)" = Open
// [4] (ParseBuffer OPENS the source); "Detach" = Close [5]; "Slot18" = Flush [6];
// "Slot1c" = Check [7] (the CheckNodes probe). Forward-declared here (SymTab.cpp
// pulls the real <Rez/RezMgr.h>).
class CRezItmBase;

// CObjListBase - the abstract list-interface base whose vtable (0x1ef760, one
// __purecall slot) is the DESTRUCTION vtable of the +0x10 CObjList sub-object: the
// CSymParser /GX dtor's last member-destruct stamps `mov [esi+0x10],0x5ef760`. Kept
// as a standalone abstract class (NOT wired as CObjList's C++ base) so the delicate
// CSymParser ctor/dtor codegen is untouched; the pair-of-vtables collapse to
// CObjList's own single ??_7 in our model (the dtor stamp reloc-masks).
// (Def extracted to <Bute/ObjListBase.h> so the Rez list family can derive it.)
#include <Bute/ObjListBase.h>

// The parser's list sub-object embedded at +0x10: an intrusive doubly-linked list
// of polymorphic reader nodes, PLUS a count word. It derives the SHARED middle base
// CObjList (<Rez/RezList.h>: {vptr,head,tail} + the one bound Remove @0x1852e0 -
// deleting the former duplicate CObjList definition here) and adds m_count + its
// own 1-slot vtable (retail ??_7 @0x1ef75c: [0] = the empty fn 0x13c4c0, a sibling
// of CRezList's 0x13c4d0). The enclosing CSymParser ctor auto-stamps the member
// vptr `mov [esi+0x10],0x5ef75c`; its /GX dtor's member-destruct inlines the dtor
// chain, dead-store-eliminating down to the ??_7CObjListBase base stamp
// (`mov [esi+0x10],0x5ef760`) - the retail shape.
#include <Rez/RezList.h>
VTBL(CParserObjList, 0x001ef75c);
struct CParserObjList : public CObjList {
    virtual void V0() OVERRIDE; // [0] 0x13c4c0 (empty body; declared-only, reloc-masked)
    ~CParserObjList() {}
    i32 m_count; // +0x0c (this+0x1c)

    // Link(node): splice a freshly-built reader node onto the list (0x1851e0 - the
    // same body as CRezList::AddHead; alias decl, reloc-masked).
    void Link(void* node);
};
SIZE(CParserObjList, 0x10); // { vptr, head, tail, count }

// The parse-slot record block CSlotNode owns is an array of n*0x3c-byte leaf-record
// slots - the same 0x3c CSymLeafBuilder record (m_node @+0x1c) the parser fills and
// re-files; defined in SymTab.cpp. (A freshly-popped slot is init'd as a CParseSource
// parse stream and later repurposed as a leaf value record - one 0x3c memory, two views.)
struct CSymLeafBuilder;

// A node owned by the +0x88 CHashSlotList: its intrusive chain link is at +0x00
// (so the list head points straight at it) and it owns a buffer at +0x08; the
// list uses the CHashSlotList::Link/Unlink (0x1390e0/0x1391e0) machinery from Hash.h.
struct CSlotNode {
    CHashLink m_link;          // +0x00  intrusive chain node { next, prev }
    CSymLeafBuilder* m_buffer; // +0x08  owned parse-slot block (RezFree'd)
};
SIZE(CSlotNode, 0xc);

// The +0x80 hash member. CHashBase carries no destructor (a standalone CHashBase
// value member in the shared hash/symtab TUs must stay trivially-destructible to
// preserve their codegen), so the destructible flavor used as a /GX member here is
// a thin local subclass whose teardown is RemoveAll. Same 8-byte layout (no new
// fields); only this TU sees the destructor, so Hash.h's siblings are unaffected.
struct CParserHash : public CHashBase {
    // The +0x80 hash-table member's 1-arg construction (0x184960) is the canonical
    // CHashBase::Construct (folded from the former CSymList::Construct view, wave5-F1),
    // reached cast-free as m_hash.Construct(1) in SymTab.cpp.
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
    void* PopParseSlot(); // 0x13c0c0

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
    i32 ResolveQualified(const char* name, void* arg); // 0x13bff0 -> root->ResolveQualified
    void* ResolvePath(const char* path);               // 0x13c030 -> root->ResolvePath
    void AddNode(void* rec);                           // 0x13c210 -> m_hash insert

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
    CHashSlotList m_nodes;      // +0x88  { head, tail }
    i32 m_parseSlotBlockCount;  // +0x90  number of parse-slot records per allocated block
};
SIZE(CSymParser, 0x94); // fields through m_parseSlotBlockCount @0x90

#endif // SRC_BUTE_SYMPARSER_H
