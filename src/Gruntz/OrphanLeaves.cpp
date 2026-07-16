// OrphanLeaves.cpp - a handful of tiny orphan-COMDAT leaf functions (getters,
// dialog confirms, an MFC wait-cursor helper) that survive in the low-RVA COMDAT
// pool with no recoverable owning class. Each is modeled from its disassembly with
// a PLACEHOLDER class/name; only the OFFSETS + emitted code bytes are load-bearing
// (campaign doctrine). All engine callees are external/no-body so their call
// rel32 / DIR32 reloc-mask.
//
// The unit's ex 0x3ac30-interval members were re-homed by birth position (dossier
// #16, waveM-judgment): FreeGlobal62c25c 0x3ac30 -> CustomWorldDialog.cpp;
// COwnerWithSubs::DtorSubC/DtorSub8 0x3cbc0/0x3cbf0 -> Demo.cpp;
// Register6446d8Range 0x3e120 -> GruntStartingPoint.cpp. The old "orphanleaves"
// membership was an aggregation artifact spanning two objs.
#include <Ints.h>
#include <rva.h>
#include <Mfc.h>
#include <Gruntz/Dialogs.h> // canonical CWnd (GetDlgItem) + CCmdTarget (EndWaitCursor)
#include <Globals.h>

// ---------------------------------------------------------------------------
// 0x183d0: CBattlezDlgCustom::GetMessageMap (slot 12) - return the dialog's MFC
// message map @0x5e8e98. (Ex the bare `GetGlobal5e8e98` free fn; the map global's
// TYPE is still the placeholder void* cell in Globals.h.)
RVA(0x000183d0, 0x6)
const AFX_MSGMAP* CBattlezDlgCustom::GetMessageMap() const {
    return (const AFX_MSGMAP*)&g_battlezCustomMsgMap;
}

// ---------------------------------------------------------------------------
// 0x183f0 (RVA-homed from src/Stub/ApiCallers.cpp) - CBattlezDlgCustom's
// list-item confirm (the message-map handler at messageMap+0x1c, via thunk
// 0x3d5f): GetDlgItem(0x516)'s window sends LB_GETCURSEL (0x188); on a valid
// selection run CDialog::OnOK. IDENTITY RESOLVED (2026-07-16, ex the
// `DlgHost_183f0` @identity-TODO shell): control 0x516 IS CBattlezDlgCustom's
// custom-level listbox (its DoDataExchange @0x180e0 fills the same id), the
// fn-table @0x5e8e98 IS that dialog's message map (GetMessageMap above returns
// it), and the whole 0x183d0..0x18430 run is that dialog's MFC boilerplate
// cluster, RVA-adjacent to its DDX.
RVA(0x000183f0, 0x2e)
void CBattlezDlgCustom::PickIfSelected() {
    HWND h = GetDlgItem(0x516)->m_hWnd;
    if (::SendMessageA(h, 0x188, 0, 0) != -1) {
        CDialog::OnOK(); // 0x1bacc3 ?OnOK@CDialog@@MAEXXZ (qualified base call, reloc-masked)
    }
}

// ---------------------------------------------------------------------------
// 0x018430 (re-homed from src/Stub/BoundaryMisc.cpp): end the wait cursor on the
// current MFC thread - tail-call CCmdTarget::EndWaitCursor (0x1beb10) on
// AfxGetModuleState()->m_pCurrentWinThread (+0x04). __cdecl, no args.
// @orphan: called only from EH unwind funclets (no single owning class); a shared
// MFC wait-cursor helper. The current thread is a CWinThread (is-a CCmdTarget); the
// call binds to the CCmdTarget-owned EndWaitCursor. Raw +0x04 module-state offset is
// load-bearing.
RVA(0x00018430, 0xd)
void EndWaitCursorOnThread() {
    CCmdTarget* thread = *(CCmdTarget**)((char*)AfxGetModuleState() + 4);
    thread->EndWaitCursor();
}

// (CPairXY::Set 0x75a10 is merged into src/Gruntz/TriggerMgrHitTest.cpp -
// called only by that TU's megafn FUN_6f2f0; interval verdict.)

// RunHelper2914 (0xb4330) re-homed to src/Gruntz/Ufo.cpp as CUFO::Tick (matcher-6).
