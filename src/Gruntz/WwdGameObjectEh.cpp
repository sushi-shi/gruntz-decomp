// WwdGameObjectEh.cpp - the /GX destructor family of CWwdGameObject and its factory
// variants. Modeled as a REAL local polymorphic hierarchy
// (docs/patterns/eh-dtor-multilevel-polymorphic-chain.md): a base CWwdGameObject
// "Mid" level (vtable 0x5f0020) owns the four polymorphic worker pointers, a CString
// name (+0xdc), and two RAII sentinel-handle members (EdgeA/EdgeB) whose call-free
// dtors clear the base fields; its grand-base WwdSeverusBase (vtable 0x5e8cb4) just
// re-stamps. The thin factory variants A/C/F derive from Mid (each with its own
// most-derived vtable) and re-run the worker pass before folding Mid. cl emits the
// per-level vptr re-stamps + /GX trylevel chain; the stamps reloc-mask against the
// retail engine vtables. Field names are placeholders; only offsets + code bytes
// are load-bearing.
#include <Ints.h>
#include <rva.h>

// Reloc-masked engine vtables still referenced by the (flat, @early-stop) B variant.
extern void* g_wwd1598d0FinalVtbl;    // 0x5f00e8
extern void* g_wwdObjVtbl;            // 0x5f00a8
extern void* g_severusWorkerDtorVtbl; // 0x5e8cb4
extern void* g_wwdGameObjectVtbl;     // 0x5f0020
extern void* g_remusNodeVtbl;         // 0x5efbc0

// An owned polymorphic worker. Its scalar-deleting destructor is vtable slot 1
// (`mov eax,[ecx]; push 1; call [eax+4]`); declared-only (foreign vtable).
class WwdWorker {
public:
    virtual void Slot00();
    virtual void DeleteThis(i32 flag); // +0x04
};

// The CString name at +0xdc (NAFXCW dtor 0x1b9cde, reloc-masked).
struct WwdName {
    void DtorImpl(); // 0x1b9cde  __thiscall ~CString
    ~WwdName() {
        DtorImpl();
    }
    char* m_data;
};

// The embedded +0x1a0 command sub-object as the (flat) B variant models it.
struct WwdSub {
    void DtorImpl(); // 0x15c2c0  __thiscall
    ~WwdSub() {
        DtorImpl();
        m_vptr = &g_severusWorkerDtorVtbl;
    }
    void* m_vptr; // 0x1a0
    i32 m_04;     // 0x1a4
    i32 m_08;     // 0x1a8
    i32 m_0c;     // 0x1ac
};

// Manual scalar-delete of an owned worker pointer (the retail idiom).
#define WORKER_FREE(p)                                                                             \
    do {                                                                                           \
        if (p) {                                                                                   \
            (p)->DeleteThis(1);                                                                    \
            (p) = 0;                                                                               \
        }                                                                                          \
    } while (0)

// Two RAII sentinel-handle members of the Mid level: each is a small object whose
// (inline, call-free) destructor resets its fields to the "invalid" sentinel. They
// are destroyed in reverse declaration order after the CString member, giving
// retail's groupY tail (EdgeA: 5c,20,38 ; then EdgeB: 04,08,0c) and bumping the /GX
// trylevel (each is a fully-constructed top-level destructible subobject).
struct WwdEdgeB { // 0x04..0x0c
    ~WwdEdgeB();
    i32 a; // 0x04
    i32 b; // 0x08
    i32 c; // 0x0c
};
inline WwdEdgeB::~WwdEdgeB() {
    a = -1;
    b = 0;
    c = 0;
}
struct WwdEdgeA { // 0x20..0x5c
    ~WwdEdgeA();
    i32 a; // 0x20
    char _p[0x38 - 0x24];
    i32 b; // 0x38
    char _p2[0x5c - 0x3c];
    i32 c; // 0x5c
};
inline WwdEdgeA::~WwdEdgeA() {
    c = (i32)0x80000000;
    a = (i32)0x80000000;
    b = -1;
}

// The severus-worker teardown grand-base (vtable 0x5e8cb4 = g_severusWorkerDtorVtbl).
// Just the vptr; empty explicit body (re-stamps only). Folded LAST, sinking the
// severus vptr store to the function tail (it is preceded by call-free field writes).
struct WwdSeverusBase {
    virtual ~WwdSeverusBase();
};
inline WwdSeverusBase::~WwdSeverusBase() {}

