#include <rva.h>

#include <Gruntz/ImageSets.h>

RVA(0x00161330, 0x1)
void CImageSet1::FreePixels() {}

RVA(0x00161340, 0x6)
i32 CImageSet1::GetKind() {
    return TILE_IMAGESET_UNIFORM;
}

RVA(0x00161370, 0x7)
CImageSet1::~CImageSet1() {}

RVA(0x00161380, 0x6)
TileCollisionKind CImageSet1::GetCollisionAt(i32, i32) {
    return static_cast<TileCollisionKind>(m_collisionValue);
}

RVA(0x00161390, 0x5)
i32 CImageSet1::ScanRunLeft(i32, i32, i32*, i32*) {
    return 0;
}

RVA(0x001613a0, 0x5)
i32 CImageSet1::ScanRunLeftForValue(i32, i32, i32, i32*) {
    return 0;
}

RVA(0x001613b0, 0x5)
i32 CImageSet1::ScanUp(i32, i32, i32*, i32*) {
    return 0;
}

RVA(0x001613c0, 0x5)
i32 CImageSet1::ScanUpForValue(i32, i32, i32, i32*) {
    return 0;
}

RVA(0x001613d0, 0x7)
i32 CImageSet1::ScanRight(i32, i32, i32*, i32*) {
    return m_width - 1;
}

RVA(0x001613e0, 0x5)
i32 CImageSet1::ScanRightForValue(i32, i32, i32, i32*) {
    return 0;
}

RVA(0x001613f0, 0x7)
i32 CImageSet1::ScanDown(i32, i32, i32*, i32*) {
    return m_height - 1;
}

RVA(0x00161400, 0x5)
i32 CImageSet1::ScanDownForValue(i32, i32, i32, i32*) {
    return 0;
}

RVA(0x00161410, 0x6)
i32 CImageSet1::GetStride() {
    return 0x14;
}

// @early-stop
RVA(0x00166d40, 0x24)
i32 CImageSet1::Parse(void* record) {
    i32* p = static_cast<WwdTileImageRecord*>(record)->m_fields;
    m_width = *p++;
    m_height = *p++;
    m_collisionValue = *p++;
    return 1;
}
