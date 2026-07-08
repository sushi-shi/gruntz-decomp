// BoundaryThunks.cpp - small vptr-stamp / tail-forward leaf thunks recovered from
// the engine_boundary backlog (C:\Proj\Gruntz). RTTI cannot attribute these
// COMDAT-folded one-liners, so the owning class names are placeholders; only the
// OFFSETS + code bytes are load-bearing. Unmodeled engine callees/globals are
// declared NO-body so their rel32/DIR32 operands reloc-mask.
#include <Bute/ButeMgr.h>    // canonical CButeMgr (one shape)
#include <Gruntz/TokenMgr.h> // canonical CTokenMgr (g_tokenMgr; Reset)
#include <Ints.h>
#include <rva.h>
class Cfa150 {
public:
    void Cleanup();
};
class CAreaMgr {
public:
    void Reset();
    i32 Dispatch(i32 a);
}; // 0x9a0b0/0x99d40

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
    // Tick1b9b93 @0x1b9b93 IS CString::~CString; cast at each call.
};
SIZE_UNKNOWN(CProfSink);
extern "C" CProfSink g_profSink;
RVA(0x00082ba0, 0xa)
void ProfSinkTick82ba0() {
    ((CString*)&g_profSink)->~CString();
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
    ((CString*)&g_obj645528)->~CString();
}
RVA(0x00082ca0, 0xa)
void ProfSinkTick82ca0() {
    ((CString*)&g_obj64552c)->~CString();
}
RVA(0x00082d20, 0xa)
void ProfSinkTick82d20() {
    ((CString*)&g_obj645530)->~CString();
}
RVA(0x00082da0, 0xa)
void ProfSinkTick82da0() {
    ((CString*)&g_obj645514)->~CString();
}
RVA(0x00082e20, 0xa)
void ProfSinkTick82e20() {
    ((CString*)&g_obj645518)->~CString();
}
RVA(0x00082ea0, 0xa)
void ProfSinkTick82ea0() {
    ((CString*)&g_obj64551c)->~CString();
}
RVA(0x00082f20, 0xa)
void ProfSinkTick82f20() {
    ((CString*)&g_obj645520)->~CString();
}

// ===========================================================================
// 0x099b80 - tail-forward a no-arg call to the token-manager singleton
// (?g_tokenMgr@@3UCTokenMgr@@A @ VA 0x6459b0). __cdecl, no args.
// ===========================================================================
extern CTokenMgr g_tokenMgr;
RVA(0x00099b80, 0xa)
void TokenMgrReset99b80() {
    ((CAreaMgr*)&g_tokenMgr)
        ->Reset(); // reaches Reset (0x49a0b0) via the ILT thunk 0x3bac (reloc-masked)
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
RELOC_VTBL(CStateSub8c470, 0x001ea21c); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x0008c470, 0xb)
CStateSub8c470::~CStateSub8c470() {
    ((Cfa150*)this)->Cleanup();
}

// ===========================================================================
// 0x137330 / 0x13aaf0 / 0x13ca30 - abstract-base vptr restores: cl's implicit
// vptr-restore stamps a pure-call vtable into [this] and returns (7-byte
// `mov [ecx],offset ??_7 + ret`). 0x13aaf0 and 0x13ca30 share the 0x5ef760 pure-call
// vtable; 0x137330 stamps ??_7PureSoundElem (0x5ef6c8, include/Dsndmgr/SoundVoiceList.h).
// Real polymorphic: an empty virtual dtor emits exactly the stamp+ret. __thiscall.
//
// de-view CONFIRMED-SAME, REQUIRED-SPLIT (not foldable into PureSoundElem):
// CAbstract137330 IS PureSoundElem's base-object destructor (retail ??1PureSoundElem,
// a NON-virtual dtor). Retail emits this as a standalone COMDAT only because the EH
// unwind funclet @0x1e0950 (its sole caller) references ~PureSoundElem out-of-line,
// while every `delete (PureSoundElem*)e` site (0x136f60/0x136e20/0x136ed0) INLINES the
// teardown (mov[e],0x5ef6c8; RezFree - see PurgeVoiceList @0x136ea8). A folded out-of-
// line PureSoundElem dtor would convert those cross-TU inline sites to calls (regress);
// an inline dtor would drop THIS standalone (we don't emit the EH funclet that forces
// it). So the two models must coexist - kept a distinct emitter here.
// ===========================================================================
struct CAbstract137330 {
    virtual ~CAbstract137330();
};
SIZE_UNKNOWN(CAbstract137330);
RELOC_VTBL(CAbstract137330, 0x001ef6c8); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x00137330, 0x7)
CAbstract137330::~CAbstract137330() {}

struct CAbstract13aaf0 {
    virtual ~CAbstract13aaf0();
};
SIZE_UNKNOWN(CAbstract13aaf0);
RELOC_VTBL(CAbstract13aaf0, 0x001ef760); // vtable reloc-masks a bound datum (dtor-stamp verified)
RVA(0x0013aaf0, 0x7)
CAbstract13aaf0::~CAbstract13aaf0() {}

struct CAbstract13ca30 {
    virtual ~CAbstract13ca30();
};
SIZE_UNKNOWN(CAbstract13ca30);
RELOC_VTBL(CAbstract13ca30, 0x001ef760); // vtable reloc-masks a bound datum (dtor-stamp verified)
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

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
