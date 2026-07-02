// CImageSpriteBlit.cpp - the five CImage sprite blit/clip routines
// (0x1538c0..0x1544d0). Each one:
//   * computes the on-screen sprite rect from the worker's draw position
//     (info->m_5c/m_60), the sprite anchor (this->m_18/m_1c) and origin
//     (this->m_20/m_24) - the X/Y anchor signs encode the horizontal / vertical
//     flip variant;
//   * optionally remaps the origin through info->m_3c (bit 0x40000);
//   * clips the rect against either the parent clip RECT (bit 0x40000) or the
//     worker clip box (info->m_64.. or the 0x80000000 "use dest extents" sentinel);
//   * builds the matching source sub-rect and blits via CSpriteDDSurface::BltEx (the
//     three "surface" variants, mode word in g_bltFxScratch[1]) or
//     CDDrawShadeBlit::Blit (the two "shaded" variants, with an optional
//     pre-notify when info->m_58 is set);
//   * records the clipped rect + dimensions back into the worker (info->m_18..).
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

// The origin-remap target reached through info->m_3c->m_5c (bit 0x40000): the
// world-coordinate wrap/transform CSpritePlaneRender::WrapCoord (0xa000, reached via the
// 0x295a ILT thunk; reloc-masked). Modeled with its real mangling.
class CSpritePlaneRender {
public:
    void WrapCoord(i32* px, i32* py); // 0xa000
};
struct CBlitXform {
    char _00[0x5c];
    CSpritePlaneRender* m_5c; // +0x5c
};

