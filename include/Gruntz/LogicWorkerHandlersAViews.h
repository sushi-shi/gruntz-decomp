// LogicWorkerHandlersAViews.h - matching scaffolding for the default-message-pump
// case-0 variant (src/Gruntz/LogicWorkerHandlersA.cpp). NOT fakes to fold onto real
// classes: these express a genuine MSVC5 codegen limitation - the inline-XOR-out-of-line
// ctor wall. The inlined leaf ctor CALLS the out-of-line CUserLogic base ctor (0x58cd0),
// whereas the sibling leaf ctors (e.g. CDoNothing) INLINE the CUserLogic init; one TU
// cannot model CUserLogic's ctor both inline AND out-of-line-called. CUserLogicOOL is the
// escape (a distinct class whose ctor IS the 0x58cd0 call), DnnRec derives it, and the
// owner/worker views model only the pump fields. Scaffolding header -> excluded from the
// .cpp-local view metric.
#ifndef GRUNTZ_LOGICWORKERHANDLERSAVIEWS_H
#define GRUNTZ_LOGICWORKERHANDLERSAVIEWS_H

#include <rva.h>
#include <Ints.h>

struct DnnOwner;

// The OUT-OF-LINE shared CUserLogic(CGameObject*) base ctor (0x58cd0). External/no-body
// so the chained `call` reloc-masks; modeled as a non-polymorphic base the leaf chains.
struct CUserLogicOOL {
    // +0x00 vptr anchor (declared-only): slot 0 of the REAL table is the scalar-deleting
    // dtor - named so, not a C++ dtor, because a dtor here would add an EH unwind funclet
    // the retail `new DnnRec` frame does not have.
    virtual void ScalarDtor();
    CUserLogicOOL(DnnOwner* owner); // 0x58cd0
    char m_pad04[0x34 - 0x04];      // +0x04..+0x33
    DnnOwner* m_34;                 // +0x34
    DnnOwner* m_38;                 // +0x38
    void* m_3c;                     // +0x3c
};

// Real polymorphic: DnnRec makes cl auto-stamp its own ??_7DnnRec at ctor entry.
struct DnnRec : CUserLogicOOL {
    char m_animRegistry[0x54 - 0x40]; // +0x40..+0x53
    DnnRec(DnnOwner* owner);
};

// The dispatch interface: same vtable slot layout as the engine record; declared-only +
// never constructed (no ??_7 emitted), reinterpreted to lower `mov eax,[rec]; call [eax+N]`.
class EngRec {
public:
    virtual void s0();       // +0x00
    virtual void s1();       // +0x04
    virtual void s2();       // +0x08
    virtual void s3();       // +0x0c
    virtual void s4();       // +0x10
    virtual void s5();       // +0x14
    virtual void Activate(); // +0x18  (slot 6)
    virtual void s7();       // +0x1c
    virtual void s8();       // +0x20
    virtual void s9();       // +0x24
    virtual void V28();      // +0x28
    virtual void V2C();      // +0x2c
    virtual void V30();      // +0x30
    virtual void V34();      // +0x34
    virtual void V38();      // +0x38
    virtual void V3C();      // +0x3c
};

// The worker held at owner->m_7c; only the pump fields are modeled.
struct DnnWorker {
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    char m_pad04[0x18 - 0x04];
    DnnRec* m_18; // +0x18  the live record
    u32 m_1c;     // +0x1c  state tag (UNSIGNED switch key)
};

// The owner game object handed to the pump; its worker hangs at +0x7c.
struct DnnOwner {
    char m_pad00[0x08];
    u32 m_08; // +0x08
    char m_pad0c[0x7c - 0x0c];
    DnnWorker* m_7c; // +0x7c
};

// HandlerA9E00's record ctor stamps 0x1e859c == CDoNothingNormal's bound table
// (DnnRec IS the DoNothingNormal record); its ??_7 reloc-masks it.
RELOC_VTBL(DnnRec, 0x001e859c);

#endif // GRUNTZ_LOGICWORKERHANDLERSAVIEWS_H
