#include <rva.h>

#include <Gruntz/ImageSets.h>

#include <string.h>

RVA(0x001614b0, 0x1c)
void CImageSet3::FreePixels() {
    if (m_pixels) {
        delete[] m_pixels;
    }
    m_pixels = NULL;
}

RVA(0x001614d0, 0x6)
i32 CImageSet3::GetKind() {
    return TILE_IMAGESET_PIXELS;
}

RVA(0x00161570, 0x1d)
TileCollisionKind CImageSet3::GetCollisionAt(i32 x, i32 y) {
    return static_cast<TileCollisionKind>(m_pixels[(y << m_heightLog2) + x]);
}

RVA(0x00161590, 0xb)
i32 CImageSet3::GetStride() {
    return m_height * m_width + offsetof(WwdTileImageRecord, m_fields);
}

RVA(0x00166d70, 0x8d)
i32 CImageSet3::Parse(WwdTileImageRecord* record) {
    i32* p = &record->m_width;
    m_width = *p++;
    m_height = *p++;
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

RVA(0x00166f20, 0x52)
i32 CImageSet3::ScanUpForValue(i32 x, i32 y, i32 val, i32* outY) {
    i32 off = (y << m_heightLog2) + x;
    while (y > 0) {
        off -= m_width;
        --y;
        if (m_pixels[off] == val) {
            *outY = y;
            return 1;
        }
    }
    return 0;
}

RVA(0x00166f80, 0x68)
i32 CImageSet3::ScanRight(i32 x, i32 y, i32* outX, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (x < m_width - 1) {
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

RVA(0x00166ff0, 0x52)
i32 CImageSet3::ScanRightForValue(i32 x, i32 y, i32 val, i32* outX) {
    i32 off = (y << m_heightLog2) + x;
    while (x < m_width - 1) {
        ++x;
        ++off;
        if (m_pixels[off] == val) {
            *outX = x;
            return 1;
        }
    }
    return 0;
}

RVA(0x00167050, 0x74)
i32 CImageSet3::ScanDown(i32 x, i32 y, i32* outY, i32* outVal) {
    i32 off = (y << m_heightLog2) + x;
    i32 target = (m_pixels)[off];
    while (y < m_height - 1) {
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

RVA(0x001670d0, 0x5d)
i32 CImageSet3::ScanDownForValue(i32 x, i32 y, i32 val, i32* outY) {
    i32 off = (y << m_heightLog2) + x;
    while (y < m_height - 1) {
        off += m_width;
        ++y;
        if ((m_pixels)[off] == val) {
            *outY = y;
            return 1;
        }
    }
    return 0;
}
