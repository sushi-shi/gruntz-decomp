// StateReleaseResources.cpp - CState::ReleaseResources (0x0fa150), the base
// game-state resource teardown: vtable slot 2's DEFAULT body (retail ??_7CState
// @0x1ea21c slot 2 holds ILT 0x3f53 -> 0xfa150, byte-verified). Every leaf state's
// own ReleaseResources override chains to it (CAttract/CBootyState/CMultiBootyState/
// CCreditsState/CPlay/..., all cross-.obj -> reloc-masked calls), and the inline
// ~CState folds the same call into every leaf destructor.
//
// Ex "CGameModeBase::BaseCleanup": CGameModeBase was a this-view of CState - RTTI
// proves CState is a ROOT (base-class array = [.?AVCState@@] only), so no such base
// exists; the view's m_c/m_14/m_18/m_3c/m_160/m_164 were CState's own members
// (m_3c == m_ready). Folded 2026-07-16 (F13).
//
// It lives in its own retail .obj (the ReleaseResources callers reference it
// cross-unit), so it stays out of GameMode.cpp to preserve those reloc-masked calls.
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
    m_ready = 0;
}
