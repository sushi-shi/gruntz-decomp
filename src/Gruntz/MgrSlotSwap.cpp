#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Ints.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/GameLevel.h>
#include <Wwd/WwdFile.h>
#include <Gruntz/SlotHolder.h>
#include <rva.h>

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
