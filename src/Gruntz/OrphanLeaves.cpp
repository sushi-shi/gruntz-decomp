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
#include <Globals.h>

// ---------------------------------------------------------------------------
// 0x183d0: return the address of a fixed global (a runtime-class / map pointer).

RVA(0x000183d0, 0x6)
void* GetGlobal5e8e98() {
    return &g_5e8e98;
}

// ---------------------------------------------------------------------------
// 0x183f0 (RVA-homed from src/Stub/ApiCallers.cpp) - a dialog list-item confirm:
// send LB_GETCURSEL (0x188) to item 0x516; if it returned a valid selection
// (!= LB_ERR), run OnPick(). __thiscall, no args.
// @orphan: only inbound edge is a fn-ptr-table slot (~g_5e8e98+0x1c, via thunk
// 0x3d5f) - no class vtable / new-site trace, so the owning dialog class is unrecovered.
struct DlgHostItem_183f0 {
    char m_pad0[0x1c];
    HWND m_hwnd; // +0x1c
};
struct DlgHost_183f0 {
    DlgHostItem_183f0* GetItem(i32 id); // thiscall, RVA 0x1be27d
    void OnPick();                      // thiscall, RVA 0x1bacc3
    void PickIfSelected();              // thiscall, RVA 0x183f0
};
RVA(0x000183f0, 0x2e)
void DlgHost_183f0::PickIfSelected() {
    HWND h = GetItem(0x516)->m_hwnd;
    if (SendMessageA(h, 0x188, 0, 0) != -1) {
        OnPick();
    }
}
SIZE_UNKNOWN(DlgHostItem_183f0);
SIZE_UNKNOWN(DlgHost_183f0);

// ---------------------------------------------------------------------------
// 0x018430 (re-homed from src/Stub/BoundaryMisc.cpp): end the wait cursor on the
// current MFC thread - tail-call CWinThread::EndWaitCursor (0x1beb10) on
// AfxGetModuleState()->m_thread (+0x04). __cdecl, no args.
// @orphan: called only from EH unwind funclets (no single owning class); a shared
// MFC wait-cursor helper. CWinThread's full definition lives in the GUI-heavy
// <afxwin.h> (not pulled by VC_EXTRALEAN <Mfc.h>), so it is completed with just the
// one method invoked (the raw +0x04 module-state offset is load-bearing).
class CWinThread {
public:
    void EndWaitCursor();
};
SIZE_UNKNOWN(CWinThread);
RVA(0x00018430, 0xd)
void EndWaitCursorOnThread() {
    CWinThread* thread = *(CWinThread**)((char*)AfxGetModuleState() + 4);
    thread->EndWaitCursor();
}

// (CPairXY::Set 0x75a10 is merged into src/Gruntz/TriggerMgrHitTest.cpp -
// called only by that TU's megafn FUN_6f2f0; interval verdict.)

// RunHelper2914 (0xb4330) re-homed to src/Gruntz/Ufo.cpp as CUFO::Tick (matcher-6).
