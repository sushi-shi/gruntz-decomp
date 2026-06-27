// WwdGameObjectEh.cpp - the /GX destructor family of CWwdGameObject and its two
// sibling factory variants (0x15b790 the complete dtor, 0x15bd10 the
// CRemusNode-derived variant, 0x15c070 the 0x159250-final variant). Each tears
// down four owned polymorphic "worker" sub-objects at +0x7c/+0x80/+0x88/+0x90,
// an embedded command sub-object at +0x1a0, and a CString name at +0xdc, while
// re-stamping the level vtables as it walks the (manual) class hierarchy.
//
// Modeled per docs/patterns/eh-dtor-model-members-as-destructible.md: the CString
// name and the +0x1a0 sub-object are REAL destructible members so cl emits the
// /GX frame + trylevel chain; the four workers use the manual scalar-delete idiom
// (`if(p){p->vt[1](1); p=0;}`). The level vtables are reloc-masked DATA externs
// (bound in CWwdObjMgrFactories.cpp / CRemusNode.cpp). Field names are
// placeholders; only the offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>

// Reloc-masked engine vtables (DATA-bound in the factory/node TUs).
extern void* g_wwd1598d0FinalVtbl; // 0x5f00e8
extern void* g_wwdObjVtbl;         // 0x5f00a8
extern void* g_wwdSubVtbl;         // 0x5f0128
extern void* g_severusWorkerDtorVtbl; // 0x5e8cb4
extern void* g_wwdGameObjectVtbl;  // 0x5f0020
extern void* g_remusNodeVtbl;      // 0x5efbc0
extern void* g_wwd159250FinalVtbl; // 0x5effd0
extern void* g_wwd159440FinalVtbl; // 0x5f0060 (own name in CWwdObjMgrFactories.cpp)

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

// The embedded +0x1a0 command sub-object: member dtor 0x15c2c0 then a base
// vtable re-stamp; modeled as a destructible member.
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
#define WORKER_FREE(p) \
    do { \
        if (p) { \
            (p)->DeleteThis(1); \
            (p) = 0; \
        } \
    } while (0)

// ---------------------------------------------------------------------------
// 0x15b790 - the complete destructor.
// ---------------------------------------------------------------------------
class CWwdGameObjectA {
public:
    ~CWwdGameObjectA(); // 0x15b790

    void* m_vptr;  // 0x00
    i32 m_04;      // 0x04
    i32 m_08;      // 0x08
    i32 m_0c;      // 0x0c
    char _p10[0x20 - 0x10];
    i32 m_20;      // 0x20
    char _p24[0x38 - 0x24];
    i32 m_38;      // 0x38
    char _p3c[0x5c - 0x3c];
    i32 m_5c;      // 0x5c
    char _p60[0x7c - 0x60];
    WwdWorker* m_7c; // 0x7c
    WwdWorker* m_80; // 0x80
    char _p84[0x88 - 0x84];
    WwdWorker* m_88; // 0x88
    char _p8c[0x90 - 0x8c];
    WwdWorker* m_90; // 0x90
    char _p94[0xc0 - 0x94];
    i32 m_c0;      // 0xc0
    char _pc4[0xd8 - 0xc4];
    i32 m_d8;      // 0xd8
    WwdName m_dc;  // 0xdc  CString name
    char _pe0[0x18c - 0xe0];
    i32 m_18c;     // 0x18c
    i32 m_190;     // 0x190
    i32 m_194;     // 0x194
    i32 m_198;     // 0x198
    char _p19c[0x1a0 - 0x19c];
    WwdSub m_1a0;  // 0x1a0
};

// @early-stop
// eh-dtor wall: /GX frame + member teardown + worker scalar-delete reproduce,
// but the manual multi-level vtable re-stamp sequence and the compiler's exact
// EH trylevel numbering across the two worker passes are not source-steerable.
RVA(0x0015b790, 0x1a6)
CWwdGameObjectA::~CWwdGameObjectA() {
    m_vptr = &g_wwdObjVtbl;
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_d8 = -1;
    m_c0 = (i32)0x80000000;
    m_5c = (i32)0x80000000;
    m_38 = -1;
    m_20 = (i32)0x80000000;
    m_18c = -1;
    m_190 = -1;
    m_198 = 0;
    m_194 = 0;
    // m_1a0 (WwdSub) auto-destroyed; then the second worker pass + name dtor:
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
    // m_dc (CString) auto-destroyed by member dtor.
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    m_vptr = &g_severusWorkerDtorVtbl;
}

// ---------------------------------------------------------------------------
// 0x15c070 - the 0x159250-final-vtable variant (m_18c is a BYTE flag here).
// ---------------------------------------------------------------------------
class CWwdGameObjectC {
public:
    ~CWwdGameObjectC(); // 0x15c070

    void* m_vptr;  // 0x00
    i32 m_04;      // 0x04
    i32 m_08;      // 0x08
    i32 m_0c;      // 0x0c
    char _p10[0x20 - 0x10];
    i32 m_20;      // 0x20
    char _p24[0x38 - 0x24];
    i32 m_38;      // 0x38
    char _p3c[0x5c - 0x3c];
    i32 m_5c;      // 0x5c
    char _p60[0x7c - 0x60];
    WwdWorker* m_7c; // 0x7c
    WwdWorker* m_80; // 0x80
    char _p84[0x88 - 0x84];
    WwdWorker* m_88; // 0x88
    char _p8c[0x90 - 0x8c];
    WwdWorker* m_90; // 0x90
    char _p94[0xc0 - 0x94];
    i32 m_c0;      // 0xc0
    char _pc4[0xd8 - 0xc4];
    i32 m_d8;      // 0xd8
    WwdName m_dc;  // 0xdc
    char _pe0[0x18c - 0xe0];
    u8 m_18c;      // 0x18c (byte flag)
};

