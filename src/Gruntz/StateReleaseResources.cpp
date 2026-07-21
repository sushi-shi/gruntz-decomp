#include <rva.h>
#include <Ints.h>
#include <Gruntz/State.h>
#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections::RemoveItemA (0x142160) + CDDSurface
#include <DDrawMgr/DDrawSurfaceMgr.h>     // the m_c holder (m_ptrColl surface pool)

// 0x0fa150 - release the four owned blit surfaces (+0x160/+0x164/+0x14/+0x18) back to
// the m_c holder's +0x1c surface pool (RemoveItemA @0x142160), then clear the ready
// gate (+0x3c). __thiscall.
//
// @early-stop
// cmp-operand-order wall: retail emits cmp val,edi (val vs the zeroed edi); cl emits
// cmp edi,val. Same semantics, 1 byte per guard. All four frees + offsets byte-faithful.
RVA(0x000fa150, 0x74)
void CState::ReleaseResources() {
    if (m_world != 0) {
        if (m_scratchSurface0 != 0) {
            m_world->m_ptrColl->RemoveItemA(m_scratchSurface0);
            m_scratchSurface0 = 0;
        }
        if (m_scratchSurface1 != 0) {
            m_world->m_ptrColl->RemoveItemA(m_scratchSurface1);
            m_scratchSurface1 = 0;
        }
        if (m_blitSurface0 != 0) {
            m_world->m_ptrColl->RemoveItemA(m_blitSurface0);
            m_blitSurface0 = 0;
        }
        if (m_blitSurface1 != 0) {
            m_world->m_ptrColl->RemoveItemA(m_blitSurface1);
            m_blitSurface1 = 0;
        }
    }
    m_ready = 0;
}
