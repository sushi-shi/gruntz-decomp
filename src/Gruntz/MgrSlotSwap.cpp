#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Ints.h>
#include <Gruntz/GruntzMgr.h> // the REAL singleton class (+ CDDrawSurfaceMgr via GameRegistry.h)
#include <Gruntz/GameLevel.h> // CGameLevel - m_world->m_level (the level; its m_mainPlane)
#include <Wwd/WwdFile.h>      // CLevelPlane/CPlaneRender - the canonical plane (the registry plane)
#include <Gruntz/SlotHolder.h> // canonical CSlotHolder (the swapping game object)
#include <rva.h>

// ---------------------------------------------------------------------------
// @early-stop
// Register-naming wall (~88%, structure byte-exact). Retail has higher register
// pressure: it keeps mgr(edi)/idx/grp live, spills newTok to a stack local
// ([esp+0x1c]/[esp+0x10]) and RE-WALKS the m_world->m_level->m_5c->cells chain for the
// write instead of CSE-ing the cell address. Two levers reproduced that shape and
// took this 54.9 -> 88: (1) cache g_gameReg in a local `mgr` (raises pressure);
// (2) idx/grp read-once locals shared between the cell index and the Notify args;
// (3) crucially, WRITE the cell through the un-cached global g_gameReg while
// READING via mgr -- this defeats MSVC's read/write address-CSE, forcing the
// spill+rewalk (62 -> 88). Residual is pure regalloc naming: retail mgr=edi/idx=eax/
// grp=ecx vs base mgr=ecx/idx=edx/grp=eax, plus the idx leaf-read scheduled after
// W/L (interleaved) vs hoisted before -- an unsteerable allocator/scheduler coin-flip
// at identical instruction count. See docs/patterns/cse-defeat-uncached-global-rewalk.md.
RVA(0x001128b0, 0x88)
i32 CSlotHolder::DoSwap() {
    i32 oldTok = this->m_34;
    if (oldTok == 0) {
        g_gameReg->ReportError(0x8009, 0x451);
        return 0;
    }
    CGruntzMgr* mgr = g_gameReg;
    i32 grp = this->m_08;
    i32 idx = this->m_0c;
    i32 newTok = mgr->m_world->m_level->m_mainPlane
                     ->m_tileGrid[mgr->m_world->m_level->m_mainPlane->m_colOffsets[idx] + grp];
    g_gameReg->m_world->m_level->m_mainPlane
        ->m_tileGrid[g_gameReg->m_world->m_level->m_mainPlane->m_colOffsets[idx] + grp] = oldTok;
    (mgr->m_tileGrid)->ComputeCellFlags(grp, idx, oldTok);
    this->m_34 = newTok;
    return 1;
}
