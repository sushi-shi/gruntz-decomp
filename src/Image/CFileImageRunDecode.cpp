// CFileImageRunDecode.cpp - the CFileImage RLE row-decoders (DIRSURF.CPP), which
// retail compiled UNOPTIMIZED (/Od): a full ebp frame with every local spilled to
// the stack, no register allocation or strength reduction. They Lock the surface,
// walk it row by row (the row base via the pitch-scale helper 0x1413c0), and RLE-
// expand the encoded byte stream into each scanline, then Unlock via the slot-0x80
// thunk (0x1413b0). DecodeRun8 writes 8bpp pixels, DecodeRun24 24bpp triples;
// RunDecode1/RunDecode3 take an explicit (dst,src,w,h) and decode one buffer.
//
// Modeled offset-faithfully: the surface width/height come from the m_1c/m_18
// getters (GetWidth/GetHeight, 0x141310/0x141320), the row base from Scale (0x1413c0,
// m_pitch * row), the teardown from UnlockThunk (0x1413b0) - all reloc-masked external
// __thiscall calls on the same surface object (CFileImage). Locals are declared in
// retail's /Od stack-slot order so the [ebp-N] displacements match byte-for-byte.
#include <Image/Image.h>
#include <rva.h>

// The RLE row-decoders dispatch to the same-object surface accessors declared on
// CFileImage: the m_width/m_height getters (0x141310/0x141320), the pitch-scale row
// base Scale (0x1413c0, m_pitch * row), the Unlock thunk (0x1413b0), and Lock
// (0x13e6d0) - all reloc-masked external __thiscall leaves (bodies in other TUs).

// ---------------------------------------------------------------------------
// CFileImage::DecodeRun8 (ret 4) - RLE-decode an 8bpp run-stream (arg0)
// into the locked surface, row by row. Each token: the high two bits set (& 0xc0
// == 0xc0) means a run of (token & 0x3f) copies of the following byte; otherwise
// the token itself is one literal pixel. A run that overflows the current scanline
// carries the remainder to the next row.
// @early-stop
// /Od local-slot-ordering wall (docs/patterns/od-local-slot-ordering.md): the
// instruction stream is byte-identical to retail; only the [ebp-N] local-slot
// displacements differ (retail lays locals out sequentially in declaration order,
// our same-order recompile permutes them) - ~99.5% fuzzy / ~85% byte.
RVA(0x00140aa0, 0x1a3)
i32 CFileImage::DecodeRun8(void* src) {
    u8* sp;
    i32 carry;
    u8 pixel;
    i32 width;
    i32 locked;
    i32 row;
    i32 run;
    u8* dst;
    i32 k;
    i32 height;
    i32 cols;
    if (src == 0) {
        return 0;
    }
    width = this->GetWidth();
    height = this->GetHeight();
    carry = 0;
    sp = (u8*)src;
    locked = this->Lock(0);
    if (locked == 0) {
        return 0;
    }
    for (row = 0; row < height; row++) {
        dst = (u8*)(locked + this->Scale(row));
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst++;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst++;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst++;
                cols--;
            }
        }
    }
    this->UnlockThunk();
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::RunDecode1 (ret 0x10) - the plain-buffer 8bpp variant of
// DecodeRun8: RLE-decode `src` into `dst` (no surface Lock; dimensions explicit).
// Each row starts at dst + width*row; same token grammar as DecodeRun8.
// @early-stop
// /Od local-slot-ordering wall (docs/patterns/od-local-slot-ordering.md): byte-
// identical instruction stream, only the [ebp-N] local displacements differ.
RVA(0x00145270, 0x17a)
i32 CFileImage::RunDecode1(void* dstBuf, void* src, i32 width, i32 height) {
    u8* sp;
    i32 carry;
    u8 pixel;
    i32 row;
    i32 run;
    u8* dst;
    i32 k;
    i32 cols;
    if (dstBuf == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    carry = 0;
    sp = (u8*)src;
    dst = 0;
    for (row = 0; row < height; row++) {
        dst = (u8*)dstBuf + width * row;
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst++;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst++;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst++;
                cols--;
            }
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::DecodeRun24 (ret 4) - the 24bpp surface RLE decoder. Like
// DecodeRun8 but planar: each row is decoded as three independent stride-3 channel
// scanlines (R at the +2 byte, G at +1, B at +0 of each BGR triple), with the run
// carry and source cursor continuous across channel and row boundaries. The row
// base is the pitch-scale helper (Scale(row)); width/height come from the geometry
// getters (re-read per use, not cached - retail's /Od shape).
// @early-stop
// /Od local-slot-ordering wall (docs/patterns/od-local-slot-ordering.md): byte-
// identical instruction stream, only the [ebp-N] local displacements differ.
RVA(0x00140c50, 0x3e2)
i32 CFileImage::DecodeRun24(void* src) {
    u8* sp;
    i32 carry;
    u8 pixel;
    i32 locked;
    i32 row;
    i32 run;
    u8* dst;
    i32 k;
    i32 cols;
    if (src == 0) {
        return 0;
    }
    locked = this->Lock(0);
    if (locked == 0) {
        return 0;
    }
    carry = 0;
    sp = (u8*)src;
    dst = 0;
    for (row = 0; row < this->GetHeight(); row++) {
        dst = (u8*)(locked + this->Scale(row) + 2);
        cols = this->GetWidth();
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
        dst = (u8*)(locked + this->Scale(row) + 1);
        cols = this->GetWidth();
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
        dst = (u8*)(locked + this->Scale(row));
        cols = this->GetWidth();
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
    }
    this->UnlockThunk();
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::RunDecode3 (ret 0x10) - the plain-buffer 24bpp variant of
// DecodeRun24: three stride-3 channel passes per row into `dst` (channels at +0,
// +1, +2), row base = dst + row*width*3 (cached once per row). No surface Lock.
// @early-stop
// /Od local-slot-ordering wall (docs/patterns/od-local-slot-ordering.md): byte-
// identical instruction stream, only the [ebp-N] local displacements differ.
RVA(0x001453f0, 0x3ac)
i32 CFileImage::RunDecode3(void* dstBuf, void* src, i32 width, i32 height) {
    u8* sp;
    i32 carry;
    u8 pixel;
    i32 row;
    i32 run;
    u8* dst;
    i32 k;
    i32 cols;
    i32 base;
    if (dstBuf == 0) {
        return 0;
    }
    if (src == 0) {
        return 0;
    }
    carry = 0;
    sp = (u8*)src;
    dst = 0;
    for (row = 0; row < height; row++) {
        base = row * width * 3;
        dst = (u8*)dstBuf + base;
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
        dst = (u8*)dstBuf + base + 1;
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
        dst = (u8*)dstBuf + base + 2;
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
    }
    return 1;
}
