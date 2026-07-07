// SymRec.cpp (renamed from Gruntz/ClassUnknown48.cpp; real Bute home) - CSymRec, the
// CSymTab leaf symbol record (the node CSymTab::FindOrAddSym builds into m_symbols).
// RTTI/Ghidra name CSymRec; XREF: both ctors (0x139bf0/0x139c80) and the teardown
// (0x139cf0) are called only by ?FindOrAddSym@CSymTab@@QAEPAXH@Z (0x13a940) - the
// same three RVAs <Bute/SymTab.h> lists as CSymRec's ctors + Clear. The +0x4
// sub-object's vtable is Ghidra-labelled CSymTab_node (0x5ef748).
//
// The teardown (0x139cf0) drains two intrusive CSymList members (m_1c is purged
// only when the owner scope's parser flag m_2c->m_owner->m_6c is set; m_24 re-files
// each node's payload as it is removed), zeroes m_0/m_owner, then the two lists are
// destroyed in reverse order (m_24 then m_1c) - each ~CSymList() inlines to a
// RemoveAll() call, which drives the /GX trylevel 1->0->-1 frame.
//
// COLLISION NOTE (SymTab.h): <Bute/SymTab.h> also declares `class CSymRec`, but only
// as a MINIMAL declared-only stub - one hash sub-table m_valTable @+0x24 (all
// SymTab.cpp needs: it derefs only `rec->m_valTable.Walk` @+0x24, which THIS full
// model's m_24 @+0x24 matches). This TU carries the FULL, more-accurate CSymRec: the
// dtor proves a SECOND live hash container @+0x1c (m_1c, drained conditionally) and
// the owner-scope back-ptr @+0x2c (m_2c) that the SymTab.h stub under-models as pad.
// Full header unification (extend SymTab.h to the two-container layout + port these
// bodies onto canonical CHashTable/CHashElement) is deferred: it must not regress
// SymTab.cpp (Insert @0x13a000) nor these near-100% ctor/dtor bodies. So the two
// definitions stay separate TUs for now; CSymTab/CSymParser/CSymList here are reduced
// TU-local views (full defs in <Bute/SymTab.h>). Only offsets + code bytes bind.
#include <rva.h>
#include <Mfc.h>

// The re-filed leaf payload (CSymListNode::m_14): reset then handed back to the
// parser's free pool on teardown (0x1397a0 is a CSymRec method per SymTab.h).
// The payload's teardown @0x1397a0 IS Obj1397a0::Teardown (header-less); local decl.
class Obj1397a0 {
public:
    void Teardown();
};
struct CSymPayload {
    // Reset1397a0 @0x1397a0 IS Obj1397a0::Teardown; cast at the call.
};
struct CSymListNode {
    char pad0[0x14];
    CSymPayload* m_14; // 0x14  the leaf payload
    CSymListNode* Next();
};
// The engine array-backed list container (CSymList: sized ctor 0x184960, default
// 0x184950); each ~CSymList inlines to RemoveAll().
struct CSymList {
    char data[8];
    CSymList();          // 0x184950  default container ctor (reloc-masked call)
    CSymList(int count); // 0x184960  sized container ctor (reloc-masked call)
    CSymListNode* First();
    void Remove(CSymListNode* n);
    void RemoveAll();
    ~CSymList() {
        RemoveAll();
    }
};
// The owning parser (CSymTab::m_owner @+0x18): supplies the leaf-ctor-select flag
// m_6c and reclaims re-filed payloads via AddNode.
struct CSymParser {
    char pad0[0x6c];
    int m_6c; // 0x6c  leaf-ctor-variant flag (FindOrAddSym branches on it)
    void AddNode(CSymPayload* n);
};
// The owning CSymTab scope (CSymRec::m_2c): reached via `this` in FindOrAddSym.
struct CSymTab {
    char pad0[0x18];
    CSymParser* m_owner; // 0x18  CSymTab::m_owner
};

