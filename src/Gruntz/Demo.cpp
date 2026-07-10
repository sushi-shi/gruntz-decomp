// Demo.cpp - CDemo (the CPlay-derived demo/attract-playback state) own methods.
// CDemo's identity + layout live in the canonical <Gruntz/Demo.h>; its /GX dtor
// is in PlayDtor.cpp and its slot-21 re-post in GruntzMgrTransition.cpp.
#include <Gruntz/Demo.h>
#include <Gruntz/GruntzMgr.h> // CGruntzMgr / CGameMgr::m_gameWnd -> CGameWnd::m_hwnd (Render's exit post)
#include <Gruntz/AttractActor.h> // the shared per-frame g_actorList view
#include <rva.h>

// PostMessageA reached through the IAT slot (the engine's ff15 indirect); the exit
// WM_COMMANDs (0x8023 / 0x8027). Symbol bound by Attract.cpp's DATA; extern here.
typedef i32(WINAPI* PostMessageFn)(void* hwnd, u32 msg, u32 wparam, i32 lparam);
extern PostMessageFn g_pPostMessageA;

// The per-frame attract actor list (DAT_00645574; bound in Globals.cpp) and the
// per-frame time delta (DAT_00645584; bound in Attract.cpp). Extern here (reloc-masked).
extern AttractActorList* g_actorList;
extern "C" u32 g_645584;

// The first arg is the game-manager/entry context whose CString at +0xc8 (a
// pending-name latch) is cleared before delegating to the CPlay base entry.
SIZE_UNKNOWN(CDemoEnterCtx);
struct CDemoEnterCtx {
    char p0[0xc8];
    CString m_c8; // +0xc8
};

// 0x3bfa0 - CDemo::Vfunc1 (slot 1): clear the entry context's pending name, run
// the CPlay base slot-1 (CPlay::Vfunc1 == the mode/object initializer at 0xc7ec0);
// on failure return 0, else latch m_520 and return 1.
RVA(0x0003bfa0, 0x42)
i32 CDemo::Vfunc1(i32 ctx, i32 a1, i32 a2) {
    ((CDemoEnterCtx*)ctx)->m_c8.Empty();
    if (CPlay::Vfunc1(ctx, a1, a2) == 0) {
        return 0;
    }
    m_520 = 0x124f80;
    return 1;
}

// 0x3c220 - CDemo::Render (slot 5, +0x14): the demo/attract per-frame render poll.
// Runs the CPlay base render, scans the g_actorList for the 0x100 exit-request flag
// (posting WM_COMMAND 0x8023 to the top-level HWND on the first match), counts the
// idle timer m_520 down by the frame delta, and posts WM_COMMAND 0x8027 when it
// expires. Returns 1. (Re-homed from Attract.cpp's placeholder `CAttractIdlePoll`
// view: the 0x3c220 code is CDemo's vtable slot 5 - proven by the .rdata data-ref at
// ??_7CDemo@@6B@+0x14 and the m_520 idle-timer match.)
// @early-stop
// 99.55%: every opcode/offset/branch is byte-identical. The residual is (1) the
// PostMessageA IAT-absolute scoring artifact (target bakes the bare 0x6c44c8, no
// symbol) and (2) a register-coloring coin-flip (pm <-> ebx/edi vs the 0x100 mask) -
// the documented regalloc back-edge wall (docs/patterns/zero-register-pinning.md).
// Not source-steerable; deferred to the final sweep.
RVA(0x0003c220, 0xa4)
i32 CDemo::Render() {
    CPlay::Render();
    PostMessageFn pm = g_pPostMessageA;
    AttractActorList* list = g_actorList;
    i32 n = list->m_count;
    for (i32 i = 0; i < n; i++) {
        if (list->m_data[i]->m_2ac & 0x100) {
            pm(m_4->m_gameWnd->m_hwnd, 0x111, 0x8023, 0);
            break;
        }
    }
    if (g_645584 >= m_520) {
        m_520 = 0;
    } else {
        m_520 -= g_645584;
    }
    if (m_520 == 0) {
        pm(m_4->m_gameWnd->m_hwnd, 0x111, 0x8027, 0);
    }
    return 1;
}