// @early-stop
// eh-dtor wall (see 0x15b790): manual two-level vtable restamp + trylevel chain.
RVA(0x0015c070, 0x159)
CWwdGameObjectC::~CWwdGameObjectC() {
    m_vptr = &g_wwd159250FinalVtbl;
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_18c = 0;
    m_c0 = (i32)0x80000000;
    m_d8 = -1;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
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
    // m_dc auto-destroyed.
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    m_vptr = &g_severusWorkerDtorVtbl;
}

// ---------------------------------------------------------------------------
// 0x15bd10 - the CRemusNode-derived variant (extra +0x1dc CObList, leading
// init call 0x166810, trailing base CRemusNode dtor 0x429b).
// ---------------------------------------------------------------------------
class CWwdGameObjectB {
public:
    ~CWwdGameObjectB(); // 0x15bd10

    void InitDtor(); // 0x166810
    void DtorList(); // 0x1b5a2b  (CObList at +0x1dc)
    void SubB();     // 0x15b5d0
    void DtorBase(); // 0x429b    (base CRemusNode)

    void* m_vptr;  // 0x00
    i32 m_04;      // 0x04
    i32 m_08;      // 0x08
    i32 m_0c;      // 0x0c
    char _p10[0x20 - 0x10];
    i32 m_20;      // 0x20
    char _p24[0x38 - 0x24];
    i32 m_38;      // 0x38
    char _p3c[0x5c - 0x3c];
    i32 m_5c;      // 0x5c
    char _p60[0x7c - 0x60];
    WwdWorker* m_7c; // 0x7c
    WwdWorker* m_80; // 0x80
    char _p84[0x88 - 0x84];
    WwdWorker* m_88; // 0x88
    char _p8c[0x90 - 0x8c];
    WwdWorker* m_90; // 0x90
    char _p94[0xc0 - 0x94];
    i32 m_c0;      // 0xc0
    char _pc4[0xd8 - 0xc4];
    i32 m_d8;      // 0xd8
    WwdName m_dc;  // 0xdc
    char _pe0[0x18c - 0xe0];
    i32 m_18c;     // 0x18c
    i32 m_190;     // 0x190
    i32 m_194;     // 0x194
    i32 m_198;     // 0x198
    char _p19c[0x1a0 - 0x19c];
    WwdSub m_1a0;  // 0x1a0
    char _p1b0[0x1dc - 0x1b0];
    i32 m_1dc;     // 0x1dc  CObList head
    char _p1e0[0x1f8 - 0x1e0];
    i32 m_1f8;     // 0x1f8
};

// @early-stop
// eh-dtor wall (see 0x15b790): multi-level vtable restamp + base CRemusNode
// teardown; trylevel numbering across three vtable phases not source-steerable.
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
// 0x15b4f0 - the base ~CWwdGameObject itself (single vtable phase: stamp the
// CWwdGameObject vtable, free the four workers, destroy the +0xdc CString name,
// then re-stamp the severus-worker base vtable). No embedded command sub-object
// and no m_18c/m_190 block (this is the bare base, not the complete object).
// ---------------------------------------------------------------------------
class CWwdGameObjectE {
public:
    ~CWwdGameObjectE(); // 0x15b4f0

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
    WwdName m_dc; // 0xdc  CString name
};

// @early-stop
// eh-dtor wall (see 0x15b790): single-phase vtable re-stamp + the /GX trylevel
// numbering across the worker pass + name dtor; not source-steerable.
RVA(0x0015b4f0, 0xde)
CWwdGameObjectE::~CWwdGameObjectE() {
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
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
    m_vptr = &g_severusWorkerDtorVtbl;
}

// ---------------------------------------------------------------------------
// 0x15bad0 - the 0x159440-final-vtable derived variant: stamp its own most-derived
// vtable (0x5f0060), free the four workers, then fold in the base ~CWwdGameObject
// (stamp g_wwdGameObjectVtbl, free the workers again, destroy the +0xdc CString,
// re-stamp the severus base). Like CWwdGameObjectE but with the extra derived
// phase; no m_18c/m_190 block and no m_1a0 sub-object.
// ---------------------------------------------------------------------------
class CWwdGameObjectF {
public:
    ~CWwdGameObjectF(); // 0x15bad0

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
    WwdName m_dc; // 0xdc  CString name
};

// @early-stop
// eh-dtor wall (see 0x15b790): two-phase manual vtable re-stamp + double worker
// pass + the /GX trylevel chain across the phases; not source-steerable.
RVA(0x0015bad0, 0x153)
CWwdGameObjectF::~CWwdGameObjectF() {
    m_vptr = &g_wwd159440FinalVtbl;
    WORKER_FREE(m_7c);
    WORKER_FREE(m_80);
    WORKER_FREE(m_88);
    WORKER_FREE(m_90);
    m_c0 = (i32)0x80000000;
    m_d8 = -1;
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
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
    m_5c = (i32)0x80000000;
    m_20 = (i32)0x80000000;
    m_38 = -1;
    m_08 = 0;
    m_0c = 0;
    m_04 = -1;
    m_vptr = &g_severusWorkerDtorVtbl;
}
