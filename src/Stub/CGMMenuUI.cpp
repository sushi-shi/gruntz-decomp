#include <rva.h>
// CGMMenuUI.cpp - engine-label stubs for CGMMenuUI (reloc-correlation).

// m_40 sub-object: the active menu node the flag handlers dispatch on (thiscall).
class CGMMenuNode {
public:
    i32 H40();          // 0x183c50
    i32 H80();          // 0x183d10
    i32 H03();          // 0x183dd0
    i32 H100(i32);      // 0x183df0
    i32 H10();          // 0x183ff0
    i32 H20();          // 0x183f70
    i32 Step1(i32);     // 0x183b30
    i32 PreHelper(i32); // 0x183b60
};

// Post blits/flips DirectDraw surfaces; the retail callees name CDDSurface.
class CDDSurface {
public:
    i32 Flip(CDDSurface*); // 0x13e850
    i32 BltFast(
        u32,
        u32,
        CDDSurface*, // 0x13ef90
        void*,
        u32
    );
};
struct CGMMenuRes {
    char m_pad[0x2c];
    CDDSurface* m_2c; // [+0x2c] owned surface
};
struct CGMMenuRes18 {
    char m_pad[0x1c];
    i32 m_1c; // [+0x1c]
    char m_pad2[0xc];
    CDDSurface* m_2c; // [+0x2c] source surface
};
struct CGMMenuSub1 {
    char m_pad[0x10];
    CGMMenuRes* m_10;   // [+0x10]
    CGMMenuRes* m_14;   // [+0x14]
    CGMMenuRes18* m_18; // [+0x18]
};
struct CGMMenuObj {
    char m_pad[0x04];
    CGMMenuSub1* m_4; // [+0x04]
};

class CGMMenuUI {
public:
    i32 OnFlag00000003();
    i32 OnFlag00000100();
    i32 OnFlag10000000();
    i32 OnFlag20000000();
    i32 OnFlag40000000();
    i32 OnFlag80000000();
    i32 Post();
    i32 Pre();
    i32 Step(u32);

private:
    CGMMenuObj* m_0; // offset 0x00
    char m_pad4[0x3c];
    CGMMenuNode* m_40; // offset 0x40

    i32 Handler2(i32); // 0x182ed0 (this method)
};

// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00182d60, 0x16)
i32 CGMMenuUI::OnFlag00000003() {
    if (!m_40) {
        return 0;
    }
    return m_40->H03() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00182d80, 0x18)
i32 CGMMenuUI::OnFlag00000100() {
    if (!m_40) {
        return 0;
    }
    return m_40->H100(1) != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00183130, 0x16)
i32 CGMMenuUI::OnFlag10000000() {
    if (!m_40) {
        return 0;
    }
    return m_40->H10() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00183150, 0x16)
i32 CGMMenuUI::OnFlag20000000() {
    if (!m_40) {
        return 0;
    }
    return m_40->H20() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00182d20, 0x16)
i32 CGMMenuUI::OnFlag40000000() {
    if (!m_40) {
        return 0;
    }
    return m_40->H40() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00182d40, 0x16)
i32 CGMMenuUI::OnFlag80000000() {
    if (!m_40) {
        return 0;
    }
    return m_40->H80() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00182ce0, 0x36)
i32 CGMMenuUI::Post() {
    CGMMenuSub1* s = m_0->m_4;
    s->m_10->m_2c->Flip(0);
    s->m_14->m_2c->BltFast(0, 0, s->m_18->m_2c, &s->m_18->m_1c, 0x10);
    return 1;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00182cb0, 0x26)
i32 CGMMenuUI::Pre() {
    if (!m_40) {
        return 0;
    }
    i32 x = (i32)m_0->m_4->m_14;
    if (!x) {
        return x;
    }
    return m_40->PreHelper(x) != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x00182c70, 0x38)
i32 CGMMenuUI::Step(u32 arg) {
    if (!m_40) {
        return 0;
    }
    if (!m_40->Step1((i32)arg)) {
        return 0;
    }
    return Handler2((i32)arg) != 0;
}
