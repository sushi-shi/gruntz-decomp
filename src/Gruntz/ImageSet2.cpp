// ImageSet2.cpp - CImageSet2 (the kind-2 WWD image-set collision record) method
// bodies, split out of the GameLevel god-TU. The class def lives in
// <Gruntz/ImageSets.h>; its ??_7CImageSet2 vtable is emitted + VTBL-bound in
// GameLevel.cpp (ReadImageSet's `new CImageSet2`). Functions in retail-RVA order.
#include <Gruntz/ImageSets.h>
#include <rva.h>

// ~CImageSet2 (0x161460, vtable slot 1): the trivial derived dtor. Same shape as
// ~CImageSet1 - /O2 dead-store-elides the derived vptr stamp under the base
// ~CObject stamp, leaving the single `mov [ecx], &??_7CObject; ret` (reloc-masked).
RVA(0x00161460, 0x7)
CImageSet2::~CImageSet2() {}

// CImageSet2::GetCollisionAt (0x161470, slot 8): return the interior collision
// kind m_10 when (x,y) is inside the {m_14..m_1c} x {m_18..m_20} box, else the
// exterior kind m_0c. __thiscall, 2 args (ret 0x8).
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
    i32* p = (i32*)(reinterpret_cast<char*>(record) + 8);
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

// CImageSet2::Query_1669e0 (0x1669e0, slot 10): reject when (a,b) is outside the
// {m_14..m_1c} x {m_18..m_20} box; otherwise, past the far x-edge (a>m_1c) report the
// x-edge m_1c + its value m_10, else the near edge (m_14-1, needing m_14>0) + m_0c.
// __thiscall, 4 args (ret 0x10).
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

// 0x166a40 (slot 11): x-edge query with a value gate - like Query_1669e0 but the paired
// cell (m_10 far / m_0c near) must equal `val`, and it reports only the near x coord.
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

// 0x166ab0 (slot 14): the far-x-edge query - reject past m_1c, clamp below m_14 (report
// m_14/m_10) else report the next cell (m_1c+1/m_0c) unless it hits the m_04-1 wall.
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

// 0x166b20 (slot 15): far-x-edge query with a value gate (twin of Query_166ab0).
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

// 0x166b90 (slot 12): the y-axis twin of Query_1669e0 - clamp on the {m_18..m_20} row.
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

// 0x166bf0 (slot 13): y-edge query with a value gate (twin of Query_166a40).
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

// 0x166c60 (slot 16): the far-y-edge query (twin of Query_166ab0 on the m_18/m_20/m_08 row).
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

// 0x166cd0 (slot 17): far-y-edge query with a value gate (twin of Query_166c60).
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
