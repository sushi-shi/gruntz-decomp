// CImageSpriteBlit.cpp - the five CImage sprite blit/clip routines
// (0x1538c0..0x1544d0). Each one:
//   * computes the on-screen sprite rect from the worker's draw position
//     (info->m_drawX/m_drawY), the sprite anchor (this->m_anchorX/m_anchorY) and
//     origin (this->m_originX/m_originY) - the X/Y anchor signs encode the
//     horizontal / vertical flip variant;
//   * optionally remaps the origin through info->m_xform (bit 0x40000);
//   * clips the rect against either the parent clip RECT (bit 0x40000) or the
//     worker clip box (info->m_clipLeft.. or the 0x80000000 "use dest extents"
//     sentinel);
//   * builds the matching source sub-rect and blits via CSpriteDDSurface::BltEx (the
//     three "surface" variants, mode word in g_bltFxScratch[1]) or
//     CDDrawShadeBlit::Blit (the two "shaded" variants, with an optional
//     pre-notify when info->m_notify is set);
//   * records the clipped rect + dimensions back into the worker (info->m_outLeft..).
//
// `this` is the source CImage, `info` the blit request (esi), `dst` the
// destination CImage (arg2). Field names are placeholders; only offsets + emitted
// bytes are load-bearing. The blit/notify/transform callees are external no-body
// fns (reloc-masked rel32); CopyRect is the user32 import.
#include <rva.h>
#include <Image/CImage.h>
#include <Win32.h> // RECT, CopyRect

#include <DDrawMgr/CDDSurface.h>    // canonical CDDSurface (BltEx blit backend)
#include <Gruntz/CDDrawShadeBlit.h> // canonical CDDrawShadeBlit (+ ShadeRect/ShadeSrc)
#include <Globals.h>

// The 25-int g_bltFxScratch block (shared with CDDrawWorkerRegistry); [1] carries
// the BltEx blend-mode word, the base is the DDBLTFX-style fx pointer.

// The origin-remap target reached through info->m_xform->m_planeRender (bit 0x40000): the
// world-coordinate wrap/transform CSpritePlaneRender::WrapCoord (0xa000, reached via the
// 0x295a ILT thunk; reloc-masked). Modeled with its real mangling.
SIZE_UNKNOWN(CSpritePlaneRender);
class CSpritePlaneRender {
public:
    void WrapCoord(i32* px, i32* py); // 0xa000
};
struct CBlitXform {
    char _00[0x5c];
    CSpritePlaneRender* m_planeRender; // +0x5c  coordinate-wrap plane renderer
};

// The blit request the worker hands in (esi). Inputs: m_flags, m_adjustX/m_adjustY
// (draw-position adjust), m_xform (origin transform), m_notifyArg1/m_notifyArg0/
// m_notify (shaded pre-notify), m_drawX/m_drawY (draw position), m_clipLeft..
// m_clipBottom (clip box / sentinel). Outputs: m_outLeft..m_result (clipped rect,
// dims, result code).
class CBlitInfo {
public:
    char _00[0x08];
    i32 m_flags; // +0x08  flags (bit 0x40000)
    char _0c[0x10 - 0x0c];
    i32 m_adjustX;       // +0x10  draw-position adjust x
    i32 m_adjustY;       // +0x14  draw-position adjust y
    i32 m_outLeft;       // +0x18  out: clipped left (top-left point)
    i32 m_outTop;        // +0x1c  out: clipped top
    BlitRect m_outRect;  // +0x20  out: clipped rect {left, top, right, bottom}
    i32 m_outWidth;      // +0x30  out: width
    i32 m_outHeight;     // +0x34  out: height
    i32 m_result;        // +0x38  out: result (0 ok / -1 culled)
    CBlitXform* m_xform; // +0x3c  origin transform
    char _40[0x4c - 0x40];
    i32 m_notifyArg1; // +0x4c  shade pre-notify arg (Notify 2nd)
    i32 m_notifyArg0; // +0x50  shade pre-notify arg (Notify 1st)
    char _54[0x58 - 0x54];
    i32 m_notify;     // +0x58  shade pre-notify gate
    i32 m_drawX;      // +0x5c  draw x
    i32 m_drawY;      // +0x60  draw y
    i32 m_clipLeft;   // +0x64  clip left / 0x80000000 sentinel
    i32 m_clipTop;    // +0x68  clip top
    i32 m_clipRight;  // +0x6c  clip right
    i32 m_clipBottom; // +0x70  clip bottom
};

// The blit backends (CDDSurface::BltEx @0x13eef0, CDDrawShadeBlit::Blit @0x1497f0)
// come from the canonical shared headers above; reloc-masked rel32 call symbols.

