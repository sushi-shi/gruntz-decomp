// DDrawShadeBlit.cpp - DDrawMgr software shaded-sprite blitter (tracer
// placeholder tomalla-86). Methods in ascending retail-RVA order.
//
// 0x1497f0 Blit       - validate the destination rect, select the per-draw-type
//                       translucency LUTs, dispatch to one of four mode loops.
// 0x14a200 BlitLoop   - the big RLE-decode + 64KB-blend-table inner blit loop.
//
// Field names are placeholders; offsets + code bytes are load-bearing. The three
// other mode-loop callees (0x149950/0x149d00/0x14b770) and the surface Lock are
// external/reloc-masked. See <DDrawMgr/DDrawShadeBlit.h> for the layout.
#include <DDrawMgr/DDSurface.h> // CDDSurface src arg (m_pitch/m_b0/Lock; m_8->Unlock COM)
#include <Win32.h>              // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>              // real IDirectDrawSurface dispatch (surf->m_8->Unlock)
#include <DDrawMgr/DDrawShadeBlit.h>

#include <rva.h>
#include <string.h> // inline rep-movs memcpy intrinsic
#include <Globals.h>

// The live screen RGB-format shift table at RVA 0x283ea0..0x283eb4 - the canonical
// binding is the extern "C" _g_683eaX (DATA-bound in GruntzMgr.cpp); the C++ ?g_rUp
// aliases lost the keep-last symbol dedup, so bind to the winning names directly.
// The mode-2 gate compares these against the magic 5/10/3/3/3 state. Reloc-masked.
extern "C" {
    DATA(0x00283ea0)
    extern i32 g_683ea0; // red   shift-up
    DATA(0x00283ea4)
    extern i32 g_683ea4; // green shift-up
    DATA(0x00283eac)
    extern i32 g_683eac; // red   shift-down
    DATA(0x00283eb0)
    extern i32 g_683eb0; // green shift-down
    DATA(0x00283eb4)
    extern i32 g_683eb4; // blue  shift-down
}

// The secondary palette/format descriptor (DAT_006bf218) used by the 16-bit alpha
// path (case 7); its +0x8 LUT base is read as u16*. Reloc-masked.
DATA(0x002bf218)
extern ShadeDescr* g_blendDescr; // 0x6bf218

// The three 2048-byte-strided translucency LUT banks selected by the light level.
//
// AUTHENTIC-FLOOR NOTE (cast audit): every `(u16*)` in this TU is a PROVEN-authentic
// pixel-mode reinterpretation and must NOT be reduced. m_palDescr->m_lut and
// m_lutBank0/1/2 are all `u8*` byte bases: the 8bpp draw-type paths read them as bytes
// (u8* pal / base), while the RGB565 16bpp paths reinterpret the SAME base as u16* for
// the channel-split blend LUT reads. Typing the members u16* would break the 8bpp byte
// reads (verified regressive). The only other cast, `(u8)m_light`, is a numeric fill-byte
// conversion. This file is at its authentic floor.

// BlitAt (0x149780, __thiscall, ret 0x14 => 5 args). Position the sprite's full
// bounds at (x,y) on `dstSurf`: build the destination rect {x,y,x+w-1,y+h-1} and
// the source clip {0,0,w-1,h-1} from the sprite's m_width/m_height, then call Blit.
RVA(0x00149780, 0x69)
i32 CDDrawShadeBlit::BlitAt(CDDSurface* dstSurf, i32 x, i32 y, i32 sel, i32 p4) {
    ShadeRect clip;
    ShadeRect dst;
    clip.left = 0;
    clip.top = 0;
    clip.right = m_width - 1;
    clip.bottom = m_height - 1;
    dst.left = x;
    dst.top = y;
    dst.right = x + m_width - 1;
    dst.bottom = y + m_height - 1;
    return Blit(&dst, dstSurf, &clip, sel, p4);
}

RVA(0x001497f0, 0x154)
i32 CDDrawShadeBlit::Blit(ShadeRect* p0, CDDSurface* src, ShadeRect* clip, i32 sel, i32 p4) {
    if (clip->left < 0 || clip->right > m_width - 1 || clip->top < 0
        || clip->bottom > m_height - 1) {
        return 0;
    }

    i32 mode = src->m_b0;
    m_dstBpp = (u8)mode;
    if ((u8)mode == 2) {
        if (g_683eac == 3 && g_683eb0 == 3 && g_683eb4 == 3 && g_683ea0 == 0xa && g_683ea4 == 5) {
            m_blendVariant = 1;
        } else {
            m_blendVariant = 0;
        }
    }

    i32 drawType = m_drawType;
    if (drawType == 1) {
        if (sel) {
            BlitMode_149d00(p0, src, clip, p4);
        } else {
            BlitMode_149950(p0, src, clip, p4);
        }
        return 1;
    }

    if (drawType == 7) {
        if (m_srcBpp != 1 || (u8)mode != 2) {
            return 0;
        }
    }
    if (drawType == 0xa || drawType == 0xb) {
        if (m_srcBpp != 1 || (u8)mode != 2) {
            return 0;
        }
    }
    if (drawType == 8 || drawType == 0xb) {
        i32 bank = (m_light >> 3) * 0x800;
        m_lutBank0 = g_lutBank0_673ca0 + bank;
        m_lutBank1 = g_lutBank1_653ca0 + bank;
        m_lutBank2 = g_lutBank2_663ca0 + bank;
    }

    if (sel) {
        BlitMode_14b770(p0, src, clip, p4);
    } else {
        BlitLoop(p0, src, clip, p4);
    }
    return 1;
}

