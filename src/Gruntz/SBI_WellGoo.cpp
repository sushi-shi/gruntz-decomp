#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies (see StatusBarItem.h)
#include <rva.h>
#include <Gruntz/CurPlayer.h>     // g_curPlayer
#include <Gruntz/SerialCounter.h> // g_serialCounter
#include <Io/FileMem.h>           // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_WellGoo.h>
#include <Image/CImage.h> // CImage::RenderFrame (0x153790) - the m_40/m_3c frames + m_owned
#include <DDrawMgr/DDrawShadeBlit.h> // CDDrawShadeBlit::Blit (0x1497f0) - the m_38 blitter; Notify + m_1c
#include <DDrawMgr/DDSurface.h> // CDDSurface::BltEx (0x13eef0) - the goo/back-buffer surfaces
#include <DDrawMgr/DDrawWorkerRegistry.h> // AnyValueMatches_155630 + the +0x10 name map (Serialize)
#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections::MakeAndAddB (Serialize mode-8)
#include <Gruntz/SpriteRefTable.h>        // CSpriteRefTable::GetSel (Serialize mode-8)
#include <Gruntz/SerialArchive.h>         // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <string.h>                       // strlen / memset (inline repne-scas / rep-stos)
// SBI_WellGoo.cpp - Gruntz CSBI_WellGoo (C:\Proj\Gruntz), the frameless method.
// RTTI .?AVCSBI_WellGoo@@; the most-derived leaf of the SBI image chain
//   CSBI_WellGoo : CSBI_Image : CSBI_RectOnly : CStatusBarItem. Vtable @0x5eadfc.
// The 4-level /GX chain destructor (0x104bb0) is defined below - the former
// SBI_WellGooEh.cpp companion split is collapsed (retail's one TU was /GX).
//
// The per-frame Tick (vtable slot 5) is modeled with the SBI family's
// manual-vtable-stamp device (no real `virtual`); sibling/engine callees are
// ILT/vtable-reloc-masked.

// The g_gameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c). Only the
// game-manager chain Tick reads (surface context + back-buffer) is modeled.
extern "C" CGooGameReg* g_gameReg;

// ---------------------------------------------------------------------------
// vtable slot 2 (0xe6020, thunk 0x24eb): CSBI_WellGoo::Setup, the slot-2 override
// (dossier #16 identity: vtbl 0x1eadfc slot [2] jmps here). The ex-AniPlayer-TU
// "StubOwner_e6020" placeholder host is DISSOLVED (2026-07-16) onto the declared
// override in <Gruntz/SBI_WellGoo.h> - same 10-dword arg shape (a1..a4 + the
// by-value SbiRect + a9/a10), same ret 0x28.
// @early-stop
// return-0 stub (the old ~86% score was a base-length normalization artifact of
// the previous unit's epilogue alignment; in this unit it scores ~1% - equally
// meaningless for a stub). A faithful full-body reconstruction (the 10-arg
// setup: geometry stash + mgr-alloc + 3 bounded map lookups + SetRect) reaches
// only ~42%: target keeps 4 callee-saved regs and reuses the dead incoming-arg
// slots as SetRect/lookup scratch, while cl spills a fresh `sub esp,0x10` RECT
// frame + drops ebp - a uniform frame shift that mismatches every [esp+X]
// operand. Frame/regalloc wall; full reconstruction deferred to the final sweep.
RVA(0x000e6020, 0x288)
i32 CSBI_WellGoo::Setup(i32, i32, i32, i32, SbiRect, i32, i32) {
    return 0;
}

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

    CGooRenderCtx* ctx = g_gameReg->m_gameMgr->m_drawable->m_renderCtx;
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
    ctx->m_backBuffer->BltEx(&m_dstRect, m_gooSrc, &m_srcRect, 0x1000000, 0);
    m_blitGuard--;
    m_drawGuard--;

    m_fgFrame->RenderFrame((void*)ctx, (void*)m_drawX, (void*)(m_fgTop - 2), 0);
    return 1;
}

// The serialize record counter (bumped before each name+index frame slot) + the
// focused-grunt sentinel keying the mode-8 selector table.

