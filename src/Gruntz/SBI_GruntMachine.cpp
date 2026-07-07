#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/GameRegistry.h> // canonical g_gameReg singleton + CSpriteFactoryHolder m_world
#include <Gruntz/ResMgr.h>       // CDrawTarget (m_world->m_drawTarget->m_14)
// SBI_GruntMachine.cpp - Gruntz CSBI_GruntMachine (C:\Proj\Gruntz), the frameless
// methods. RTTI .?AVCSBI_GruntMachine@@; a sibling leaf of the SBI family
//   CSBI_GruntMachine : CStatusBarItem. Vtable @0x5eadbc. The /GX-framed scalar
// destructor (0x104ce0) lives in SBI_GruntMachineEh.cpp.
//
// These are concrete virtual-slot methods (slots 3 and 5) plus a non-virtual
// frame-prime helper, modeled with the SBI family's manual-vtable-stamp device (no
// real `virtual`); sibling/engine callees are ILT/vtable-reloc-masked.

// The g_gameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Only the
// game-manager chain Render reads is modeled.
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// ---------------------------------------------------------------------------

// vtable slot 3 (0xe8c70): drop the standalone frame handle (m_30 config? no -
// m_30 is reused as the config record) and the two resolved frame records. Reached
// by the destructor as the member teardown. Zeroes m_34, m_3c, m_30.
RVA(0x000e8c70, 0xc)
void CSBI_GruntMachine::Reset() {
    m_34 = 0;
    m_3c = 0;
    m_30 = 0;
}

// vtable slot 5 (0xe8cb0): the per-frame render. Idle (return 1) while the frame
// countdown is non-positive; otherwise tick it down, resolve the two indexed frame
// records (m_38 -> m_34, m_40 -> m_3c) through the config record's gated frame
// table, pull the surface context from the active drawable, then blit up to three
// frames: the standalone handle (m_44), the second resolved record (m_3c, drawn
// shifted +0x2c in x), and the first resolved record (m_34). Each draws at the base
// origin plus the frame record's own m_rect14.m_4/m_1c offset.
// @early-stop
// reloc-residual plateau: code bytes byte-identical to retail; the three
// `call RenderFrame` (0x153790) rel32 + the g_gameReg DIR32 are reloc-masked against
// differently-named symbols (docs/patterns/reloc-typing-vptr-global.md). Same
// plateau as CSBI_WarlordHead::Render / CSBI_SideTab::Render.
RVA(0x000e8cb0, 0xc4)
i32 CSBI_GruntMachine::Render(i32 z) {
    if (m_28 <= 0) {
        return 1;
    }
    i32 idx = m_38;
    m_28--;
    CGmConfig* cfg = m_30;

    m_34 = (idx < cfg->m_64 || idx > cfg->m_68) ? 0 : cfg->m_14[idx];
    idx = m_40;
    m_3c = (idx < cfg->m_64 || idx > cfg->m_68) ? 0 : cfg->m_14[idx];

    i32 ctx = (i32)g_gameReg->m_world->m_drawTarget->m_14;

    CImage* f = m_44;
    if (f) {
        f->RenderFrame(
            (void*)ctx,
            (void*)(m_rect14.m_0 + f->m_anchorX),
            (void*)(m_rect14.m_4 + f->m_anchorY),
            0
        );
    }
    f = m_3c;
    if (f) {
        f->RenderFrame(
            (void*)ctx,
            (void*)(m_rect14.m_0 + f->m_anchorX + 0x2c),
            (void*)(m_rect14.m_4 + f->m_anchorY),
            0
        );
    }
    f = m_34;
    if (f) {
        f->RenderFrame(
            (void*)ctx,
            (void*)(m_rect14.m_0 + f->m_anchorX),
            (void*)(m_rect14.m_4 + f->m_anchorY),
            0
        );
    }
    return 1;
}

// 0xe8dc0 (__thiscall, ret 8): prime the two frame indices and arm the countdown.
// Each index is only written when it differs from -1 (a "keep current" sentinel);
// the countdown is unconditionally set to 2 so the next two Render ticks play the
// frames.
RVA(0x000e8dc0, 0x22)
void CSBI_GruntMachine::SetFrames(i32 idxA, i32 idxB) {
    if (idxA != -1) {
        m_38 = idxA;
    }
    if (idxB != -1) {
        m_40 = idxB;
    }
    m_28 = 2;
}