// ===========================================================================
// CDDrawShadeBlit::BlitMode_149950  (0x149950) - the unselected, non-shaded RLE
// sprite blitter (draw type 1, !sel). Lock the destination surface, fast-forward
// the RLE stream past the top clip->top rows, compute the dest row-start for the
// (optionally v-flipped) rect, then run one of three per-row inner loops depending
// on the horizontal clip: full-width (no clip), right-only, or left(+right). Each
// loop decodes the high-bit RLE (`b&0x80` = transparent skip of `b-0x80`, else an
// opaque run of `b` pixels) and memcpy's opaque runs into the surface.
// ===========================================================================
// @early-stop
// Regalloc / loop-scheduling wall (the RLE-blit family): logic is complete and
// correct - prepass, v-flip start, all three clip loops match the retail control
// flow, and opaque runs lower to the inline rep-movs memcpy idiom. Two residual
// walls: (1) MSVC pins `this` in ebx and frames `sub esp,0xc` (1 spill local),
// while retail pins `this` in ebp / frames `sub esp,8` and reuses the dead arg0
// slot for the pitch - a callee-saved-register choice that renames the modrm byte
// of every member read; (2) the row counter / x / pos register threading and the
// redundant clip-dispatch `test;je` are scheduled differently across the three
// near-identical sub-loops. Inlining the W/clip caches lifted it from ~15% to
// ~31%; the rest is the zero-register-pinning family. Deferred to the final sweep.
RVA(0x00149950, 0x3a1)
void CDDrawShadeBlit::BlitMode_149950(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = (u8*)surf->Lock(0);

    i32 row = 0, pos = 0, x = 0;
    // Prepass: skip the top clip->top rows of the RLE stream.
    if (clip->top > 0) {
        do {
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        } while (row < clip->top);
    }

    // Dest row-start for the (optionally v-flipped) rect.
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        pitch = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
    }

    x = 0;
    if (clip->left != 0) {
        // Left edge clipped (the run that crosses clip->left is partially copied).
        while (row < clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (x < clip->left) {
                i32 trans = 0;
                do {
                    u32 b = m_rleData[pos];
                    if (b & 0x80) {
                        x += b - 0x80;
                        pos++;
                        trans = 1;
                    } else {
                        x += b;
                        pos += (i32)b * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x < clip->left);
                if (x > clip->left && trans == 0) {
                    i32 bytes = (x - clip->left) * m_srcBpp;
                    memcpy(base, &m_rleData[pos] - bytes, bytes);
                }
            }
            if (x >= m_width) {
                row++;
                base += pitch;
                x = 0;
            } else {
                u32 b = m_rleData[pos];
                if (b & 0x80) {
                    x += b - 0x80;
                    pos++;
                } else {
                    memcpy(
                        base + (x - clip->left) * m_dstBpp,
                        &m_rleData[pos + 1],
                        (i32)b * m_srcBpp
                    );
                    x += b;
                    pos += (i32)b * m_srcBpp + 1;
                }
            }
        }
    } else if (clip->right != m_width - 1) {
        // Right edge clipped.
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                i32 bytes;
                if (x + (i32)b < clip->right) {
                    bytes = (i32)b * m_srcBpp;
                } else {
                    i32 vis = (clip->right - x) * m_srcBpp;
                    bytes = vis < 0 ? 0 : vis;
                }
                memcpy(base + x * m_dstBpp, &m_rleData[pos + 1], bytes);
                x += b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += pitch;
                x = 0;
            }
        }
    } else {
        // Full-width: no horizontal clipping.
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                memcpy(base + x * m_dstBpp, &m_rleData[pos + 1], (i32)b * m_srcBpp);
                x += b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += pitch;
                x = 0;
            }
        }
    }

    surf->m_8->Unlock(0);
}

// ===========================================================================
// CDDrawShadeBlit::BlitMode_149d00  (0x149d00) - the SELECTED (horizontally
// flipped) sibling of BlitMode_149950. Same prepass / v-flip start / three clip
// loops, but x runs from the sprite width down to 0 and each opaque run is copied
// in reverse (dest pointer decreasing). The per-pixel size (m_srcBpp == 1 -> byte,
// else word) selects a manual byte/word reverse-copy loop instead of memcpy.
// ===========================================================================
// @early-stop
// Loop-scheduling / regalloc wall + the reverse-copy idiom: logic complete and
// correct (right-to-left mirror blit with the same RLE decode and clip cases), but
// MSVC's lowering of the `for (k=cnt; k>0; k--) *d-- = *s++;` reverse copy differs
// from retail's `mov edx,eax; dec eax; test; jle; inc eax; do{..}while(--)` peeled
// count-down, and the cross-sub-loop register pinning diverges as in 149950.
// Deferred to the final sweep.
RVA(0x00149d00, 0x4f8)
void CDDrawShadeBlit::BlitMode_149d00(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = (u8*)surf->Lock(0);

    i32 row = 0, pos = 0, x = 0;
    if (clip->top > 0) {
        do {
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        } while (row < clip->top);
    }
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        pitch = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
    }

    x = m_width;
    if (clip->left != 0) {
        // Left edge clipped, h-flipped (run crossing clip->left partially copied).
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += 0x80 - (i32)b;
                pos++;
            } else {
                i32 cnt = b;
                u8* sd = &m_rleData[pos + 1];
                i32 bytes;
                if (x - cnt > clip->left) {
                    bytes = cnt * m_srcBpp;
                } else {
                    i32 vis = (x - clip->left) * m_srcBpp;
                    bytes = vis < 0 ? 0 : vis;
                }
                u8* dbase = base + (x - clip->left) * m_dstBpp;
                if (m_srcBpp == 1) {
                    u8* d = dbase;
                    for (i32 k = bytes; k > 0; k--) {
                        *d-- = *sd++;
                    }
                } else {
                    u16* d = (u16*)dbase;
                    u16* sw = (u16*)sd;
                    for (i32 k = bytes / 2; k > 0; k--) {
                        *d-- = *sw++;
                    }
                }
                x -= cnt;
                pos += cnt * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    } else if (clip->right != m_width - 1) {
        // Right edge clipped, h-flipped: skip runs while x > clip->right.
        while (row < clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (x > clip->right) {
                i32 trans = 0;
                do {
                    u32 b = m_rleData[pos];
                    if (b & 0x80) {
                        x += 0x80 - (i32)b;
                        pos++;
                        trans = 1;
                    } else {
                        x -= b;
                        pos += (i32)b * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x > clip->right);
                if (x >= 0 && trans == 0) {
                    i32 bytes = (clip->right - x) * m_srcBpp;
                    u8* s = &m_rleData[pos] - bytes;
                    if (m_srcBpp == 1) {
                        u8* d = base + clip->right * m_dstBpp;
                        for (i32 k = bytes; k > 0; k--) {
                            *d-- = *s++;
                        }
                    } else {
                        u16* d = (u16*)(base + clip->right * m_dstBpp);
                        u16* sw = (u16*)s;
                        for (i32 k = bytes / 2; k > 0; k--) {
                            *d-- = *sw++;
                        }
                    }
                }
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            } else {
                u32 b = m_rleData[pos];
                if (b & 0x80) {
                    x += 0x80 - (i32)b;
                    pos++;
                } else {
                    i32 cnt = b;
                    u8* s = &m_rleData[pos + 1];
                    if (m_srcBpp == 1) {
                        u8* d = base + x * m_dstBpp;
                        for (i32 k = cnt; k > 0; k--) {
                            *d-- = *s++;
                        }
                    } else {
                        u16* d = (u16*)(base + x * m_dstBpp);
                        u16* sw = (u16*)s;
                        for (i32 k = cnt; k > 0; k--) {
                            *d-- = *sw++;
                        }
                    }
                    x -= cnt;
                    pos += cnt * m_srcBpp + 1;
                }
            }
        }
    } else {
        // Full-width, h-flipped: x: m_width -> 0, runs copied reversed.
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += 0x80 - (i32)b;
                pos++;
            } else {
                i32 cnt = b;
                u8* s = &m_rleData[pos + 1];
                if (m_srcBpp == 1) {
                    u8* d = base + x * m_dstBpp;
                    for (i32 k = cnt; k > 0; k--) {
                        *d-- = *s++;
                    }
                } else {
                    u16* d = (u16*)(base + x * m_dstBpp);
                    u16* sw = (u16*)s;
                    for (i32 k = cnt; k > 0; k--) {
                        *d-- = *sw++;
                    }
                }
                x -= cnt;
                pos += cnt * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_width;
            }
        }
    }

    surf->m_8->Unlock(0);
}

