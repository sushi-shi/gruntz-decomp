#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ImageSets.h>
#include <Ints.h>

RVA(0x00166e00, 0x60)
i32 CPixelTileImageSet::ScanRunLeft(i32 x, i32 y, i32* outX, i32* outValue) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (x > 0) {
        --x;
        --off;
        if ((m_pixels)[off] != target) {
            *outX = x;
            *outValue = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

RVA(0x00166e60, 0x48)
i32 CPixelTileImageSet::ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX) {
    i32 off = (y << m_heightLog2) + x;
    while (x > 0) {
        --x;
        --off;
        if (m_pixels[off] == value) {
            *outX = x;
            return 1;
        }
    }
    return 0;
}
