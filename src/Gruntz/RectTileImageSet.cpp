#include <rva.h>

#include <Gruntz/ImageSetInline.h>

// @early-stop
RVA(0x00167060, 0x4c)
i32 CRectTileImageSet::Parse(WwdTileImageRecord* record) {
    READ_TILE_IMAGE_DIMENSIONS(record, p)
    m_outsideValue = *p++;
    m_insideValue = *p++;
    m_left = *p++;
    m_top = *p++;
    m_right = *p++;
    m_bottom = *p++;
    return 1;
}

RVA(0x001670b0, 0x5e)
i32 CRectTileImageSet::ScanRunLeft(i32 x, i32 y, i32* outX, i32* outValue) {
    if (y < m_top || y > m_bottom || x < m_left) {
        return 0;
    }
    if (x > m_right) {
        *outX = m_right;
        *outValue = m_insideValue;
        return 1;
    }
    if (m_left <= 0) {
        return 0;
    }
    *outX = m_left - 1;
    *outValue = m_outsideValue;
    return 1;
}

RVA(0x00167110, 0x62)
i32 CRectTileImageSet::ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX) {
    if (y < m_top || y > m_bottom || x < m_left) {
        return 0;
    }
    if (x > m_right) {
        if (m_insideValue != value) {
            return 0;
        }
        *outX = m_right;
        return 1;
    }
    if (m_left <= 0) {
        return 0;
    }
    if (m_outsideValue != value) {
        return 0;
    }
    *outX = m_left - 1;
    return 1;
}

RVA(0x00167180, 0x62)
i32 CRectTileImageSet::ScanRight(i32 x, i32 y, i32* outX, i32* outValue) {
    if (y < m_top || y > m_bottom || x > m_right) {
        return 0;
    }
    if (x < m_left) {
        *outX = m_left;
        *outValue = m_insideValue;
        return 1;
    }
    if (m_right >= m_width - 1) {
        return 0;
    }
    *outX = m_right + 1;
    *outValue = m_outsideValue;
    return 1;
}

RVA(0x001671f0, 0x66)
i32 CRectTileImageSet::ScanRightForValue(i32 x, i32 y, i32 value, i32* outX) {
    if (y < m_top || y > m_bottom || x > m_right) {
        return 0;
    }
    if (x < m_left) {
        if (m_insideValue != value) {
            return 0;
        }
        *outX = m_left;
        return 1;
    }
    if (m_right >= m_width - 1) {
        return 0;
    }
    if (m_outsideValue != value) {
        return 0;
    }
    *outX = m_right + 1;
    return 1;
}

RVA(0x00167260, 0x5e)
i32 CRectTileImageSet::ScanUp(i32 x, i32 y, i32* outY, i32* outValue) {
    if (x < m_left || x > m_right || y < m_top) {
        return 0;
    }
    if (y > m_bottom) {
        *outY = m_bottom;
        *outValue = m_insideValue;
        return 1;
    }
    if (m_top <= 0) {
        return 0;
    }
    *outY = m_top - 1;
    *outValue = m_outsideValue;
    return 1;
}

RVA(0x001672c0, 0x62)
i32 CRectTileImageSet::ScanUpForValue(i32 x, i32 y, i32 value, i32* outY) {
    if (x < m_left || x > m_right || y < m_top) {
        return 0;
    }
    if (y > m_bottom) {
        if (m_insideValue != value) {
            return 0;
        }
        *outY = m_bottom;
        return 1;
    }
    if (m_top <= 0) {
        return 0;
    }
    if (m_outsideValue != value) {
        return 0;
    }
    *outY = m_top - 1;
    return 1;
}

RVA(0x00167330, 0x62)
i32 CRectTileImageSet::ScanDown(i32 x, i32 y, i32* outY, i32* outValue) {
    if (x < m_left || x > m_right || y > m_bottom) {
        return 0;
    }
    if (y < m_top) {
        *outY = m_top;
        *outValue = m_insideValue;
        return 1;
    }
    if (m_bottom >= m_height - 1) {
        return 0;
    }
    *outY = m_bottom + 1;
    *outValue = m_outsideValue;
    return 1;
}

RVA(0x001673a0, 0x66)
i32 CRectTileImageSet::ScanDownForValue(i32 x, i32 y, i32 value, i32* outY) {
    if (x < m_left || x > m_right || y > m_bottom) {
        return 0;
    }
    if (y < m_top) {
        if (m_insideValue != value) {
            return 0;
        }
        *outY = m_top;
        return 1;
    }
    if (m_bottom >= m_height - 1) {
        return 0;
    }
    if (m_outsideValue != value) {
        return 0;
    }
    *outY = m_bottom + 1;
    return 1;
}
