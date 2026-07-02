#include <rva.h>
// TraceDiscoveredLeaves.cpp - trace-discovered leaf methods/ctors/vtable-restore
// thunks whose owning class the dynamic this/ecx trace could NOT attribute to a
// named RTTI class. Re-homed out of src/Stub/Discovered.cpp (the auto-generated
// engine_discovered unit) into a real module TU; the placeholder ClassUnknown_N
// names are kept (only offsets + code bytes are load-bearing per campaign
// doctrine). Minimal per-class decls inlined here (the shared auto-gen header
// include/Stub/discovered.h was deleted - it was included by nobody else and its
// other ~50 decls were re-homed into real class TUs long ago). flags = base.

// ---- ClassUnknown_13 ----
// Two __cdecl forwarder twins (trace mis-attributed as __thiscall methods): read a
// handler off p->m_4 (at +0x18 / +0x14), and if non-null forward all 9 stack args
// to a 10-arg __cdecl callee (0x115930 via the 0x1262 ILT), inserting the handler's
// m_2c as the 4th argument. The argument is `p` (a stack arg, NOT this).
// Owning class is NOT resolvable: these are free __cdecl forwarders (the callee
// 0x115930 is winapi_115930_CopyRect_OffsetRect, a Win32-rect helper); ClassUnknown_13
// is a stale-ecx trace artifact.
SIZE_UNKNOWN(U13Handler);
struct U13Handler {
    char m_pad00[0x2c];
    i32 m_2c; // +0x2c
};
SIZE_UNKNOWN(U13Inner);
struct U13Inner {
    char m_pad00[0x14];
    U13Handler* m_14; // +0x14
    U13Handler* m_18; // +0x18
};
SIZE_UNKNOWN(U13Obj);
struct U13Obj {
    char m_pad00[4];
    U13Inner* m_4; // +0x04
};
extern "C" void U13Callee(
    U13Obj* p,
    i32 a2,
    i32 a3,
    i32 m,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
); // 0x115930
// @early-stop
// tail-merge / block-layout wall (~94.3%, topic:wall topic:scheduling): the handler
// load + the 10-arg `[esp+0x24]`-reload forwarding push chain + the call/`add esp`
// are byte-IDENTICAL; the sole residual is the null guard, where retail emits
// `jne body; ret` (a separate early ret, no tail-merge) but cl tail-merges the two
// rets to `je <shared end ret>`. An MSVC5 block-ordering coin-flip; not
// source-steerable (early-return / explicit-return-in-body both still tail-merge).
RVA(0x001154b0, 0x45)
void U13Forward1154b0(U13Obj* p, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9) {
    U13Handler* h = p->m_4->m_18;
    if (h == 0) {
        return;
    }
    U13Callee(p, a2, a3, h->m_2c, a4, a5, a6, a7, a8, a9);
}
// @early-stop
// same tail-merge wall as U13Forward1154b0 (twin; inner offset +0x14 vs +0x18).
RVA(0x00115520, 0x45)
void U13Forward115520(U13Obj* p, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7, i32 a8, i32 a9) {
    U13Handler* h = p->m_4->m_14;
    if (h == 0) {
        return;
    }
    U13Callee(p, a2, a3, h->m_2c, a4, a5, a6, a7, a8, a9);
}

// ---- ClassUnknown_15 ----
// A bare ctor: stamp the class vtable (0x5ed36c) and return this. Reconstructed.
SIZE_UNKNOWN(ClassUnknown_15);
class ClassUnknown_15 {
public:
    ClassUnknown_15();
    void* m_0; // +0x00  vtable (manual stamp)
};
DATA(0x005ed36c)
extern void* g_vtbl5ed36c;
RVA(0x001d38a1, 0x9)
ClassUnknown_15::ClassUnknown_15() {
    *(void**)this = &g_vtbl5ed36c;
}

// ---- ClassUnknown_16 ----
SIZE_UNKNOWN(ClassUnknown_16);
class ClassUnknown_16 {
public:
    void ClassUnknown_16_1d496b();
};
RVA(0x001d496b, 0x1e)
void ClassUnknown_16::ClassUnknown_16_1d496b() {}

// ---- ClassUnknown_33 ----
SIZE_UNKNOWN(ClassUnknown_33);
class ClassUnknown_33 {
public:
    void ClassUnknown_33_11f6b9();
};
RVA(0x0011f6b9, 0x17)
void ClassUnknown_33::ClassUnknown_33_11f6b9() {}

// ---- ClassUnknown_45 ----
// A ctor: zero m_4, stamp the class vtable (0x5ea2a4), return this. Reconstructed.
SIZE_UNKNOWN(ClassUnknown_45);
class ClassUnknown_45 {
public:
    ClassUnknown_45();
    void* m_0; // +0x00  vtable (manual stamp)
    i32 m_4;   // +0x04
};
DATA(0x005ea2a4)
extern void* g_vtbl5ea2a4;
RVA(0x0008c3b0, 0x10)
ClassUnknown_45::ClassUnknown_45() {
    m_4 = 0;
    *(void**)this = &g_vtbl5ea2a4;
}

// ---- ClassUnknown_50 ----
// scalar-deleting-destructor vtable at 0x5f04d8 stamped by ClassUnknown_50.
extern void* g_vtbl5f04d8;
SIZE_UNKNOWN(ClassUnknown_50);
class ClassUnknown_50 {
public:
    void ClassUnknown_50_16dfc0();
};
RVA(0x0016dfc0, 0x7)
void ClassUnknown_50::ClassUnknown_50_16dfc0() {
    *(void**)this = &g_vtbl5f04d8;
}

// ---- ClassUnknown_51..54 ----
// Base vtable restored by ~CSeverusWorker-derived dtors (0x5e8cb4); these 7-byte
// `mov [ecx],&vtbl; ret` thunks are the still-manual vtable stamps for those
// classes (kept as the manual stamp, not yet modelled as real virtuals).
extern void* g_severusWorkerDtorVtbl;
SIZE_UNKNOWN(ClassUnknown_51);
class ClassUnknown_51 {
public:
    void ClassUnknown_51_11eaf5();
};
RVA(0x0011eaf5, 0x7)
void ClassUnknown_51::ClassUnknown_51_11eaf5() {
    *(void**)this = &g_severusWorkerDtorVtbl;
}

SIZE_UNKNOWN(ClassUnknown_52);
class ClassUnknown_52 {
public:
    void ClassUnknown_52_11eaa8();
};
RVA(0x0011eaa8, 0x7)
void ClassUnknown_52::ClassUnknown_52_11eaa8() {
    *(void**)this = &g_severusWorkerDtorVtbl;
}

SIZE_UNKNOWN(ClassUnknown_53);
class ClassUnknown_53 {
public:
    void ClassUnknown_53_11cf30();
};
RVA(0x0011cf30, 0x7)
void ClassUnknown_53::ClassUnknown_53_11cf30() {
    *(void**)this = &g_severusWorkerDtorVtbl;
}

SIZE_UNKNOWN(ClassUnknown_54);
class ClassUnknown_54 {
public:
    void ClassUnknown_54_11cee3();
};
RVA(0x0011cee3, 0x7)
void ClassUnknown_54::ClassUnknown_54_11cee3() {
    *(void**)this = &g_severusWorkerDtorVtbl;
}
