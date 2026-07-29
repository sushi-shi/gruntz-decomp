#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies (see StatusBarItem.h)
#include <rva.h>
#include <DDrawMgr/DDrawSurfaceMgr.h> // the m_24 config host (real type)
#include <Gruntz/CurPlayer.h>         // g_curPlayer
#include <Gruntz/SerialCounter.h>     // g_serialCounter
#include <Io/FileMem.h> // the serialize stream (CFileMemBase == the real CFileMemBase)
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_WellGoo.h>
#include <Image/CImage.h> // CImage::RenderFrame (0x153790) - the m_40/m_3c frames + m_owned
#include <DDrawMgr/DDrawShadeBlit.h> // CDDrawShadeBlit::Blit (0x1497f0) - the m_38 blitter; Notify + m_1c
#include <DDrawMgr/DDSurface.h> // CDDSurface::BltEx (0x13eef0) - the goo/back-buffer surfaces
#include <DDrawMgr/DDrawWorkerRegistry.h> // AnyValueMatches + the +0x10 name map (Serialize)
#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections::MakeAndAddB (Serialize mode-8)
#include <Gruntz/SpriteRefTable.h>        // CSpriteRefTable::GetSel (Serialize mode-8)
#include <Gruntz/SerialArchive.h>         // CFileMemBase (Read @+0x2c / Write @+0x30)
#include <string.h>                       // strlen / memset (inline repne-scas / rep-stos)

#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type
#include <Gruntz/GruntzMgr.h>

VTBL(CSBI_WellGoo, 0x001eadfc); // vtable_names -> code (RTTI game class)
// ---------------------------------------------------------------------------
// vtable slot 2 (0xe6020, thunk 0x24eb): CSBI_WellGoo::Setup, the slot-2 override
// (dossier #16 identity: vtbl 0x1eadfc slot [2] jmps here). The ex-AniPlayer-TU
// "StubOwner_e6020" placeholder host is DISSOLVED (2026-07-16) onto the declared
// override in <Gruntz/SBI_WellGoo.h> - same 10-dword arg shape (a1..a4 + the
// by-value RECT + a9/a10), same ret 0x28.
// Bind the widget: stash the geometry, allocate the goo scratch surface, resolve the
// three frames (indices 4 / 2 / 3 of the worker registered under `key`) and rebind each
// one's owned blitter to the local player's palette, then measure the source rect off
// the resolved base frame. The SetRect scratch IS the incoming by-value RECT parameter
// re-used after its four words have been copied into m_rect14 - which is why retail
// needs no extra stack for it.
// @early-stop
// 83.5% (from 0.86%). Residual is a callee-saved COLOURING swap: retail holds the
// registry key in ebp and the resolved palette node in ebx, cl5 does the reverse,
// which also flips where the `found = 0` store lands relative to the Lookup argument
// pushes. Not source-steerable. (The prior note's "~42% ceiling" no longer applies -
// it predated both the RECT layout fix and the shared-epilogue spelling.)
RVA(0x000e6020, 0x288)
i32 CSBI_WellGoo::Setup(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT rc,
    i32 key,
    i32 fillScale
) {
    // Every bail is a `goto fail` onto ONE shared epilogue - retail has a single
    // `xor eax,eax; pop..; ret 0x28` block at 0xe629f that all eight guards jump to,
    // where per-guard `return 0;` statements made cl emit eight copies of it
    // (docs/patterns/positive-gate-enables-shrink-wrap.md, the b_ret > t_ret test).
    // Declarations are hoisted because cl 5.0 rejects a goto that skips an
    // initialisation (C2362).
    i32 sel;
    CShadeTable* node;
    CObject* found;
    CDDrawWorker* set;
    if (host == 0) {
        goto fail;
    }
    if (owner == 0) {
        goto fail;
    }
    m_2c = owner;
    m_24 = host;
    m_tab = tab;
    m_28 = 0;
    m_enabled = 1;
    m_rect14 = rc;
    m_cmd = cmd;
    m_fillScale = fillScale;
    m_dstRect.left = m_rect14.left;
    m_dstRect.right = m_rect14.right + 1;
    m_dstRect.bottom = m_rect14.bottom + 1;
    if (key == 0) {
        goto fail;
    }
    m_gooSrc = g_gameReg->m_world->m_ptrColl->MakeAndAddB(0x14, 5, 0x10, 0, -1);
    if (m_gooSrc == 0) {
        goto fail;
    }
    sel = g_gameReg->m_options[g_curPlayer].m_008;
    node = g_gameReg->m_spriteFactory->GetSel(sel, 0);
    if (node == 0) {
        node = g_gameReg->m_spriteFactory->GetSel(1, 0);
    }

    // The registry key arrives as an i32 - retail's own mangling for this slot ends
    // `UtagRECT@@HH@Z`, i.e. the two trailing parameters really are declared `int` - so
    // the string type has to be re-applied at the one place it is used as one.
    found = 0;
    m_24->m_imageRegistry->m_10map.Lookup(reinterpret_cast<LPCTSTR>(key), found);
    set = static_cast<CDDrawWorker*>(found);
    m_frame = (set != 0) ? set->GetAt(4) : 0;
    if (m_frame == 0) {
        goto fail;
    }
    if (m_frame->m_owned != 0) {
        m_frame->m_owned->Select(0xa, 0);
    }
    if (node != 0 && m_frame->m_owned != 0) {
        m_frame->m_owned->m_palDescr = node;
    }
    m_blitter = m_frame->m_owned;
    if (m_blitter == 0) {
        goto fail;
    }

    found = 0;
    m_24->m_imageRegistry->m_10map.Lookup(reinterpret_cast<LPCTSTR>(key), found);
    set = static_cast<CDDrawWorker*>(found);
    m_baseFrame = (set != 0) ? set->GetAt(2) : 0;
    if (m_baseFrame == 0) {
        goto fail;
    }
    if (m_baseFrame->m_owned != 0) {
        m_baseFrame->m_owned->Select(0xa, 0);
    }
    if (node != 0 && m_baseFrame->m_owned != 0) {
        m_baseFrame->m_owned->m_palDescr = node;
    }

    found = 0;
    m_24->m_imageRegistry->m_10map.Lookup(reinterpret_cast<LPCTSTR>(key), found);
    set = static_cast<CDDrawWorker*>(found);
    m_fgFrame = (set != 0) ? set->GetAt(3) : 0;
    if (m_fgFrame == 0) {
        goto fail;
    }
    if (m_fgFrame->m_owned != 0) {
        m_fgFrame->m_owned->Select(0xa, 0);
    }
    if (node != 0 && m_fgFrame->m_owned != 0) {
        m_fgFrame->m_owned->m_palDescr = node;
    }

    ::SetRect(&rc, 0, 0, m_frame->m_width - 1, m_frame->m_height - 1);
    m_srcRect = rc;
    m_drawX = m_rect14.left + (m_rect14.right - m_rect14.left) / 2 + 1;
    return 1;
fail:
    return 0;
}

