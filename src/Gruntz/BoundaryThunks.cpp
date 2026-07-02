// BoundaryThunks.cpp - small vptr-stamp / tail-forward leaf thunks recovered from
// the engine_boundary backlog (C:\Proj\Gruntz). RTTI cannot attribute these
// COMDAT-folded one-liners, so the owning class names are placeholders; only the
// OFFSETS + code bytes are load-bearing. Unmodeled engine callees/globals are
// declared NO-body so their rel32/DIR32 operands reloc-mask.
#include <Bute/ButeMgr.h> // canonical CButeMgr (one shape)
#include <Ints.h>
#include <rva.h>

// ===========================================================================
// 0x082b20 - tail-forward a no-arg call to the bute-manager singleton
// (?g_buteMgr@@3VCButeMgr@@A @ VA 0x6453d8). __cdecl, no args. CButeMgr::Term
// (0x170210) is on the canonical CButeMgr (include/Bute/ButeMgr.h).
// ===========================================================================
extern CButeMgr g_buteMgr;
RVA(0x00082b20, 0xa)
void ButeMgrFlush82b20() {
    g_buteMgr.Term();
}

// ===========================================================================
// 0x082ba0 - tail-forward a no-arg call to the profiling-sink singleton
// (_g_profSink @ VA 0x645524). __cdecl, no args.
// ===========================================================================
struct CProfSink {
    void Tick1b9b93(); // 0x1b9b93 (reloc-masked)
};
SIZE_UNKNOWN(CProfSink);
extern "C" CProfSink g_profSink;
RVA(0x00082ba0, 0xa)
void ProfSinkTick82ba0() {
    g_profSink.Tick1b9b93();
}

// ===========================================================================
// 0x082c20 .. 0x082f20 - the sibling tail-forwards over the adjacent global-object
// band (VA 0x645514 .. 0x645530), each handing its object to the same 0x1b9b93
// helper. __cdecl, no args. (Defined in retail-RVA order; the global addresses
// run out of order across that band.)
// ===========================================================================
DATA(0x00245514)
extern "C" CProfSink g_obj645514;
DATA(0x00245518)
extern "C" CProfSink g_obj645518;
DATA(0x0024551c)
extern "C" CProfSink g_obj64551c;
DATA(0x00245520)
extern "C" CProfSink g_obj645520;
DATA(0x00245528)
extern "C" CProfSink g_obj645528;
DATA(0x0024552c)
extern "C" CProfSink g_obj64552c;
DATA(0x00245530)
extern "C" CProfSink g_obj645530;

RVA(0x00082c20, 0xa)
void ProfSinkTick82c20() {
    g_obj645528.Tick1b9b93();
}
RVA(0x00082ca0, 0xa)
void ProfSinkTick82ca0() {
    g_obj64552c.Tick1b9b93();
}
RVA(0x00082d20, 0xa)
void ProfSinkTick82d20() {
    g_obj645530.Tick1b9b93();
}
RVA(0x00082da0, 0xa)
void ProfSinkTick82da0() {
    g_obj645514.Tick1b9b93();
}
RVA(0x00082e20, 0xa)
void ProfSinkTick82e20() {
    g_obj645518.Tick1b9b93();
}
RVA(0x00082ea0, 0xa)
void ProfSinkTick82ea0() {
    g_obj64551c.Tick1b9b93();
}
RVA(0x00082f20, 0xa)
void ProfSinkTick82f20() {
    g_obj645520.Tick1b9b93();
}

// ===========================================================================
// 0x099b80 - tail-forward a no-arg call to the token-manager singleton
// (?g_tokenMgr@@3UCTokenMgr@@A @ VA 0x6459b0). __cdecl, no args.
// ===========================================================================
struct CTokenMgr {
    void Reset3bac(); // 0x3bac (thunk; reloc-masked)
};
SIZE_UNKNOWN(CTokenMgr);
extern CTokenMgr g_tokenMgr;
RVA(0x00099b80, 0xa)
void TokenMgrReset99b80() {
    g_tokenMgr.Reset3bac();
}

// ===========================================================================
// 0x08c470 - a CState-base vptr restore: cl's implicit vptr-restore stamps the CState
// vtable (0x5ea21c) then tail-jumps the base init/teardown (0x3f53). Placeholder
// polymorphic class (the real CState dtor is modeled in GameMode.cpp; this is a
// distinct restore, so its ??_7 reloc-masks by shape). __thiscall.
// ===========================================================================
struct CStateSub8c470 {
    void BaseInit3f53(); // 0x3f53 (reloc-masked)
    virtual ~CStateSub8c470();
};
SIZE_UNKNOWN(CStateSub8c470);
RVA(0x0008c470, 0xb)
CStateSub8c470::~CStateSub8c470() {
    BaseInit3f53();
}

// ===========================================================================
// 0x137330 / 0x13aaf0 / 0x13ca30 - abstract-base vptr restores: cl's implicit
// vptr-restore stamps a pure-call vtable into [this] and returns (7-byte
// `mov [ecx],offset ??_7 + ret`). 0x13aaf0 and 0x13ca30 share the 0x5ef760 pure-call
// vtable. Real polymorphic: an empty virtual dtor emits exactly the stamp+ret (the
// abstract element base's concrete owner is unmodeled, so the emitted ??_7 reloc-masks
// against the retail pure-call vtable by shape). __thiscall, no args.
// ===========================================================================
struct CAbstract137330 {
    virtual ~CAbstract137330();
};
SIZE_UNKNOWN(CAbstract137330);
RVA(0x00137330, 0x7)
CAbstract137330::~CAbstract137330() {}

struct CAbstract13aaf0 {
    virtual ~CAbstract13aaf0();
};
SIZE_UNKNOWN(CAbstract13aaf0);
RVA(0x0013aaf0, 0x7)
CAbstract13aaf0::~CAbstract13aaf0() {}

struct CAbstract13ca30 {
    virtual ~CAbstract13ca30();
};
SIZE_UNKNOWN(CAbstract13ca30);
RVA(0x0013ca30, 0x7)
CAbstract13ca30::~CAbstract13ca30() {}

// ===========================================================================
// 0x0853d0 - __stdcall forwarder: hand the single arg to the __cdecl rez-free
// helper (0x1b9b82). Returns void; one arg (ret 4).
// ===========================================================================
extern "C" void RezFree(void* p); // 0x1b9b82 (__cdecl; reloc-masked)
RVA(0x000853d0, 0x10)
void __stdcall Forward853d0(void* a) {
    RezFree(a);
}
