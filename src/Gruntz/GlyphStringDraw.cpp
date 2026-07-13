// GlyphStringDraw.cpp - DrawGlyphString (0x115220), a __cdecl text helper: walks
// a string, maps each char through a glyph font's [lo..hi] range table, and
// dispatches a draw call per printable glyph onto the context's drawable.
#include <Ints.h>
#include <rva.h>

#include <string.h>             // strlen
#include <DDrawMgr/DDSurface.h> // CDDSurface (BltFast) + RECT/SetRect (via Mfc.h) for the layer-blit helper
#include <DDrawMgr/DDrawSurfacePair.h> // the CDrawTarget pages (real class of m_10/m_14/m_18)
#include <Gruntz/ResMgr.h> // CResMgr / CDrawTarget (SurfaceA/SurfaceB) - the 0x115300 blit host
#include <Image/CImage.h>  // CImage - the 0x115300 blit source (was the CImageFrame view)

struct Drawable {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void Draw(i32 x, i32 y, i32 glyph, i32 z); // slot +0x28
};

struct DrawCtx {
    char pad00[0xc];
    Drawable* m_0c; // +0x0c
};

struct GlyphFont {
    char pad00[0x14];
    i32* m_14; // +0x14 glyph table
    char pad18[0x64 - 0x18];
    i32 m_64; // +0x64 lo char
    i32 m_68; // +0x68 hi char
};

RVA(0x00115220, 0xa4)
i32 DrawGlyphString(DrawCtx* ctx, i32 x, i32 y, const char* str, GlyphFont* font, i32 advance) {
    if (!ctx) {
        return 0;
    }
    if (!str) {
        return 0;
    }
    if (!font) {
        return 0;
    }
    i32 len = (i32)strlen(str);
    if (len <= 0) {
        return 0;
    }
    for (i32 i = 0; i < len; i++) {
        i32 c = (signed char)str[i];
        i32 glyph;
        if (c >= font->m_64 && c <= font->m_68) {
            glyph = font->m_14[c];
        } else {
            glyph = 0;
        }
        if (glyph) {
            ctx->m_0c->Draw(x, y, glyph, 0);
        }
        x += advance;
    }
    return 1;
}

SIZE_UNKNOWN(Drawable);
SIZE_UNKNOWN(DrawCtx);
SIZE_UNKNOWN(GlyphFont);

// ---------------------------------------------------------------------------
// LayerBlitFrame (0x115300) - blit a CImage frame into the active draw-target layer,
// CENTERED by the frame's draw anchor. RVA-adjacent to DrawGlyphString; also called
// cross-TU from src/Gruntz/PlayMessageImage.cpp (CPlay's GAME_MESSAGEZ overlay). Pick the
// front (useFront) SurfaceA page or the back SurfaceB page off the CResMgr's CDrawTarget,
// get its CDDSurface, subtract the frame's anchor from (x,y), and BltFast the frame
// surface into it. The four ApiCallerStubs LayerHost/LayerSet/LayerNode/RectSrc views
// were fake facets of CResMgr / CDrawTarget / its Surface pages / CImageFrame - now
// dissolved onto those real classes. Re-homed from src/Stub/ApiCallers.cpp.
//
// The +0x18/+0x1c it subtracts are CImage's m_anchorX/m_anchorY, NOT the origin: retail's
// CImage::Create (0x152e90) fills them with `width>>1` / `height>>1` (`sar edx,1`) - a
// half-extent CENTER - while the true m_originX/m_originY live at +0x20/+0x24. The old
// CImageFrame view mislabelled +0x18 "m_originX"; its own comment ("the layer-blit
// centers by it") already described an anchor.
RVA(0x00115300, 0xf5)
i32 LayerBlitFrame(CResMgr* host, CImage* src, i32 x, i32 y, i32 useFront, i32 mode) {
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
        node = host->m_drawTarget->m_10; // same class as m_14 - the old SurfaceA->SurfaceB cast is gone
        if (!node) {
            return 0;
        }
    } else {
        node = host->m_drawTarget->m_14;
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
// message-sprite helpers (re-homed from src/Stub/Discovered.cpp, where the this/ecx
// trace mis-attributed them to a placeholder "tomalla-13"). Identity recovered
// from GameMode.cpp / BootyMessages.cpp, which already call 0x1154b0 as
// `ShowHudMessage` ("push a transient text sprite carrying `text` into `rect`");
// Ghidra places both in this font/text-draw cluster (between EngStr_DrawText @0x115440
// and the CopyRect/OffsetRect rect helper @0x115930 they forward to). Each reads a
// handler off sink->m_4 (at +0x18 / +0x14) and, if non-null, forwards all 9 stack args
// to the 10-arg __cdecl callee (0x115930 via the 0x1262 ILT), inserting the handler's
// m_2c as the 4th argument. `sink` is a stack arg, NOT this - free __cdecl helpers.
SIZE_UNKNOWN(HudMsgHandler);
struct HudMsgHandler {
    char m_pad00[0x2c];
    i32 m_2c; // +0x2c
};
SIZE_UNKNOWN(HudMsgInner);
struct HudMsgInner {
    char m_pad00[0x14];
    HudMsgHandler* m_14; // +0x14
    HudMsgHandler* m_18; // +0x18
};
SIZE_UNKNOWN(HudMsgSink);
struct HudMsgSink {
    char m_pad00[4];
    HudMsgInner* m_4; // +0x04
};
extern "C" void HudMsgPush(
    HudMsgSink* sink,
    i32 a2,
    i32 a3,
    i32 m,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
); // 0x115930

// @early-stop
// tail-merge / block-layout wall (~94.3%): the handler load + the 10-arg
// `[esp+0x24]`-reload forwarding push chain + the call/`add esp` are byte-IDENTICAL;
// the sole residual is the null guard, where retail emits `jne body; ret` (a separate
// early ret, no tail-merge) but cl tail-merges the two rets to `je <shared end ret>`.
// An MSVC5 block-ordering coin-flip; not source-steerable.
RVA(0x001154b0, 0x45)
void ShowHudMessage(
    HudMsgSink* sink,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
) {
    HudMsgHandler* h = sink->m_4->m_18;
    if (h == 0) {
        return;
    }
    HudMsgPush(sink, a2, a3, h->m_2c, a4, a5, a6, a7, a8, a9);
}
// @early-stop
// same tail-merge wall as ShowHudMessage (twin; inner handler at +0x14 vs +0x18).
RVA(0x00115520, 0x45)
void ShowHudMessageAlt(
    HudMsgSink* sink,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8,
    i32 a9
) {
    HudMsgHandler* h = sink->m_4->m_14;
    if (h == 0) {
        return;
    }
    HudMsgPush(sink, a2, a3, h->m_2c, a4, a5, a6, a7, a8, a9);
}

// --- vtable catalog ---
