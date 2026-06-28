// CImageSpriteBlit.cpp - the five CImage sprite blit/clip routines
// (0x1538c0..0x1544d0). Each one:
//   * computes the on-screen sprite rect from the worker's draw position
//     (info->m_drawX/m_drawY), the sprite anchor (this->m_18/m_1c) and origin
//     (this->m_20/m_24) - the X/Y anchor signs encode the horizontal / vertical
//     flip variant;
//   * optionally remaps the origin through info->m_transform (bit 0x40000);
//   * clips the rect against either the parent clip RECT (bit 0x40000) or the
//     worker clip box (info->m_clipLeft.. or the 0x80000000 sentinel);
//   * builds the matching source sub-rect and blits via CDDSurface::BltEx (the
//     three "surface" variants, mode word in g_severusScratch[1]) or
//     CDDrawShadeBlit::Blit (the two "shaded" variants, with an optional
//     pre-notify when info->m_notifyGate is set);
//   * records the clipped rect + dimensions back into the worker.
//
// `this` is the source CImage, `info` the blit request (esi), `dst` the
// destination CImage (arg2). Offsets + emitted bytes are load-bearing. The
// blit/notify/transform callees are external no-body fns (reloc-masked rel32);
// CopyRect is the user32 import.
#include <rva.h>
#include <Image/CImage.h>
#include <Win32.h> // RECT, CopyRect

// The 25-int severus scratch block (shared with CDDrawWorkerRegistry); [1] carries
// the BltEx blend-mode word, the base is the DDBLTFX-style fx pointer.
extern i32 g_severusScratch[25];

// The origin-remap target reached through info->m_transform->m_renderer
// (bit 0x40000): the
// world-coordinate wrap/transform CPlaneRender::WrapCoord (0xa000, reached via the
// 0x295a ILT thunk; reloc-masked). Modeled with its real mangling.
class CPlaneRender {
public:
    void WrapCoord(i32* px, i32* py); // 0xa000
};
struct CBlitXform {
    char _00[0x5c];
    CPlaneRender* m_renderer; // +0x5c
};

// The blit request the worker hands in (esi). Inputs: flags, anchor adjust,
// origin transform, shaded pre-notify args, draw position, and clip box/sentinel.
// Outputs: clipped rect, dimensions, and result code.
class CBlitInfo {
public:
    char _00[0x08];
    i32 m_flags; // +0x08  flags (bit 0x40000)
    char _0c[0x10 - 0x0c];
    i32 m_anchorAdjustX;     // +0x10  anchor adjust x
    i32 m_anchorAdjustY;     // +0x14  anchor adjust y
    i32 m_clippedLeft;       // +0x18  out: clipped left (top-left point)
    i32 m_clippedTop;        // +0x1c  out: clipped top
    BlitRect m_clippedRect;  // +0x20  out: clipped rect {left, top, right, bottom}
    i32 m_clippedWidth;      // +0x30  out: width
    i32 m_clippedHeight;     // +0x34  out: height
    i32 m_result;            // +0x38  out: result (0 ok / -1 culled)
    CBlitXform* m_transform; // +0x3c  origin transform
    char _40[0x4c - 0x40];
    i32 m_notifyArg1; // +0x4c  pre-notify arg
    i32 m_notifyArg0; // +0x50  pre-notify arg
    char _54[0x58 - 0x54];
    i32 m_notifyGate; // +0x58  pre-notify gate
    i32 m_drawX;      // +0x5c  draw x
    i32 m_drawY;      // +0x60  draw y
    i32 m_clipLeft;   // +0x64  clip left / 0x80000000 sentinel
    i32 m_clipTop;    // +0x68  clip top
    i32 m_clipRight;  // +0x6c  clip right
    i32 m_clipBottom; // +0x70  clip bottom
};

