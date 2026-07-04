// WwdGameObjectEh.cpp - the /GX destructor family of CWwdGameObject and its factory
// variants. Modeled as a REAL local polymorphic hierarchy
// (docs/patterns/eh-dtor-multilevel-polymorphic-chain.md): a base CWwdGameObject
// "Mid" level (vtable 0x5f0020) owns the four polymorphic worker pointers, a CString
// name (+0xdc), and two RAII sentinel-handle members (EdgeA/EdgeB) whose call-free
// dtors clear the base fields; its grand-base Wap::CObject (vtable 0x5e8cb4) just
// re-stamps. The thin factory variants A/C/F derive from Mid (each with its own
// most-derived vtable) and re-run the worker pass before folding Mid. cl emits the
// per-level vptr re-stamps + /GX trylevel chain; the stamps reloc-mask against the
// retail engine vtables. Field names are placeholders; only offsets + code bytes
// are load-bearing.
#include <Ints.h>
#include <Wap32/Object.h> // Wap::CObject - the shared engine grand-base
#include <rva.h>

// The wap-object teardown grand-base is Wap::CObject (vtable 0x5e8cb4, shared
// ??_7Wap@@CObject; Wap32/Object.h). Its empty inline dtor re-stamps only, folded
// LAST (sinking the base vptr store to the function tail, preceded by call-free
// field writes) - was the manual `m_vptr = &g_wapObjectDtorVtbl` store.

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

