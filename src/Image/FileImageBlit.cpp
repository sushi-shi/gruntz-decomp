// FileImageBlit.cpp - the CFileImage::Blit<dest><src> pixel-format converters
// (DIRSURF.CPP). Each is the leaf blitter CFileImage::Blit (0x13faa0) dispatches
// to by (dest bpp = m_bitDepth, src bpp = bitcount); the trailing digits encode
// dest_src depths (168 = dest 16bpp / src 8bpp, etc.). They all share the same
// Blit248 outer frame: optional palette null-check, Lock(0), a mode-2 (bottom-up
// flipped) vs top-down row walk, then Unlock; the per-format inner loop converts
// one pixel.
//
//   * 168 (8->16)  build a 256-entry palette->16bpp LUT (the shared g_lut16 at
//     0x683ca0) once, then per source index write LUT[idx] as a 16bpp word.
//   * 1624 (24->16) pack each B,G,R source triple into a 16bpp word inline.
//   * 2416 (16->24) unpack each 16bpp word into an R,G,B triple (word writes).
//   * 824 (24->8) / 816 (16->8) quantise each source colour to the nearest
//     palette entry by minimum sum-of-squared channel differences.
//
// The 16bpp pack/unpack uses the live screen RGB shift table at 0x683ea0..0x683eb4
// (g_rUp/g_gUp/g_rDown/g_gDown/g_bDown - the same table SaveRle16/EncodeRle16 use).
// Offsets + code bytes are load-bearing; the surface dims come through the
// CDDSurface DDSURFACEDESC scratch (m_height @+0x18, m_width @+0x1c, m_pitch @+0x20
// inside the m_desc union). Lock() returns the locked surface-bits base.
#include <Image/Image.h>
#include <rva.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <ddraw.h> // real IDirectDrawSurface dispatch (this->m_8->Unlock); Image.h above supplies windows.h
#include <Globals.h>

// The shared 256-entry palette->16bpp lookup table (file RVA 0x283ca0 = VA
// 0x683ca0, 512 bytes, ending exactly at g_rUp). Built by the 8->16 blitters.

// The live screen RGB-format shift table (file RVA 0x283ea0..0x283eb4). Same
// differently-named symbols as elsewhere; reloc-masked.
DATA(0x00283ea0)
extern i32 g_rUp; // red   up-shift   (channel position in the 16bpp word)
DATA(0x00283ea4)
extern i32 g_gUp; // green up-shift
DATA(0x00283eac)
extern i32 g_rDown; // red   down-shift (scale 8-bit -> 5/6-bit)
DATA(0x00283eb0)
extern i32 g_gDown; // green down-shift
DATA(0x00283eb4)
extern i32 g_bDown; // blue  down-shift

