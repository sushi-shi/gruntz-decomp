// OrphanLeaves.cpp - a handful of tiny orphan-COMDAT leaf functions (getters,
// global teardowns, member sub-object dtors, a range registrar, a 2-field setter)
// that survive in the low-RVA COMDAT pool with no recoverable owning class. Each
// is modeled from its disassembly with a PLACEHOLDER class/name; only the OFFSETS
// + emitted code bytes are load-bearing (campaign doctrine). All engine callees
// are external/no-body so their call rel32 / DIR32 reloc-mask.
#include <Ints.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// 0x183d0: return the address of a fixed global (a runtime-class / map pointer).
DATA(0x001e8e98)
extern void* g_5e8e98;

RVA(0x000183d0, 0x6)
void* GetGlobal5e8e98() {
    return &g_5e8e98;
}

// ---------------------------------------------------------------------------
// 0x3ac30: tear down a global CString-like object (tail-call its Free at 0x1b9b93).
struct CGlobalStr {
    void Free(); // 0x1b9b93 (__thiscall)
};
DATA(0x0022c25c)
extern CGlobalStr g_62c25c;

RVA(0x0003ac30, 0xa)
void FreeGlobal62c25c() {
    g_62c25c.Free();
}

// ---------------------------------------------------------------------------
// 0x3cbc0 / 0x3cbf0: member sub-object destructors - run the embedded sub-object's
// two-phase teardown (a derived clear + the shared base dtor 0x169d70).
struct CSubObjC {
    void Clear();    // 0x16a240
    void BaseDtor(); // 0x169d70
};
struct CSubObj8 {
    void Clear();    // 0x16a8e0
    void BaseDtor(); // 0x169d70
};
struct COwnerWithSubs {
    char _00[0x0c];
    CSubObjC m_0c;   // +0x0c
    void DtorSubC(); // 0x3cbc0
    void DtorSub8(); // 0x3cbf0  (acts on +0x08)
};

RVA(0x0003cbc0, 0x14)
void COwnerWithSubs::DtorSubC() {
    CSubObjC* s = (CSubObjC*)((char*)this + 0xc);
    s->Clear();
    s->BaseDtor();
}

RVA(0x0003cbf0, 0x14)
void COwnerWithSubs::DtorSub8() {
    CSubObj8* s = (CSubObj8*)((char*)this + 8);
    s->Clear();
    s->BaseDtor();
}

// ---------------------------------------------------------------------------
// 0x3e120: register the default activation-id range [0x7d0, 0x7da] on a per-class
// registry (g_6446d8) via the shared SetActiveRange ILT thunk (0x3742).
struct CActReg6446d8 {
    void SetActiveRange(i32 lo, i32 hi); // 0x3742 (ILT thunk)
};
DATA(0x002446d8)
extern CActReg6446d8 g_6446d8;

RVA(0x0003e120, 0x15)
void Register6446d8Range() {
    g_6446d8.SetActiveRange(0x7d0, 0x7da);
}

// ---------------------------------------------------------------------------
// 0x75a10: a 2-field setter (CPoint/CSize-style) that fills m_0/m_4 and returns this.
struct CPairXY {
    i32 m_0;
    i32 m_4;
    CPairXY* Set(i32 a, i32 b); // 0x75a10
};

RVA(0x00075a10, 0x12)
CPairXY* CPairXY::Set(i32 a, i32 b) {
    m_0 = a;
    m_4 = b;
    return this;
}

// ---------------------------------------------------------------------------
// 0xb4330: run a one-shot helper (ILT 0x2914) and return 0.
extern "C" void Helper2914(); // 0x2914 (ILT thunk)

RVA(0x000b4330, 0x8)
i32 RunHelper2914() {
    Helper2914();
    return 0;
}
