#include <Gruntz/TriggerMgrViews.h> // CTriggerMgr + CTmRecNode (MFC-first)
#include <Gruntz/GruntPuddle.h>   // CGruntPuddle (the baseList element; m_tileX/m_tileY/m_pending)
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// 0xf0db0 (__cdecl) - true if a live (m_occupied==0) candidate matches (a1,a2).
// ---------------------------------------------------------------------------
// @early-stop
// regalloc-cascade wall (73%): the loop structure (cur=node; node=node->next;
// payload; nested m_occupied==0 then gridX/gridY compares) is byte-faithful, but retail
// reuses the dead payload register (eax) and the m_occupied temp (edx) for the gridX/
// gridY loads, needing only esi/edi callee-saved; cl spends a 3rd callee-saved reg (ebx
// for gridX), so the whole push/pop frame + the gridX compare diverge. A register-pressure
// coin-flip (linked-list-walk-node-eax-rotation.md family); not source-steerable. Logic
// 100% correct; deferred.
RVA(0x000f0db0, 0x48)
i32 MgrListFind(i32 a1, i32 a2) {
    CTmRecNode* node =
        reinterpret_cast<CTmRecNode*>(g_gameReg->m_cmdGrid->m_baseList.GetHeadPosition());
    while (node) {
        CTmRecNode* cur = node;
        node = node->m_next;
        CGruntPuddle* p = cur->m_obj;
        if (p->m_pending == 0) {
            i32 v54 = p->m_tileX;
            i32 v58 = p->m_tileY;
            if (v54 == a1 && v58 == a2) {
                return 1;
            }
        }
    }
    return 0;
}