// The embedded +0x1a0 command sub-object (B variant). Real polymorphic base
// (Wap::CObject, vtable 0x5e8cb4): cl auto-emits the grand-base vptr re-stamp at
// teardown (was a manual `m_vptr = &g_wapObjectDtorVtbl` store). Mirrors WwdSubA.
struct WwdSub : public Wap::CObject {
    void DtorImpl(); // 0x15c2c0  __thiscall
    ~WwdSub() OVERRIDE {
        DtorImpl();
    }
    i32 m_04; // 0x1a4
    i32 m_08; // 0x1a8
    i32 m_0c; // 0x1ac
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

// A's embedded +0x1a0 command sub-object, modeled polymorphically: its own vtable
// 0x5f0128, a member-teardown helper (0x15c2c0), an EdgeB sentinel, then the wap-object base
// base re-stamp folded in.
struct WwdSubA : public Wap::CObject {
    ~WwdSubA() OVERRIDE;
    void DtorImpl(); // 0x15c2c0
    WwdEdgeB m_04;   // +0x04 (0x1a4/0x1a8/0x1ac)
};
inline WwdSubA::~WwdSubA() {
    DtorImpl();
}

// ---------------------------------------------------------------------------
// 0x15b4f0 - the base ~CWwdGameObject ("Mid"): vtable 0x5f0020. Frees the four
// workers, clears m_c0/m_d8 + the EdgeA shadow (groupX), then the CString member
// dtor, then folds EdgeA, EdgeB and the wap-object grand base (groupY + base-vtable stamp).
// ---------------------------------------------------------------------------
class CWwdGameObjectE : public Wap::CObject {
public:
    ~CWwdGameObjectE() OVERRIDE; // 0x15b4f0

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
    // m_dc (CString) destroyed as a member; then EdgeA, EdgeB, base fold in.
}

// ---------------------------------------------------------------------------
// 0x15b790 - the complete destructor: a thin derived class (vtable 0x5f00a8) on top
// of Mid, adding the m_18c block + the embedded WwdSubA command object at +0x1a0.
// ---------------------------------------------------------------------------
class CWwdGameObjectA : public CWwdGameObjectE {
public:
    ~CWwdGameObjectA() OVERRIDE; // 0x15b790

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
// Mid -> wap-object base) + trylevel chain reproduced; residual is the callee-saved const
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
// of Mid. Re-runs the worker pass + groupX, then folds Mid + wap-object base.
// ---------------------------------------------------------------------------
class CWwdGameObjectF : public CWwdGameObjectE {
public:
    ~CWwdGameObjectF() OVERRIDE; // 0x15bad0
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
    // Mid (CWwdGameObjectE) folds the CString member + EdgeA/EdgeB + base-vtable stamp.
}

// ---------------------------------------------------------------------------
// 0x15bd10 - the CResolveNode-derived variant (extra +0x1dc CObList, leading init call
// 0x166810, trailing base CResolveNode dtor 0x429b). REAL-POLYMORPHIC 4-level chain
// (the vtable @0x5f00e8 was g_wwd1598d0FinalVtbl / vtbl-50): the destructor's
// four manual vtable restamps become the cl-emitted per-level vptr stamps of
//   CWwdGameObjectB (0x5f00e8) : WwdBLevel2 (0x5f00a8) : WwdBMid (0x5f0020)
//                             : WwdBResolve (0x5efbc0, virtual dtor -> DtorBase 0x429b)
// so cl auto-generates the multi-phase restamp + /GX trylevel chain; each stamp
// reloc-masks against the retail engine vtable. Each level owns a contiguous field
// range + one destructible member (CString@+0xdc / WwdSub@+0x1a0 / CObList@+0x1dc);
// the derived-level dtor bodies re-clear inherited base fields exactly like retail's
// per-phase re-clears. This is the CResolveNode-derived variant the flat model was
// @early-stop on (eh-dtor-multilevel-polymorphic-chain.md).
// ---------------------------------------------------------------------------

// The +0x1dc CObList member; its dtor is DtorList (0x1b5a2b, reloc-masked __thiscall).
struct WwdObList {
    void DtorImpl(); // 0x1b5a2b
    ~WwdObList() {
        DtorImpl();
    }
    i32 m_head; // +0x1dc
};

// Grand-base (vtable 0x5efbc0): a CResolveNode-style base with a virtual dtor (making
// the whole chain polymorphic). Restamps its vftable then tail-calls the base
// CResolveNode teardown (0x429b). Owns the +0x04..+0x5c fields; folded LAST.
struct WwdBResolve {
    virtual ~WwdBResolve();
    void DtorBase(); // 0x429b
    i32 m_04;        // +0x04
    i32 m_08;        // +0x08
    i32 m_0c;        // +0x0c
    char _p10[0x20 - 0x10];
    i32 m_20; // +0x20
    char _p24[0x38 - 0x24];
    i32 m_38; // +0x38
    char _p3c[0x5c - 0x3c];
    i32 m_5c;               // +0x5c
    char _p60[0x7c - 0x60]; // pad so WwdBMid's m_7c lands at +0x7c
};
inline WwdBResolve::~WwdBResolve() {
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    DtorBase();
}

// Mid level (vtable 0x5f0020): frees the four workers, clears m_c0/m_d8 + the
// inherited edge fields, then its CString member folds, then ~WwdBResolve.
struct WwdBMid : public WwdBResolve {
    ~WwdBMid() OVERRIDE;
    WwdWorker* m_7c; // +0x7c
    WwdWorker* m_80; // +0x80
    char _p84[0x88 - 0x84];
    WwdWorker* m_88; // +0x88
    char _p8c[0x90 - 0x8c];
    WwdWorker* m_90; // +0x90
    char _p94[0xc0 - 0x94];
    i32 m_c0; // +0xc0
    char _pc4[0xd8 - 0xc4];
    i32 m_d8;                // +0xd8
    WwdName m_dc;            // +0xdc  CString
    char _pe0[0x18c - 0xe0]; // pad so WwdBLevel2's m_18c lands at +0x18c
};
inline WwdBMid::~WwdBMid() {
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_c0 = (i32)0x80000000;
    m_d8 = -1;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    // m_dc (CString) auto-destroyed, then ~WwdBResolve folds.
}

// Level-2 (vtable 0x5f00a8): clears the m_18c block + runs SubB, then its embedded
// WwdSub command object folds, then ~WwdBMid.
struct WwdBLevel2 : public WwdBMid {
    ~WwdBLevel2() OVERRIDE;
    void SubB(); // 0x15b5d0
    i32 m_18c;   // +0x18c
    i32 m_190;   // +0x190
    i32 m_194;   // +0x194
    i32 m_198;   // +0x198
    char _p19c[0x1a0 - 0x19c];
    WwdSub m_1a0;              // +0x1a0
    char _p1b0[0x1dc - 0x1b0]; // pad so CWwdGameObjectB's m_1dc lands at +0x1dc
};
inline WwdBLevel2::~WwdBLevel2() {
    m_18c = -1;
    m_190 = -1;
    m_198 = 0;
    m_194 = 0;
    SubB();
    // m_1a0 (WwdSub) auto-destroyed, then ~WwdBMid folds.
}

// Most-derived (vtable 0x5f00e8): leading InitDtor, the worker + field pass, then
// its CObList member folds (DtorList), then ~WwdBLevel2.
class CWwdGameObjectB : public WwdBLevel2 {
public:
    ~CWwdGameObjectB() OVERRIDE; // 0x15bd10
    void InitDtor();             // 0x166810
    WwdObList m_1dc;             // +0x1dc  CObList
    char _p1e0[0x1f8 - 0x1e0];
    i32 m_1f8; // +0x1f8
};

// @early-stop
// eh-dtor multi-level trylevel wall: the real 4-level polymorphic chain reproduces
// the four cl-emitted vptr restamps + the per-phase field re-clears + the CString/
// WwdSub/CObList member folds; residual is the /GX trylevel numbering across the four
// destruct phases (the same zero-register-pinning const coloring as the A/C/F
// variants) - not source-steerable.
RVA(0x0015bd10, 0x1ef)
CWwdGameObjectB::~CWwdGameObjectB() {
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
    // m_1dc (CObList) auto-destroyed (DtorList), then ~WwdBLevel2 folds.
}

// ---------------------------------------------------------------------------
// 0x15c070 - the 0x159250-final variant: thin derived class (vtable 0x5effd0) on top
// of Mid; clears the byte flag m_18c, re-runs the worker pass + groupX, then folds
// Mid + wap-object base.
// ---------------------------------------------------------------------------
class CWwdGameObjectC : public CWwdGameObjectE {
public:
    ~CWwdGameObjectC() OVERRIDE; // 0x15c070

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
    // Mid (CWwdGameObjectE) folds the CString member + EdgeA/EdgeB + base-vtable stamp.
}
// Exact retail object sizes from the CWwdObjMgrFactories RezAlloc(0xNN) calls:
// A=0x166640 (0x1dc), B=0x1598d0 (0x1fc), C=0x159250 (0x190), F=0x159440 (0x18c).
// E (Mid) is the shared base subobject, not directly allocated -> size unresolved.
SIZE(CWwdGameObjectA, 0x1dc);
SIZE(CWwdGameObjectB, 0x1fc);
SIZE(CWwdGameObjectC, 0x190);
SIZE_UNKNOWN(CWwdGameObjectE);
SIZE(CWwdGameObjectF, 0x18c);
SIZE_UNKNOWN(WwdEdgeA);
SIZE_UNKNOWN(WwdEdgeB);
SIZE_UNKNOWN(WwdName);
SIZE_UNKNOWN(Wap::CObject);
SIZE_UNKNOWN(WwdSub);
SIZE_UNKNOWN(WwdSubA);
SIZE_UNKNOWN(WwdWorker);
SIZE_UNKNOWN(WwdBResolve);
SIZE_UNKNOWN(WwdBMid);
SIZE_UNKNOWN(WwdBLevel2);
SIZE_UNKNOWN(WwdObList);
