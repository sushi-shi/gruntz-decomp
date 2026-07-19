// GlyphStringDraw.cpp - DrawGlyphString (0x115220), a __cdecl text helper: walks a
// string, maps each char through a CImageSet's [m_minIndex..m_maxIndex] frame range,
// and creates a sprite worker per printable glyph on the holder's per-frame worker pump.
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
#include <Image/CImage.h>        // CImage - the 0x115300 blit source + the CImageSet frame element
#include <Image/ImageSet.h> // CImageSet - the glyph atlas (m_frames@+0x14, min/max frame index @+0x64/+0x68)

// The drawable is CDDrawSurfaceMgr::m_workerList (a CDDrawWorkerList, +0x0c): each
// printable glyph is a frame index into the CImageSet, and the per-glyph "draw" is really
// CDDrawWorkerList::CreateWorkerB28 (vtable slot 10 / +0x28) - it spawns a text-sprite
// worker at (x,y) rendering the CImage* frame (passed as an opaque i32 handle). The old
// Drawable(10 placeholder virtuals)/DrawCtx/GlyphFont views are dissolved onto these
// canonical classes.
RVA(0x00115220, 0xa4)
i32 DrawGlyphString(
    CDDrawSurfaceMgr* ctx,
    i32 x,
    i32 y,
    const char* str,
    CImageSet* font,
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
        i32 c = (signed char)str[i];
        i32 glyph;
        if (c >= font->m_minIndex && c <= font->m_maxIndex) {
            glyph = reinterpret_cast<i32>((CImage*)font->m_items.GetAt(c)); // the CImage* frame, as an opaque worker-factory handle
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

// ---------------------------------------------------------------------------
// LayerBlitFrame (0x115300) - blit a CImage frame into the active draw-target layer,
// CENTERED by the frame's draw anchor. RVA-adjacent to DrawGlyphString; also called
// cross-TU from src/Gruntz/PlayMessageImage.cpp (CPlay's GAME_MESSAGEZ overlay). Pick the
// front (useFront) SurfaceA page or the back SurfaceB page off the CDDrawSurfaceMgr's CDDrawSubMgrPages,
// get its CDDSurface, subtract the frame's anchor from (x,y), and BltFast the frame
// surface into it. The four ApiCallerStubs LayerHost/LayerSet/LayerNode/RectSrc views
// were fake facets of CDDrawSurfaceMgr / CDDrawSubMgrPages / its Surface pages / CImageFrame - now
// dissolved onto those real classes. Re-homed from src/Stub/ApiCallers.cpp.
//
// The +0x18/+0x1c it subtracts are CImage's m_anchorX/m_anchorY, NOT the origin: retail's
// CImage::Create (0x152e90) fills them with `width>>1` / `height>>1` (`sar edx,1`) - a
// half-extent CENTER - while the true m_originX/m_originY live at +0x20/+0x24. The old
// CImageFrame view mislabelled +0x18 "m_originX"; its own comment ("the layer-blit
// centers by it") already described an anchor.
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

// ---------------------------------------------------------------------------
// ShowHudMessage (0x1154b0) + its +0x14-slot twin (0x115520): the shared HUD
// message-sprite helpers (re-homed from src/Stub/Discovered.cpp). Identity recovered from
// GameMode.cpp / BootyMessages.cpp, which call 0x1154b0 as `ShowHudMessage(m_c, ...)` where
// m_c is the inherited CState render holder: `sink` IS CDDrawSurfaceMgr. Each reads a
// CDDrawSurfacePair page off the holder's CDDrawSubMgrPages (+0x04) - the present page (m_18) for
// ShowHudMessage, the draw page (m_14) for the twin - and if non-null forwards all 9 stack
// args to the 10-arg __cdecl push helper (0x115930 via the 0x1262 ILT), inserting the page's
// target surface (m_surface @+0x2c, a CDDSurface* - same field LayerBlitFrame blits into) as
// the 4th argument. The former HudMsgSink/HudMsgInner/HudMsgHandler offset views are dissolved
// onto CDDrawSurfaceMgr / CDDrawSubMgrPages / CDDrawSurfacePair.
extern "C" void HudMsgPush(
    CDDrawSurfaceMgr* sink,
    i32 a2,
    i32 a3,
    CDDSurface* surf,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
); // 0x115930

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

// --- vtable catalog ---