// ===========================================================================
// CDDrawShadeBlit::BlitLoop (0x14a200) - the big !sel (unselected) shaded RLE
// blitter. Lock the dest surface, skip clip->top rows, compute the (optionally
// v-flipped) dest row start, then run one of three horizontal-clip loops
// (full-width / right / left). Each decodes the high-bit RLE stream and, for an
// opaque run, blends `count` source pixels through the shade LUTs into the locked
// surface. m_00 (the vertical-double flag) makes each drawn row write to dst AND
// dst+pitch, but only on rows where (dst->top + row) is odd - a 2x-vertical
// interlace fill. FULL-WIDTH inlines the row converter (ConvertRow / the
// vertical-double ConvertRowDoubleFwd); the CLIPPED paths call the out-of-line
// helper. The blend math is the ConvertRow/ConvertRowDoubleFwd nine-case switch
// on (m_drawType - 2): 8/16-bit palette LUTs (m_palDescr->m_lut / g_blendDescr->
// m_lut), RGB565 channel-split blends via m_lutBank0/1/2, the m_light fill/lerp.
// ===========================================================================
// @early-stop
// ~23% (0.06 -> 23.3, complete + correct). Full reconstruction: prepass, v-flip
// row start, 3 horizontal-clip loops, the vertical-2x interlace, and the inlined
// full-width blend (all 14 draw-type cases) match retail's control flow. Three
// stacked walls cap it: (1) the whole-function REGALLOC pins `this` in ebp here
// vs ebx in retail (frame sub esp,0x2c vs 0x34) - the zero-register-pinning
// family (see BlitMode_149950 @early-stop) renames the modrm byte of every one of
// the ~200 member reads; (2) retail INLINES the vertical-double blend in the
// left-clip path (jump tables 0x54b738/0x54b754) but this reconstruction CALLS
// ConvertRowDoubleFwd there (the out-of-line/inline split is an MSVC5 /Ob1
// heuristic that can't be steered from source), so the clip locals + regalloc
// diverge; (3) the two inlined full-width jump tables (0x54b6f4/0x54b710) hit the
// .rdata jump-table-data-overlap scoring artifact (docs/patterns/jumptable-data-
// overlap.md). Deferred to the final sweep.
RVA(0x0014a200, 0x14f3)
void CDDrawShadeBlit::BlitLoop(ShadeRect* dst, CDDSurface* src, ShadeRect* clip, i32 vflip) {
    i32 pitch = src->m_pitch;
    u8* base = (u8*)src->Lock(0);

    i32 pos = 0, row = 0, x = 0;
    // Prepass: skip the top clip->top rows of the RLE stream.
    if (clip->top > 0) {
        do {
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        } while (row < clip->top);
    }

    i32 rowInc;
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        rowInc = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
        rowInc = pitch;
    }

    x = 0;
    if (clip->left != 0) {
        // LEFT edge clipped: skip runs up to clip->left, then blend the visible
        // portion of the crossing run and subsequent full runs (calls the shaded
        // row converter; retail inlines the vertical-double variant here).
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (x < clip->left) {
                i32 trans = 0;
                do {
                    u32 b = m_rleData[pos];
                    if (b & 0x80) {
                        x += b - 0x80;
                        pos++;
                        trans = 1;
                    } else {
                        x += b;
                        pos += (i32)b * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x < clip->left);
                if (x > clip->left && trans == 0) {
                    i32 vis = x - clip->left;
                    u8* dd = base;
                    u8* ss = &m_rleData[pos] - vis * m_srcBpp;
                    if (m_00) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDoubleFwd(dd, ss, vis, pitch);
                        }
                    } else {
                        ConvertRow(dd, ss, vis);
                    }
                }
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            } else {
                u32 b = m_rleData[pos];
                if (b & 0x80) {
                    x += b - 0x80;
                    pos++;
                } else {
                    u8* dd = base + (x - clip->left) * m_dstBpp;
                    u8* ss = &m_rleData[pos + 1];
                    i32 count = b;
                    if (m_00) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDoubleFwd(dd, ss, count, pitch);
                        }
                    } else {
                        ConvertRow(dd, ss, count);
                    }
                    x += b;
                    pos += (i32)b * m_srcBpp + 1;
                }
            }
        }
    } else if (clip->right != m_width - 1) {
        // RIGHT edge clipped: clamp each opaque run to clip->right (calls).
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                i32 vis;
                if (x + (i32)b < clip->right) {
                    vis = b;
                } else {
                    i32 v = clip->right - x;
                    vis = v < 0 ? 0 : v;
                }
                u8* dd = base + x * m_dstBpp;
                u8* ss = &m_rleData[pos + 1];
                if (m_00) {
                    if ((dst->top + row) % 2) {
                        ConvertRowDoubleFwd(dd, ss, vis, pitch);
                    }
                } else {
                    ConvertRow(dd, ss, vis);
                }
                x += b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    } else {
        // FULL-WIDTH: no horizontal clip; the blend is inlined per run (the
        // vertical-double variant writes each pixel to dst and dst+pitch).
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                u8* dst0 = base + x * m_dstBpp;
                u8* src0 = &m_rleData[pos + 1];
                i32 count = b;
                i32 i;
                if (m_00) {
                    if ((dst->top + row) % 2) {
                        // inline ConvertRowDoubleFwd(dst0, src0, count, pitch)
                        u8* d = dst0;
                        u8* s = src0;
                        switch (m_drawType) {
                            case 2: {
                                u8* pal = m_palDescr->m_lut;
                                memcpy(g_scratch, d, count);
                                u8* sc = g_scratch;
                                for (i = count; i > 0; i--) {
                                    d[0] = pal[(*sc << 8) + *s];
                                    d[pitch] = pal[(*sc << 8) + *s];
                                    d++;
                                    sc++;
                                    s++;
                                }
                                break;
                            }
                            case 3: {
                                u8* pal = m_palDescr->m_lut;
                                memcpy(g_scratch, d, count);
                                u8* sc = g_scratch;
                                for (i = count; i > 0; i--) {
                                    d[0] = pal[(*sc << 8) + m_light];
                                    d[pitch] = pal[(*sc << 8) + m_light];
                                    d++;
                                    sc++;
                                }
                                break;
                            }
                            case 7: {
                                u16* pal1 = (u16*)m_palDescr->m_lut;
                                u16* pal2 = (u16*)g_blendDescr->m_lut;
                                memcpy(g_scratch, d, count * 2);
                                u16* sc = (u16*)g_scratch;
                                i32 rd = pitch & ~1;
                                for (i = count; i > 0; i--) {
                                    u32 idx = pal2[*sc++];
                                    idx += (*s++ >> 4) << 12;
                                    u16 v = pal1[idx];
                                    *(u16*)d = v;
                                    *(u16*)(d + rd) = v;
                                    d += 2;
                                }
                                break;
                            }
                            case 8: {
                                memcpy(g_scratch, d, count * 2);
                                u16* sc = (u16*)g_scratch;
                                u16* ss2 = (u16*)s;
                                i32 rd = pitch & ~1;
                                if (m_blendVariant) {
                                    for (i = count; i > 0; i--) {
                                        u32 dv = *sc++;
                                        u32 a = *ss2++;
                                        u32 r =
                                            ((u16*)m_lutBank0)[(a >> 0xa) + ((dv >> 5) & 0xffe0)];
                                        r |= ((
                                            u16*
                                        )m_lutBank1)[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                        r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        *(u16*)d = (u16)r;
                                        *(u16*)(d + rd) = (u16)r;
                                        d += 2;
                                    }
                                } else {
                                    for (i = count; i > 0; i--) {
                                        u32 dv = *sc++;
                                        u32 a = *ss2++;
                                        u32 r =
                                            ((u16*)m_lutBank0)[(a >> 0xb) + ((dv >> 6) & 0xffe0)];
                                        r |= ((
                                            u16*
                                        )m_lutBank1)[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                        r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        *(u16*)d = (u16)r;
                                        *(u16*)(d + rd) = (u16)r;
                                        d += 2;
                                    }
                                }
                                break;
                            }
                        }
                    }
                } else {
                    // inline ConvertRow(dst0, src0, count)
                    u8* d = dst0;
                    u8* s = src0;
                    switch (m_drawType) {
                        case 2: {
                            u8* pal = m_palDescr->m_lut;
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
                            for (i = count; i > 0; i--) {
                                *d++ = pal[(*sc++ << 8) + *s++];
                            }
                            break;
                        }
                        case 7: {
                            u16* pal1 = (u16*)m_palDescr->m_lut;
                            u16* pal2 = (u16*)g_blendDescr->m_lut;
                            u16* sc = (u16*)g_scratch;
                            memcpy(g_scratch, d, count * 2);
                            for (i = count; i > 0; i--) {
                                u32 idx = pal2[*sc++];
                                idx += (*s++ >> 4) << 12;
                                *(u16*)d = pal1[idx];
                                d += 2;
                            }
                            break;
                        }
                        case 10: {
                            u16* pal = (u16*)m_palDescr->m_lut;
                            for (i = count; i > 0; i--) {
                                *(u16*)d = pal[*s++];
                                d += 2;
                            }
                            break;
                        }
                        case 8: {
                            memcpy(g_scratch, d, count * 2);
                            if (m_blendVariant) {
                                u16* sd = (u16*)g_scratch;
                                u16* ss2 = (u16*)s;
                                for (i = count; i > 0; i--) {
                                    u32 a = *ss2++;
                                    u32 bb = *sd++;
                                    u32 r = ((u16*)m_lutBank2)[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    r |= ((
                                        u16*
                                    )m_lutBank1)[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    r |= ((u16*)m_lutBank0)[(a >> 0xa) + (bb & 0xffe0)];
                                    *(u16*)d = (u16)r;
                                    d += 2;
                                }
                            } else {
                                u16* sd = (u16*)g_scratch;
                                u16* ss2 = (u16*)s;
                                for (i = count; i > 0; i--) {
                                    u32 a = *sd++;
                                    u32 bb = *ss2++;
                                    u32 r = ((
                                        u16*
                                    )m_lutBank0)[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= ((u16*)m_lutBank1)[((a >> 0xb)) + (bb & 0xffe0)];
                                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    *(u16*)d = (u16)r;
                                    d += 2;
                                }
                            }
                            break;
                        }
                        case 11: {
                            u16* pal = (u16*)m_palDescr->m_lut;
                            memcpy(g_scratch, d, count * 2);
                            if (m_blendVariant) {
                                u16* sd = (u16*)g_scratch;
                                for (i = count; i > 0; i--) {
                                    u32 a = pal[*s++];
                                    u32 bb = *sd++;
                                    u32 r = ((
                                        u16*
                                    )m_lutBank1)[((a >> 5) & 0x1f) + (((bb >> 5) & 0x1f) << 5)];
                                    r |= ((u16*)m_lutBank0)[(a >> 0xa) + (bb & 0xffe0)];
                                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    *(u16*)d = (u16)r;
                                    d += 2;
                                }
                            } else {
                                u16* sd = (u16*)g_scratch;
                                for (i = count; i > 0; i--) {
                                    u32 a = pal[*s++];
                                    u32 bb = *sd++;
                                    u32 r = ((
                                        u16*
                                    )m_lutBank0)[((a >> 6) & 0x1f) + (((bb >> 6) & 0x1f) << 5)];
                                    r |= ((u16*)m_lutBank1)[(a >> 0xb) + (bb & 0xffe0)];
                                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((bb & 0x1f) << 5)];
                                    *(u16*)d = (u16)r;
                                    d += 2;
                                }
                            }
                            break;
                        }
                        case 3: {
                            u8* pbase = m_palDescr->m_lut;
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
                            for (i = count; i > 0; i--) {
                                *d++ = pbase[(*sc++ << 8) + m_light];
                            }
                            break;
                        }
                        case 4: {
                            u8* pbase = m_palDescr->m_lut;
                            for (i = count; i > 0; i--) {
                                *d++ = pbase[(*s++ << 8) + m_light];
                            }
                            break;
                        }
                        case 5: {
                            for (i = count; i > 0; i--) {
                                *d++ = (u8)m_light;
                            }
                            break;
                        }
                        case 6: {
                            u8* pal = m_palDescr->m_lut;
                            u8* sc = g_scratch;
                            memcpy(g_scratch, d, count);
                            for (i = count; i > 0; i--) {
                                i32 sv = pal[*sc++ + 0x100];
                                i32 dv = pal[*s + 0x100];
                                i32 t = (dv - sv) * m_light / 255 + sv;
                                *d++ = pal[t];
                                s++;
                            }
                            break;
                        }
                    }
                }
                x += b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                base += rowInc;
                x = 0;
            }
        }
    }

    src->m_8->Unlock(0);
}

// ===========================================================================
// CDDrawShadeBlit::BlitMode_14b770 (0x14b770) - the SELECTED (h-flipped) shaded
// RLE blitter, the sel twin of BlitLoop (0x14a200). Same prepass / v-flip start /
// three horizontal-clip loops / vertical-2x interlace, but x walks from m_width
// down to 0 and each opaque run is blended right-to-left (dst decreasing). The
// FULL-WIDTH path inlines the mirror row converters (ConvertRowFlip single /
// ConvertRowDouble doubled); the CLIPPED paths call the out-of-line helpers.
// ===========================================================================
// @early-stop
// Complete + correct h-flipped reconstruction (mirror of BlitLoop 0x14a200; same
// walls). Full-width inlines the reversed blend (all 14 draw-type cases); the two
// clip loops call ConvertRowFlip / ConvertRowDouble. Capped by the same three
// stacked walls as BlitLoop: (1) whole-function regalloc / this-pinning; (2)
// retail inlines the vertical-double blend in one clip path (jump table 0x54c9d4)
// where this calls it (uncontrollable MSVC5 /Ob1 inline split); (3) the two
// full-width jump tables (0x54c990/0x54c9ac) hit the jump-table-data-overlap
// scoring artifact (docs/patterns/jumptable-data-overlap.md). Deferred to sweep.
RVA(0x0014b770, 0x121d)
void CDDrawShadeBlit::BlitMode_14b770(
    ShadeRect* dst,
    CDDSurface* surf,
    ShadeRect* clip,
    i32 vflip
) {
    i32 pitch = surf->m_pitch;
    u8* base = (u8*)surf->Lock(0);

    i32 pos = 0, row = 0, x = 0;
    // Prepass: skip the top clip->top rows of the RLE stream.
    if (clip->top > 0) {
        do {
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x >= m_width) {
                row++;
                x = 0;
            }
        } while (row < clip->top);
    }

    i32 rowInc;
    if (vflip) {
        base += dst->bottom * pitch + dst->left * m_dstBpp;
        rowInc = -pitch;
    } else {
        base += dst->top * pitch + dst->left * m_dstBpp;
        rowInc = pitch;
    }

    x = m_width;
    if (clip->left != 0) {
        // LEFT edge clipped (h-flipped): clamp the run crossing clip->left (calls).
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += 0x80 - (i32)b;
                pos++;
            } else {
                i32 cnt = b;
                u8* ss = &m_rleData[pos + 1];
                i32 vis;
                if (x - cnt > clip->left) {
                    vis = cnt;
                } else {
                    i32 v = x - clip->left;
                    vis = v < 0 ? 0 : v;
                }
                u8* dd = base + (x - clip->left) * m_dstBpp;
                if (m_00) {
                    if ((dst->top + row) % 2) {
                        ConvertRowDouble(dd, ss, vis, pitch);
                    }
                } else {
                    ConvertRowFlip(dd, ss, vis);
                }
                x -= cnt;
                pos += cnt * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += rowInc;
                x = m_width;
            }
        }
    } else if (clip->right != m_width - 1) {
        // RIGHT edge clipped (h-flipped): skip runs while x > clip->right (calls).
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            if (x > clip->right) {
                i32 trans = 0;
                do {
                    u32 b = m_rleData[pos];
                    if (b & 0x80) {
                        x += 0x80 - (i32)b;
                        pos++;
                        trans = 1;
                    } else {
                        x -= b;
                        pos += (i32)b * m_srcBpp + 1;
                        trans = 0;
                    }
                } while (x > clip->right);
                if (x >= 0 && trans == 0) {
                    i32 vis = clip->right - x;
                    u8* ss = &m_rleData[pos] - vis * m_srcBpp;
                    u8* dd = base + clip->right * m_dstBpp;
                    if (m_00) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDouble(dd, ss, vis, pitch);
                        }
                    } else {
                        ConvertRowFlip(dd, ss, vis);
                    }
                }
            }
            if (x <= 0) {
                row++;
                base += rowInc;
                x = m_width;
            } else {
                u32 b = m_rleData[pos];
                if (b & 0x80) {
                    x += 0x80 - (i32)b;
                    pos++;
                } else {
                    u8* dd = base + x * m_dstBpp;
                    u8* ss = &m_rleData[pos + 1];
                    i32 cnt = b;
                    if (m_00) {
                        if ((dst->top + row) % 2) {
                            ConvertRowDouble(dd, ss, cnt, pitch);
                        }
                    } else {
                        ConvertRowFlip(dd, ss, cnt);
                    }
                    x -= cnt;
                    pos += cnt * m_srcBpp + 1;
                }
            }
        }
    } else {
        // FULL-WIDTH (h-flipped): the blend is inlined per run, dst walking down.
        while (row <= clip->bottom) {
            if (pos >= m_rleLen) {
                break;
            }
            u32 b = m_rleData[pos];
            if (b & 0x80) {
                x += 0x80 - (i32)b;
                pos++;
            } else {
                u8* dst0 = base + x * m_dstBpp;
                u8* src0 = &m_rleData[pos + 1];
                i32 count = b;
                i32 i;
                if (m_00) {
                    if ((dst->top + row) % 2) {
                        // inline ConvertRowDouble(dst0, src0, count, pitch)
                        u8* d = dst0;
                        u8* s = src0;
                        switch (m_drawType) {
                            case 2: {
                                u8* pbase = m_palDescr->m_lut;
                                memcpy(g_scratch, d - count + 1, count);
                                u8* sc = &g_scratch[count - 1];
                                for (i = count; i > 0; i--) {
                                    d[0] = pbase[(*sc << 8) + *s];
                                    d[pitch] = pbase[(*sc << 8) + *s];
                                    d--;
                                    sc--;
                                    s++;
                                }
                                break;
                            }
                            case 3: {
                                u8* pbase = m_palDescr->m_lut;
                                memcpy(g_scratch, d - count + 1, count);
                                u8* sc = &g_scratch[count - 1];
                                for (i = count; i > 0; i--) {
                                    d[0] = pbase[(*sc << 8) + m_light];
                                    d[pitch] = pbase[(*sc << 8) + *s];
                                    d--;
                                    sc--;
                                }
                                break;
                            }
                            case 7: {
                                u16* pal1 = (u16*)m_palDescr->m_lut;
                                u16* pal2 = (u16*)g_blendDescr->m_lut;
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u16* sc = (u16*)&g_scratch[count * 2 - 2];
                                i32 rd = pitch & ~1;
                                for (i = count; i > 0; i--) {
                                    u32 idx = pal2[*sc--];
                                    idx += (*s++ >> 4) << 12;
                                    u16 v = pal1[idx];
                                    *(u16*)d = v;
                                    *(u16*)(d + rd) = v;
                                    d -= 2;
                                }
                                break;
                            }
                            case 8: {
                                memcpy(g_scratch, d - count * 2 - 2, count * 2);
                                u16* sc = (u16*)&g_scratch[count * 2 - 2];
                                u16* ss2 = (u16*)s;
                                i32 rd = pitch & ~1;
                                if (m_blendVariant) {
                                    for (i = count; i > 0; i--) {
                                        u32 a = *ss2++;
                                        u32 dv = *sc--;
                                        u32 r = ((
                                            u16*
                                        )m_lutBank0)[(a >> 0xa) + (((dv >> 0xa) & 0x1f) << 5)];
                                        r |= ((
                                            u16*
                                        )m_lutBank1)[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                        r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        *(u16*)d = (u16)r;
                                        *(u16*)(d + rd) = (u16)r;
                                        d -= 2;
                                    }
                                } else {
                                    for (i = count; i > 0; i--) {
                                        u32 a = *ss2++;
                                        u32 dv = *sc--;
                                        u32 r = ((
                                            u16*
                                        )m_lutBank0)[(a >> 0xb) + (((dv >> 0xb) & 0x1f) << 5)];
                                        r |= ((
                                            u16*
                                        )m_lutBank1)[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                        r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                        *(u16*)d = (u16)r;
                                        *(u16*)(d + rd) = (u16)r;
                                        d -= 2;
                                    }
                                }
                                break;
                            }
                        }
                    }
                } else {
                    // inline ConvertRowFlip(dst0, src0, count)
                    u8* d = dst0;
                    u8* s = src0;
                    u8* cbase = m_palDescr ? m_palDescr->m_lut : s;
                    switch (m_drawType) {
                        case 2: {
                            memcpy(g_scratch, d - count + 1, count);
                            u8* sc = &g_scratch[count - 1];
                            for (i = count; i > 0; i--) {
                                *d-- = cbase[(*sc-- << 8) + *s++];
                            }
                            break;
                        }
                        case 7: {
                            u16* pal1 = (u16*)m_palDescr->m_lut;
                            u16* pal2 = (u16*)g_blendDescr->m_lut;
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* sc = (u16*)&g_scratch[count * 2 - 2];
                            for (i = count; i > 0; i--) {
                                u32 idx = pal2[*sc--];
                                idx += (*s++ >> 4) << 12;
                                *(u16*)d = pal1[idx];
                                d -= 2;
                            }
                            break;
                        }
                        case 10: {
                            u16* pal = (u16*)m_palDescr->m_lut;
                            for (i = count; i > 0; i--) {
                                *(u16*)d = pal[*s++];
                                d -= 2;
                            }
                            break;
                        }
                        case 8: {
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* sc = (u16*)&g_scratch[count * 2 - 2];
                            u16* ss2 = (u16*)s;
                            if (m_blendVariant) {
                                for (i = count; i > 0; i--) {
                                    u32 a = *ss2++;
                                    u32 dv = *sc--;
                                    u32 r = ((
                                        u16*
                                    )m_lutBank1)[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= ((u16*)
                                              m_lutBank0)[(a >> 0xa) + (((dv >> 0xa) & 0x1f) << 5)];
                                    *(u16*)d = (u16)r;
                                    d -= 2;
                                }
                            } else {
                                for (i = count; i > 0; i--) {
                                    u32 a = *ss2++;
                                    u32 dv = *sc--;
                                    u32 r = ((
                                        u16*
                                    )m_lutBank0)[(a >> 0xb) + (((dv >> 0xb) & 0x1f) << 5)];
                                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= ((
                                        u16*
                                    )m_lutBank1)[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    *(u16*)d = (u16)r;
                                    d -= 2;
                                }
                            }
                            break;
                        }
                        case 11: {
                            u16* pal = (u16*)m_palDescr->m_lut;
                            memcpy(g_scratch, d - count * 2 - 2, count * 2);
                            u16* sc = (u16*)&g_scratch[count * 2 - 2];
                            if (m_blendVariant) {
                                for (i = count; i > 0; i--) {
                                    u32 a = pal[*s++];
                                    u32 dv = *sc--;
                                    u32 r = ((
                                        u16*
                                    )m_lutBank1)[((a >> 5) & 0x1f) + (((dv >> 5) & 0x1f) << 5)];
                                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= ((u16*)
                                              m_lutBank0)[(a >> 0xa) + (((dv >> 0xa) & 0x1f) << 5)];
                                    *(u16*)d = (u16)r;
                                    d -= 2;
                                }
                            } else {
                                for (i = count; i > 0; i--) {
                                    u32 a = pal[*s++];
                                    u32 dv = *sc--;
                                    u32 r = ((
                                        u16*
                                    )m_lutBank1)[((a >> 6) & 0x1f) + (((dv >> 6) & 0x1f) << 5)];
                                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((dv & 0x1f) << 5)];
                                    r |= ((u16*)
                                              m_lutBank0)[(a >> 0xb) + (((dv >> 0xb) & 0x1f) << 5)];
                                    *(u16*)d = (u16)r;
                                    d -= 2;
                                }
                            }
                            break;
                        }
                        case 3: {
                            memcpy(g_scratch, d - count + 1, count);
                            u8* sc = &g_scratch[count - 1];
                            for (i = count; i > 0; i--) {
                                *d-- = cbase[(*sc-- << 8) + m_light];
                            }
                            break;
                        }
                        case 4: {
                            for (i = count; i > 0; i--) {
                                *d-- = cbase[(*s++ << 8) + m_light];
                            }
                            break;
                        }
                        case 5: {
                            for (i = count; i > 0; i--) {
                                *d-- = (u8)m_light;
                            }
                            break;
                        }
                        case 6: {
                            memcpy(g_scratch, d - count - 1, count);
                            u8* sc = &g_scratch[count + 1];
                            for (i = count; i > 0; i--) {
                                i32 sv = cbase[*sc-- + 0x100];
                                i32 dv = cbase[*s + 0x100];
                                i32 t = (dv - sv) * m_light / 255 + sv;
                                *d-- = cbase[t];
                                s++;
                            }
                            break;
                        }
                    }
                }
                x -= b;
                pos += (i32)b * m_srcBpp + 1;
            }
            if (x <= 0) {
                row++;
                base += rowInc;
                x = m_width;
            }
        }
    }

    surf->m_8->Unlock(0);
}

// @early-stop
// ~56% (logic complete + correct). 1446 B dense-jump-table per-row format/blend
// converter (one of the four tables the BlitLoop family dispatches). Nine cases
// on (m_drawType - 2) over a single row: 8/16-bit palette LUTs (m_palDescr->m_lut /
// g_blendDescr->m_lut), RGB565 channel-split blends via m_lutBank0/m_lutBank1/m_lutBank2, and a
// magic-divide (/255) alpha lerp (case 6). Each case body is within 1-3
// instructions of retail (per-case insn counts: case 7 matches exactly). Two
// stacked walls: (1) the jump-table .rdata region scoring artifact
// (docs/patterns/jumptable-data-overlap.md); (2) every counted loop's entry
// guard + counter is the predecrement-guard-lea-recover-count regalloc shape
// (docs/patterns/predecrement-guard-lea-recover-count.md) - retail keeps the
// counter in a callee-saved reg freed after the rep-movs (re-materializing the
// const scratch addr 0x6bed08), our cl spills it; `for`/`do-while(--i)`/
// `if(--count>=0)` all diverge and `for` scores highest. Deferred to the final
// sweep. Cases written in retail .text body order (2,7,10,8,11,3,4,5,6).
RVA(0x0014c9f0, 0x5a6)
void CDDrawShadeBlit::ConvertRow(u8* dst, u8* src, i32 count) {
    i32 i;
    switch (m_drawType) {
        case 2: {
            u8* pal = m_palDescr->m_lut;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
                *dst++ = pal[(*sc++ << 8) + *src++];
            }
            break;
        }
        case 7: {
            u16* pal1 = (u16*)m_palDescr->m_lut;
            u16* pal2 = (u16*)g_blendDescr->m_lut;
            u16* sc = (u16*)g_scratch;
            memcpy(g_scratch, dst, count * 2);
            for (i = count; i > 0; i--) {
                u32 idx = pal2[*sc++];
                idx += (*src++ >> 4) << 12;
                *(u16*)dst = pal1[idx];
                dst += 2;
            }
            break;
        }
        case 10: {
            u16* pal = (u16*)m_palDescr->m_lut;
            for (i = count; i > 0; i--) {
                *(u16*)dst = pal[*src++];
                dst += 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst, count * 2);
            if (m_blendVariant) {
                u16* sd = (u16*)g_scratch;
                u16* ss = (u16*)src;
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 b = *sd++;
                    u32 r = ((u16*)m_lutBank2)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank1)[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank0)[(a >> 0xa) + (b & 0xffe0)];
                    *(u16*)dst = (u16)r;
                    dst += 2;
                }
            } else {
                u16* sd = (u16*)g_scratch;
                u16* ss = (u16*)src;
                for (i = count; i > 0; i--) {
                    u32 a = *sd++;
                    u32 b = *ss++;
                    u32 r = ((u16*)m_lutBank0)[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank1)[((a >> 0xb)) + (b & 0xffe0)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst += 2;
                }
            }
            break;
        }
        case 11: {
            u16* pal = (u16*)m_palDescr->m_lut;
            memcpy(g_scratch, dst, count * 2);
            if (m_blendVariant) {
                u16* sd = (u16*)g_scratch;
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 b = *sd++;
                    u32 r = ((u16*)m_lutBank1)[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank0)[(a >> 0xa) + (b & 0xffe0)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst += 2;
                }
            } else {
                u16* sd = (u16*)g_scratch;
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 b = *sd++;
                    u32 r = ((u16*)m_lutBank0)[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank1)[(a >> 0xb) + (b & 0xffe0)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst += 2;
                }
            }
            break;
        }
        case 3: {
            u8* base = m_palDescr->m_lut;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
                *dst++ = base[(*sc++ << 8) + m_light];
            }
            break;
        }
        case 4: {
            u8* base = m_palDescr->m_lut;
            for (i = count; i > 0; i--) {
                *dst++ = base[(*src++ << 8) + m_light];
            }
            break;
        }
        case 5: {
            for (i = count; i > 0; i--) {
                *dst++ = (u8)m_light;
            }
            break;
        }
        case 6: {
            u8* pal = m_palDescr->m_lut;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
                i32 s = pal[*sc++ + 0x100];
                i32 d = pal[*src + 0x100];
                i32 t = (d - s) * m_light / 255 + s;
                *dst++ = pal[t];
                src++;
            }
            break;
        }
    }
}