// The blit request the worker hands in (esi). Inputs: m_08 (flags), m_10/m_14
// (anchor adjust), m_3c (origin transform), m_4c/m_50/m_58 (shaded pre-notify),
// m_5c/m_60 (draw position), m_64..m_70 (clip box / sentinel). Outputs: m_18..m_38
// (clipped rect, dims, result code). Placeholder layout.
class CBlitInfo {
public:
    char _00[0x08];
    i32 m_08; // +0x08  flags (bit 0x40000)
    char _0c[0x10 - 0x0c];
    i32 m_10;         // +0x10  anchor adjust x
    i32 m_14;         // +0x14  anchor adjust y
    i32 m_18;         // +0x18  out: clipped left (top-left point)
    i32 m_1c;         // +0x1c  out: clipped top
    BlitRect m_20;    // +0x20  out: clipped rect {left, top, right, bottom}
    i32 m_30;         // +0x30  out: width
    i32 m_34;         // +0x34  out: height
    i32 m_38;         // +0x38  out: result (0 ok / -1 culled)
    CBlitXform* m_3c; // +0x3c  origin transform
    char _40[0x4c - 0x40];
    i32 m_4c; // +0x4c  pre-notify arg
    i32 m_50; // +0x50  pre-notify arg
    char _54[0x58 - 0x54];
    i32 m_58; // +0x58  pre-notify gate
    i32 m_5c; // +0x5c  draw x
    i32 m_60; // +0x60  draw y
    i32 m_64; // +0x64  clip left / 0x80000000 sentinel
    i32 m_68; // +0x68  clip top
    i32 m_6c; // +0x6c  clip right
    i32 m_70; // +0x70  clip bottom
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
    i32 x = info->m_5c - m_20 - info->m_10 - m_18;
    i32 y = info->m_60 - m_24 - info->m_14 - m_1c;
    if (info->m_08 & 0x40000) {
        info->m_3c->m_5c->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_08 & 0x40000) {
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
    } else if (info->m_64 == (i32)0x80000000) {
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
        if (x < info->m_64) {
            d.left = info->m_64;
        }
        if (right > info->m_6c) {
            d.right = info->m_6c;
        }
        if (y < info->m_68) {
            d.top = info->m_68;
        }
        if (bottom > info->m_70) {
            d.bottom = info->m_70;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_38 = -1;
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
    ((CDDSurface*)dst->m_2c)->BltEx(&d, (CDDSurface*)m_2c, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_18 = d.left;
    info->m_1c = d.top;
    info->m_20 = *(BlitRect*)&d;
    info->m_30 = w;
    info->m_34 = h;
    info->m_38 = 0;
}

// ---------------------------------------------------------------------------
// Vertical flip, surface blit (BltEx, blend mode 2).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct (formulas verified against retail). The vertical flip makes
// the Y anchor a mixed-sign chain (m_24 - m_1c + m_14 + m_60); MSVC5 reassociates
// it to (m_14 + m_24 + m_60) - m_1c and picks a different Y-accumulator base than
// retail, which co-schedules the X subtract chain into different registers. That
// one divergence cascades through the whole function (no source spelling pins the
// reassociation - compound-assign / anchor-temp / x<->y reorder all tried). Plus
// the WrapCoord/CopyRect reloc-name artifacts.
RVA(0x00153b20, 0x270)
void CImage::BlitFlipV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_5c - info->m_10 - m_18 - m_20;
    i32 y = m_24 - m_1c + info->m_14 + info->m_60;
    if (info->m_08 & 0x40000) {
        info->m_3c->m_5c->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_08 & 0x40000) {
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
    } else if (info->m_64 == (i32)0x80000000) {
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
        if (x < info->m_64) {
            d.left = info->m_64;
        }
        if (right > info->m_6c) {
            d.right = info->m_6c;
        }
        if (y < info->m_68) {
            d.top = info->m_68;
        }
        if (bottom > info->m_70) {
            d.bottom = info->m_70;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_38 = -1;
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
    ((CDDSurface*)dst->m_2c)->BltEx(&d, (CDDSurface*)m_2c, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_18 = d.left;
    info->m_1c = d.top;
    info->m_20 = *(BlitRect*)&d;
    info->m_30 = w;
    info->m_34 = h;
    info->m_38 = 0;
}

// ---------------------------------------------------------------------------
// Horizontal flip, surface blit (BltEx, blend mode 4).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct. Same wall as BlitFlipV: the horizontal flip makes X a
// mixed-sign chain (m_10 - m_18 + m_20 + m_5c) that MSVC5 reassociates + reorders
// vs retail, cascading the co-scheduled X/Y register assignment. Plus the
// WrapCoord/CopyRect reloc-name artifacts.
RVA(0x00153d90, 0x259)
void CImage::BlitFlipH(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_10 - m_18 + m_20 + info->m_5c;
    i32 y = info->m_60 - m_24 - m_1c - info->m_14;
    if (info->m_08 & 0x40000) {
        info->m_3c->m_5c->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_08 & 0x40000) {
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
    } else if (info->m_64 == (i32)0x80000000) {
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
        if (x < info->m_64) {
            d.left = info->m_64;
        }
        if (right > info->m_6c) {
            d.right = info->m_6c;
        }
        if (y < info->m_68) {
            d.top = info->m_68;
        }
        if (bottom > info->m_70) {
            d.bottom = info->m_70;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_38 = -1;
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
    ((CDDSurface*)dst->m_2c)->BltEx(&d, (CDDSurface*)m_2c, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_18 = d.left;
    info->m_1c = d.top;
    info->m_20 = *(BlitRect*)&d;
    info->m_30 = w;
    info->m_34 = h;
    info->m_38 = 0;
}

// ---------------------------------------------------------------------------
// X+Y flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 0/0).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct - the fourth member of the shaded family, structurally
// identical to BlitShadeNorm/FlipV. Both anchor axes are flipped (X: the
// m_18/m_20 signs; Y: the m_24/m_14/m_1c mixed-sign chain), so it inherits the
// SAME whole-function regalloc/reassociation wall the other flip variants hit:
// MSVC5 reassociates the mixed-sign X/Y accumulator chains and picks a different
// this-member->register mapping than retail, cascading downstream. Plus the
// WrapCoord (0x295a ILT thunk) / CopyRect (IAT import) / 0x14dd90 pre-notify
// reloc-name operand artifacts. Clip + inclusive-rect struct-copy end match.
RVA(0x00153ff0, 0x280)
void CImage::BlitShadeFlipHV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_5c - m_18 + m_20 + info->m_10;
    i32 y = info->m_60 - m_1c + m_24 + info->m_14;
    if (info->m_08 & 0x40000) {
        info->m_3c->m_5c->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_08 & 0x40000) {
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
    } else if (info->m_64 == (i32)0x80000000) {
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
        if (x < info->m_64) {
            d.left = info->m_64;
        }
        if (right > info->m_6c) {
            d.right = info->m_6c;
        }
        if (y < info->m_68) {
            d.top = info->m_68;
        }
        if (bottom > info->m_70) {
            d.bottom = info->m_70;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_38 = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_58) {
        ((CDDrawShadeBlit*)m_30)->Notify(info->m_50, info->m_4c);
    }
    ((CDDrawShadeBlit*)m_30)->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_2c, (ShadeRect*)&s, 0, 0);
    info->m_18 = d.left;
    info->m_1c = d.top;
    info->m_20 = *(BlitRect*)&d;
    info->m_30 = w;
    info->m_34 = h;
    info->m_38 = 0;
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
    i32 x = info->m_5c - m_20 - m_18 - info->m_10;
    i32 y = info->m_60 - m_24 - m_1c - info->m_14;
    if (info->m_08 & 0x40000) {
        info->m_3c->m_5c->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_08 & 0x40000) {
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
    } else if (info->m_64 == (i32)0x80000000) {
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
        if (x < info->m_64) {
            d.left = info->m_64;
        }
        if (right > info->m_6c) {
            d.right = info->m_6c;
        }
        if (y < info->m_68) {
            d.top = info->m_68;
        }
        if (bottom > info->m_70) {
            d.bottom = info->m_70;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_38 = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_58) {
        ((CDDrawShadeBlit*)m_30)->Notify(info->m_50, info->m_4c);
    }
    ((CDDrawShadeBlit*)m_30)->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_2c, (ShadeRect*)&s, 1, 1);
    info->m_18 = d.left;
    info->m_1c = d.top;
    info->m_20 = *(BlitRect*)&d;
    info->m_30 = w;
    info->m_34 = h;
    info->m_38 = 0;
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
    i32 x = info->m_5c - m_18 - info->m_10 - m_20;
    i32 y = m_24 + info->m_14 + info->m_60 - m_1c;
    if (info->m_08 & 0x40000) {
        info->m_3c->m_5c->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_08 & 0x40000) {
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
    } else if (info->m_64 == (i32)0x80000000) {
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
        if (x < info->m_64) {
            d.left = info->m_64;
        }
        if (right > info->m_6c) {
            d.right = info->m_6c;
        }
        if (y < info->m_68) {
            d.top = info->m_68;
        }
        if (bottom > info->m_70) {
            d.bottom = info->m_70;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_38 = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_58) {
        ((CDDrawShadeBlit*)m_30)->Notify(info->m_50, info->m_4c);
    }
    ((CDDrawShadeBlit*)m_30)->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_2c, (ShadeRect*)&s, 1, 0);
    info->m_18 = d.left;
    info->m_1c = d.top;
    info->m_20 = *(BlitRect*)&d;
    info->m_30 = w;
    info->m_34 = h;
    info->m_38 = 0;
}

// ---------------------------------------------------------------------------
// X flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 0/1).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct - the shaded twin of BlitFlipH. The horizontal flip makes
// X the mixed-sign chain (m_10 + m_20 + m_5c - m_18) that MSVC5 reassociates +
// reorders vs retail, cascading the co-scheduled X/Y register assignment (same
// wall as the surface BlitFlipH). Plus the WrapCoord/CopyRect/0x14dd90 reloc-name
// operand artifacts. Clip + inclusive-rect struct-copy end match retail.
RVA(0x00154750, 0x275)
void CImage::BlitShadeFlipH(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_10 + m_20 + info->m_5c - m_18;
    i32 y = info->m_60 - m_24 - info->m_14 - m_1c;
    if (info->m_08 & 0x40000) {
        info->m_3c->m_5c->WrapCoord(&x, &y);
    }
    i32 right = m_10 + x - 1;
    i32 bottom = m_14 + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_08 & 0x40000) {
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
    } else if (info->m_64 == (i32)0x80000000) {
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
        if (x < info->m_64) {
            d.left = info->m_64;
        }
        if (right > info->m_6c) {
            d.right = info->m_6c;
        }
        if (y < info->m_68) {
            d.top = info->m_68;
        }
        if (bottom > info->m_70) {
            d.bottom = info->m_70;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_38 = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_58) {
        ((CDDrawShadeBlit*)m_30)->Notify(info->m_50, info->m_4c);
    }
    ((CDDrawShadeBlit*)m_30)->Blit((ShadeRect*)&d, (ShadeSrc*)dst->m_2c, (ShadeRect*)&s, 0, 1);
    info->m_18 = d.left;
    info->m_1c = d.top;
    info->m_20 = *(BlitRect*)&d;
    info->m_30 = w;
    info->m_34 = h;
    info->m_38 = 0;
}
// Class-metadata annotations (EOF-hosted, /O2 sprite-blit TU).
SIZE_UNKNOWN(CBlitXform);
SIZE_UNKNOWN(CBlitInfo);
