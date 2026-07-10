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

// (0x8b90 CFinalize8b90::Finalize -> src/Gruntz/LogicTypeTable.cpp; 0xaf50
// ResetDat6295d8 -> src/Gruntz/GameObjectFactory.cpp; 0x1f870 CGuardedDispatch1f870::
// Run -> src/Gruntz/BootyStateActivate.cpp; 0x238d0 Init238d0 (+ its g_container62b5d0
// DATA pin) -> src/Gruntz/Dialogs.cpp - all RVA-homed to their linker-layout TUs.)

// ===========================================================================
// 0x023960 - register thunk: invoke a 1-int method (0x1b4867) on a global container
// object (VA 0x62b640) with arg 0xa. __cdecl free fn. (Its RVA-contiguous sibling
// 0x238d0 re-homed to Dialogs.cpp.)
// @orphan: both callers are unrecovered fns; a free init thunk with no owner class.
// ===========================================================================
struct CGlobalContainer {
    void Register(i32 n); // 0x1b4867 (reloc-masked)
};
SIZE_UNKNOWN(CGlobalContainer);
DATA(0x0022b640)
extern CGlobalContainer g_container62b640;

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
