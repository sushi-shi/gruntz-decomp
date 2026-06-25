#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_WellGoo.h>
// SBI_WellGoo.cpp - Gruntz CSBI_WellGoo (C:\Proj\Gruntz), the frameless method.
// RTTI .?AVCSBI_WellGoo@@; the most-derived leaf of the SBI image chain
//   CSBI_WellGoo : CSBI_Image : CSBI_RectOnly : CStatusBarItem. Vtable @0x5eadfc.
// The 4-level /GX-framed scalar destructor (0x104bb0) lives in SBI_WellGooEh.cpp.
//
// The per-frame Tick (vtable slot 5) is modeled with the SBI family's
// manual-vtable-stamp device (no real `virtual`); sibling/engine callees are
// ILT/vtable-reloc-masked.

// The g_gameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Only the
// game-manager chain Tick reads (surface context + back-buffer) is modeled.
DATA(0x0024556c)
extern CGooGameReg* g_gameReg;

// ---------------------------------------------------------------------------

// vtable slot 5 (0xe6380): the per-frame goo Tick. Idle (return 1) while the
// countdown is non-positive or no fill scale is set; otherwise tick the countdown
// down, draw the base anim frame, compute the goo fill height as a fraction of the
// (m_20 - m_18) progress (clamped to a 1.0 ceiling, then ftol'd into m_60),
// shade-blit + BltEx the goo source for that height, and finally draw the
// foreground anim frame whose top sits at m_60 - 2. The m_54/m_58 inc-around-dec is
// a draw-depth re-entrancy guard spanning the BltEx.
// @early-stop
// reloc-residual plateau: code bytes byte-identical to retail; the two
// `call RenderFrame` (0x153790) + `Blit` (0x1497f0) + `BltEx` (0x13eef0) rel32
// and the g_gameReg DIR32 are reloc-masked against differently-named symbols
// (docs/patterns/reloc-typing-vptr-global.md). Same plateau as
// CSBI_GruntMachine::Render / CSBI_WarlordHead::Render. Exact once the SBI family
// callees co-name in the final sweep.
RVA(0x000e6380, 0xf9)
i32 CSBI_WellGoo::Tick() {
    if (m_28 <= 0 || m_44 == 0) {
        return 1;
    }
    m_28--;

    i32 ctx = g_gameReg->m_30->m_4->m_14;
    m_40->RenderFrame(ctx, m_48, m_20 + 3, 0);

    // Goo fill height: a fraction of the (m_20 - m_18) progress, ceiling-clamped
    // to 1.0, subtracted off the current water line and rounded to an int.
    double fill = (double)(m_20 - m_18) * m_44 * 0.01f - 3.0f;
    if (fill > 1.0) {
        fill = 1.0;
    }
    m_60 = (i32)((double)m_20 - fill);

    ((CGooShadeBlit*)m_38)->Blit((i32)&m_4c, m_34, &m_4c, 0, 0);

    m_54++;
    m_58++;
    ((CGooSurface*)g_gameReg->m_30->m_4->m_2c)->BltEx(&m_5c, m_34, &m_4c, 0x1000000, 0);
    m_58--;
    m_54--;

    m_3c->RenderFrame(ctx, m_48, m_60 - 2, 0);
    return 1;
}
