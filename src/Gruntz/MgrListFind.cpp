#include <Gruntz/TriggerMgrViews.h> // CTriggerMgr (MFC-first)
#include <Gruntz/GruntPuddle.h>   // CGruntPuddle (the baseList element; m_tileX/m_tileY/m_pending)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// 0xf0db0 (__cdecl) - true if a live (m_occupied==0) candidate matches (a1,a2).
// ---------------------------------------------------------------------------
// @early-stop
// The "regalloc-cascade wall" note here was WRONG: it was exit-block layout, and the
// register spend followed from it (measured 2026-07-27, 73.22 -> 89.47). The ret-count
// screen read base 3 / retail 2 - plain `while (pos) {...} return 0;` makes cl rotate
// the walk and emit a SECOND `return 0` epilogue as the loop's fall-through, while
// retail 0xf0df3 has one miss block that both the empty-list gate and the bottom test
// branch into (back-edge = unconditional jmp). Hoisting the gate (`if (pos) { do {...}
// while (pos); }`) reproduces that and drops the third callee-saved register with it.
// Residual is the remaining tileX/tileY register colouring.
RVA(0x000f0db0, 0x48)
i32 MgrListFind(i32 a1, i32 a2) {
    CPtrList& list = g_gameReg->m_cmdGrid->m_baseList;
    POSITION pos = list.GetHeadPosition();
    // ONE miss exit (retail 0xf0df3): the empty-list gate and the walk's bottom test
    // branch into the same block, so the back-edge is an unconditional jmp
    if (pos != 0) {
        do {
            CGruntPuddle* p = static_cast<CGruntPuddle*>(list.GetNext(pos));
            if (p->m_pending == 0) {
                i32 v54 = p->m_tileX;
                i32 v58 = p->m_tileY;
                if (v54 == a1 && v58 == a2) {
                    return 1;
                }
            }
        } while (pos != 0);
    }
    return 0;
}
