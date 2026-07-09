#include <rva.h>
// ImageOwned.cpp - the build/decode methods of the RTTI CImage's +0x30 owned
// object, which IS a CDDrawShadeBlit (the shaded sprite; former placeholder name
// "CImageOwned"). Methods in retail-RVA order: the ctor (0x148ce0) that primes the
// blit-descriptor defaults, and Build (0x1490d0) - decodes one frame out of a
// CImageBuildDesc into the decoded RLE pixel buffer (+0x0c) and a 256-entry palette
// (+0x20), copying the desc's dimension/format metadata. Teardown (0x148d10) and the
// palette-remap helper (0x1495d0) are external engine callees (reloc-masked). The
// blit side of the same class lives in src/DDrawMgr/DDrawShadeBlit.cpp; see
// <DDrawMgr/DDrawShadeBlit.h> for the shared layout.
//
// Field names are placeholders; only the OFFSETS + emitted bytes are load-bearing.
// No destructible stack local -> no /GX frame (flags="base").
// ---------------------------------------------------------------------------

#include <DDrawMgr/DDrawShadeBlit.h>
#include <Io/FileStream.h> // CFileIO (Open/Read/GetLength/Close, reloc-masked) + CString

#include <string.h> // memcpy (inlined to rep movs)

// The engine allocator/deallocator (reloc-masked rel32). Global operator new
// @0x1b9b46 (NAFXCW) is declared by <Mfc.h> (via FileStream.h); _RezFree @0x1b9b82.
extern "C" void RezFree(void* p);

// ---------------------------------------------------------------------------
// The constructor. Zero the buffers/counters; prime m_drawType=1, m_light=0x80,
// m_colorKey=-1, and both format-flag bytes m_srcBpp/m_dstBpp=1. __thiscall.
// ---------------------------------------------------------------------------
RVA(0x00148ce0, 0x2f)
CDDrawShadeBlit::CDDrawShadeBlit() {
    m_rleData = 0;
    m_rleLen = 0;
    m_palDescr = 0;
    m_drawType = 1;
    m_light = 0x80;
    m_00 = 0;
    m_palette = 0;
    m_srcBpp = 1;
    m_dstBpp = 1;
    m_colorKey = -1;
}

// The transient RLE-output buffer: an MFC CByteArray (ctor 0x1b527e, SetSize
// 0x1b52e8, SetAtGrow 0x1b5485, ~CByteArray 0x1b52b1 - all NAFXCW, reloc-masked).
// Layout: vptr@0, data@+4, size@+8, alloc@+0xc. Modeled as a tiny host so the
// thiscall calls lower with callee-side cleanup and the destructible local forces
// the /GX frame on BuildRle.
struct CRleByteArray {
    void* vptr;                          // +0x00
    u8* m_data;                          // +0x04  element buffer
    i32 m_size;                          // +0x08  element count
    i32 m_alloc;                         // +0x0c
    CRleByteArray();                     // 0x1b527e
    void SetSize(i32 n, i32 growBy);     // 0x1b52e8
    void SetAtGrow(i32 index, u8 value); // 0x1b5485
    ~CRleByteArray();                    // 0x1b52b1
};