// CSBI_WellGoo::Serialize (0xe64c0) - vtable slot 1. Bail on a null archive / no
// game manager; chain the base CSBI_Image serialize; then mode 4/7 round-trip the
// fill scale + draw origin + src/dst rects, plus the fg/base frame handles by
// name(+0x80)+index(4) through the m_30->m_10 registry (write: reverse-lookup the
// frame's key via AnyValueMatches; read: Lookup the key + bounds-index into the
// resolved frame set). Mode 8 (post-load) re-makes the goo surface + rebinds each
// frame's owned-blitter shade node from the sprite-ref selector.
//
// @early-stop
// ~83%: logic byte-shaped end to end (the mode sub-chain dispatch - mode 8 is the
// fall-through `switch` case, key to the block layout; the field round-trips; the two
// name+index frame legs with the g_serialCounter bumps + inline strlen/memset; the
// mode-8 MakeAndAddB / GetSel / Notify rebind). The `mgr` cache reproduces retail's
// spill of g_gameReg->m_30 (frame 0x8c, without it the 4-byte-smaller frame shifted
// every displacement -> 0%). Residue is the megafunction tail: the mgr spill lands in
// [esp+0x18] vs retail [esp+0x14] (regalloc), several engine-call relocs reached
// direct where retail uses ILT thunks + the differently-named folded base leg, and
// the inline repne-scas/rep-stos scheduling. Not source-steerable; final sweep.
RVA(0x000e64c0, 0x3e7)
i32 CSBI_WellGoo::Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    CGooGameMgr* mgr = g_gameReg->m_gameMgr; // cached across the calls (retail spills it @[esp+0x14])
    if (mgr == 0) {
        return 0;
    }
    if (SerializeChain(arc, mode, a3, a4) == 0) {
        return 0;
    }
    switch (mode) {
        case 4: {
            // WRITE
            arc->Write(&m_fillScale, 4);
            arc->Write(&m_drawX, 4);
            arc->Write(&m_srcRect, 0x10);
            arc->Write(&m_dstRect, 0x10);
            char buf[0x80];
            i32 idx;
            g_serialCounter++;
            memset(buf, 0, 0x80);
            idx = 0;
            if (m_fgFrame != 0) {
                mgr->m_frameSetRegistry->AnyValueMatches_155630((i32)m_fgFrame, (i32)buf, (i32)&idx);
            }
            arc->Write(buf, 0x80);
            arc->Write(&idx, 4);
            g_serialCounter++;
            memset(buf, 0, 0x80);
            idx = 0;
            if (m_baseFrame != 0) {
                mgr->m_frameSetRegistry->AnyValueMatches_155630((i32)m_baseFrame, (i32)buf, (i32)&idx);
            }
            arc->Write(buf, 0x80);
            arc->Write(&idx, 4);
            return 1;
        }
        case 7: {
            // READ
            arc->Read(&m_fillScale, 4);
            arc->Read(&m_drawX, 4);
            arc->Read(&m_srcRect, 0x10);
            arc->Read(&m_dstRect, 0x10);
            char buf[0x80];
            i32 idx;
            g_serialCounter++;
            arc->Read(buf, 0x80);
            arc->Read(&idx, 4);
            if (strlen(buf) != 0) {
                CSbiFrameSet* set = 0;
                ((CMapStringToPtr*)((char*)mgr->m_frameSetRegistry + 0x10))->Lookup(buf, (void*&)set);
                if (set != 0 && idx >= set->m_64 && idx <= set->m_68) {
                    m_fgFrame = set->m_frames[idx];
                } else {
                    m_fgFrame = 0;
                }
            } else {
                m_fgFrame = 0;
            }
            g_serialCounter++;
            arc->Read(buf, 0x80);
            arc->Read(&idx, 4);
            if (strlen(buf) != 0) {
                CSbiFrameSet* set = 0;
                ((CMapStringToPtr*)((char*)mgr->m_frameSetRegistry + 0x10))->Lookup(buf, (void*&)set);
                if (set != 0 && idx >= set->m_64 && idx <= set->m_68) {
                    m_baseFrame = set->m_frames[idx];
                } else {
                    m_baseFrame = 0;
                }
            } else {
                m_baseFrame = 0;
            }
            return 1;
        }
        case 8: {
            // RESOLVE (post-load): remake the goo surface + rebind each frame's shade node.
            m_gooSrc = mgr->m_surfacePool->MakeAndAddB(0x14, 5, 0x10, 0, -1);
            if (m_gooSrc == 0) {
                return 0;
            }
            i32 sel = *(i32*)((char*)g_gameReg + 0x158 + (g_curPlayer * 71) * 8);
            i32 node = g_gameReg->m_refTable->GetSel(sel, 0);
            if (node == 0) {
                node = g_gameReg->m_refTable->GetSel(1, 0);
            }
            CImage* fr = (CImage*)m_30;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && ((CImage*)m_30)->m_owned != 0) {
                ((CImage*)m_30)->m_owned->m_palDescr = (ShadeDescr*)node;
            }
            fr = m_baseFrame;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && m_baseFrame->m_owned != 0) {
                m_baseFrame->m_owned->m_palDescr = (ShadeDescr*)node;
            }
            fr = m_fgFrame;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && m_fgFrame->m_owned != 0) {
                m_fgFrame->m_owned->m_palDescr = (ShadeDescr*)node;
            }
            break;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// ~CSBI_WellGoo (0x104bb0): the /GX chain destructor - stamp ??_7CSBI_WellGoo,
// release the owned goo source surface back to the pool (the same teardown the
// slot-3 Free below carries, folded inline here by retail), then MSVC folds the
// three inline base dtors in (Image/RectOnly/StatusBarItem - the SBI_DTOR_CHAIN
// device) behind the /GX SEH frame. Collapsed from SBI_WellGooEh.cpp.
RVA(0x00104bb0, 0x94)
CSBI_WellGoo::~CSBI_WellGoo() {
    if (m_gooSrc != 0) {
        ((CGooGameMgr*)m_24)->m_surfacePool->RemoveItemA(m_gooSrc);
        m_gooSrc = 0;
    }
}

// vtable slot 3 (0x104c80): release the owned goo source surface (+0x34) through the
// cached manager's (+0x24) surface pool (RemoveItemA @0x142160), then clear it.
// Re-homed from src/Stub/BoundaryLowerMethods.cpp (was the C104c80 placeholder view);
// the vtable slot-3 thunk 0x30b7 jmps here, so this IS CSBI_WellGoo's slot-3 override.
// The +0x24 cache holds the CGooGameMgr (== g_gameReg->m_30); the FLAT CStatusBarItem
// base models it as an int (Setup arg2), so reinterpret to reach its pool - a
// heterogeneous base field (@flag: int/ptr overlay at +0x24).
RVA(0x00104c80, 0x1f)
void CSBI_WellGoo::Free() {
    if (m_gooSrc != 0) {
        ((CGooGameMgr*)m_24)->m_surfacePool->RemoveItemA(m_gooSrc);
        m_gooSrc = 0;
    }
}
