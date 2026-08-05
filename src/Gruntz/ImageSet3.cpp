#include <rva.h>

#include <Gruntz/ImageSets.h>

#include <string.h>

RVA(0x001614b0, 0x1c)
void CImageSet3::FreePixels() {
    if (m_pixels) {
        ::operator delete(m_pixels);
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
    return m_height * m_width + 0x10;
}

// @early-stop
RVA(0x00166d70, 0x8d)
i32 CImageSet3::Parse(void* record) {
    i32* p = static_cast<WwdTileImageRecord*>(record)->m_fields;
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

    u8* dst = static_cast<u8*>(::operator new(m_byteSize));
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

// @early-stop
RVA(0x00166f20, 0x52)
i32 CImageSet3::ScanUpForValue(i32 x, i32 y, i32 val, i32* outY) {
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

// @early-stop
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

// @early-stop
RVA(0x00166ff0, 0x52)
i32 CImageSet3::ScanRightForValue(i32 x, i32 y, i32 val, i32* outX) {
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

// @early-stop
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

// @early-stop
RVA(0x001670d0, 0x5d)
i32 CImageSet3::ScanDownForValue(i32 x, i32 y, i32 val, i32* outY) {
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
