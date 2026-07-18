// MgrSlotSwap.cpp - 0x1128b0 (__thiscall) on a small game object holding a token at
// +0x34 plus the (group m_08, index m_0c) coordinates. If the token is live it swaps
// the token currently parked in the global registry's plane table
// (((RegM30*)g_gameReg->m_world)->m_24->m_5c->m_tileGrid[ m_colOffsets[m_0c] + m_08 ]) with its own, notifies
// the registry's m_70 sub-manager, and adopts the previously-parked token. An empty
// token reports the 0x8009/0x451 diagnostic and returns 0. Every callee + the global
// are reloc-masked.
#include <Gruntz/Brickz.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Ints.h>
#include <Gruntz/GruntzMgr.h> // the REAL singleton class (+ CDDrawSurfaceMgr via GameRegistry.h)
#include <Gruntz/GameLevel.h> // CGameLevel - m_world->m_level (the level; its m_mainPlane)
#include <Wwd/WwdFile.h>      // CLevelPlane/CPlaneRender - the canonical plane (the registry plane)
#include <Gruntz/SlotHolder.h> // canonical CSlotHolder (the swapping game object)
#include <rva.h>

// The registry plane table (g_gameReg->m_world->m_level->m_mainPlane) is the shared
// world-plane CLevelPlane: value plane m_tileGrid indexed by offset plane m_colOffsets.
// canonical CDDrawSurfaceMgr, its +0x24 m_24 IS CGameLevel, and CGameLevel::m_mainPlane
// @+0x5c IS the plane - all reached cast-free.)

// The 0x64556c singleton IS CGruntzMgr (RTTI-confirmed, vftable 0x5e9b64) - declared at
// the REAL class so its methods emit DEFINED symbols instead of CGameRegistry phantoms.
// Now possible because its +0x70 sub-object folded: CGruntzMgr::m_tileGrid is a
// CGruntzMapMgr*, and the CTileGrid this TU reads IS its CMapMgr base (one class, two
// names) - so the read is a plain upcast, no cast needed.

// The owning object (group/index coordinates + the parked token) is the canonical
// CSlotHolder (<Gruntz/SlotHolder.h>).

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
