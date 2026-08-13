#include <rva.h>

#include <Gruntz/ImageSets.h>

// @early-stop
RVA(0x00166990, 0x4c)
i32 CImageSet2::Parse(void* record) {
    i32* p = static_cast<WwdTileImageRecord*>(record)->m_fields;
    m_width = *p++;
    m_height = *p++;
    m_outsideValue = *p++;
    m_insideValue = *p++;
    m_left = *p++;
    m_top = *p++;
    m_right = *p++;
    m_bottom = *p++;
    return 1;
}

RVA(0x001669e0, 0x5e)
i32 CImageSet2::ScanRunLeft(i32 a, i32 b, i32* outA, i32* outB) {
    if (b < m_top || b > m_bottom || a < m_left) {
        return 0;
    }
    if (a > m_right) {
        *outA = m_right;
        *outB = m_insideValue;
        return 1;
    }
    if (m_left <= 0) {
        return 0;
    }
    *outA = m_left - 1;
    *outB = m_outsideValue;
    return 1;
}

RVA(0x00166a40, 0x62)
i32 CImageSet2::ScanRunLeftForValue(i32 a, i32 b, i32 val, i32* out) {
    if (b < m_top || b > m_bottom || a < m_left) {
        return 0;
    }
    if (a > m_right) {
        if (m_insideValue != val) {
            return 0;
        }
        *out = m_right;
        return 1;
    }
    if (m_left <= 0) {
        return 0;
    }
    if (m_outsideValue != val) {
        return 0;
    }
    *out = m_left - 1;
    return 1;
}

RVA(0x00166ab0, 0x62)
i32 CImageSet2::ScanRight(i32 a, i32 b, i32* outA, i32* outB) {
    if (b < m_top || b > m_bottom || a > m_right) {
        return 0;
    }
    if (a < m_left) {
        *outA = m_left;
        *outB = m_insideValue;
        return 1;
    }
    if (m_right >= m_width - 1) {
        return 0;
    }
    *outA = m_right + 1;
    *outB = m_outsideValue;
    return 1;
}

RVA(0x00166b20, 0x66)
i32 CImageSet2::ScanRightForValue(i32 a, i32 b, i32 val, i32* out) {
    if (b < m_top || b > m_bottom || a > m_right) {
        return 0;
    }
    if (a < m_left) {
        if (m_insideValue != val) {
            return 0;
        }
        *out = m_left;
        return 1;
    }
    if (m_right >= m_width - 1) {
        return 0;
    }
    if (m_outsideValue != val) {
        return 0;
    }
    *out = m_right + 1;
    return 1;
}

RVA(0x00166b90, 0x5e)
i32 CImageSet2::ScanUp(i32 a, i32 b, i32* outA, i32* outB) {
    if (a < m_left || a > m_right || b < m_top) {
        return 0;
    }
    if (b > m_bottom) {
        *outA = m_bottom;
        *outB = m_insideValue;
        return 1;
    }
    if (m_top <= 0) {
        return 0;
    }
    *outA = m_top - 1;
    *outB = m_outsideValue;
    return 1;
}

RVA(0x00166bf0, 0x62)
i32 CImageSet2::ScanUpForValue(i32 a, i32 b, i32 val, i32* out) {
    if (a < m_left || a > m_right || b < m_top) {
        return 0;
    }
    if (b > m_bottom) {
        if (m_insideValue != val) {
            return 0;
        }
        *out = m_bottom;
        return 1;
    }
    if (m_top <= 0) {
        return 0;
    }
    if (m_outsideValue != val) {
        return 0;
    }
    *out = m_top - 1;
    return 1;
}

RVA(0x00166c60, 0x62)
i32 CImageSet2::ScanDown(i32 a, i32 b, i32* outA, i32* outB) {
    if (a < m_left || a > m_right || b > m_bottom) {
        return 0;
    }
    if (b < m_top) {
        *outA = m_top;
        *outB = m_insideValue;
        return 1;
    }
    if (m_bottom >= m_height - 1) {
        return 0;
    }
    *outA = m_bottom + 1;
    *outB = m_outsideValue;
    return 1;
}

RVA(0x00166cd0, 0x66)
i32 CImageSet2::ScanDownForValue(i32 a, i32 b, i32 val, i32* out) {
    if (a < m_left || a > m_right || b > m_bottom) {
        return 0;
    }
    if (b < m_top) {
        if (m_insideValue != val) {
            return 0;
        }
        *out = m_top;
        return 1;
    }
    if (m_bottom >= m_height - 1) {
        return 0;
    }
    if (m_outsideValue != val) {
        return 0;
    }
    *out = m_bottom + 1;
    return 1;
}
