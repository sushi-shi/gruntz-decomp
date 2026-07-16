// MgrListFind.cpp - a g_gameReg list scan (0xf0db0): walk the world command grid's base
// object-list (CTriggerMgr::m_baseList) for a live candidate whose grid pair (m_gridX,
// m_gridY) matches the two query coordinates. Self-contained; g_gameReg is a reloc-masked
// DATA load.
//
// onto the canonicals (xref-proven, byte-identical shape):
//   * MgrFindPayload IS CTmCandidate (<Gruntz/TriggerMgr.h>) - m_54/m_58/m_5c ARE
//     m_gridX/m_gridY/m_occupied at the same offsets;
//   * MgrFindNode IS the CPtrList node CTmRecNode (<Gruntz/TriggerMgrViews.h>) -
//     next@+0, CTmCandidate* obj@+8;
//   * MgrFindList IS CTriggerMgr::m_baseList (the +0 CPtrList) - its GetHeadPosition()
//     reads m_pNodeHead @+4, byte-identical to the old (MgrFindList*)cmdGrid->m_4 read.
#include <Gruntz/TriggerMgrViews.h> // CTriggerMgr + CTmCandidate + CTmRecNode (MFC-first)
#include <rva.h>

extern "C" CGameRegistry* g_gameReg; // ?g_gameReg (VA 0x64556c)

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
    CTmRecNode* node = (CTmRecNode*)g_gameReg->m_cmdGrid->m_baseList.GetHeadPosition();
    while (node) {
        CTmRecNode* cur = node;
        node = node->m_next;
        CTmCandidate* p = cur->m_obj;
        if (p->m_occupied == 0) {
            i32 v54 = p->m_gridX;
            i32 v58 = p->m_gridY;
            if (v54 == a1 && v58 == a2) {
                return 1;
            }
        }
    }
    return 0;
}