// ===========================================================================
// 0x14cfc0 - ConvertRowFlip: the horizontally-mirrored twin of ConvertRow (the
// selected-blit row converter). Same dense (m_drawType-2) jump table over nine blend
// cases, but the destination run is walked right-to-left and the saved-dest
// scratch line is read back to front (rep-movs saves the run ending at dst). The
// 8-bit cases write `*dst--`; the 16-bit RGB565 channel blends decrement by 2.
// `base` = m_palDescr ? m_palDescr->m_lut : src (computed once before the switch).
// ===========================================================================
// @early-stop
// Same stacked walls as ConvertRow (~56%): (1) the jump-table .rdata scoring
// artifact (docs/patterns/jumptable-data-overlap.md); (2) every counted loop is
// the predecrement-guard-lea-recover-count regalloc shape
// (docs/patterns/predecrement-guard-lea-recover-count.md) plus the reverse-walk
// pointer threading - retail re-materializes the scratch addr and reuses the dead
// arg slot as the loop counter, our spill schedule diverges. Per-pixel blend math
// (8/16-bit LUTs, RGB565 channel splits, the /255 alpha lerp) is byte-faithful;
// scheduling parks it. Cases in retail .text body order (2,7,10,8,11,3,4,5,6).
RVA(0x0014cfc0, 0x5f1)
void CDDrawShadeBlit::ConvertRowFlip(u8* dst, u8* src, i32 count) {
    u8* base = m_palDescr ? m_palDescr->m_lut : src;
    i32 i;
    switch (m_drawType) {
        case 2: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                *dst-- = base[(*sc-- << 8) + *src++];
            }
            break;
        }
        case 7: {
            u16* pal1 = (u16*)m_palDescr->m_lut;
            u16* pal2 = (u16*)g_blendDescr->m_lut;
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (u16*)&g_scratch[count * 2 - 2];
            for (i = count; i > 0; i--) {
                u32 idx = pal2[*sc--];
                idx += (*src++ >> 4) << 12;
                *(u16*)dst = pal1[idx];
                dst -= 2;
            }
            break;
        }
        case 10: {
            u16* pal = (u16*)m_palDescr->m_lut;
            for (i = count; i > 0; i--) {
                *(u16*)dst = pal[*src++];
                dst -= 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (u16*)&g_scratch[count * 2 - 2];
            u16* ss = (u16*)src;
            if (m_blendVariant) {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = ((u16*)m_lutBank1)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank0)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = ((u16*)m_lutBank0)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank1)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst -= 2;
                }
            }
            break;
        }
        case 11: {
            u16* pal = (u16*)m_palDescr->m_lut;
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (u16*)&g_scratch[count * 2 - 2];
            if (m_blendVariant) {
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 d = *sc--;
                    u32 r = ((u16*)m_lutBank1)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank0)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 d = *sc--;
                    u32 r = ((u16*)m_lutBank1)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank0)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst -= 2;
                }
            }
            break;
        }
        case 3: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                *dst-- = base[(*sc-- << 8) + m_light];
            }
            break;
        }
        case 4: {
            for (i = count; i > 0; i--) {
                *dst-- = base[(*src++ << 8) + m_light];
            }
            break;
        }
        case 5: {
            for (i = count; i > 0; i--) {
                *dst-- = (u8)m_light;
            }
            break;
        }
        case 6: {
            memcpy(g_scratch, dst - count - 1, count);
            u8* sc = &g_scratch[count + 1];
            for (i = count; i > 0; i--) {
                i32 s = base[*sc-- + 0x100];
                i32 d = base[*src + 0x100];
                i32 t = (d - s) * m_light / 255 + s;
                *dst-- = base[t];
                src++;
            }
            break;
        }
    }
}

