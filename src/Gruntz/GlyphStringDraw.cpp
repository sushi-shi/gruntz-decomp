#include <Gruntz/GlyphStringDraw.h> // own header (the moved thunk-name decls)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <Ints.h>
#include <rva.h>

#include <string.h>             // strlen
#include <DDrawMgr/DDSurface.h> // CDDSurface (BltFast) + RECT/SetRect (via Mfc.h) for the layer-blit helper
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18 + m_surface@+0x2c)
#include <DDrawMgr/DDrawWorkerList.h> // CDDrawWorkerList - the +0x0c worker pump (Draw@slot10 = CreateWorkerB28)
#include <DDrawMgr/DDrawSurfaceMgr.h> // CDDrawSurfaceMgr / CDDrawSubMgrPages (SurfaceA/SurfaceB pages) - the 0x115300 blit host
#include <Gruntz/GameRegistry.h> // CDDrawSurfaceMgr - the CState::m_c render/resource holder (ctx/sink)
#include <Image/CImage.h>        // CImage - the 0x115300 blit source + the CDDrawWorker frame element
#include <Image/ImageSet.h> // CDDrawWorker - the glyph atlas (m_frames@+0x14, min/max frame index @+0x64/+0x68)

RVA(0x00115220, 0xa4)
i32 DrawGlyphString(
    CDDrawSurfaceMgr* ctx,
    i32 x,
    i32 y,
    const char* str,
    CDDrawWorker* font,
    i32 advance
) {
    if (!ctx) {
        return 0;
    }
    if (!str) {
        return 0;
    }
    if (!font) {
        return 0;
    }
    i32 len = static_cast<i32>(strlen(str));
    if (len <= 0) {
        return 0;
    }
    for (i32 i = 0; i < len; i++) {
        i32 c = static_cast<signed char>(str[i]);
        i32 glyph;
        if (c >= font->m_minIndex && c <= font->m_maxIndex) {
            glyph = reinterpret_cast<i32>(static_cast<CImage*>(font->m_items.GetAt(c))); // the CImage* frame, as an opaque worker-factory handle
        } else {
            glyph = 0;
        }
        if (glyph) {
            ctx->m_workerList->CreateWorkerB28(x, y, glyph, 0);
        }
        x += advance;
    }
    return 1;
}

RVA(0x00115300, 0xf5)
i32 LayerBlitFrame(CDDrawSurfaceMgr* host, CImage* src, i32 x, i32 y, i32 useFront, i32 mode) {
    if (!host) {
        return 0;
    }
    if (!src) {
        return 0;
    }
    // Front page is the SurfaceA frame page, back is the SurfaceB draw page; both expose
    // their target surface at +0x2c (SurfaceA's Surface2c* is used as a CDDSurface here).
    CDDrawSurfacePair* node;
    if (useFront) {
        node = host->m_drawTarget->m_frontPair; // same class as m_backPair
        if (!node) {
            return 0;
        }
    } else {
        node = host->m_drawTarget->m_backPair;
        if (!node) {
            return 0;
        }
    }
    CDDSurface* dst = node->m_surface;
    if (!dst) {
        return 0;
    }
    CDDSurface* srcHandle = src->m_surface;
    if (!srcHandle) {
        return 0;
    }
    i32 dx = x - src->m_anchorX;
    i32 dy = y - src->m_anchorY;
    RECT rc;
    SetRect(&rc, 0, 0, src->m_width - 1, src->m_height - 1);
    RECT rc2 = rc;
    i32 flags = 0x10;
    if (mode) {
        flags = 0x11;
    }
    dst->BltFast(dx, dy, srcHandle, &rc2, flags);
    return 1;
}

// @early-stop
// tail-merge / block-layout wall (~94.3%): the page load + the 10-arg
// `[esp+0x24]`-reload forwarding push chain + the call/`add esp` are byte-IDENTICAL;
// the sole residual is the null guard, where retail emits `jne body; ret` (a separate
// early ret, no tail-merge) but cl tail-merges the two rets to `je <shared end ret>`.
// An MSVC5 block-ordering coin-flip; not source-steerable.
RVA(0x001154b0, 0x45)
void ShowHudMessage(
    CDDrawSurfaceMgr* sink,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
) {
    CDDrawSurfacePair* page = sink->m_drawTarget->m_overlayPair;
    if (page == 0) {
        return;
    }
    HudMsgPush(sink, a2, a3, page->m_surface, a4, a5, a6, a7, a8, a9);
}
// @early-stop
// same tail-merge wall as ShowHudMessage (twin; draw page m_14 vs present page m_18).
RVA(0x00115520, 0x45)
void ShowHudMessageAlt(
    CDDrawSurfaceMgr* sink,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
) {
    CDDrawSurfacePair* page = sink->m_drawTarget->m_backPair;
    if (page == 0) {
        return;
    }
    HudMsgPush(sink, a2, a3, page->m_surface, a4, a5, a6, a7, a8, a9);
}
