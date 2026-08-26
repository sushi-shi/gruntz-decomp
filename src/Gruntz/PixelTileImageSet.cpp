#include <rva.h>

#include <Gruntz/ImageSetInline.h>

#include <string.h>

RVA(0x00161790, 0x1c)
void CPixelTileImageSet::FreePixels() {
    if (m_pixels) {
        delete[] m_pixels;
    }
    m_pixels = NULL;
}

RVA(0x001617b0, 0x6)
i32 CPixelTileImageSet::GetKind() {
    return TILE_IMAGESET_PIXELS;
}

RVA(0x00161850, 0x1d)
TileCollisionKind CPixelTileImageSet::GetCollisionAt(i32 x, i32 y) {
    return static_cast<TileCollisionKind>(m_pixels[(y << m_heightLog2) + x]);
}

RVA(0x00161870, 0xb)
i32 CPixelTileImageSet::GetStride() {
    return m_height * m_width + offsetof(WwdTileImageRecord, m_fields);
}

RVA(0x00166ca0, 0x8d)
i32 CPixelTileImageSet::Parse(WwdTileImageRecord* record) {
    READ_TILE_IMAGE_DIMENSIONS(record, p)
    i32 h = m_height;
    m_heightLog2 = 0;
    i32 size = m_width * m_height;
    m_byteSize = size;
    for (; h > 1; h >>= 1) {
        m_heightLog2++;
    }
    if ((1 << m_heightLog2) != m_width) {
        return 0;
    }

    u8* dst = new u8[size];
    m_pixels = dst;
    if (dst == NULL) {
        return 0;
    }
    memcpy(dst, p, m_byteSize);
    return 1;
}

RVA(0x00166de0, 0x6a)
i32 CPixelTileImageSet::ScanUp(i32 x, i32 y, i32* outY, i32* outValue) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (y > 0) {
        off -= m_width;
        --y;
        if ((m_pixels)[off] != target) {
            *outY = y;
            *outValue = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

RVA(0x00166e50, 0x52)
i32 CPixelTileImageSet::ScanUpForValue(i32 x, i32 y, i32 value, i32* outY) {
    i32 off = (y << m_heightLog2) + x;
    while (y > 0) {
        off -= m_width;
        --y;
        if (m_pixels[off] == value) {
            *outY = y;
            return 1;
        }
    }
    return 0;
}

RVA(0x00166eb0, 0x68)
i32 CPixelTileImageSet::ScanRight(i32 x, i32 y, i32* outX, i32* outValue) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (x < m_width - 1) {
        ++x;
        ++off;
        if ((m_pixels)[off] != target) {
            *outX = x;
            *outValue = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

RVA(0x00166f20, 0x52)
i32 CPixelTileImageSet::ScanRightForValue(i32 x, i32 y, i32 value, i32* outX) {
    i32 off = (y << m_heightLog2) + x;
    while (x < m_width - 1) {
        ++x;
        ++off;
        if (m_pixels[off] == value) {
            *outX = x;
            return 1;
        }
    }
    return 0;
}

RVA(0x00166f80, 0x74)
i32 CPixelTileImageSet::ScanDown(i32 x, i32 y, i32* outY, i32* outValue) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (y < m_height - 1) {
        off += m_width;
        ++y;
        if ((m_pixels)[off] != target) {
            *outY = y;
            *outValue = (m_pixels)[off];
            return 1;
        }
    }
    return 0;
}

RVA(0x00167000, 0x5d)
i32 CPixelTileImageSet::ScanDownForValue(i32 x, i32 y, i32 value, i32* outY) {
    i32 off = (y << m_heightLog2) + x;
    while (y < m_height - 1) {
        off += m_width;
        ++y;
        if ((m_pixels)[off] == value) {
            *outY = y;
            return 1;
        }
    }
    return 0;
}
