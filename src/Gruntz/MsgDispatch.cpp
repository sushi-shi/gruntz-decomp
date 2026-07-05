// MsgDispatch.cpp - three near-identical command dispatchers (orphan COMDATs
// @0xaa0a0 / 0xaa1e0 / 0xaa460) that route a UI message (arg->m_7c->m_1c) to the
// handler sub-object (m_18): command 0 lazily constructs the handler (new CObj(arg)
// + slot-6 init, /GX EH-framed), the mid commands fan out to its vtable slots, and
// the rest serialize through the shared default. The three differ only in their
// constructed handler class (reloc-masked ctor) + EH funclet, so one body matches
// all three. Placeholder names; only OFFSETS + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

struct CMsg;
struct CObjVtbl;

// The handler object (m_18): 0x54 bytes. This is a FOREIGN engine class - its ??_7
// and the dispatched slot bodies (6 / 10..15) are unreconstructed engine code, so
// the honest model is a manual vptr at +0 into a vtable struct that names ONLY the
// used slots (the gaps are documented `char` pad). Constructed via the external
// ctor (reloc-masked). Class COMPLETE before the T::* typedef (4-byte PMF, see
// docs/patterns/pmf-complete-class-4byte.md).
class CObj {
public:
    CObjVtbl* vptr;
    char _pad[0x54 - 4];
    CObj(CMsg* arg); // 0x2e4b (external, no body)
    void Slot18();   // +0x18 slot 6
    void Slot28();   // +0x28 slot 10
    void Slot2c();   // +0x2c slot 11
    void Slot30();   // +0x30 slot 12
    void Slot34();   // +0x34 slot 13
    void Slot38();   // +0x38 slot 14
    void Slot3c();   // +0x3c slot 15
};
typedef void (CObj::*ObjFn)();
struct CObjVtbl {
    char _00[0x18];
    ObjFn s18; // +0x18
    char _1c[0x28 - 0x1c];
    ObjFn s28; // +0x28
    ObjFn s2c; // +0x2c
    ObjFn s30; // +0x30
    ObjFn s34; // +0x34
    ObjFn s38; // +0x38
    ObjFn s3c; // +0x3c
};
inline void CObj::Slot18() {
    (this->*(vptr->s18))();
}
inline void CObj::Slot28() {
    (this->*(vptr->s28))();
}
inline void CObj::Slot2c() {
    (this->*(vptr->s2c))();
}
inline void CObj::Slot30() {
    (this->*(vptr->s30))();
}
inline void CObj::Slot34() {
    (this->*(vptr->s34))();
}
inline void CObj::Slot38() {
    (this->*(vptr->s38))();
}
inline void CObj::Slot3c() {
    (this->*(vptr->s3c))();
}

struct CSub {
    char _00[0x18];
    CObj* m_18; // +0x18
    u32 m_1c;   // +0x1c  command id (unsigned switch)
};
struct CMsg {
    char _00[0x7c];
    CSub* m_7c; // +0x7c
};

// The default case runs the type-keyed record transfer/dispatch: NOT a bespoke
// "dispatch default" - it is the shared CTypeKeyColl serializer ProjTypeXfer
// (0x16e4f0, owned + matched in TypeKeyColl.cpp), which resolves the handler's
// type-registry entry (ar->m_14->m_1c) and xfers it through the handler's own
// vtable slots. The handler (m_18) is passed as the archive-record arg.
struct CXferArchive;                // TypeKeyColl.cpp-local archive-record view
i32 ProjTypeXfer(CXferArchive* ar); // 0x16e4f0

// The dispatcher body, identical across the three instantiations (the only
// retail difference is the reloc-masked ctor + EH funclet).
#define DISPATCH_BODY                                                                              \
    CSub* sub = arg->m_7c;                                                                         \
    switch (sub->m_1c) {                                                                           \
        case 0: {                                                                                  \
            sub->m_1c = 0x3e8;                                                                     \
            CObj* o = new CObj(arg);                                                               \
            o->Slot18();                                                                           \
            sub->m_18 = o;                                                                         \
            break;                                                                                 \
        }                                                                                          \
        case 0x1d:                                                                                 \
            sub->m_18->Slot2c();                                                                   \
            break;                                                                                 \
        case 0x1e:                                                                                 \
            sub->m_18->Slot28();                                                                   \
            break;                                                                                 \
        case 0x50:                                                                                 \
            sub->m_18->Slot38();                                                                   \
            break;                                                                                 \
        case 0x51:                                                                                 \
            sub->m_18->Slot34();                                                                   \
            break;                                                                                 \
        case 0x52:                                                                                 \
            sub->m_18->Slot30();                                                                   \
            break;                                                                                 \
        case 0x53:                                                                                 \
            sub->m_18->Slot3c();                                                                   \
            break;                                                                                 \
        case 0x3e8:                                                                                \
            break;                                                                                 \
        default:                                                                                   \
            ProjTypeXfer((CXferArchive*)sub->m_18);                                                \
            break;                                                                                 \
    }                                                                                              \
    return 1;

RVA(0x000aa0a0, 0xf1)
i32 Dispatch_aa0a0(CMsg* arg){DISPATCH_BODY}

RVA(0x000aa1e0, 0xf1)
i32 Dispatch_aa1e0(CMsg* arg){DISPATCH_BODY}

RVA(0x000aa460, 0xf1)
i32 Dispatch_aa460(CMsg* arg){DISPATCH_BODY}

// class-metadata SIZE sweep (misc-Gruntz A-C): matching-neutral, hosted at
// .cpp EOF (see docs/class-metadata-sweep-log.md). SIZE_UNKNOWN = size not yet pinned.
SIZE_UNKNOWN(CMsg);
SIZE_UNKNOWN(CObj);
SIZE_UNKNOWN(CObjVtbl);
SIZE_UNKNOWN(CSub);