// ---------------------------------------------------------------------------
// BuildRle - run-length encode the source plane into the owned pixel
// buffer (+0x0c). Each row of `width` pixels is scanned: a run of NON-key bytes
// emits the run length then the literal bytes; a run of key (== keyVal) bytes
// emits (length | 0x80). Runs cap at 0x7e. The encoding accumulates in a transient
// CByteArray, which is then copied into a fresh operator-new'd m_rleData; finally, if
// a palette source was supplied, 256 DWORDs are copied into a fresh m_palette. Returns 1
// (0 only if the source pointer is null). __thiscall, ret 0x18 (6 stack args).
//
// The CByteArray local -> /GX EH frame; SetAtGrow per byte; m_rleData/m_palette copies are
// inline byte/dword loops. width/height/stride come from args (stride defaults to
// width when -1). The run discriminator is `if (px != key) {literal} else {key}`
// so the literal path falls through inline and the key path floats to the tail via
// the forward `je` (docs/patterns/nested-if-success-deepest-error-tail.md).
// @early-stop
// 97.75% - the whole RLE state machine + the two run-scan extend loops + the m_rleData
// byte-copy + the m_palette palette dword-copy are byte-identical to retail. The only
// residual is the /GX scope-table EH-frame artifact: retail emits `sub esp,0x18` /
// `push 0x8` (scope cookie) / `add esp,0x24` where MSVC5 here emits `sub esp,0x14` /
// `push 0x0` / `add esp,0x20`, shifting every [esp+N] local by 4. Not source-
// steerable (docs/patterns/gx-scoped-local-eh-frame-size.md). Logic complete.
// ---------------------------------------------------------------------------
RVA(0x00148d40, 0x202)
i32 CDDrawShadeBlit::BuildRle(
    void* pixels,
    i32 width,
    i32 height,
    i32 stride,
    i32 keyVal,
    void* palette
) {
    u8* src = (u8*)pixels;
    if (src == 0) {
        return 0;
    }
    m_colorKey = keyVal;
    if (stride == -1) {
        stride = width;
    }
    m_width = width;
    m_height = height;

    CRleByteArray ba;
    ba.SetSize(0x3e8, 0);

    i32 row = 0;
    if (m_height > 0) {
        do {
            i32 i = 0;
            i32 runStart = 0;
            if (m_width > 0) {
                do {
                    if ((i32)src[i] != keyVal) {
                        // literal run (the fall-through / primary path)
                        while (i < m_width && (i - runStart) < 0x7e && (i32)src[i] != keyVal) {
                            i++;
                        }
                        ba.SetAtGrow(ba.m_size, (u8)(i - runStart));
                        for (i32 j = runStart; j < i; j++) {
                            ba.SetAtGrow(ba.m_size, src[j]);
                        }
                        runStart = i;
                    } else {
                        // key run (floated to the tail)
                        while (i < m_width && (i - runStart) < 0x7e && (i32)src[i] == keyVal) {
                            i++;
                        }
                        ba.SetAtGrow(ba.m_size, (u8)((i - runStart) | 0x80));
                        runStart = i;
                    }
                } while (i < m_width);
            }
            row++;
            src += stride;
        } while (row < m_height);
    }

    if (m_rleData != 0) {
        RezFree(m_rleData);
    }
    m_rleLen = ba.m_size;
    m_rleData = (u8*)operator new(ba.m_size);
    i32 n = m_rleLen;
    for (i32 k = 0; k < n; k++) {
        m_rleData[k] = ba.m_data[k];
    }

    if (palette != 0) {
        if (m_palette != 0) {
            RezFree(m_palette);
        }
        m_palette = (u8*)operator new(0x400);
        memcpy(m_palette, palette, 0x400);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// LoadFromFile - open the named file, slurp it whole into a fresh
// operator-new buffer, hand it to Build, then free the buffer + close the stream.
// The stack CFileIO + the by-value CString name -> a /GX EH frame. __thiscall,
// ret 8 (name + fmt). Returns Build's result, or 0 if the open failed.
// ---------------------------------------------------------------------------
RVA(0x00148fc0, 0x104)
i32 CDDrawShadeBlit::LoadFromFile(CString name, i32 fmt) {
    CFileIO file;
    if (!file.Open(name, 0x8000, 0)) {
        return 0;
    }
    void* buf = operator new(file.GetLength());
    file.Read(buf, file.GetLength());
    i32 r = Build((CImageBuildDesc*)buf, file.GetLength(), fmt);
    file.Close();
    RezFree(buf);
    return r;
}

// ---------------------------------------------------------------------------
// Decode a frame from the descriptor. The desc flag word (+0x04) and the
// format code steer two flag bytes (m_srcBpp/m_dstBpp) and the palette/pixel layout;
// on a 16-bit palette frame the 768-byte RGB palette is unpacked into a padded 0x400
// hardware buffer, then the pixels are copied into a fresh m_rleData. When m_srcBpp
// came out as 2 the pixels are run through the palette-remap helper. __thiscall, ret 0xc.
// @early-stop
// 79.7% - body byte-faithful through the palette-loop entry (prologue, flag-byte
// branches, m_colorKey/m_rleLen setup, the operator-new + 0xfffffd00 stride, the do-while
// counter structure with the mid-body `i += 3` and `cmp 0x300/jl` all exact). The
// residual is the zero/const-register-pinning wall (docs/patterns/
// zero-register-pinning.md): retail pins the constant 2 in `bl` across the whole
// body (used for the m_srcBpp/m_dstBpp byte stores AND the trailing `cmp [0x28],bl`)
// and keeps the m_palette palette pointer in `edi` inside the loop; our cl puts m_palette in
// `ebx` (clobbering bl -> a reload `mov bl,2` before the compare) and folds the
// induction var `i` into the address base (`lea (i,src)` + `m_rleLen` as index) where
// retail forms `src+m_rleLen` as the base + `i` as the scaled index. The downstream
// memcpy-remainder + Remap-tail register naming all cascade from that one loop
// allocation. No source lever flips it under /O2. Logic complete; deferred to the
// final sweep.
RVA(0x001490d0, 0x173)
i32 CDDrawShadeBlit::Build(CImageBuildDesc* src, i32 size, i32 fmt) {
    i32 flags = src->m_flags;
    if ((flags & 0x40) || (flags & 0x200)) {
        if ((u8)fmt == 0x10) {
            m_srcBpp = 1;
            m_dstBpp = 2;
        } else {
            m_srcBpp = 1;
            m_dstBpp = 1;
        }
    } else if ((u8)fmt == 0x10) {
        m_srcBpp = 2;
        m_dstBpp = 2;
    } else {
        m_srcBpp = 1;
        m_dstBpp = 1;
    }

    if (src->m_flags & 0x100) {
        m_colorKey = src->m_srcKey;
    } else {
        m_colorKey = -1;
    }

    i32 stride = size - 0x20;
    m_rleLen = stride;
    if ((u8)fmt != 0x8 && (u8)fmt != 0x10) {
        return 0;
    }

    if (src->m_flags & 0x80) {
        stride -= 0x300;
        m_rleLen = stride;
        if ((u8)fmt == 0x10) {
            if (m_palette != 0) {
                RezFree(m_palette);
            }
            m_palette = (u8*)operator new(0x400);
            i32 i = 0;
            i32 d = 0;
            do {
                d += 4;
                m_palette[d - 4] = ((u8*)src + m_rleLen)[i + 0x20];
                i += 3;
                m_palette[d - 3] = ((u8*)src + m_rleLen)[i + 0x1e];
                m_palette[d - 2] = ((u8*)src + m_rleLen)[i + 0x1f];
            } while (i < 0x300);
        }
    }

    m_width = src->m_width;
    m_height = src->m_height;
    if (m_rleData != 0) {
        RezFree(m_rleData);
    }
    m_rleData = (u8*)operator new(m_rleLen);
    memcpy(m_rleData, src->m_frameData, m_rleLen);

    if (m_srcBpp == 2) {
        void* remapped = Remap(m_rleData);
        RezFree(m_rleData);
        m_rleData = (u8*)remapped;
        RezFree(m_palette);
        m_palette = 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Rebuild - when the sprite is in format-state 1, build an 8-dword
// frame descriptor out of the current dimensions/key + the two caller ints and the
// by-value name, then hand it to DecodeFrame (0x149250). The `name` CString arrives
// by value -> the callee destroys it (the early-return path also runs ~CString),
// which forces the /GX EH frame. __thiscall, ret 0xc. Returns DecodeFrame's result,
// or 0 when m_srcBpp != 1.
//
// The descriptor's flags word (desc.f1) is computed in a register but its store was
// eliminated in retail (a partial-dead-store the optimizer left half-done): the
// branch chain (0x3d/0xbd | 0x100 | 0x80) is emitted, the result clobbered without a
// write. We keep the assignment (the only faithful source) so the chain stays live.
// @early-stop
// ~90% dead-store + regalloc wall: the m_srcBpp guard, the whole descriptor build, the
// by-value CString + rep-movs desc passing, and the DecodeFrame tail are byte-exact.
// The residual is a register coin-flip in the (dead) flags computation: retail pins
// m_palette in eax and the flags accumulator in esi (so `or esi,0x100` / `or esi,0x80`,
// 32-bit), where our cl pins flags in eax / m_palette in esi (`or ah,1` / `or al,0x80`,
// 8-bit sub-register). Retail also ELIMINATES the `mov [..],flags` store entirely
// (a partial-DCE artifact) while cl keeps it parked past the struct copy (dead).
// Neither the register pinning nor the half-DCE is source-steerable under /O2.
// ---------------------------------------------------------------------------
RVA(0x001493b0, 0xfd)
i32 CDDrawShadeBlit::Rebuild(CString name, i32 a1, i32 a2) {
    if (m_srcBpp != 1) {
        return 0;
    }
    CImageFrameRebuildDesc desc;
    i32 flags = 0x3d;
    if (m_palette != 0) {
        flags = 0xbd;
    }
    desc.f0 = 0;
    desc.f2 = m_width;
    desc.f4 = a1;
    desc.f3 = m_height;
    desc.f5 = a2;
    desc.f6 = 0;
    desc.f7 = 0;
    if (m_colorKey != -1) {
        flags |= 0x100;
        desc.f6 = (u8)m_colorKey;
    }
    if (m_palette != 0) {
        flags |= 0x80;
    }
    desc.f1 = flags;
    return DecodeFrame(name, desc);
}

// CDDrawShadeBlit::Decompress (0x1494b0, __thiscall, ret 4). Expand the internal
// high-bit RLE sprite stream (m_rleData) into the flat 8-bit destination buffer,
// one scanline at a time. Only valid for an 8-bit source (m_srcBpp==1). Each run
// control byte: bit7 set => fill (byte-0x80) pixels with the transparent/color-key
// fill value (m_colorKey, or 0 when unset); else copy `byte` literal pixels from
// the stream. When the accumulated x reaches m_width the row advances. Returns 1.
RVA(0x001494b0, 0x11a)
i32 CDDrawShadeBlit::Decompress(void* dest) {
    if (m_srcBpp != 1) {
        return 0;
    }
    if (dest == 0) {
        return 0;
    }
    i32 fill = m_colorKey;
    if (fill == -1) {
        fill = 0;
    }
    i32 x = 0;
    i32 cursor = 0;
    for (i32 y = 0; y < m_height;) {
        if (m_rleData[cursor] & 0x80) {
            memset((u8*)dest + y * m_width + x, fill, m_rleData[cursor] - 0x80);
            x += m_rleData[cursor] - 0x80;
            cursor += 1;
        } else {
            memcpy((u8*)dest + y * m_width + x, m_rleData + cursor + 1, m_rleData[cursor]);
            x += m_rleData[cursor];
            cursor += m_rleData[cursor] + 1;
        }
        if (x >= m_width) {
            y++;
            x = 0;
        }
    }
    return 1;
}

SIZE_UNKNOWN(CRleByteArray);
SIZE_UNKNOWN(CImageBuildDesc);
SIZE(CImageFrameRebuildDesc, 0x20); // 8-dword by-value frame descriptor