// The polymorphic hash-node prefix embedded at +0x4 (vtable 0x5ef748, Ghidra
// CSymTab_node): the reloc-masked interface that lets the record be inserted into
// CSymTab's m_symbols hash. Spans +0x4..+0x1c; its (inlined) ctor stamps the vptr
// and zeroes the owner back-ptr at +0x14 (record +0x18); the record ctor then
// re-points that back-ptr at `this`.
SIZE_UNKNOWN(CHashInsertNode);
struct CHashInsertNode {
    virtual void Vf0(); // slot 0 (declared-only -> vptr reloc-masks vs 0x5ef748)
    char pad4[0x14 - 0x4];
    void* m_owner; // +0x14 (record +0x18)
    CHashInsertNode() {
        m_owner = 0;
    }
};

SIZE_UNKNOWN(CSymRec);
class CSymRec {
public:
    void* m_0;             // 0x0
    CHashInsertNode m_hdr; // 0x4  (spans +0x4..+0x1c; m_hdr.m_owner == record +0x18)
    CSymList m_1c;         // 0x1c
    CSymList m_24;         // 0x24
    CSymTab* m_2c;         // 0x2c  the owning scope
    // The two ctors (re-homed from src/Stub/MallocConstructors): xref confirms both
    // are called by CSymTab::FindOrAddSym (0x13a940), so CSymRec is the CSymTab
    // symbol-entry node whose teardown is the dtor below (its layout - vptr stamped
    // @+0x4=0x5ef748, the m_1c/m_24 array-backed lists built by the CSymList container
    // ctor 0x184960/0x184950, m_2c owner scope - matches the dtor's exactly).
    CSymRec(void* a, CSymTab* owner, int c, int d); // 0x139bf0
    CSymRec(void* a, CSymTab* owner, int c);        // 0x139c80
    ~CSymRec();
};

// The two CSymTab symbol-entry node ctors. Both stamp the +0x4 sub-object vtable
// (via CHashInsertNode's inlined ctor), build the two array-backed CSymList members
// m_1c/m_24 (reloc-masked: sized 0x184960, default 0x184950), then wire m_0/back-
// ptr/m_2c. The destructible CSymList members force the /GX EH frame (member-teardown
// on a throwing sub-ctor). Field roles are placeholders; only offsets + code bytes bind.
// @early-stop
// ~99.1% register-assignment tail: body byte-faithful (the IDENTICAL body gives the
// 3-arg overload 100%). The extra `d` param's stack shift only flips cl's a<->owner GPR
// choice (a->eax/owner->edx vs retail a->edx/owner->eax) on the two trailing arg reloads +
// one store slot. Pure allocator coin-flip; source-order tuned to the best of 3.
RVA(0x00139bf0, 0x71)
CSymRec::CSymRec(void* a, CSymTab* owner, int c, int d) : m_1c(c), m_24(d) {
    m_2c = owner;
    m_0 = a;
    m_hdr.m_owner = this;
}

// The 3-arg overload: m_1c uses the default CSymList ctor (0x184950), m_24 the sized
// one (0x184960).
RVA(0x00139c80, 0x6c)
CSymRec::CSymRec(void* a, CSymTab* owner, int c) : m_1c(), m_24(c) {
    m_0 = a;
    m_hdr.m_owner = this;
    m_2c = owner;
}

RVA(0x00139cf0, 0xd7)
CSymRec::~CSymRec() {
    if (m_2c->m_owner->m_6c != 0) {
        CSymListNode* n = m_1c.First();
        while (n) {
            CSymListNode* cur = n;
            n = cur->Next();
            m_1c.Remove(cur);
        }
    }
    CSymListNode* n = m_24.First();
    while (n) {
        CSymListNode* cur = n;
        n = cur->Next();
        m_24.Remove(cur);
        ((Obj1397a0*)cur->m_14)->Teardown();
        m_2c->m_owner->AddNode(cur->m_14);
    }
    m_0 = 0;
    m_hdr.m_owner = 0;
}

SIZE_UNKNOWN(CSymPayload);
SIZE_UNKNOWN(CSymList);
SIZE_UNKNOWN(CSymParser);
SIZE_UNKNOWN(CSymTab);
SIZE_UNKNOWN(CSymListNode);