// The two reloc-masked blit backends, modeled with the same mangling as their real
// definitions (src/Gruntz/CDirectDrawMgr.cpp / CDDrawShadeBlit.cpp) so the rel32
// call symbols correlate.
class CDDSurface {
public:
    i32 BltEx(void* dstRect, CDDSurface* src, void* srcRect, u32 flags, void* fx); // 0x13eef0
};
struct ShadeRect;
struct ShadeSrc;
class CDDrawShadeBlit {
public:
    i32 Blit(ShadeRect* dst, ShadeSrc* src, ShadeRect* clip, i32 sel, i32 p4); // 0x1497f0
    void Notify(i32 a, i32 b);                                                 // 0x14dd90
};

// ---------------------------------------------------------------------------
// 0x1538c0 - no flip, surface blit (BltEx, blend mode 6).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct. Residual = 4 origin-load insns whose this-member->register
// assignment differs (the loaded members die right after the subtract chain, so
// the pick is a whole-function regalloc tie-break MSVC5 resolves differently than
// retail) + the WrapCoord (0x295a ILT thunk) / CopyRect (IAT import) reloc-name
// operand artifacts. Code bytes otherwise byte-exact (clip + struct-copy end).
RVA(0x001538c0, 0x257)
void CImage::BlitNorm(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_20 - info->m_anchorAdjustX - m_18;
    i32 y = info->m_drawY - m_24 - info->m_anchorAdjustY - m_1c;
    if (info->m_flags & 0x40000) {
        info->m_transform->m_renderer->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_0c->m_24->m_10;
        RECT clip;
        CopyRect(&clip, (const RECT*)&clipA);
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == (i32)0x80000000) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_10) {
            d.right = dst->m_10 - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_14) {
            d.bottom = dst->m_14 - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_severusScratch[1] = 6;
    ((CDDSurface*)dst->m_2c)->BltEx(&d, (CDDSurface*)m_2c, &s, 0x8800, g_severusScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_clippedLeft = d.left;
    info->m_clippedTop = d.top;
    info->m_clippedRect = *(BlitRect*)&d;
    info->m_clippedWidth = w;
    info->m_clippedHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// 0x153b20 - vertical flip, surface blit (BltEx, blend mode 2).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct (formulas verified against retail). The vertical flip makes
// the Y anchor a mixed-sign chain (m_24 - m_1c + anchorAdjustY + drawY); MSVC5
// reassociates it to (anchorAdjustY + m_24 + drawY) - m_1c and picks a different
// Y-accumulator base than retail, which co-schedules the X subtract chain. That
// one divergence cascades through the whole function (no source spelling pins the
// reassociation - compound-assign / anchor-temp / x<->y reorder all tried). Plus
// the WrapCoord/CopyRect reloc-name artifacts.
RVA(0x00153b20, 0x270)
void CImage::BlitFlipV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - info->m_anchorAdjustX - m_18 - m_20;
    i32 y = m_24 - m_1c + info->m_anchorAdjustY + info->m_drawY;
    if (info->m_flags & 0x40000) {
        info->m_transform->m_renderer->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_0c->m_24->m_10;
        RECT clip;
        CopyRect(&clip, (const RECT*)&clipA);
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == (i32)0x80000000) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_10) {
            d.right = dst->m_10 - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_14) {
            d.bottom = dst->m_14 - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_severusScratch[1] = 2;
    ((CDDSurface*)dst->m_2c)->BltEx(&d, (CDDSurface*)m_2c, &s, 0x8800, g_severusScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_clippedLeft = d.left;
    info->m_clippedTop = d.top;
    info->m_clippedRect = *(BlitRect*)&d;
    info->m_clippedWidth = w;
    info->m_clippedHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// 0x153d90 - horizontal flip, surface blit (BltEx, blend mode 4).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct. Same wall as BlitFlipV: the horizontal flip makes X a
// mixed-sign chain (anchorAdjustX - m_18 + m_20 + drawX) that MSVC5 reassociates
// vs retail, cascading the co-scheduled X/Y register assignment. Plus the
// WrapCoord/CopyRect reloc-name artifacts.
RVA(0x00153d90, 0x259)
void CImage::BlitFlipH(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_anchorAdjustX - m_18 + m_20 + info->m_drawX;
    i32 y = info->m_drawY - m_24 - m_1c - info->m_anchorAdjustY;
    if (info->m_flags & 0x40000) {
        info->m_transform->m_renderer->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_0c->m_24->m_10;
        RECT clip;
        CopyRect(&clip, (const RECT*)&clipA);
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == (i32)0x80000000) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_10) {
            d.right = dst->m_10 - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_14) {
            d.bottom = dst->m_14 - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_severusScratch[1] = 4;
    ((CDDSurface*)dst->m_2c)->BltEx(&d, (CDDSurface*)m_2c, &s, 0x8800, g_severusScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_clippedLeft = d.left;
    info->m_clippedTop = d.top;
    info->m_clippedRect = *(BlitRect*)&d;
    info->m_clippedWidth = w;
    info->m_clippedHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// 0x154270 - no flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 1/1).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct, ~99.85%. Residual = the same 4 origin-load regalloc-tiebreak
// insns + 3 reloc-name operand artifacts (WrapCoord ILT thunk, CopyRect IAT import,
// the 0x14dd90 pre-notify whose retail symbol is the no-arg ClassUnknown_11 alias
// while the call site is a 2-arg thiscall). All other code bytes are byte-exact -
// the inclusive-rect struct-copy end-store matches retail exactly.
RVA(0x00154270, 0x257)
void CImage::BlitShadeNorm(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_20 - m_18 - info->m_anchorAdjustX;
    i32 y = info->m_drawY - m_24 - m_1c - info->m_anchorAdjustY;
    if (info->m_flags & 0x40000) {
        info->m_transform->m_renderer->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_0c->m_24->m_10;
        RECT clip;
        CopyRect(&clip, (const RECT*)&clipA);
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == (i32)0x80000000) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_10) {
            d.right = dst->m_10 - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_14) {
            d.bottom = dst->m_14 - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_notifyGate) {
        ((CDDrawShadeBlit*)m_30)->Notify(info->m_notifyArg0, info->m_notifyArg1);
    }
    ((CDDrawShadeBlit*)m_30)->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_2c, (ShadeRect*)&s, 1, 1);
    info->m_clippedLeft = d.left;
    info->m_clippedTop = d.top;
    info->m_clippedRect = *(BlitRect*)&d;
    info->m_clippedWidth = w;
    info->m_clippedHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// 0x1544d0 - vertical flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 1/0).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct - the X/Y formulas already match retail's operation order.
// The wall is pure whole-function regalloc: retail pins `this` in EBX (`mov ebx,ecx`,
// push edi later) where our cl picks EDI, and reorders the all-sub X chain to a
// different this-member->register mapping; that one prologue choice shifts every
// downstream register. Plus the WrapCoord/CopyRect/0x14dd90 reloc-name artifacts.
// End-store struct-copy matches retail.
RVA(0x001544d0, 0x275)
void CImage::BlitShadeFlipV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_18 - info->m_anchorAdjustX - m_20;
    i32 y = m_24 + info->m_anchorAdjustY + info->m_drawY - m_1c;
    if (info->m_flags & 0x40000) {
        info->m_transform->m_renderer->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_0c->m_24->m_10;
        RECT clip;
        CopyRect(&clip, (const RECT*)&clipA);
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == (i32)0x80000000) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_10) {
            d.right = dst->m_10 - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_14) {
            d.bottom = dst->m_14 - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_notifyGate) {
        ((CDDrawShadeBlit*)m_30)->Notify(info->m_notifyArg0, info->m_notifyArg1);
    }
    ((CDDrawShadeBlit*)m_30)->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_2c, (ShadeRect*)&s, 1, 0);
    info->m_clippedLeft = d.left;
    info->m_clippedTop = d.top;
    info->m_clippedRect = *(BlitRect*)&d;
    info->m_clippedWidth = w;
    info->m_clippedHeight = h;
    info->m_result = 0;
}
