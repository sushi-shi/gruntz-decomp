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

class ClassUnknown_48 {
public:
    int m_0; // 0x0
    char pad4[0x18 - 0x4];
    int m_18;    // 0x18
    List m_1c;   // 0x1c
    List m_24;   // 0x24
    Mgr2c* m_2c; // 0x2c
    // The two ctors (re-homed from src/Stub/MallocConstructors): xref confirms both
    // are called by CSymTab::FindOrAddSym (0x13a940), so ClassUnknown_48 is the
    // CSymTab symbol-entry node whose teardown is the dtor below (its layout - vptr
    // stamped @+0x4=0x5ef744, the m_1c/m_24 array-backed lists built by the container
    // ctor 0x184960/0x184950, m_18=this, m_2c - matches the dtor's exactly).
    ClassUnknown_48(void* a, void* b, void* c, void* d); // 0x139bf0
    ClassUnknown_48(void* a, void* b, void* c);          // 0x139c80
    ~ClassUnknown_48();
};

// reconstruction deferred: the m_1c/m_24 members are array-backed containers built
// by 0x184960/0x184950 (a shared sym-subsystem container, homed in SymTab.cpp),
// which is not yet modeled; the attribution (CSymTab entry ctor) is proven by xref.
// @confidence: high
// @source: xref
// @stub
RVA(0x00139bf0, 0x71)
ClassUnknown_48::ClassUnknown_48(void* a, void* b, void* c, void* d) {}

// reconstruction deferred (see the 4-arg ctor above); same class, 3-arg overload.
// @confidence: high
// @source: xref
// @stub
RVA(0x00139c80, 0x6c)
ClassUnknown_48::ClassUnknown_48(void* a, void* b, void* c) {}

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
    m_18 = 0;
}

SIZE_UNKNOWN(Inner);
SIZE_UNKNOWN(List);
SIZE_UNKNOWN(Mgr18);
SIZE_UNKNOWN(Mgr2c);
SIZE_UNKNOWN(Node);
