// CDDrawShadeBlit.cpp - DDrawMgr software shaded-sprite blitter (tracer
// placeholder ClassUnknown_86). Methods in ascending retail-RVA order.
//
// 0x1497f0 Blit       - validate the destination rect, select the per-draw-type
//                       translucency LUTs, dispatch to one of four mode loops.
// 0x14a200 BlitLoop   - the big RLE-decode + 64KB-blend-table inner blit loop.
//
// Field names are placeholders; offsets + code bytes are load-bearing. The three
// other mode-loop callees (0x149950/0x149d00/0x14b770) and the surface Lock are
// external/reloc-masked. See <Gruntz/CDDrawShadeBlit.h> for the layout.
#include <Gruntz/CDDrawShadeBlit.h>

#include <rva.h>
#include <string.h> // inline rep-movs memcpy intrinsic

// The live screen RGB-format shift table at 0x683ea0..0x683eb4 - already named by
// CLightFxRender.cpp; the mode-2 gate compares these against the magic 5/10/3/3/3
// state. Reloc-masked.
DATA(0x00283ea0)
extern i32 g_rUp; // 0x683ea0
DATA(0x00283ea4)
extern i32 g_gUp; // 0x683ea4
DATA(0x00283eac)
extern i32 g_rDown; // 0x683eac
DATA(0x00283eb0)
extern i32 g_gDown; // 0x683eb0
DATA(0x00283eb4)
extern i32 g_bDown; // 0x683eb4

// The three 2048-byte-strided translucency LUT banks selected by the light level.
extern u8 g_lutBank1_653ca0[];
extern u8 g_lutBank2_663ca0[];
extern u8 g_lutBank0_673ca0[];

