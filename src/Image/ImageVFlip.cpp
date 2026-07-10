// ImageVFlip.cpp - CRezImage::FlipVertical (0x176840): flip the tightly-packed 8bpp
// pixel plane (m_pixels, rows of m_width bytes, m_height rows) top-to-bottom by
// swapping row i with row (m_height-1-i) through a m_width-byte scratch row.
#include <Image/Image.h>
#include <rva.h>

#include <Rez/RezMgr.h> // RezAlloc (0x1b9b46) / RezFree (0x1b9b82)

// @early-stop
// strength-reduction / merged-induction wall (~43.6%). Logic verified exact: guard
// (m_height<=1), RezAlloc(m_width) scratch, the height/2 row-pair swap (scratch=bot;
// bot=top; top=scratch), RezFree. Retail's /O2 fuses the three per-pair byte copies
// into merged inductions where one register is BOTH the scratch index and the source
// offset (`[eax+ecx-1]`, `[edx+ebp-1]`) and accumulates the row offsets in stack spill
// slots; the wine MSVC5 keeps separate `x` index + `top`/`bot` pointer accumulators.
// Tried per-iteration multiply vs pointer-accumulator forms (40.3 -> 43.6); the merged
// 3-loop induction shape is not source-expressible. Bytes for the prologue/guard/alloc/
// free + the swap ordering match; the residue is the inner-loop addressing selection.
RVA(0x00176840, 0x288)
void CRezImage::FlipVertical() {
    if (m_height <= 1) {
        return;
    }
    u8* scratch = (u8*)RezAlloc(m_width);
    if (scratch == 0) {
        return;
    }
    i32 pairs = m_height / 2;
    u8* top = m_pixels;
    u8* bot = m_pixels + (m_height - 1) * m_width;
    i32 x;
    for (i32 i = 0; i < pairs; i++) {
        for (x = 0; x < m_width; x++) {
            scratch[x] = bot[x];
        }
        for (x = 0; x < m_width; x++) {
            bot[x] = top[x];
        }
        for (x = 0; x < m_width; x++) {
            top[x] = scratch[x];
        }
        top += m_width;
        bot -= m_width;
    }
    RezFree(scratch);
}
