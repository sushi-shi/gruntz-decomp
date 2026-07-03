// ClassUnknown48.cpp - the ClassUnknown_48 destructor (0x139cf0): it drains two
// intrusive `List` members (m_1c is
// purged only when the owner flag m_2c->m_18->m_6c is set; m_24 re-files each
// node's m_14 payload as it is removed), zeroes m_0/m_18, then the two List
// members are destroyed in reverse order (m_24 then m_1c) - each ~List() inlines
// to a RemoveAll() call, which is what drives the /GX trylevel 1->0->-1 frame.
// Field names placeholders; list/node/manager methods external (reloc-masked).
#include <rva.h>
#include <Mfc.h>

struct Inner {
    void Func1397a0();
};
struct Node {
    char pad0[0x14];
    Inner* m_14; // 0x14
    Node* Next();
};
struct List {
    char data[8];
    List();          // 0x184950  default container ctor (reloc-masked call)
    List(int count); // 0x184960  sized container ctor (reloc-masked call)
    Node* First();
    void Remove(Node* n);
    void RemoveAll();
    ~List() {
        RemoveAll();
    }
};
struct Mgr18 {
    char pad0[0x6c];
    int m_6c; // 0x6c
    void AddNode(Inner* n);
};
struct Mgr2c {
    char pad0[0x18];
    Mgr18* m_18; // 0x18
};

// The polymorphic sub-object embedded at +0x4 (vtable 0x5ef744): spans +0x4..+0x1c.
// Its (inlined) ctor stamps the vptr and zeroes the owner back-ptr at +0x14 (parent
// +0x18); the enclosing ctor then re-points that back-ptr at `this`.
SIZE_UNKNOWN(SymEntryHdr);
struct SymEntryHdr {
    virtual void Vf0(); // slot 0 (declared-only -> vptr reloc-masks vs 0x5ef744)
    char pad4[0x14 - 0x4];
    void* m_owner; // +0x14 (parent +0x18)
    SymEntryHdr() {
        m_owner = 0;
    }
};

SIZE_UNKNOWN(ClassUnknown_48);
class ClassUnknown_48 {
public:
    void* m_0;         // 0x0
    SymEntryHdr m_hdr; // 0x4  (spans +0x4..+0x1c; m_hdr.m_owner == the old m_18 @+0x18)
    List m_1c;         // 0x1c
    List m_24;         // 0x24
    Mgr2c* m_2c;       // 0x2c
    // The two ctors (re-homed from src/Stub/MallocConstructors): xref confirms both
    // are called by CSymTab::FindOrAddSym (0x13a940), so ClassUnknown_48 is the
    // CSymTab symbol-entry node whose teardown is the dtor below (its layout - vptr
    // stamped @+0x4=0x5ef744, the m_1c/m_24 array-backed lists built by the container
    // ctor 0x184960/0x184950, m_18=this, m_2c - matches the dtor's exactly).
    ClassUnknown_48(void* a, Mgr2c* b, int c, int d); // 0x139bf0
    ClassUnknown_48(void* a, Mgr2c* b, int c);        // 0x139c80
    ~ClassUnknown_48();
};

// The two CSymTab symbol-entry node ctors. Both stamp the +0x4 sub-object vtable
// (via SymEntryHdr's inlined ctor), build the two array-backed list members m_1c/m_24
// (reloc-masked CSymList calls: sized 0x184960, default 0x184950), then wire m_0/back-
// ptr/m_2c. The destructible List members force the /GX EH frame (member-teardown on a
// throwing sub-ctor). Field roles are placeholders; only offsets + code bytes bind.
// @early-stop
// ~99.1% register-assignment tail: body byte-faithful (the IDENTICAL body gives the
// 3-arg overload 100%). The extra `d` param's stack shift only flips cl's a<->b GPR
// choice (a->eax/b->edx vs retail a->edx/b->eax) on the two trailing arg reloads +
// one store slot. Pure allocator coin-flip; source-order tuned to the best of 3.
RVA(0x00139bf0, 0x71)
ClassUnknown_48::ClassUnknown_48(void* a, Mgr2c* b, int c, int d) : m_1c(c), m_24(d) {
    m_2c = b;
    m_0 = a;
    m_hdr.m_owner = this;
}

// The 3-arg overload: m_1c uses the default container ctor (0x184950), m_24 the sized
// one (0x184960).
RVA(0x00139c80, 0x6c)
ClassUnknown_48::ClassUnknown_48(void* a, Mgr2c* b, int c) : m_1c(), m_24(c) {
    m_0 = a;
    m_hdr.m_owner = this;
    m_2c = b;
}

RVA(0x00139cf0, 0xd7)
ClassUnknown_48::~ClassUnknown_48() {
    if (m_2c->m_18->m_6c != 0) {
        Node* n = m_1c.First();
        while (n) {
            Node* cur = n;
            n = cur->Next();
            m_1c.Remove(cur);
        }
    }
    Node* n = m_24.First();
    while (n) {
        Node* cur = n;
        n = cur->Next();
        m_24.Remove(cur);
        cur->m_14->Func1397a0();
        m_2c->m_18->AddNode(cur->m_14);
    }
    m_0 = 0;
    m_hdr.m_owner = 0;
}

SIZE_UNKNOWN(Inner);
SIZE_UNKNOWN(List);
SIZE_UNKNOWN(Mgr18);
SIZE_UNKNOWN(Mgr2c);
SIZE_UNKNOWN(Node);
