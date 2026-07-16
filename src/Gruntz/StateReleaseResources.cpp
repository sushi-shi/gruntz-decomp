// GameModeBase.cpp - CGameModeBase::BaseCleanup (0x0fa150), the WAP32 base cleanup
// every game-state ReleaseResources chains to (CAttract / CBootyState /
// CMultiBootyState / CCreditsState, all cross-.obj -> reloc-masked calls). Re-homed
// from src/Stub/BoundaryLowerMethods.cpp (was the Cfa150::Cleanup placeholder view);
// caller graph (gruntz sema xref --tree 0x000fa150) confirms the attribution.
//
// It lives in its own retail .obj (the ReleaseResources callers + Reset/ResetPreview
// reference it cross-unit), so keeping it out of GameMode.cpp preserves those
// reloc-masked calls.
#include <rva.h>
#include <Ints.h>
#include <Gruntz/GameModeBase.h>
#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections::RemoveItemA (0x142160) + CDDSurface

// 0x0fa150 - release the four owned blit surfaces (+0x160/+0x164/+0x14/+0x18) back to
// the mode holder's +0x1c surface pool (RemoveItemA @0x142160), then clear +0x3c.
// __thiscall.
//
// @early-stop
// cmp-operand-order wall: retail emits cmp val,edi (val vs the zeroed edi); cl emits
// cmp edi,val. Same semantics, 1 byte per guard. All four frees + offsets byte-faithful.
RVA(0x000fa150, 0x74)
void CGameModeBase::BaseCleanup() {
    if (m_c != 0) {
        if (m_160 != 0) {
            m_c->m_ptrColl->RemoveItemA(m_160);
            m_160 = 0;
        }
        if (m_164 != 0) {
            m_c->m_ptrColl->RemoveItemA(m_164);
            m_164 = 0;
        }
        if (m_14 != 0) {
            m_c->m_ptrColl->RemoveItemA(m_14);
            m_14 = 0;
        }
        if (m_18 != 0) {
            m_c->m_ptrColl->RemoveItemA(m_18);
            m_18 = 0;
        }
    }
    m_3c = 0;
}
