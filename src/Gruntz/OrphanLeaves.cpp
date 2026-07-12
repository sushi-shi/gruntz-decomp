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
// 0x183d0: return the address of a fixed global (a runtime-class / map pointer).

RVA(0x000183d0, 0x6)
void* GetGlobal5e8e98() {
    return &g_5e8e98;
}

// ---------------------------------------------------------------------------
// 0x183f0 (RVA-homed from src/Stub/ApiCallers.cpp) - a dialog list-item confirm:
// GetDlgItem(0x516)'s window sends LB_GETCURSEL (0x188); if it returned a valid
// selection (!= LB_ERR), run CDialog::OnOK (0x1bacc3). __thiscall, no args.
// @orphan: only inbound edge is a fn-ptr-table slot (~g_5e8e98+0x1c, via thunk
// 0x3d5f) - no class vtable / new-site trace, so the concrete CDialog subclass is
// unrecovered; modeled on the canonical CWnd (@identity-TODO for the leaf dialog).
struct DlgHost_183f0 : public CWnd {
    // OnPick == CDialog::OnOK (0x1bacc3), dispatched with this dialog's `this`. Binding
    // it needs the CDialog vtable slot renamed to OnOK (shared Dialogs.h/Dialogs.cpp/
    // MultiStartDlgRoster.cpp change); DEFERRED cross-lane, left reloc-masked here.
    void OnPick();         // thiscall, RVA 0x1bacc3
    void PickIfSelected(); // thiscall, RVA 0x183f0
};
RVA(0x000183f0, 0x2e)
void DlgHost_183f0::PickIfSelected() {
    HWND h = GetDlgItem(0x516)->m_hWnd;
    if (SendMessageA(h, 0x188, 0, 0) != -1) {
        OnPick();
    }
}
SIZE_UNKNOWN(DlgHost_183f0);

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
