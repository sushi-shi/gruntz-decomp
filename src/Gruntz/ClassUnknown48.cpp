// ClassUnknown48.cpp - re-homed from src/Stub/Discovered.cpp (0x139cf0). This is
// the ClassUnknown_48 destructor: it drains two intrusive `List` members (m_1c is
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
    ~ClassUnknown_48();
};

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

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(Inner);
SIZE_UNKNOWN(List);
SIZE_UNKNOWN(Mgr18);
SIZE_UNKNOWN(Mgr2c);
SIZE_UNKNOWN(Node);