// ---------------------------------------------------------------------------
// No flip, surface blit (BltEx, blend mode 6).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct. Residual = 4 origin-load insns whose this-member->register
// assignment differs (the loaded members die right after the subtract chain, so
// the pick is a whole-function regalloc tie-break MSVC5 resolves differently than
// retail) + the WrapCoord (0x295a ILT thunk) / CopyRect (IAT import) reloc-name
// operand artifacts. Code bytes otherwise byte-exact (clip + struct-copy end).
RVA(0x001538c0, 0x257)
void CImage::BlitNorm(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_originX - info->m_adjustX - m_anchorX;
    i32 y = info->m_drawY - m_originY - info->m_adjustY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
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
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
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
    g_bltFxScratch[1] = 6;
    ((CDDSurface*)dst->m_surface)->BltEx(&d, (CDDSurface*)m_surface, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(BlitRect*)&d;
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// Vertical flip, surface blit (BltEx, blend mode 2).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct (formulas verified against retail). The vertical flip makes
// the Y anchor a mixed-sign chain (m_originY - m_anchorY + m_adjustY + m_drawY);
// MSVC5 reassociates it to (m_adjustY + m_originY + m_drawY) - m_anchorY and picks a
// different Y-accumulator base than
// retail, which co-schedules the X subtract chain into different registers. That
// one divergence cascades through the whole function (no source spelling pins the
// reassociation - compound-assign / anchor-temp / x<->y reorder all tried). Plus
// the WrapCoord/CopyRect reloc-name artifacts.
RVA(0x00153b20, 0x270)
void CImage::BlitFlipV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - info->m_adjustX - m_anchorX - m_originX;
    i32 y = m_originY - m_anchorY + info->m_adjustY + info->m_drawY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
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
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
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
    g_bltFxScratch[1] = 2;
    ((CDDSurface*)dst->m_surface)->BltEx(&d, (CDDSurface*)m_surface, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(BlitRect*)&d;
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// Horizontal flip, surface blit (BltEx, blend mode 4).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct. Same wall as BlitFlipV: the horizontal flip makes X a
// mixed-sign chain (m_adjustX - m_anchorX + m_originX + m_drawX) that MSVC5 reassociates + reorders
// vs retail, cascading the co-scheduled X/Y register assignment. Plus the
// WrapCoord/CopyRect reloc-name artifacts.
RVA(0x00153d90, 0x259)
void CImage::BlitFlipH(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_adjustX - m_anchorX + m_originX + info->m_drawX;
    i32 y = info->m_drawY - m_originY - m_anchorY - info->m_adjustY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
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
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
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
    g_bltFxScratch[1] = 4;
    ((CDDSurface*)dst->m_surface)->BltEx(&d, (CDDSurface*)m_surface, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(BlitRect*)&d;
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// X+Y flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 0/0).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct - the fourth member of the shaded family, structurally
// identical to BlitShadeNorm/FlipV. Both anchor axes are flipped (X: the
// m_anchorX/m_originX signs; Y: the m_originY/m_adjustY/m_anchorY mixed-sign chain), so it inherits the
// SAME whole-function regalloc/reassociation wall the other flip variants hit:
// MSVC5 reassociates the mixed-sign X/Y accumulator chains and picks a different
// this-member->register mapping than retail, cascading downstream. Plus the
// WrapCoord (0x295a ILT thunk) / CopyRect (IAT import) / 0x14dd90 pre-notify
// reloc-name operand artifacts. Clip + inclusive-rect struct-copy end match.
RVA(0x00153ff0, 0x280)
void CImage::BlitShadeFlipHV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_anchorX + m_originX + info->m_adjustX;
    i32 y = info->m_drawY - m_anchorY + m_originY + info->m_adjustY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
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
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
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
    if (info->m_notify) {
        ((CDDrawShadeBlit*)m_owned)->Notify(info->m_notifyArg0, info->m_notifyArg1);
    }
    ((CDDrawShadeBlit*)m_owned)
        ->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_surface, (ShadeRect*)&s, 0, 0);
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(BlitRect*)&d;
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// No flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 1/1).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct, ~99.85%. Residual = the same 4 origin-load regalloc-tiebreak
// insns + 3 reloc-name operand artifacts (WrapCoord ILT thunk, CopyRect IAT import,
// the 0x14dd90 pre-notify whose retail symbol is the no-arg ClassUnknown_11 alias
// while the call site is a 2-arg thiscall). All other code bytes are byte-exact -
// the inclusive-rect struct-copy end-store matches retail exactly.
RVA(0x00154270, 0x257)
void CImage::BlitShadeNorm(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_originX - m_anchorX - info->m_adjustX;
    i32 y = info->m_drawY - m_originY - m_anchorY - info->m_adjustY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
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
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
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
    if (info->m_notify) {
        ((CDDrawShadeBlit*)m_owned)->Notify(info->m_notifyArg0, info->m_notifyArg1);
    }
    ((CDDrawShadeBlit*)m_owned)
        ->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_surface, (ShadeRect*)&s, 1, 1);
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(BlitRect*)&d;
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// Vertical flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 1/0).
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
    i32 x = info->m_drawX - m_anchorX - info->m_adjustX - m_originX;
    i32 y = m_originY + info->m_adjustY + info->m_drawY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
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
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
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
    if (info->m_notify) {
        ((CDDrawShadeBlit*)m_owned)->Notify(info->m_notifyArg0, info->m_notifyArg1);
    }
    ((CDDrawShadeBlit*)m_owned)
        ->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_surface, (ShadeRect*)&s, 1, 0);
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(BlitRect*)&d;
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// X flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 0/1).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct - the shaded twin of BlitFlipH. The horizontal flip makes
// X the mixed-sign chain (m_adjustX + m_originX + m_drawX - m_anchorX) that MSVC5 reassociates +
// reorders vs retail, cascading the co-scheduled X/Y register assignment (same
// wall as the surface BlitFlipH). Plus the WrapCoord/CopyRect/0x14dd90 reloc-name
// operand artifacts. Clip + inclusive-rect struct-copy end match retail.
RVA(0x00154750, 0x275)
void CImage::BlitShadeFlipH(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_adjustX + m_originX + info->m_drawX - m_anchorX;
    i32 y = info->m_drawY - m_originY - info->m_adjustY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
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
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
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
    if (info->m_notify) {
        ((CDDrawShadeBlit*)m_owned)->Notify(info->m_notifyArg0, info->m_notifyArg1);
    }
    ((CDDrawShadeBlit*)m_owned)
        ->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_surface, (ShadeRect*)&s, 0, 1);
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(BlitRect*)&d;
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}
// Class-metadata annotations (EOF-hosted, /O2 sprite-blit TU).
SIZE_UNKNOWN(CBlitXform);
SIZE_UNKNOWN(CBlitInfo);