RVA(0x001497f0, 0x154)
i32 CDDrawShadeBlit::Blit(ShadeRect* p0, ShadeSrc* src, ShadeRect* clip, i32 sel, i32 p4) {
    if (clip->m_00 < 0 || clip->m_08 > m_04 - 1 || clip->m_04 < 0 || clip->m_0c > m_08 - 1) {
        return 0;
    }

    i32 mode = src->m_b0;
    m_29 = (u8)mode;
    if ((u8)mode == 2) {
        if (g_rDown == 3 && g_gDown == 3 && g_bDown == 3 && g_rUp == 0xa && g_gUp == 5) {
            m_2c = 1;
        } else {
            m_2c = 0;
        }
    }

    i32 drawType = m_14;
    if (drawType == 1) {
        if (sel) {
            BlitMode_149d00(p0, src, clip, p4);
        } else {
            BlitMode_149950(p0, src, clip, p4);
        }
        return 1;
    }

    if (drawType == 7) {
        if (m_28 != 1 || (u8)mode != 2) {
            return 0;
        }
    }
    if (drawType == 0xa || drawType == 0xb) {
        if (m_28 != 1 || (u8)mode != 2) {
            return 0;
        }
    }
    if (drawType == 8 || drawType == 0xb) {
        i32 bank = (m_18 >> 3) * 0x800;
        m_30 = g_lutBank0_673ca0 + bank;
        m_34 = g_lutBank1_653ca0 + bank;
        m_38 = g_lutBank2_663ca0 + bank;
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
// the RLE stream past the top clip->m_04 rows, compute the dest row-start for the
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
void CDDrawShadeBlit::BlitMode_149950(ShadeRect* dst, ShadeSrc* surf, ShadeRect* clip, i32 vflip) {
    i32 pitch = surf->m_20;
    u8* base = surf->Lock(0);

    i32 row = 0, pos = 0, x = 0;
    // Prepass: skip the top clip->m_04 rows of the RLE stream.
    if (clip->m_04 > 0) {
        do {
            u32 b = (u8)m_0c[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += (i32)b * m_28 + 1;
            }
            if (x >= m_04) {
                row++;
                x = 0;
            }
        } while (row < clip->m_04);
    }

    // Dest row-start for the (optionally v-flipped) rect.
    if (vflip) {
        base += dst->m_0c * pitch + dst->m_00 * m_29;
        pitch = -pitch;
    } else {
        base += dst->m_04 * pitch + dst->m_00 * m_29;
    }

    x = 0;
    if (clip->m_00 != 0) {
        // Left edge clipped (the run that crosses clip->m_00 is partially copied).
        while (row < clip->m_0c) {
            if (pos >= m_10) {
                break;
            }
            if (x < clip->m_00) {
                i32 trans = 0;
                do {
                    u32 b = (u8)m_0c[pos];
                    if (b & 0x80) {
                        x += b - 0x80;
                        pos++;
                        trans = 1;
                    } else {
                        x += b;
                        pos += (i32)b * m_28 + 1;
                        trans = 0;
                    }
                } while (x < clip->m_00);
                if (x > clip->m_00 && trans == 0) {
                    i32 bytes = (x - clip->m_00) * m_28;
                    memcpy(base, &m_0c[pos] - bytes, bytes);
                }
            }
            if (x >= m_04) {
                row++;
                base += pitch;
                x = 0;
            } else {
                u32 b = (u8)m_0c[pos];
                if (b & 0x80) {
                    x += b - 0x80;
                    pos++;
                } else {
                    memcpy(base + (x - clip->m_00) * m_29, &m_0c[pos + 1], (i32)b * m_28);
                    x += b;
                    pos += (i32)b * m_28 + 1;
                }
            }
        }
    } else if (clip->m_08 != m_04 - 1) {
        // Right edge clipped.
        while (row <= clip->m_0c) {
            if (pos >= m_10) {
                break;
            }
            u32 b = (u8)m_0c[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                i32 bytes;
                if (x + (i32)b < clip->m_08) {
                    bytes = (i32)b * m_28;
                } else {
                    i32 vis = (clip->m_08 - x) * m_28;
                    bytes = vis < 0 ? 0 : vis;
                }
                memcpy(base + x * m_29, &m_0c[pos + 1], bytes);
                x += b;
                pos += (i32)b * m_28 + 1;
            }
            if (x >= m_04) {
                row++;
                base += pitch;
                x = 0;
            }
        }
    } else {
        // Full-width: no horizontal clipping.
        while (row <= clip->m_0c) {
            if (pos >= m_10) {
                break;
            }
            u32 b = (u8)m_0c[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                memcpy(base + x * m_29, &m_0c[pos + 1], (i32)b * m_28);
                x += b;
                pos += (i32)b * m_28 + 1;
            }
            if (x >= m_04) {
                row++;
                base += pitch;
                x = 0;
            }
        }
    }

    surf->m_08->vtbl->Unlock(surf->m_08, 0);
}

// ===========================================================================
// CDDrawShadeBlit::BlitMode_149d00  (0x149d00) - the SELECTED (horizontally
// flipped) sibling of BlitMode_149950. Same prepass / v-flip start / three clip
// loops, but x runs from the sprite width down to 0 and each opaque run is copied
// in reverse (dest pointer decreasing). The per-pixel size (m_28 == 1 -> byte,
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
void CDDrawShadeBlit::BlitMode_149d00(ShadeRect* dst, ShadeSrc* surf, ShadeRect* clip, i32 vflip) {
    i32 pitch = surf->m_20;
    u8* base = surf->Lock(0);

    i32 row = 0, pos = 0, x = 0;
    if (clip->m_04 > 0) {
        do {
            u32 b = (u8)m_0c[pos];
            if (b & 0x80) {
                x += b - 0x80;
                pos++;
            } else {
                x += b;
                pos += (i32)b * m_28 + 1;
            }
            if (x >= m_04) {
                row++;
                x = 0;
            }
        } while (row < clip->m_04);
    }
    if (vflip) {
        base += dst->m_0c * pitch + dst->m_00 * m_29;
        pitch = -pitch;
    } else {
        base += dst->m_04 * pitch + dst->m_00 * m_29;
    }

    x = m_04;
    if (clip->m_00 != 0) {
        // Left edge clipped, h-flipped (run crossing clip->m_00 partially copied).
        while (row <= clip->m_0c) {
            if (pos >= m_10) {
                break;
            }
            u32 b = (u8)m_0c[pos];
            if (b & 0x80) {
                x += 0x80 - (i32)b;
                pos++;
            } else {
                i32 cnt = b;
                u8* sd = &m_0c[pos + 1];
                i32 bytes;
                if (x - cnt > clip->m_00) {
                    bytes = cnt * m_28;
                } else {
                    i32 vis = (x - clip->m_00) * m_28;
                    bytes = vis < 0 ? 0 : vis;
                }
                u8* dbase = base + (x - clip->m_00) * m_29;
                if (m_28 == 1) {
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
                pos += cnt * m_28 + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_04;
            }
        }
    } else if (clip->m_08 != m_04 - 1) {
        // Right edge clipped, h-flipped: skip runs while x > clip->m_08.
        while (row < clip->m_0c) {
            if (pos >= m_10) {
                break;
            }
            if (x > clip->m_08) {
                i32 trans = 0;
                do {
                    u32 b = (u8)m_0c[pos];
                    if (b & 0x80) {
                        x += 0x80 - (i32)b;
                        pos++;
                        trans = 1;
                    } else {
                        x -= b;
                        pos += (i32)b * m_28 + 1;
                        trans = 0;
                    }
                } while (x > clip->m_08);
                if (x >= 0 && trans == 0) {
                    i32 bytes = (clip->m_08 - x) * m_28;
                    u8* s = &m_0c[pos] - bytes;
                    if (m_28 == 1) {
                        u8* d = base + clip->m_08 * m_29;
                        for (i32 k = bytes; k > 0; k--) {
                            *d-- = *s++;
                        }
                    } else {
                        u16* d = (u16*)(base + clip->m_08 * m_29);
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
                x = m_04;
            } else {
                u32 b = (u8)m_0c[pos];
                if (b & 0x80) {
                    x += 0x80 - (i32)b;
                    pos++;
                } else {
                    i32 cnt = b;
                    u8* s = &m_0c[pos + 1];
                    if (m_28 == 1) {
                        u8* d = base + x * m_29;
                        for (i32 k = cnt; k > 0; k--) {
                            *d-- = *s++;
                        }
                    } else {
                        u16* d = (u16*)(base + x * m_29);
                        u16* sw = (u16*)s;
                        for (i32 k = cnt; k > 0; k--) {
                            *d-- = *sw++;
                        }
                    }
                    x -= cnt;
                    pos += cnt * m_28 + 1;
                }
            }
        }
    } else {
        // Full-width, h-flipped: x: m_04 -> 0, runs copied reversed.
        while (row <= clip->m_0c) {
            if (pos >= m_10) {
                break;
            }
            u32 b = (u8)m_0c[pos];
            if (b & 0x80) {
                x += 0x80 - (i32)b;
                pos++;
            } else {
                i32 cnt = b;
                u8* s = &m_0c[pos + 1];
                if (m_28 == 1) {
                    u8* d = base + x * m_29;
                    for (i32 k = cnt; k > 0; k--) {
                        *d-- = *s++;
                    }
                } else {
                    u16* d = (u16*)(base + x * m_29);
                    u16* sw = (u16*)s;
                    for (i32 k = cnt; k > 0; k--) {
                        *d-- = *sw++;
                    }
                }
                x -= cnt;
                pos += cnt * m_28 + 1;
            }
            if (x <= 0) {
                row++;
                base += pitch;
                x = m_04;
            }
        }
    }

    surf->m_08->vtbl->Unlock(surf->m_08, 0);
}

// @early-stop
// 5363 B software alpha-compositor: deferred to the final sweep (too large to
// converge in budget; a partial body diverges regalloc and under-counts). It
// Lock()s the destination DirectDraw surface (src->m_20 via CDirSurf::Lock
// 0x13e6d0, the only call carrying the "C:\Proj\DDrawMgr\DIRSURF.CPP" string),
// then runs the per-blend-mode RLE blit: it scans the high-bit RLE sprite stream
// (`test cl,0x80` -> run vs literal), decodes each row into the global scratch
// line DAT_006bed08, and writes pixels through the 64KB 2D blend tables
// (LUT[(srcByte<<8)|dstByte], via this->m_30/m_34/m_38 and the [this->m_1c]+8
// channel table) into the locked surface. FOUR dense jump tables dispatch on
// (this->m_14 - 2): 0x54b6f4 / 0x54b710 (8/10 cases) for the 8bpp paths and
// 0x54b738 / 0x54b754 for the 16bpp paths (`mov si,[esi+ebp*2]; or; mov [eax],cx`).
// DAT_006bf218 is a secondary surface/format descriptor. Identity, switch tags,
// and the LUT plumbing are confirmed; the per-case blend bodies remain.
RVA(0x0014a200, 0x14f3)
void CDDrawShadeBlit::BlitLoop(ShadeRect* p0, ShadeSrc* src, ShadeRect* clip, i32 p4) {}

// The global scratch line the row converter saves the destination into before an
// in-place blend (DAT_006bed08), and the secondary palette/format descriptor
// (DAT_006bf218) used by the 16-bit alpha path. Reloc-masked.
extern u8 g_scratch[]; // 0x6bed08
DATA(0x002bf218)
extern ShadeDescr* g_blendDescr; // 0x6bf218

// @early-stop
// 4637 B SELECTED (h-flipped) shaded RLE blitter - the sel-path twin of the
// BlitLoop (0x14a200) software alpha-compositor: deferred to the final sweep (too
// large to converge in budget; a partial body diverges regalloc and under-counts).
// Lock()s the destination surface (surf->Lock, 0x13e6d0), fast-forwards the RLE
// stream past clip->m_04 rows, then runs the per-blend-mode RLE blit reversed
// (h-flip). THREE dense jump tables dispatch on (m_14 - 2): 0x54c990, 0x54c9ac,
// 0x54c9d4 (one per horizontal-clip case = full/right/left). Each decodes the
// high-bit RLE into the global scratch line DAT_006bed08 (+ the sub-offset
// DAT_006bed06/07/09 partial-pixel taps), reads the 16bpp blend descriptor
// DAT_006bf218 (g_blendDescr), and writes through this->m_30/m_34/m_38 +
// [this->m_1c]+8 channel tables. Calls the row-blend helpers 0x14cfc0 and 0x14d950
// per case, and the IDirectDrawSurface Unlock via [vtbl+0x80]. Identity, switch
// tags, LUT plumbing and the clip control flow are confirmed; the per-case reverse
// blend bodies remain. See docs/patterns/jumptable-data-overlap.md.
RVA(0x0014b770, 0x121d)
void CDDrawShadeBlit::BlitMode_14b770(ShadeRect* dst, ShadeSrc* surf, ShadeRect* clip, i32 vflip) {}

// @early-stop
// ~56% (logic complete + correct). 1446 B dense-jump-table per-row format/blend
// converter (one of the four tables the BlitLoop family dispatches). Nine cases
// on (m_14 - 2) over a single row: 8/16-bit palette LUTs (m_1c->m_8 /
// g_blendDescr->m_8), RGB565 channel-split blends via m_30/m_34/m_38, and a
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
    switch (m_14) {
        case 2: {
            u8* pal = m_1c->m_08;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
                *dst++ = pal[(*sc++ << 8) + *src++];
            }
            break;
        }
        case 7: {
            u16* pal1 = (u16*)m_1c->m_08;
            u16* pal2 = (u16*)g_blendDescr->m_08;
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
            u16* pal = (u16*)m_1c->m_08;
            for (i = count; i > 0; i--) {
                *(u16*)dst = pal[*src++];
                dst += 2;
            }
            break;
        }
        case 8: {
            memcpy(g_scratch, dst, count * 2);
            if (m_2c) {
                u16* sd = (u16*)g_scratch;
                u16* ss = (u16*)src;
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 b = *sd++;
                    u32 r = ((u16*)m_38)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    r |= ((u16*)m_34)[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_30)[(a >> 0xa) + (b & 0xffe0)];
                    *(u16*)dst = (u16)r;
                    dst += 2;
                }
            } else {
                u16* sd = (u16*)g_scratch;
                u16* ss = (u16*)src;
                for (i = count; i > 0; i--) {
                    u32 a = *sd++;
                    u32 b = *ss++;
                    u32 r = ((u16*)m_30)[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_34)[((a >> 0xb)) + (b & 0xffe0)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst += 2;
                }
            }
            break;
        }
        case 11: {
            u16* pal = (u16*)m_1c->m_08;
            memcpy(g_scratch, dst, count * 2);
            if (m_2c) {
                u16* sd = (u16*)g_scratch;
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 b = *sd++;
                    u32 r = ((u16*)m_34)[((a >> 5) & 0x1f) + (((b >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_30)[(a >> 0xa) + (b & 0xffe0)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst += 2;
                }
            } else {
                u16* sd = (u16*)g_scratch;
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 b = *sd++;
                    u32 r = ((u16*)m_30)[((a >> 6) & 0x1f) + (((b >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_34)[(a >> 0xb) + (b & 0xffe0)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((b & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst += 2;
                }
            }
            break;
        }
        case 3: {
            i32 pal = (i32)m_1c->m_08;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
                *dst++ = ((u8*)m_18)[(*sc++ << 8) + pal];
            }
            break;
        }
        case 4: {
            i32 pal = (i32)m_1c->m_08;
            for (i = count; i > 0; i--) {
                *dst++ = ((u8*)m_18)[(*src++ << 8) + pal];
            }
            break;
        }
        case 5: {
            for (i = count; i > 0; i--) {
                *dst++ = (u8)m_18;
            }
            break;
        }
        case 6: {
            u8* pal = m_1c->m_08;
            u8* sc = g_scratch;
            memcpy(g_scratch, dst, count);
            for (i = count; i > 0; i--) {
                i32 s = pal[*sc++ + 0x100];
                i32 d = pal[*src + 0x100];
                i32 t = (d - s) * m_18 / 255 + s;
                *dst++ = pal[t];
                src++;
            }
            break;
        }
    }
}

// ===========================================================================
// 0x14cfc0 - ConvertRowFlip: the horizontally-mirrored twin of ConvertRow (the
// selected-blit row converter). Same dense (m_14-2) jump table over nine blend
// cases, but the destination run is walked right-to-left and the saved-dest
// scratch line is read back to front (rep-movs saves the run ending at dst). The
// 8-bit cases write `*dst--`; the 16-bit RGB565 channel blends decrement by 2.
// `base` = m_1c ? m_1c->m_08 : src (computed once before the switch).
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
    u8* base = m_1c ? m_1c->m_08 : src;
    i32 i;
    switch (m_14) {
        case 2: {
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                *dst-- = base[(*sc-- << 8) + *src++];
            }
            break;
        }
        case 7: {
            u16* pal1 = (u16*)m_1c->m_08;
            u16* pal2 = (u16*)g_blendDescr->m_08;
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
            u16* pal = (u16*)m_1c->m_08;
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
            if (m_2c) {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = ((u16*)m_34)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= ((u16*)m_30)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = ((u16*)m_30)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= ((u16*)m_34)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst -= 2;
                }
            }
            break;
        }
        case 11: {
            u16* pal = (u16*)m_1c->m_08;
            memcpy(g_scratch, dst - count * 2 - 2, count * 2);
            u16* sc = (u16*)&g_scratch[count * 2 - 2];
            if (m_2c) {
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 d = *sc--;
                    u32 r = ((u16*)m_34)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= ((u16*)m_30)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = pal[*src++];
                    u32 d = *sc--;
                    u32 r = ((u16*)m_34)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    r |= ((u16*)m_30)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    dst -= 2;
                }
            }
            break;
        }
        case 3: {
            i32 pal = (i32)base;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                *dst-- = ((u8*)m_18)[(*sc-- << 8) + pal];
            }
            break;
        }
        case 4: {
            i32 pal = (i32)base;
            for (i = count; i > 0; i--) {
                *dst-- = ((u8*)m_18)[(*src++ << 8) + pal];
            }
            break;
        }
        case 5: {
            for (i = count; i > 0; i--) {
                *dst-- = (u8)m_18;
            }
            break;
        }
        case 6: {
            memcpy(g_scratch, dst - count - 1, count);
            u8* sc = &g_scratch[count + 1];
            for (i = count; i > 0; i--) {
                i32 s = base[*sc-- + 0x100];
                i32 d = base[*src + 0x100];
                i32 t = (d - s) * m_18 / 255 + s;
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
// dst and the saved-dest scratch line UP. Dense (m_14-2) jump table over cases
// 2/3/7/8 (4/5/6 fall through). Case 3 is symmetric (both rows get the m_18 LUT of
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
    switch (m_14) {
        case 2: {
            u8* base = m_1c->m_08;
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
            i32 pal = (i32)m_1c->m_08;
            memcpy(g_scratch, dst, count);
            u8* sc = g_scratch;
            for (i = count; i > 0; i--) {
                dst[0] = ((u8*)m_18)[(*sc << 8) + pal];
                dst[rowDelta] = ((u8*)m_18)[(*sc << 8) + pal];
                dst++;
                sc++;
            }
            break;
        }
        case 7: {
            u16* pal1 = (u16*)m_1c->m_08;
            u16* pal2 = (u16*)g_blendDescr->m_08;
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
            if (m_2c) {
                for (i = count; i > 0; i--) {
                    u32 d = *sc++;
                    u32 a = *ss++;
                    u32 r = ((u16*)m_30)[(a >> 0xa) + ((d >> 5) & 0xffe0)];
                    r |= ((u16*)m_34)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    *(u16*)(dst + rd) = (u16)r;
                    dst += 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 d = *sc++;
                    u32 a = *ss++;
                    u32 r = ((u16*)m_30)[(a >> 0xb) + ((d >> 6) & 0xffe0)];
                    r |= ((u16*)m_34)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((d & 0x1f) << 5)];
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
// converted pixel to dst AND dst+rowDelta). Five (m_14-2) cases (2/3/7/8; 4/5/6
// fall through to the empty default). The byte cases reverse-walk dst with the
// saved-dest scratch line; the word cases round rowDelta down to even. Case 3 is
// asymmetric: the dst row gets the m_18 LUT of the saved dest, the dst+rowDelta
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
    switch (m_14) {
        case 2: {
            u8* base = m_1c->m_08;
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
            i32 pal = (i32)m_1c->m_08;
            u8* base = m_1c->m_08;
            memcpy(g_scratch, dst - count + 1, count);
            u8* sc = &g_scratch[count - 1];
            for (i = count; i > 0; i--) {
                dst[0] = ((u8*)m_18)[(*sc << 8) + pal];
                dst[rowDelta] = base[(*sc << 8) + *src];
                dst--;
                sc--;
            }
            break;
        }
        case 7: {
            u16* pal1 = (u16*)m_1c->m_08;
            u16* pal2 = (u16*)g_blendDescr->m_08;
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
            if (m_2c) {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = ((u16*)m_30)[(a >> 0xa) + (((d >> 0xa) & 0x1f) << 5)];
                    r |= ((u16*)m_34)[((a >> 5) & 0x1f) + (((d >> 5) & 0x1f) << 5)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    *(u16*)(dst + rd) = (u16)r;
                    dst -= 2;
                }
            } else {
                for (i = count; i > 0; i--) {
                    u32 a = *ss++;
                    u32 d = *sc--;
                    u32 r = ((u16*)m_30)[(a >> 0xb) + (((d >> 0xb) & 0x1f) << 5)];
                    r |= ((u16*)m_34)[((a >> 6) & 0x1f) + (((d >> 6) & 0x1f) << 5)];
                    r |= ((u16*)m_38)[(a & 0x1f) + ((d & 0x1f) << 5)];
                    *(u16*)dst = (u16)r;
                    *(u16*)(dst + rd) = (u16)r;
                    dst -= 2;
                }
            }
            break;
        }
    }
}