// ---------------------------------------------------------------------------
// CFileImage::Blit168  (8bpp src -> 16bpp dest, palette remap)
// Build a 256-entry 16bpp LUT from the source palette (the RGB shift table packs
// each {R,G,B} entry into a screen-native 16bpp word), then walk the surface row
// by row writing LUT[index] per source pixel.
// @early-stop
// Regalloc wall (~66%): the LUT-build loop's two loop-carried pointers (palette,
// LUT) take both callee-saved slots, pushing `this` out of esi into edi - which
// cascades into the blit loop (retail keeps this=esi/src=edi with no spill; ours
// shifts this=edi/src=edx and spills the source index to the stack). Logic exact;
// the inner LUT-lookup idiom is correct, only the register file is permuted.
RVA(0x0013fbb0, 0x126)
i32 CFileImage::Blit168(void* srcv, void* palv, i32 mode) {
    u8* pal = (u8*)palv;
    if (pal == 0) {
        return 0;
    }
    u16* lut = g_lut16;
    do {
        u8 r = (u8)((u8)pal[0] >> g_rDown);
        pal += 4;
        u8 g = (u8)((u8)pal[-3] >> g_gDown);
        u8 b = (u8)((u8)pal[-2] >> g_bDown);
        *lut++ = (u16)(((u32)r << g_rUp) | ((u32)g << g_gUp) | (u32)b);
    } while (lut < g_lut16 + 256);
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* src = (u8*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = g_lut16[idx];
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = g_lut16[idx];
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::Blit1624  (24bpp src -> 16bpp dest)
// Pack each B,G,R source triple straight into a screen-native 16bpp word.
// @early-stop
// Entropy wall (~71%): the per-pixel 3-byte read + shift-pack needs a stack temp
// under register pressure; retail's spill-slot scheduling and the 8/16-bit shift
// narrowing (movb vs movzx) of the channel packs diverge from our equivalent
// codegen. Logic exact; documented MSVC5 /O2 register-allocation plateau.
RVA(0x0013fce0, 0x17f)
i32 CFileImage::Blit1624(void* srcv, i32 mode) {
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* src = (u8*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 b = src[0];
                u8 g = src[1];
                u8 r = src[2];
                src += 3;
                *dst++ = (u16)(((u32)((u8)((u8)g >> g_gDown)) << g_gUp)
                               | ((u32)((u8)((u8)r >> g_rDown)) << g_rUp)
                               | (u32)((u8)((u8)b >> g_bDown)));
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 b = src[0];
                u8 g = src[1];
                u8 r = src[2];
                src += 3;
                *dst++ = (u16)(((u32)((u8)((u8)g >> g_gDown)) << g_gUp)
                               | ((u32)((u8)((u8)r >> g_rDown)) << g_rUp)
                               | (u32)((u8)((u8)b >> g_bDown)));
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::Blit2416  (16bpp src -> 24bpp dest, 6-byte/pixel word writes)
// Unpack each 16bpp word into an R,G,B triple, each stored as a zero-extended
// 16bpp word (the retail dest stride is 6 bytes per source pixel).
// @early-stop
// Entropy wall (~82%): the 16->8-bit shift narrowing on each channel unpack
// (shr bx then shl bl, with the shift-count load width varying word/dword) and
// the one stack temp are MSVC5 /O2 register-allocation coin-flips. Logic exact.
RVA(0x0013ff80, 0x184)
i32 CFileImage::Blit2416(void* srcv, i32 mode) {
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u16* src = (u16*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                dst[0] = (u16)(u8)((u8)(u16)(px >> g_rUp) << g_rDown);
                dst[1] = (u16)(u8)((u8)(u16)(px >> g_gUp) << g_gDown);
                dst[2] = (u16)(u8)((u8)px << g_bDown);
                dst += 3;
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                dst[0] = (u16)(u8)((u8)(u16)(px >> g_rUp) << g_rDown);
                dst[1] = (u16)(u8)((u8)(u16)(px >> g_gUp) << g_gDown);
                dst[2] = (u16)(u8)((u8)px << g_bDown);
                dst += 3;
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::Blit824  (24bpp src -> 8bpp dest, nearest-palette quantize)
// For each B,G,R source triple, find the palette index whose entry minimizes the
// sum of squared channel differences (entry 0 seeds the best; entries 1..255 are
// scanned, breaking early on an exact match), and write that index.
// @early-stop
// Entropy wall (large /O2 body, ~0x30b): the SSD inner search spills the source
// channels and the best/bestdist accumulators across ~7 stack temps; retail's
// exact spill-slot scheduling is an MSVC5 register-allocation coin-flip. Logic
// (channel pairing s0<->pal[+2], s1<->pal[+1], s2<->pal[+0], min-SSD, exact-match
// break) is faithful; only the regalloc/scheduling of the spills diverges.
RVA(0x00140110, 0x30b)
i32 CFileImage::Blit824(void* srcv, void* palv, i32 mode) {
    u8* pal = (u8*)palv;
    if (pal == 0) {
        return 0;
    }
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* src = (u8*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                i32 s0 = src[0];
                i32 s1 = src[1];
                i32 s2 = src[2];
                src += 3;
                i32 best = 0;
                i32 d0 = s2 - pal[0];
                i32 d1 = s1 - pal[1];
                i32 d2 = s0 - pal[2];
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < 256; k++) {
                    i32 e0 = s2 - pal[k * 4];
                    i32 e1 = s1 - pal[k * 4 + 1];
                    i32 e2 = s0 - pal[k * 4 + 2];
                    i32 d = e0 * e0 + e1 * e1 + e2 * e2;
                    if (d < bestd) {
                        best = k;
                        bestd = d;
                        if (d == 0) {
                            break;
                        }
                    }
                }
                *dst = (u8)best;
                dst++;
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                i32 s0 = src[0];
                i32 s1 = src[1];
                i32 s2 = src[2];
                src += 3;
                i32 best = 0;
                i32 d0 = s2 - pal[0];
                i32 d1 = s1 - pal[1];
                i32 d2 = s0 - pal[2];
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < 256; k++) {
                    i32 e0 = s2 - pal[k * 4];
                    i32 e1 = s1 - pal[k * 4 + 1];
                    i32 e2 = s0 - pal[k * 4 + 2];
                    i32 d = e0 * e0 + e1 * e1 + e2 * e2;
                    if (d < bestd) {
                        best = k;
                        bestd = d;
                        if (d == 0) {
                            break;
                        }
                    }
                }
                *dst = (u8)best;
                dst++;
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::Blit816  (16bpp src -> 8bpp dest, nearest-palette quantize)
// Unpack each 16bpp source word into an R,G,B triple (via the screen shift table),
// then find the palette index minimizing the sum of squared channel differences
// (entry 0 seeds the best; 1..255 scanned, exact-match break) and write it.
// @early-stop
// Entropy wall (large /O2 body, ~0x34f): the 16bpp unpack + SSD search spills the
// three channels and the best/bestdist accumulators across ~8 stack temps; retail's
// exact spill-slot scheduling and the 8/16-bit unpack narrowing are MSVC5 /O2
// register-allocation coin-flips. Logic (RGB unpack, red<->pal[0]/green<->pal[1]/
// blue<->pal[2] min-SSD, exact-match break) is faithful.
RVA(0x00140420, 0x34f)
i32 CFileImage::Blit816(void* srcv, void* palv, i32 mode) {
    u8* pal = (u8*)palv;
    if (pal == 0) {
        return 0;
    }
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u16* src = (u16*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                i32 red = (u8)((u8)(u16)(px >> g_rUp) << g_rDown);
                i32 green = (u8)((u8)(u16)(px >> g_gUp) << g_gDown);
                i32 blue = (u8)((u8)px << g_bDown);
                i32 best = 0;
                i32 d1 = green - pal[1];
                i32 d2 = blue - pal[2];
                i32 d0 = red - pal[0];
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < 256; k++) {
                    i32 e0 = red - pal[k * 4];
                    i32 e1 = green - pal[k * 4 + 1];
                    i32 e2 = blue - pal[k * 4 + 2];
                    i32 d = e0 * e0 + e1 * e1 + e2 * e2;
                    if (d < bestd) {
                        best = k;
                        bestd = d;
                        if (d == 0) {
                            break;
                        }
                    }
                }
                *dst = (u8)best;
                dst++;
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                i32 red = (u8)((u8)(u16)(px >> g_rUp) << g_rDown);
                i32 green = (u8)((u8)(u16)(px >> g_gUp) << g_gDown);
                i32 blue = (u8)((u8)px << g_bDown);
                i32 best = 0;
                i32 d1 = green - pal[1];
                i32 d2 = blue - pal[2];
                i32 d0 = red - pal[0];
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < 256; k++) {
                    i32 e0 = red - pal[k * 4];
                    i32 e1 = green - pal[k * 4 + 1];
                    i32 e2 = blue - pal[k * 4 + 2];
                    i32 d = e0 * e0 + e1 * e1 + e2 * e2;
                    if (d < bestd) {
                        best = k;
                        bestd = d;
                        if (d == 0) {
                            break;
                        }
                    }
                }
                *dst = (u8)best;
                dst++;
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}
