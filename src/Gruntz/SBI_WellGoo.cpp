#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_WellGoo.h>
#include <Image/CImage.h>            // CImage::RenderFrame (0x153790) - the m_40/m_3c frames
#include <DDrawMgr/DDrawShadeBlit.h> // CDDrawShadeBlit::Blit (0x1497f0) - the m_38 blitter
#include <DDrawMgr/DDSurface.h>      // CDDSurface::BltEx (0x13eef0) - the goo/back-buffer surfaces
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
// countdown is non-positive; then tick it down and idle again if no fill scale is
// set; otherwise draw the base anim frame, compute the goo fill height as a
// fraction of the (m_rect14.m_c - m_rect14.m_4) progress (FLOORED to 1.0, then ftol'd
// into m_fgTop), shade-blit + BltEx the goo source for that height, and finally draw
// the foreground anim frame whose top sits at m_fgTop - 2. The m_drawGuard/m_blitGuard
// inc-around-dec is a draw-depth re-entrancy guard spanning the BltEx.
// @early-stop
// ~99.96% reloc-residual plateau: the CODE BYTES are byte-identical to retail
// (verified llvm-objdump base vs target). The residual is only DATA-reloc naming:
// the g_gameReg DIR32 + the three FP-constant-pool DIR32s (0.01f/3.0f/1.0 land in
// the compiler's $T literals vs retail's DAT_005eab28/2c/30) - the documented
// reloc-typing scoring artifact (docs/patterns/reloc-typing-vptr-global.md). Raised
// from 83% by (1) unifying the CGoo* views to CImage/CDDrawShadeBlit/CDDSurface so
// the three call rel32s co-name, (2) the (float) cast keeping 0.01f/3.0f single-
// precision (fmuls/fsubs), (3) fixing the clamp to a 1.0 FLOOR + (4) decrementing
// m_28 between the two guards + reusing the ctx pointer for ctx->m_2c (the BltEx
// receiver), all matching retail's byte stream.
RVA(0x000e6380, 0xf9)
i32 CSBI_WellGoo::Tick() {
    if (m_28 <= 0) {
        return 1;
    }
    m_28--; // retail decrements between the two guards (before the m_fillScale gate)
    if (m_fillScale == 0) {
        return 1;
    }

    CGooRenderCtx* ctx = g_gameReg->m_30->m_4->m_14;
    m_baseFrame->RenderFrame((void*)ctx, (void*)m_drawX, (void*)(m_rect14.m_c + 3), 0);

    // Goo fill height: a fraction of the (m_rect14.m_c - m_rect14.m_4) progress,
    // ceiling-clamped to 1.0, subtracted off the current water line and rounded to an
    // int. The (float) cast keeps the 0.01f/3.0f factors single-precision (fmuls/fsubs,
    // the 32-bit float constant pool) while the 1.0 clamp stays double (fcoml).
    double fill = (float)(m_rect14.m_c - m_rect14.m_4) * m_fillScale * 0.01f - 3.0f;
    if (fill <= 1.0) {
        fill = 1.0;
    }
    m_fgTop = (i32)((double)m_rect14.m_c - fill);

    m_blitter->Blit((ShadeRect*)&m_srcRect, m_gooSrc, (ShadeRect*)&m_srcRect, 0, 0);

    m_drawGuard++;
    m_blitGuard++;
    ctx->m_2c->BltEx(&m_dstRect, m_gooSrc, &m_srcRect, 0x1000000, 0);
    m_blitGuard--;
    m_drawGuard--;

    m_fgFrame->RenderFrame((void*)ctx, (void*)m_drawX, (void*)(m_fgTop - 2), 0);
    return 1;
}
