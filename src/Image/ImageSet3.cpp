#include <rva.h>

#include <Mfc.h> // CObject grand-base of the collapsed ~CImageSet3 (folds ??_7CObject @0x5e8cb4)
#include <Ints.h>
#include <Gruntz/ImageSets.h> // the canonical 18-slot CImageSet3 (RTTI vtbl 0x1f0228)

RVA(0x00161500, 0x58)
CImageSet3::~CImageSet3() {
    if (m_pixels) {
        ::operator delete(m_pixels);
    }
    m_pixels = 0;
}

// ---------------------------------------------------------------------------
// 0x166e00: from the pixel at (x,y), walk LEFT along the row while the pixel value
// stays equal to that start pixel. On the first differing pixel, record its column
// in *outX and its value in *outVal and return 1; if the row edge (x reaches 0) is
// hit first, return 0. __thiscall, 4 args (ret 0x10).
// 100%: the former SIB base/index-swap residual (a 99.778% @early-stop) flipped to
// retail's pick when the TU regained its retail /GX profile + the collapsed dtor
// (the *Eh.cpp merge) - the codegen residue was TU-composition-sensitive, not a wall.
RVA(0x00166e00, 0xa8)
i32 CImageSet3::ScanRunLeft_166e00(i32 x, i32 y, i32* outX, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (x > 0) {
        --x;
        --off;
        if ((m_pixels)[off] != target) {
            *outX = x;
            *outVal = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}
