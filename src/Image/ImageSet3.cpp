#include <rva.h>

#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/ImageSets.h>

RVA(0x00161500, 0x58)
CImageSet3::~CImageSet3() {
    if (m_pixels) {
        ::operator delete(m_pixels);
    }
    m_pixels = 0;
}

RVA(0x00166e00, 0x60)
i32 CImageSet3::ScanRunLeft(i32 x, i32 y, i32* outX, i32* outVal) {
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

RVA(0x00166e60, 0x48)
i32 CImageSet3::ScanRunLeftForValue(i32 x, i32 y, i32 val, i32* outX) {
    u8* p = m_pixels + ((y << m_heightLog2) + x);
    while (x > 0) {
        --x;
        --p;
        if (*p == val) {
            *outX = x;
            return 1;
        }
    }
    return 0;
}
