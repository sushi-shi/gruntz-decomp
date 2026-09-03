#include <rva.h>

#include <Gruntz/GlyphStringDraw.h>

#include <MfcWin.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDSurface.h>
#include <Gruntz/GameRegistry.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Wap32/EngStr.h>

#include <string.h>

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
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
        CImage* glyph = font->GetAt(c);
        if (glyph) {
            ctx->m_workerList->CreateFrameWorker(x, y, glyph, 0);
        }
        x += advance;
    }
    return 1;
}

RVA(0x00115300, 0xf5)
i32 LayerBlitFrame(
    CDDrawSurfaceMgr* surfaceMgr,
    CImage* src,
    i32 x,
    i32 y,
    b32 useFront,
    b32 useColorKey
) {
    if (!surfaceMgr) {
        return 0;
    }
    if (!src) {
        return 0;
    }

    CDrawSubWorker* node;
    if (useFront) {
        node = surfaceMgr->m_drawTarget->m_frontSurface;
        if (!node) {
            return 0;
        }
    } else {
        node = surfaceMgr->m_drawTarget->m_backPair;
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
    CPoint destination(x - src->m_anchor.x, y - src->m_anchor.y);
    CRect rc(0, 0, src->m_width - 1, src->m_height - 1);
    CRect rc2 = rc;
    u32 flags = DDBLTFAST_WAIT;
    if (useColorKey) {
        flags |= DDBLTFAST_SRCCOLORKEY;
    }
    dst->BltFast(destination.x, destination.y, srcHandle, &rc2, flags);
    return 1;
}

RVA(0x00115440, 0x45)
i32 DrawTextToFrontSurface(
    CDDrawSurfaceMgr* surfaceMgr,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
) {
    CDDrawFrontSurface* frontSurface = surfaceMgr->m_drawTarget->m_frontSurface;

    if (frontSurface == NULL) {
        return 0;
    }
    return EngStr_RenderText(
        surfaceMgr,
        text,
        box,
        frontSurface->m_surface,
        fontSel,
        shadow,
        r,
        g,
        b,
        flag
    );
}

RVA(0x001154b0, 0x45)
i32 DrawTextToOverlaySurface(
    CDDrawSurfaceMgr* surfaceMgr,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
) {
    CDDrawSurfacePair* overlaySurface = surfaceMgr->m_drawTarget->m_overlayPair;

    if (overlaySurface == NULL) {
        return 0;
    }
    return EngStr_RenderText(
        surfaceMgr,
        text,
        box,
        overlaySurface->m_surface,
        fontSel,
        shadow,
        r,
        g,
        b,
        flag
    );
}
RVA(0x00115520, 0x45)
i32 DrawTextToBackSurface(
    CDDrawSurfaceMgr* surfaceMgr,
    CString* text,
    RECT* box,
    i32 fontSel,
    i32 shadow,
    i32 r,
    i32 g,
    i32 b,
    i32 flag
) {
    CDDrawSurfacePair* backSurface = surfaceMgr->m_drawTarget->m_backPair;
    if (backSurface == NULL) {
        return 0;
    }
    return EngStr_RenderText(
        surfaceMgr,
        text,
        box,
        backSurface->m_surface,
        fontSel,
        shadow,
        r,
        g,
        b,
        flag
    );
}