// vtable slot 5 (0xe6380): the per-frame goo Tick. Idle (return 1) while the
// countdown is non-positive; then tick it down and idle again if no fill scale is
// set; otherwise draw the base anim frame, compute the goo fill height as a
// fraction of the (m_rect14.bottom - m_rect14.top) progress (FLOORED to 1.0, then ftol'd
// into m_dstRect.top, the goo water line), shade-blit + BltEx the goo source for that
// height, and finally draw the foreground anim frame whose top sits two pixels above
// it. The inc/dec pair around the BltEx widens m_srcRect's far edges by one - the
// inclusive->exclusive fixup DirectDraw wants - it is NOT a re-entrancy guard (that
// was the old m_drawGuard/m_blitGuard mis-model; see the header).
RVA(0x000e6360, 0x8)
i32 CSBI_WellGoo::Refresh(i32) {
    return 1;
}

RVA(0x000e6380, 0xf9)
i32 CSBI_WellGoo::Render() {
    if (m_28 <= 0) {
        return 1;
    }
    m_28--; // retail decrements between the two guards (before the m_fillScale gate)
    if (m_fillScale == 0) {
        return 1;
    }

    CDDrawSurfacePair* ctx = g_gameReg->m_world->m_drawTarget->m_backPair;
    m_baseFrame->RenderFrame(ctx, m_drawX, m_rect14.bottom + 3, 0);

    // Goo fill height: a fraction of the (m_rect14.bottom - m_rect14.top) progress,
    // ceiling-clamped to 1.0, subtracted off the current water line and rounded to an
    // int. The (float) cast keeps the 0.01f/3.0f factors single-precision (fmuls/fsubs,
    // the 32-bit float constant pool) while the 1.0 clamp stays double (fcoml).
    double fill = static_cast<float>((m_rect14.bottom - m_rect14.top)) * m_fillScale * 0.01f - 3.0f;
    if (fill <= 1.0) {
        fill = 1.0;
    }
    m_dstRect.top = static_cast<i32>((static_cast<double>(m_rect14.bottom) - fill));

    m_blitter->Blit(&m_srcRect, m_gooSrc, &m_srcRect, 0, 0);

    // Inclusive -> exclusive: SetRect seeded m_srcRect with (0,0,w-1,h-1) in Setup, and
    // DirectDraw wants the far edges one past. (These two were the phantom
    // "m_drawGuard"/"m_blitGuard" counters - see the header.)
    m_srcRect.right++;
    m_srcRect.bottom++;
    ctx->m_surface->BltEx(&m_dstRect, m_gooSrc, &m_srcRect, 0x1000000, 0);
    m_srcRect.right--;
    m_srcRect.bottom--;

    m_fgFrame->RenderFrame(ctx, m_drawX, m_dstRect.top - 2, 0);
    return 1;
}

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
i32 CSBI_WellGoo::SerializeFields(CFileMemBase* arc, i32 mode, i32 a3, i32 a4) {
    if (arc == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr =
        g_gameReg->m_world; // cached across the calls (retail spills it @[esp+0x14])
    if (mgr == 0) {
        return 0;
    }
    // QUALIFIED = the direct CSBI_Image base leg (0xe6e40); unqualified is recursion.
    if (CSBI_Image::SerializeFields(arc, mode, a3, a4) == 0) {
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
                mgr->m_imageRegistry->AnyValueMatches(m_fgFrame, buf, &idx);
            }
            arc->Write(buf, 0x80);
            arc->Write(&idx, 4);
            g_serialCounter++;
            memset(buf, 0, 0x80);
            idx = 0;
            if (m_baseFrame != 0) {
                mgr->m_imageRegistry->AnyValueMatches(m_baseFrame, buf, &idx);
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
                CObject* found = 0;
                mgr->m_imageRegistry->m_10map.Lookup(buf, found);
                CDDrawWorker* set = static_cast<CDDrawWorker*>(found);
                if (set != 0) {
                    m_fgFrame = set->GetAt(idx); // same bounds gate, inline
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
                CObject* found = 0;
                mgr->m_imageRegistry->m_10map.Lookup(buf, found);
                CDDrawWorker* set = static_cast<CDDrawWorker*>(found);
                if (set != 0) {
                    m_baseFrame = set->GetAt(idx); // same bounds gate, inline
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
            m_gooSrc = mgr->m_ptrColl->MakeAndAddB(0x14, 5, 0x10, 0, -1);
            if (m_gooSrc == 0) {
                return 0;
            }
            i32 sel = g_gameReg->m_options[g_curPlayer]
                          .m_008; // ex the '+0x158 selector table' raw read (0x150 + 8 + i*0x238)
            CShadeTable* node = g_gameReg->m_spriteFactory->GetSel(sel, 0);
            if (node == 0) {
                node = g_gameReg->m_spriteFactory->GetSel(1, 0);
            }
            CImage* fr = m_frame;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && m_frame->m_owned != 0) {
                m_frame->m_owned->m_palDescr = node;
            }
            fr = m_baseFrame;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && m_baseFrame->m_owned != 0) {
                m_baseFrame->m_owned->m_palDescr = node;
            }
            fr = m_fgFrame;
            if (fr->m_owned != 0) {
                fr->m_owned->Select(0xa, 0);
            }
            if (node != 0 && m_fgFrame->m_owned != 0) {
                m_fgFrame->m_owned->m_palDescr = node;
            }
            break;
        }
    }
    return 1;
}

RVA_COMPGEN(0x00104b80, 0x1e, ??_GCSBI_WellGoo@@UAEPAXI@Z)
RVA(0x00104bb0, 0x94)
CSBI_WellGoo::~CSBI_WellGoo() {
    if (m_gooSrc != 0) {
        m_24->m_ptrColl->RemoveItemA(m_gooSrc);
        m_gooSrc = 0;
    }
}

RVA(0x00104c80, 0x1f)
void CSBI_WellGoo::Reset() {
    if (m_gooSrc != 0) {
        m_24->m_ptrColl->RemoveItemA(m_gooSrc);
        m_gooSrc = 0;
    }
}
