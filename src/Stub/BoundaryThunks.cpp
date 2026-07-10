// BoundaryThunks.cpp - small vptr-stamp / tail-forward leaf thunks recovered from
// the engine_boundary backlog (C:\Proj\Gruntz). RTTI cannot attribute these
// COMDAT-folded one-liners, so the owning class names are placeholders; only the
// OFFSETS + code bytes are load-bearing. Unmodeled engine callees/globals are
// declared NO-body so their rel32/DIR32 operands reloc-mask.
//
// RE-HOME STATUS (matcher-1 verify pass): NONE are ILT/incremental-link artifacts (all
// real bodies above the 0x1000-0x7c20 ILT band). HOMED: g_tokenMgr reset (0x99b80 ->
// MgrTokenQuery.cpp). REMAINING (no clean home):
//   * ButeMgrFlush82b20 (g_buteMgr.Term): g_buteMgr is DATA-pinned in ~12 TUs (a shared
//     singleton) - no single defining owner for the atexit thunk. DEFER.
//   * ProfSinkTick82ba0..82f20 (8 CString atexit dtors over the 0x645514-0x645530 band):
//     the whole global band is DATA-pinned HERE (no external owner TU).
//   * base-vptr-restore dtors 0x8c470 (CStateSub8c470) / 0x137330+0x13aaf0+0x13ca30
//     (CAbstract*, a documented REQUIRED-SPLIT vs PureSoundElem): placeholder identities.
//   * Forward853d0 (__stdcall RezFree wrapper): a standalone forwarder, no owning class.
#include <Bute/ButeMgr.h>        // canonical CButeMgr (one shape)
#include <Gruntz/GameModeBase.h> // CGameModeBase::BaseCleanup (0xfa150, the state teardown)
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

// (0x099b80 TokenMgrReset99b80 re-homed to src/Gruntz/MgrTokenQuery.cpp - the
// token-manager singleton g_tokenMgr is single-owner DATA-pinned there, next to
// QueryToken which uses the same CAreaMgr::Reset call.)

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
    // 0xfa150 == CGameModeBase::BaseCleanup (this aliases CState at offset 0).
    ((CGameModeBase*)this)->BaseCleanup();
}

// (0x137330 / 0x13aaf0 / 0x13ca30 - abstract-base vptr-restore dtor thunks -
// re-homed to their RVA-neighborhood TUs: 0x137330 -> src/Dsndmgr/DirectSoundMgr.cpp
// (PureSoundElem base-object dtor), 0x13aaf0 -> src/Bute/SymParser.cpp,
// 0x13ca30 -> src/Rez/RezMgr.cpp. Each keeps its RELOC_VTBL pure-call-vtable model.)

// (0x0853d0 Forward853d0 re-homed to src/Rez/RezSync.cpp as RezFreeStdcall - a
// standalone __stdcall RezFree wrapper, homed next to that TU's rez-managed bootstrap.)

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
