#include <Gruntz/ImageSets.h>
#include <rva.h>

RVA(0x00161460, 0x7)
CImageSet2::~CImageSet2() {}

RVA(0x00161470, 0x2c)
i32 CImageSet2::GetCollisionAt(i32 x, i32 y) {
    if (x < m_14 || x > m_1c || y < m_18 || y > m_20) {
        return m_0c;
    }
    return m_10;
}

// CImageSet2::Parse (0x166990, ??_7CImageSet2 slot +0x14). Copies eight dwords
// from the WWD record at +0x08.. into m_04..m_20 via an advancing source pointer
// and returns TRUE.
// @early-stop
// tail-peephole wall (~94%): retail advances the source pointer on the 7th store
// (add eax,4) then reads the 8th via [eax]; cl folds the 7th advance into the
// 8th's +4 displacement. Body otherwise byte-exact; not source-steerable.
RVA(0x00166990, 0x4c)
i32 CImageSet2::Parse(void* record) {
    i32* p = reinterpret_cast<i32*>((reinterpret_cast<char*>(record) + 8));
    m_04 = *p++;
    m_08 = *p++;
    m_0c = *p++;
    m_10 = *p++;
    m_14 = *p++;
    m_18 = *p++;
    m_1c = *p++;
    m_20 = *p++;
    return 1;
}

RVA(0x001669e0, 0x5e)
i32 CImageSet2::Query_1669e0(i32 a, i32 b, i32* outA, i32* outB) {
    if (b < m_18 || b > m_20 || a < m_14) {
        return 0;
    }
    if (a > m_1c) {
        *outA = m_1c;
        *outB = m_10;
        return 1;
    }
    if (m_14 <= 0) {
        return 0;
    }
    *outA = m_14 - 1;
    *outB = m_0c;
    return 1;
}

RVA(0x00166a40, 0x62)
i32 CImageSet2::Query_166a40(i32 a, i32 b, i32 val, i32* out) {
    if (b < m_18 || b > m_20 || a < m_14) {
        return 0;
    }
    if (a > m_1c) {
        if (m_10 != val) {
            return 0;
        }
        *out = m_1c;
        return 1;
    }
    if (m_14 <= 0) {
        return 0;
    }
    if (m_0c != val) {
        return 0;
    }
    *out = m_14 - 1;
    return 1;
}

RVA(0x00166ab0, 0x62)
i32 CImageSet2::Query_166ab0(i32 a, i32 b, i32* outA, i32* outB) {
    if (b < m_18 || b > m_20 || a > m_1c) {
        return 0;
    }
    if (a < m_14) {
        *outA = m_14;
        *outB = m_10;
        return 1;
    }
    if (m_1c >= m_04 - 1) {
        return 0;
    }
    *outA = m_1c + 1;
    *outB = m_0c;
    return 1;
}

RVA(0x00166b20, 0x66)
i32 CImageSet2::Query_166b20(i32 a, i32 b, i32 val, i32* out) {
    if (b < m_18 || b > m_20 || a > m_1c) {
        return 0;
    }
    if (a < m_14) {
        if (m_10 != val) {
            return 0;
        }
        *out = m_14;
        return 1;
    }
    if (m_1c >= m_04 - 1) {
        return 0;
    }
    if (m_0c != val) {
        return 0;
    }
    *out = m_1c + 1;
    return 1;
}

RVA(0x00166b90, 0x5e)
i32 CImageSet2::Query_166b90(i32 a, i32 b, i32* outA, i32* outB) {
    if (a < m_14 || a > m_1c || b < m_18) {
        return 0;
    }
    if (b > m_20) {
        *outA = m_20;
        *outB = m_10;
        return 1;
    }
    if (m_18 <= 0) {
        return 0;
    }
    *outA = m_18 - 1;
    *outB = m_0c;
    return 1;
}

RVA(0x00166bf0, 0x62)
i32 CImageSet2::Query_166bf0(i32 a, i32 b, i32 val, i32* out) {
    if (a < m_14 || a > m_1c || b < m_18) {
        return 0;
    }
    if (b > m_20) {
        if (m_10 != val) {
            return 0;
        }
        *out = m_20;
        return 1;
    }
    if (m_18 <= 0) {
        return 0;
    }
    if (m_0c != val) {
        return 0;
    }
    *out = m_18 - 1;
    return 1;
}

RVA(0x00166c60, 0x62)
i32 CImageSet2::Query_166c60(i32 a, i32 b, i32* outA, i32* outB) {
    if (a < m_14 || a > m_1c || b > m_20) {
        return 0;
    }
    if (b < m_18) {
        *outA = m_18;
        *outB = m_10;
        return 1;
    }
    if (m_20 >= m_08 - 1) {
        return 0;
    }
    *outA = m_20 + 1;
    *outB = m_0c;
    return 1;
}

RVA(0x00166cd0, 0x66)
i32 CImageSet2::Query_166cd0(i32 a, i32 b, i32 val, i32* out) {
    if (a < m_14 || a > m_1c || b > m_20) {
        return 0;
    }
    if (b < m_18) {
        if (m_10 != val) {
            return 0;
        }
        *out = m_18;
        return 1;
    }
    if (m_20 >= m_08 - 1) {
        return 0;
    }
    if (m_0c != val) {
        return 0;
    }
    *out = m_20 + 1;
    return 1;
}
