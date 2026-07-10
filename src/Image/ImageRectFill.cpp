// ImageRectFill.cpp - CRezImage's 8bpp scanline rectangle fill (0x176d20) and its
// translate-and-fill wrapper (0x176da0). The former CImg176d20 / FillRect176d20 views
// (BoundaryTailViews.h) were the delinker-guessed placeholders for this RAW-PIXEL DIB
// surface (+0x42c m_pixels / +0x430 m_rowOffsets) - i.e. CRezImage; folded onto it here
// (the wrapper is Fill's only caller, so the pair is one complete TU).
#include <Image/Image.h>
#include <rva.h>

#include <string.h> // inline memset intrinsic

// 0x176d20 - fill scanlines [top..bottom] of the rect with `color`, each row base being
// m_pixels + m_rowOffsets[y] + rect.left, `width` bytes. 100% (the earlier 91% read-order
// wall in BoundaryTail dissolved once folded into this complete CRezImage TU with its caller).
RVA(0x00176d20, 0x71)
void CRezImage::FillRect(CRezFillRect* r, i32 color) {
    i32 width = r->right - r->left;
    for (i32 y = r->top; y <= r->bottom; ++y) {
        i32 off = m_rowOffsets[y] + r->left;
        memset(m_pixels + off, color, width);
    }
}

// 0x176da0 - build a translated fill rect at origin (dx,dy) sized from `src`
// (right = dx + src.width, bottom = dy + src.height) and scanline-fill it.
// @early-stop
// regalloc-pressure wall (~66.4%). Full body/logic exact (the rect arithmetic, the
// stack-built CRezFillRect, the reloc-masked FillRect call). Retail computes right as
// `(src->right + dx) - src->left` with only 2 callee-saved regs (esi,edi), reading
// src->left transiently into edx; this wine MSVC5 reassociates to `(right-left)+dx`,
// preloads src->left into a 3rd callee-saved reg (ebx, +1 push) and reorders the four
// field stores. Not source-steerable (explicit parens, sequential-temp evaluation, and
// the permuter all keep the 3-register allocation).
RVA(0x00176da0, 0x4b)
void CRezImage::FillRectAt(i32 dx, i32 dy, CRezFillRect* src, i32 color) {
    CRezFillRect r;
    r.left = dx;
    r.top = dy;
    r.right = src->right + dx - src->left;
    r.bottom = src->bottom - src->top + dy;
    FillRect(&r, color);
}