// ===========================================================================
// 0x14d5e0 - ConvertRowDoubleFwd: the forward (left-to-right) twin of
// ConvertRowDouble. Writes each converted pixel to dst AND dst+rowDelta, walking
// dst and the saved-dest scratch line UP. Dense (m_drawType-2) jump table over cases
// 2/3/7/8 (4/5/6 fall through). Case 3 is symmetric (both rows get the m_light LUT of
// the saved dest; src is unused). __thiscall, ret 0x10.
// ===========================================================================
// @early-stop
// Same family wall as ConvertRow/ConvertRowFlip/ConvertRowDouble (~56%): the
// jump-table .rdata scoring artifact (docs/patterns/jumptable-data-overlap.md) +
// the predecrement-guard-lea-recover-count regalloc shape
// (docs/patterns/predecrement-guard-lea-recover-count.md), here with the dual-store
// thread (retail recomputes the blended pixel for the second store - reproduced by
// writing the index expression twice). Per-pixel blend math is byte-faithful; the
// spill/recompute schedule parks it. Deferred to the row-converter final sweep.
RVA(0x0014d5e0, 0x345)
void CDDrawShadeBlit::ConvertRowDoubleFwd(u8* dst, u8* src, i32 count, i32 rowDelta) {
    i32 i;
    switch (m_drawType) {
        case 2: {
            u8* base = m_palDescr->m_lut;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            for (i = count; i > 0; i--) {
                dst[0] = base[(*sc << 8) + *src];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst++;
                sc++;
                src++;
            }
            break;
        }
        case 3: {
            u8* base = m_palDescr->m_lut;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            for (i = count; i > 0; i--) {
                dst[0] = base[(*sc << 8) + m_light];
                dst[rowDelta] = base[(*sc << 8) + m_light];
                dst++;
                sc++;
            }
            break;
        }
        case 7: {
            u16* pal1 = (u16*)m_palDescr->m_lut;
            u16* pal2 = (u16*)g_blendDescr->m_lut;
            memcpy(g_scratch, dst, count * 2);
            u16* sc = (u16*)g_scratch;
            i32 rd = rowDelta & ~1;
            for (i = count; i > 0; i--) {
                u32 idx = pal2[*sc++];
                idx += (*src++ >> 4) << 12;
                u16 v = pal1[idx];
                *(u16*)dst = v;
                *(u16*)(dst + rd) = v;
                dst += 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst, count * 2);
            u16* sc = (u16*)g_scratch;
            u16* ss = (u16*)src;
            i32 rd = rowDelta & ~1;
            if (m_blendVariant) {
                for (i = count; i > 0; i--) {
                    u32 d = *sc++;
                    u32 a = *ss++;
                    u32 r = ((u16*)m_lutBank0)[(a >> 0xa) + ((d >> 5) & 0xffe0)];
                    r |= ((u16*)m_lutBank1)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    *(u16*)(dst + rd) = (u16)r;
                    dst += 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 d = *sc++;
                    u32 a = *ss++;
                    u32 r = ((u16*)m_lutBank0)[(a >> 0xb) + ((d >> 6) & 0xffe0)];
                    r |= ((u16*)m_lutBank1)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    *(u16*)(dst + rd) = (u16)r;
                    dst += 2;
                }
            }
            break;
        }
    }
}

