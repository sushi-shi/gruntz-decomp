// MgrSlotSwap.cpp - 0x1128b0 (__thiscall) on a small game object holding a token at
// +0x34 plus the (group m_08, index m_0c) coordinates. If the token is live it swaps
// the token currently parked in the global registry's plane table
// (((RegM30*)g_gameReg->m_world)->m_24->m_5c->m_cells[ m_rowBase[m_0c] + m_08 ]) with its own, notifies
// the registry's m_70 sub-manager, and adopts the previously-parked token. An empty
// token reports the 0x8009/0x451 diagnostic and returns 0. Every callee + the global
// are reloc-masked.
#include <Gruntz/Brickz.h>
#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Viewport.h> // shared world-plane grid (the registry plane)
#include <rva.h>

// The registry plane table (g_gameReg->m_world->m_24->m_5c) is the shared
// world-plane CViewport: value plane m_cells indexed by offset plane m_rowBase.

struct RegLevel { // g_gameReg->m_world->m_24
    char m_pad0[0x5c];
    CViewport* m_5c; // +0x5c
};

struct RegM30 {
    char m_pad0[0x24];
    RegLevel* m_24; // +0x24
};

extern "C" CGameRegistry* g_gameReg;
DATA(0x0024556c)
extern "C" CGameRegistry* g_gameReg; // ?g_gameReg (VA 0x64556c)

// The owning object: group/index coordinates + the parked token.
struct CSlotHolder {
    i32 DoSwap(); // 0x1128b0
    char m_pad0[0x08];
    i32 m_08; // +0x08  group
    i32 m_0c; // +0x0c  index
    char m_pad10[0x34 - 0x10];
    i32 m_34; // +0x34  token
};

// ---------------------------------------------------------------------------
// @early-stop
// Register-naming wall (~88%, structure byte-exact). Retail has higher register
// pressure: it keeps mgr(edi)/idx/grp live, spills newTok to a stack local
// ([esp+0x1c]/[esp+0x10]) and RE-WALKS the m_world->m_24->m_5c->cells chain for the
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
    CGameRegistry* mgr = g_gameReg;
    i32 grp = this->m_08;
    i32 idx = this->m_0c;
    i32 newTok =
        ((RegM30*)mgr->m_world)
            ->m_24->m_5c->m_cells[((RegM30*)mgr->m_world)->m_24->m_5c->m_rowBase[idx] + grp];
    ((RegM30*)g_gameReg->m_world)
        ->m_24->m_5c->m_cells[((RegM30*)g_gameReg->m_world)->m_24->m_5c->m_rowBase[idx] + grp] =
        oldTok;
    ((CBrickzGrid*)mgr->m_tileGrid)->ComputeCellFlags(grp, idx, oldTok);
    this->m_34 = newTok;
    return 1;
}

SIZE_UNKNOWN(CSlotHolder);
SIZE_UNKNOWN(RegLevel);
SIZE_UNKNOWN(RegM30);
SIZE_UNKNOWN(RegSubMgr);
