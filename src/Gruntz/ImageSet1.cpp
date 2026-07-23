#include <Gruntz/ImageSets.h>
#include <rva.h>

// The kind-1 set owns no pixel buffer; slot 6 is the empty family default.
RVA(0x00161330, 0x1)
void CImageSet1::FreePixels() {}

RVA(0x00161340, 0x6)
i32 CImageSet1::GetKind() {
    return 1;
}

RVA(0x00161370, 0x7)
CImageSet1::~CImageSet1() {}

RVA(0x00161380, 0x6)
i32 CImageSet1::GetCollisionAt(i32, i32) {
    return m_0c;
}

// The kind-1 set has no collision box: six edge queries report 0; the two
// far-edge forms report the extent minus one.
RVA(0x00161390, 0x5)
i32 CImageSet1::Query_161390(i32, i32, i32*, i32*) {
    return 0;
}

RVA(0x001613a0, 0x5)
i32 CImageSet1::Query_1613a0(i32, i32, i32, i32*) {
    return 0;
}

RVA(0x001613b0, 0x5)
i32 CImageSet1::Query_1613b0(i32, i32, i32*, i32*) {
    return 0;
}

RVA(0x001613c0, 0x5)
i32 CImageSet1::Query_1613c0(i32, i32, i32, i32*) {
    return 0;
}

RVA(0x001613d0, 0x7)
i32 CImageSet1::Query_1613d0(i32, i32, i32*, i32*) {
    return m_04 - 1;
}

RVA(0x001613e0, 0x5)
i32 CImageSet1::Query_1613e0(i32, i32, i32, i32*) {
    return 0;
}

RVA(0x001613f0, 0x7)
i32 CImageSet1::Query_1613f0(i32, i32, i32*, i32*) {
    return m_08 - 1;
}

RVA(0x00161400, 0x5)
i32 CImageSet1::Query_161400(i32, i32, i32, i32*) {
    return 0;
}

RVA(0x00161410, 0x6)
i32 CImageSet1::GetStride() {
    return 0x14;
}

// CImageSet1::Parse (0x166d40, ??_7CImageSet1 slot +0x14). Copies three dwords
// from the WWD record at +0x08.. into m_04/m_08/m_0c via an advancing source
// pointer (retail's `add eax,8; mov (eax); add eax,4` cursor walk) and returns TRUE.
// @early-stop
// tail-peephole wall (same as CImageSet2/3): retail keeps the 2nd store's `add eax,4`
// then reads the 3rd via [eax]; cl folds that advance into the 3rd's +4 displacement.
// The advancing-cursor prologue + first read are byte-exact; the final fold is the
// documented entropy-tail wall (docs/patterns/header-fields-through-cursor-not-index.md).
RVA(0x00166d40, 0x24)
i32 CImageSet1::Parse(void* record) {
    i32* p = reinterpret_cast<i32*>((reinterpret_cast<char*>(record) + 8));
    m_04 = *p++;
    m_08 = *p++;
    m_0c = *p++;
    return 1;
}
