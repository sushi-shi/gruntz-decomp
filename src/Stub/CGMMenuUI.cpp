#include <rva.h>
// CGMMenuUI.cpp - engine-label stubs for CGMMenuUI (reloc-correlation).

// m_40 sub-object: the active menu node the flag handlers dispatch on (thiscall).
class CGMMenuNode {
public:
    int H40();      // 0x183c50
    int H80();      // 0x183d10
    int H03();      // 0x183dd0
    int H100(int);  // 0x183df0
    int H10();          // 0x183ff0
    int H20();          // 0x183f70
    int Step1(int);     // 0x183b30
    int PreHelper(int); // 0x183b60
};

// Post blits/flips DirectDraw surfaces; the retail callees name CDDSurface.
class CDDSurface {
public:
    int Flip(CDDSurface*);                                   // 0x13e850
    int BltFast(unsigned long, unsigned long, CDDSurface*,   // 0x13ef90
                void*, unsigned long);
};
struct CGMMenuRes {
    char m_pad[0x2c];
    CDDSurface* m_2c;  // [+0x2c] owned surface
};
struct CGMMenuRes18 {
    char m_pad[0x1c];
    int m_1c;  // [+0x1c]
    char m_pad2[0xc];
    CDDSurface* m_2c;  // [+0x2c] source surface
};
struct CGMMenuSub1 {
    char m_pad[0x10];
    CGMMenuRes* m_10;     // [+0x10]
    CGMMenuRes* m_14;     // [+0x14]
    CGMMenuRes18* m_18;   // [+0x18]
};
struct CGMMenuObj {
    char m_pad[0x04];
    CGMMenuSub1* m_4;  // [+0x04]
};

class CGMMenuUI {
public:
    int OnFlag00000003();
    int OnFlag00000100();
    int OnFlag10000000();
    int OnFlag20000000();
    int OnFlag40000000();
    int OnFlag80000000();
    int Post();
    int Pre();
    int Step(unsigned int);

private:
    CGMMenuObj* m_0;  // offset 0x00
    char m_pad4[0x3c];
    CGMMenuNode* m_40;  // offset 0x40

    int Handler2(int);  // 0x182ed0 (this method)
};

// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x182d60, 0x16)
int CGMMenuUI::OnFlag00000003() {
    if (!m_40)
        return 0;
    return m_40->H03() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x182d80, 0x18)
int CGMMenuUI::OnFlag00000100() {
    if (!m_40)
        return 0;
    return m_40->H100(1) != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x183130, 0x16)
int CGMMenuUI::OnFlag10000000() {
    if (!m_40)
        return 0;
    return m_40->H10() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x183150, 0x16)
int CGMMenuUI::OnFlag20000000() {
    if (!m_40)
        return 0;
    return m_40->H20() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x182d20, 0x16)
int CGMMenuUI::OnFlag40000000() {
    if (!m_40)
        return 0;
    return m_40->H40() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x182d40, 0x16)
int CGMMenuUI::OnFlag80000000() {
    if (!m_40)
        return 0;
    return m_40->H80() != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x182ce0, 0x36)
int CGMMenuUI::Post() {
    CGMMenuSub1* s = m_0->m_4;
    s->m_10->m_2c->Flip(0);
    s->m_14->m_2c->BltFast(0, 0, s->m_18->m_2c, &s->m_18->m_1c, 0x10);
    return 1;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x182cb0, 0x26)
int CGMMenuUI::Pre() {
    if (!m_40)
        return 0;
    int x = (int)m_0->m_4->m_14;
    if (!x)
        return x;
    return m_40->PreHelper(x) != 0;
}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x182c70, 0x38)
int CGMMenuUI::Step(unsigned int arg) {
    if (!m_40)
        return 0;
    if (!m_40->Step1((int)arg))
        return 0;
    return Handler2((int)arg) != 0;
}
