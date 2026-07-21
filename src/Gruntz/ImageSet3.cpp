#include <Gruntz/ImageSets.h>
#include <rva.h>

#include <string.h> // memcpy

RVA(0x001614b0, 0x1c)
void CImageSet3::FreePixels() {
    if (m_pixels) {
        ::operator delete(m_pixels);
    }
    m_pixels = 0;
}

// CImageSet3::Parse (0x166d70, ??_7CImageSet3 slot +0x14). Reads tile width/height
// from the record at +0x08/+0x0c, derives the height log2 shift and the byte size,
// and - only when the width is the matching power of two - allocates and copies the
// tile pixels from the record at +0x10 (inline memcpy). TRUE on a successful copy.
// @early-stop
// regalloc wall (~88%): retail parks the width in callee-saved edi (push edi, mov
// edi,ecx) and multiplies via edx; cl keeps the width in edx and multiplies into
// ecx (one fewer move). Logic + memcpy byte-exact; not source-steerable.
RVA(0x00166d70, 0x8d)
i32 CImageSet3::Parse(void* record) {
    i32* p = reinterpret_cast<i32*>((reinterpret_cast<char*>(record) + 8));
    i32 w = *p++;
    m_width = w;
    i32 h = *p++;
    m_height = h;
    m_heightLog2 = 0;
    m_byteSize = w * h;
    for (; h > 1; h >>= 1) {
        m_heightLog2++;
    }
    if ((1 << m_heightLog2) != w) {
        return 0;
    }
    // the (u8*) is language-forced (operator new returns void*), not a mis-model cast
    u8* dst = static_cast<u8*>(::operator new(m_byteSize));
    m_pixels = dst;
    if (dst == 0) {
        return 0;
    }
    memcpy(dst, p, m_byteSize);
    return 1;
}

RVA(0x00166eb0, 0x6a)
i32 CImageSet3::ScanUp(i32 x, i32 y, i32* outY, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (y > 0) {
        off -= m_width;
        --y;
        if ((m_pixels)[off] != target) {
            *outY = y;
            *outVal = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

// 0x166f20 (slot 13): scan UP for the first row whose column-x pixel EQUALS `val`;
// report the row in *outY. Pointer walk (the value gate uses the free register).
// @early-stop
// ~82% regalloc wall (same instr count; value gate + coord out compete for the
// callee-saved reg, flipping this/base spill). docs/patterns/zero-register-pinning.md.
RVA(0x00166f20, 0x52)
i32 CImageSet3::ScanUpGate(i32 x, i32 y, i32 val, i32* outY) {
    u8* p = m_pixels + ((y << m_heightLog2) + x);
    while (y > 0) {
        p -= m_width;
        --y;
        if (*p == val) {
            *outY = y;
            return 1;
        }
    }
    return 0;
}

// 0x166f80 (slot 14): scan RIGHT for the first column whose pixel differs from (x,y)'s;
// report it + its value. Stops at the m_width-1 edge.
// @early-stop
// ~78% regalloc wall: the lim (m_width-1) local competes with `this` for a callee-
// saved reg (the lim-free ScanUp is 100%); same instrs, swapped operands.
RVA(0x00166f80, 0x68)
i32 CImageSet3::ScanRight(i32 x, i32 y, i32* outX, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    i32 lim = m_width - 1;
    while (x < lim) {
        ++x;
        ++off;
        if ((m_pixels)[off] != target) {
            *outX = x;
            *outVal = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

// 0x166ff0 (slot 15): scan RIGHT for the first column whose pixel EQUALS `val`; report
// the column in *outX. Pointer walk, stops at the m_width-1 edge.
// @early-stop
// ~72% regalloc wall (lim + pointer-walk + value-gate pressure). Logic byte-faithful.
RVA(0x00166ff0, 0x52)
i32 CImageSet3::ScanRightGate(i32 x, i32 y, i32 val, i32* outX) {
    i32 lim = m_width - 1;
    u8* p = m_pixels + ((y << m_heightLog2) + x);
    while (x < lim) {
        ++x;
        ++p;
        if (*p == val) {
            *outX = x;
            return 1;
        }
    }
    return 0;
}

// 0x167050 (slot 16): scan DOWN for the first row whose column-x pixel differs from
// (x,y)'s; report it + its value. Stops at the m_height-1 edge.
// @early-stop
// ~70% regalloc wall: retail keeps this in ebp (found-block m_pixels re-read) + spills
// lim; our cl spills this + keeps lim. Same instrs. docs/patterns/zero-register-pinning.md.
RVA(0x00167050, 0x74)
i32 CImageSet3::ScanDown(i32 x, i32 y, i32* outY, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    i32 lim = m_height - 1;
    while (y < lim) {
        off += m_width;
        ++y;
        if ((m_pixels)[off] != target) {
            *outY = y;
            *outVal = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

// 0x1670d0 (slot 17): scan DOWN for the first row whose column-x pixel EQUALS `val`;
// report the row in *outY. Offset form, stops at the m_height-1 edge.
// @early-stop
// ~94% regalloc coin-flip (lim vs this callee-saved coloring). Logic byte-faithful.
RVA(0x001670d0, 0x5d)
i32 CImageSet3::ScanDownGate(i32 x, i32 y, i32 val, i32* outY) {
    i32 off = (y << m_heightLog2) + x;
    i32 lim = m_height - 1;
    while (y < lim) {
        off += m_width;
        ++y;
        if ((m_pixels)[off] == val) {
            *outY = y;
            return 1;
        }
    }
    return 0;
}