// A's embedded +0x1a0 command sub-object, modeled polymorphically: its own vtable
// 0x5f0128, a member-teardown helper (0x15c2c0), an EdgeB sentinel, then the severus
// base re-stamp folded in.
struct WwdSubA : public WwdSeverusBase {
    ~WwdSubA();
    void DtorImpl(); // 0x15c2c0
    WwdEdgeB m_04;   // +0x04 (0x1a4/0x1a8/0x1ac)
};
inline WwdSubA::~WwdSubA() {
    DtorImpl();
}

// ---------------------------------------------------------------------------
// 0x15b4f0 - the base ~CWwdGameObject ("Mid"): vtable 0x5f0020. Frees the four
// workers, clears m_c0/m_d8 + the EdgeA shadow (groupX), then the CString member
// dtor, then folds EdgeA, EdgeB and the severus grand-base (groupY + severus stamp).
// ---------------------------------------------------------------------------
class CWwdGameObjectE : public WwdSeverusBase {
public:
    ~CWwdGameObjectE(); // 0x15b4f0

    WwdEdgeB m_04; // 0x04
    char _p10[0x20 - 0x10];
    WwdEdgeA m_20; // 0x20
    char _p60[0x7c - 0x60];
    WwdWorker* m_7c; // 0x7c
    WwdWorker* m_80; // 0x80
    char _p84[0x88 - 0x84];
    WwdWorker* m_88; // 0x88
    char _p8c[0x90 - 0x8c];
    WwdWorker* m_90; // 0x90
    char _p94[0xc0 - 0x94];
    i32 m_c0; // 0xc0
    char _pc4[0xd8 - 0xc4];
    i32 m_d8;     // 0xd8
    WwdName m_dc; // 0xdc  CString name
};

// @early-stop
// zero-register-pinning regalloc wall (docs/patterns/zero-register-pinning.md):
// logic + /GX trylevel chain (3->2) byte-exact, residual is the callee-saved
// zero/0x80000000/-1 register coloring (edi/ebx/ebp vs retail ebp/edi/ebx).
RVA(0x0015b4f0, 0xde)
inline CWwdGameObjectE::~CWwdGameObjectE() {
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_c0 = (i32)0x80000000;
    m_d8 = -1;
    m_20.c = (i32)0x80000000; // 0x5c
    m_20.a = (i32)0x80000000; // 0x20
    m_20.b = -1;              // 0x38
    // m_dc (CString) destroyed as a member; then EdgeA, EdgeB, severus fold in.
}

// ---------------------------------------------------------------------------
// 0x15b790 - the complete destructor: a thin derived class (vtable 0x5f00a8) on top
// of Mid, adding the m_18c block + the embedded WwdSubA command object at +0x1a0.
// ---------------------------------------------------------------------------
class CWwdGameObjectA : public CWwdGameObjectE {
public:
    ~CWwdGameObjectA(); // 0x15b790

    char _pe0[0x18c - 0xe0];
    i32 m_18c; // 0x18c
    i32 m_190; // 0x190
    i32 m_194; // 0x194
    i32 m_198; // 0x198
    char _p19c[0x1a0 - 0x19c];
    WwdSubA m_1a0; // 0x1a0
};

// @early-stop
// zero-register-pinning regalloc wall: three-level fold (A -> WwdSubA member ->
// Mid -> severus) + trylevel chain reproduced; residual is the callee-saved const
// register coloring across the two worker passes.
RVA(0x0015b790, 0x1a6)
CWwdGameObjectA::~CWwdGameObjectA() {
    m_18c = -1;
    m_190 = -1;
    m_198 = 0;
    m_194 = 0;
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_d8 = -1;
    m_c0 = (i32)0x80000000;
    m_20.c = (i32)0x80000000; // 0x5c
    m_20.b = -1;              // 0x38
    m_20.a = (i32)0x80000000; // 0x20
    // m_1a0 (WwdSubA) member destroyed; then Mid (E) folds.
}

// ---------------------------------------------------------------------------
// 0x15bad0 - the 0x159440-final variant: thin derived class (vtable 0x5f0060) on top
// of Mid. Re-runs the worker pass + groupX, then folds Mid + severus.
// ---------------------------------------------------------------------------
class CWwdGameObjectF : public CWwdGameObjectE {
public:
    ~CWwdGameObjectF(); // 0x15bad0
};

// @early-stop
// zero-register-pinning regalloc wall: two-level fold + double worker pass +
// trylevel chain reproduced; residual is callee-saved const register coloring.
RVA(0x0015bad0, 0x153)
CWwdGameObjectF::~CWwdGameObjectF() {
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_c0 = (i32)0x80000000;
    m_d8 = -1;
    m_20.c = (i32)0x80000000; // 0x5c
    m_20.a = (i32)0x80000000; // 0x20
    m_20.b = -1;              // 0x38
    // Mid (CWwdGameObjectE) folds the CString member + EdgeA/EdgeB + severus stamp.
}

