// BoundaryMisc.cpp - the residual UN-ATTRIBUTABLE small non-EH leaves of the
// engine_boundary backlog (C:\Proj\Gruntz). The attributable bodies here were
// re-homed to their real class TUs (matcher-2):
//   0x24ac0 IsActive2         -> src/Gruntz/GruntzCmdMgr.cpp (twin already used there)
//   0x37870 DialogInit37870   -> src/Gruntz/VideoConfig.cpp  (already declared/called)
//   0x212a0 CButeStore::Reset -> src/Bute/ButeStoreClear.cpp (out-of-line twin placeholder)
//   0x29af0 TileSwitch29af0   -> src/Gruntz/GruntMoveStep.cpp (CGruntMover::Step caller)
//   0x87b0  ~CUserBase / 0xb940 CUserBaseSubB940 -> src/Gruntz/WorldSoundSet.cpp
//           (CUserBase-family; 0xb940 is RVA-inside that TU's band)
// The six that remain have a genuinely UNRECOVERED owner (only-a-stub / unrecovered /
// no caller) - flagged @orphan for the identity-recovery sweep. Only OFFSETS + code
// bytes are load-bearing. Unmodeled engine callees are declared NO-body so their
// rel32/DIR32 operands reloc-mask.
#include <Ints.h>
#include <rva.h>
#include <Mfc.h>     // AfxGetModuleState / CWinThread (0x18430); afx-first umbrella
#include <Globals.h> // g_dat6295d8 (0xaf50)

// ===========================================================================
// 0x008b90 - a finalize/teardown that fires up to two registered __thiscall
// callbacks (m_4, m_8) passing `this`, the m_8 one guarded by m_14->m_1c == m_28,
// nulls each fired slot, and resets m_28 to 0x3e9 (1001). __thiscall, one unused
// stack arg (ret 4). Self-contained (the callbacks are indirect).
// @orphan: only caller is Gap_05ecd0 (an engine_label_stubs placeholder); `this`
// class genuinely unrecovered.
// ===========================================================================
struct CFinalizeSub8b90 {
    char m_pad0[0x1c];
    i32 m_1c; // +0x1c
};
SIZE_UNKNOWN(CFinalizeSub8b90);
struct CFinalize8b90 {
    typedef void (CFinalize8b90::*PMF)(); // single-inheritance, non-virtual -> 4 bytes
    void* m_0;                            // +0x00
    PMF m_4;                              // +0x04  callback (called with `this`)
    PMF m_8;                              // +0x08  callback (guarded)
    char m_pad0c[0x14 - 0x0c];
    CFinalizeSub8b90* m_14; // +0x14
    char m_pad18[0x28 - 0x18];
    i32 m_28; // +0x28
    void Finalize(i32 arg);
};
SIZE_UNKNOWN(CFinalize8b90);
RVA(0x00008b90, 0x40)
void CFinalize8b90::Finalize(i32 arg) {
    if (m_4 == 0) {
        return;
    }
    if (m_8 != 0 && m_14->m_1c == m_28) {
        (this->*m_8)();
        m_8 = 0;
    }
    (this->*m_4)();
    m_4 = 0;
    m_28 = 0x3e9;
}

// ===========================================================================
// 0x00af50 - reset a global DWORD to 0 (the global at VA 0x6295d8 / RVA 0x2295d8).
// __cdecl free function.
// @orphan: only caller is an unrecovered fn (~0xaa92); free reset with no owner.
// ===========================================================================
RVA(0x0000af50, 0xb)
void ResetDat6295d8() {
    g_dat6295d8 = 0;
}

// ===========================================================================
// 0x01f870 - guarded virtual dispatch: if the 4th virtual (vtbl slot +0xc) reports
// nonzero, run the non-virtual handler (0xfac70) and return its success as a bool
// (the retail neg/sbb/neg idiom); else return 0. __thiscall, no args.
// @orphan: no .text caller; `this` class genuinely unrecovered.
// ===========================================================================
struct CGuardedDispatch1f870 {
    virtual i32 v0();
    virtual i32 v1();
    virtual i32 v2();
    virtual i32 IsActive(); // slot +0x0c
    i32 Handle();           // 0xfac70 (non-virtual; reloc-masked)
    i32 Run();
};
SIZE_UNKNOWN(CGuardedDispatch1f870);
RVA(0x0001f870, 0x1d)
i32 CGuardedDispatch1f870::Run() {
    if (!IsActive()) {
        return 0;
    }
    return Handle() != 0;
}

// ===========================================================================
// 0x0238d0 / 0x023960 - register thunks: invoke a 1-int method (0x1b4867) on a
// global container object (VA 0x62b5d0 / 0x62b640) with arg 0xa. __cdecl free fns.
// @orphan: both callers are unrecovered fns; free init thunks with no owner class.
// ===========================================================================
struct CGlobalContainer {
    void Register(i32 n); // 0x1b4867 (reloc-masked)
};
SIZE_UNKNOWN(CGlobalContainer);
DATA(0x0022b5d0)
extern CGlobalContainer g_container62b5d0;
DATA(0x0022b640)
extern CGlobalContainer g_container62b640;

RVA(0x000238d0, 0xd)
void Init238d0() {
    g_container62b5d0.Register(0xa);
}

RVA(0x00023960, 0xd)
void Init23960() {
    g_container62b640.Register(0xa);
}

// ===========================================================================
// 0x018430 - tail-call wrapper: end the wait cursor on the current MFC thread
// (((CWinThread*)AfxGetModuleState()->m_thread)->EndWaitCursor()). __cdecl, no args.
// @orphan: called from many EH unwind funclets (no single owning class); a shared
// MFC wait-cursor helper.
// ===========================================================================
// Real MFC AfxGetModuleState() (@0x1d3631) from <Mfc.h>; its +0x04 word is the current
// CWinThread*. CWinThread's full definition lives in the GUI-heavy <afxwin.h> (NOT pulled
// in by <Mfc.h>, which is VC_EXTRALEAN), so complete MFC's forward decl with just the one
// method we call - CWinThread::EndWaitCursor @0x1beb10 (raw +0x04 offset load-bearing).
class CWinThread {
public:
    void EndWaitCursor();
};
SIZE_UNKNOWN(CWinThread); // real size is MFC's (afxwin.h not pulled in)
RVA(0x00018430, 0xd)
void EndWaitCursor18430() {
    CWinThread* thread = *(CWinThread**)((char*)AfxGetModuleState() + 4);
    thread->EndWaitCursor();
}
