#include <Gruntz/TriggerMgrViews.h> // CTriggerMgr (MFC-first)
#include <Gruntz/GruntPuddle.h>   // CGruntPuddle (the baseList element; m_tileX/m_tileY/m_pending)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// 0xf0db0 (__cdecl) - true if a live (m_occupied==0) candidate matches (a1,a2).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x000f0db0, 0x48)
// The two words are compared against a CGruntPuddle's m_tileX/m_tileY below.
i32 MgrListFind(i32 tileX, i32 tileY) {
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
                if (v54 == tileX && v58 == tileY) {
                    return 1;
                }
            }
        } while (pos != 0);
    }
    return 0;
}