// ===========================================================================
// 0x14d950 - ConvertRowDouble: the vertical-double row converter (writes every
// converted pixel to dst AND dst+rowDelta). Five (m_drawType-2) cases (2/3/7/8; 4/5/6
// fall through to the empty default). The byte cases reverse-walk dst with the
// saved-dest scratch line; the word cases round rowDelta down to even. Case 3 is
// asymmetric: the dst row gets the m_light LUT of the saved dest, the dst+rowDelta
// row gets the palette blend of the source. __thiscall, ret 0x10.
// ===========================================================================
// @early-stop
// Same family wall as ConvertRow/ConvertRowFlip: jump-table .rdata scoring
// artifact + the predecrement-guard-lea-recover-count regalloc shape, here with
// the dual-store thread (retail recomputes the blended pixel for the second store
// rather than reusing it - reproduced by writing the index expression twice). The
// per-pixel blend math is byte-faithful; the spill/recompute schedule parks it.
RVA(0x0014d950, 0x377)
void CDDrawShadeBlit::ConvertRowDouble(u8* dst, u8* src, i32 count, i32 rowDelta) {
    i32 i;
    switch (m_drawType) {
        case 2: {
            u8* base = m_palDescr->m_lut;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                dst[0] = base[(*sc << 8) + *src];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst--;
                sc--;
                src++;
            }
            break;
        }
        case 3: {
            u8* base = m_palDescr->m_lut;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                dst[0] = base[(*sc << 8) + m_light];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst--;
                sc--;
            }
            break;
        }
        case 7: {
            u16* pal1 = (u16*)m_palDescr->m_lut;
            u16* pal2 = (u16*)g_blendDescr->m_lut;
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (u16*)&g_scratch[count * 2 - 2];
            i32 rd = rowDelta & ~1;
            for (i = count; i > 0; i--) {
                u32 idx = pal2[*sc--];
                idx += (*src++ >> 4) << 12;
                u16 v = pal1[idx];
                *(u16*)dst = v;
                *(u16*)(dst + rd) = v;
                dst -= 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (u16*)&g_scratch[count * 2 - 2];
            u16* ss = (u16*)src;
            i32 rd = rowDelta & ~1;
            if (m_blendVariant) {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = ((u16*)m_lutBank0)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank1)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    *(u16*)(dst + rd) = (u16)r;
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = ((u16*)m_lutBank0)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank1)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_lutBank2)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    *(u16*)(dst + rd) = (u16)r;
                    dst -= 2;
                }
            }
            break;
        }
    }
}
