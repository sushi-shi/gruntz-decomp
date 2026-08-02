#include <Wap32/EngStr.h>
#include <Gruntz/GlyphStringDraw.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Ints.h>
#include <rva.h>
#include <AddrWord.h>

#include <string.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>

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

            AddrWord<CImage> g;
            g.m_addr = static_cast<CImage*>(font->m_items.GetAt(c));
            glyph = g.m_word;
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

    CDrawSubWorker* node;
    if (useFront) {
        node = host->m_drawTarget->m_frontPair;
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
RVA(0x001154b0, 0x45)
void ShowHudMessage(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
) {
    CDDrawSurfacePair* page = sink->m_drawTarget->m_overlayPair;

    if (page == 0) {
        return;
    }
    EngStr_RenderText(sink, text, box, page->m_surface, fontSel, shadow, r, g, b, flag);
}
// @early-stop
RVA(0x00115520, 0x45)
void ShowHudMessageAlt(
    CDDrawSurfaceMgr* sink,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
) {
    CDDrawSurfacePair* page = sink->m_drawTarget->m_backPair;
    if (page == 0) {
        return;
    }
    EngStr_RenderText(sink, text, box, page->m_surface, fontSel, shadow, r, g, b, flag);
}
