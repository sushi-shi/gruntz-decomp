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
    i32 oldTok = m_tileToken;
    if (oldTok == 0) {
        g_gameReg->ReportError(0x8009, 0x451);
        return 0;
    }
    CGruntzMgr* mgr = g_gameReg;
    i32 tileX = m_tileX;
    i32 tileY = m_tileY;
    i32 newTok = mgr->m_world->m_level->m_mainPlane
                     ->m_tileGrid[mgr->m_world->m_level->m_mainPlane->m_colOffsets[tileY] + tileX];
    g_gameReg->m_world->m_level->m_mainPlane
        ->m_tileGrid[g_gameReg->m_world->m_level->m_mainPlane->m_colOffsets[tileY] + tileX] =
        oldTok;
    (mgr->m_tileGrid)->ComputeCellFlags(tileX, tileY, oldTok);
    m_tileToken = newTok;
    return 1;
}