// ---------------------------------------------------------------------------
// 0x15bd10 - the CRemusNode-derived variant (extra +0x1dc CObList, leading init call
// 0x166810, trailing base CRemusNode dtor 0x429b). Still modeled flat/manual.
// ---------------------------------------------------------------------------
class CWwdGameObjectB {
public:
    ~CWwdGameObjectB(); // 0x15bd10

    void InitDtor(); // 0x166810
    void DtorList(); // 0x1b5a2b  (CObList at +0x1dc)
    void SubB();     // 0x15b5d0
    void DtorBase(); // 0x429b    (base CRemusNode)

    void* m_vptr; // 0x00
    i32 m_04;     // 0x04
    i32 m_08;     // 0x08
    i32 m_0c;     // 0x0c
    char _p10[0x20 - 0x10];
    i32 m_20; // 0x20
    char _p24[0x38 - 0x24];
    i32 m_38; // 0x38
    char _p3c[0x5c - 0x3c];
    i32 m_5c; // 0x5c
    char _p60[0x7c - 0x60];
    WwdWorker* m_7c; // 0x7c
    WwdWorker* m_80; // 0x80
    char _p84[0x88 - 0x84];
    WwdWorker* m_88; // 0x88
    char _p8c[0x90 - 0x8c];
    WwdWorker* m_90; // 0x90
    char _p94[0xc0 - 0x94];
    i32 m_c0; // 0xc0
    char _pc4[0xd8 - 0xc4];
    i32 m_d8;     // 0xd8
    WwdName m_dc; // 0xdc
    char _pe0[0x18c - 0xe0];
    i32 m_18c; // 0x18c
    i32 m_190; // 0x190
    i32 m_194; // 0x194
    i32 m_198; // 0x198
    char _p19c[0x1a0 - 0x19c];
    WwdSub m_1a0; // 0x1a0
    char _p1b0[0x1dc - 0x1b0];
    i32 m_1dc; // 0x1dc  CObList head
    char _p1e0[0x1f8 - 0x1e0];
    i32 m_1f8; // 0x1f8
};

// @early-stop
// eh-dtor wall: multi-level vtable restamp + base CRemusNode teardown; trylevel
// numbering across three vtable phases not source-steerable (flat model).
RVA(0x0015bd10, 0x1ef)
CWwdGameObjectB::~CWwdGameObjectB() {
    m_vptr = &g_wwd1598d0FinalVtbl;
    InitDtor();
    WORKER_FREE(m_7c);
    m_1f8 = 0;
    m_18c = -1;
    m_190 = -1;
    m_198 = 0;
    m_194 = 0;
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_d8 = -1;
    m_c0 = (i32)0x80000000;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    DtorList();
    m_vptr = &g_wwdObjVtbl;
    m_18c = -1;
    m_190 = -1;
    m_198 = 0;
    m_194 = 0;
    SubB();
    // m_1a0 (WwdSub) auto-destroyed.
    m_vptr = &g_wwdGameObjectVtbl;
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_c0 = (i32)0x80000000;
    m_d8 = -1;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    // m_dc (CString) auto-destroyed.
    m_vptr = &g_remusNodeVtbl;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    DtorBase();
}

// ---------------------------------------------------------------------------
// 0x15c070 - the 0x159250-final variant: thin derived class (vtable 0x5effd0) on top
// of Mid; clears the byte flag m_18c, re-runs the worker pass + groupX, then folds
// Mid + severus.
// ---------------------------------------------------------------------------
class CWwdGameObjectC : public CWwdGameObjectE {
public:
    ~CWwdGameObjectC(); // 0x15c070

    char _pe0[0x18c - 0xe0];
    u8 m_18c; // 0x18c (byte flag)
};

// @early-stop
// zero-register-pinning regalloc wall: two-level fold + byte-flag clear + double
// worker pass + trylevel chain reproduced; residual is callee-saved const coloring.
RVA(0x0015c070, 0x159)
CWwdGameObjectC::~CWwdGameObjectC() {
    m_18c = 0;
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_c0 = (i32)0x80000000;
    m_d8 = -1;
    m_20.c = (i32)0x80000000; // 0x5c
    m_20.a = (i32)0x80000000; // 0x20
    m_20.b = -1;              // 0x38
    // Mid (CWwdGameObjectE) folds the CString member + EdgeA/EdgeB + severus stamp.
}
