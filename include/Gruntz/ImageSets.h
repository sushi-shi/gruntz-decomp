#ifndef GRUNTZ_IMAGESETS_H
#define GRUNTZ_IMAGESETS_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>
#include <Wap32/Object.h>

#include <stddef.h>

GZ_ENUM_CONST_BEGIN(TileImageSetKind)
    TILE_IMAGESET_UNIFORM = 1,
    TILE_IMAGESET_RECT = 2,
    TILE_IMAGESET_PIXELS = 3
GZ_ENUM_CONST_END(TileImageSetKind)

struct WwdTileImageRecord {
    i32 m_kind;
    i32 m_reserved4;
    i32 m_width;
    i32 m_height;
    i32 m_fields[1];
};

class CTileImageSet : public CObject {
public:
    virtual i32 Parse(WwdTileImageRecord* record);
    virtual void FreePixels();
    virtual i32 GetKind();

    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y);
    virtual i32 GetStride();

    i32 m_width;
    i32 m_height;
};

struct CUniformTileImageSet : public CTileImageSet {
    virtual ~CUniformTileImageSet() OVERRIDE {}

    virtual i32 Parse(WwdTileImageRecord* record) OVERRIDE;
    RVA(0x00161330, 0x1)
    virtual void FreePixels() OVERRIDE {}

    RVA(0x00161340, 0x6)
    virtual i32 GetKind() OVERRIDE {
        return TILE_IMAGESET_UNIFORM;
    }

    RVA(0x00161380, 0x6)
    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y) OVERRIDE {
        return static_cast<TileCollisionKind>(m_collisionValue);
    }
    RVA(0x00161410, 0x6)
    virtual i32 GetStride() OVERRIDE {
        return sizeof(WwdTileImageRecord);
    }

    RVA(0x00161390, 0x5)
    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outValue) {
        return 0;
    }
    RVA(0x001613a0, 0x5)
    virtual i32 ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX) {
        return 0;
    }
    RVA(0x001613b0, 0x5)
    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outValue) {
        return 0;
    }
    RVA(0x001613c0, 0x5)
    virtual i32 ScanUpForValue(i32 x, i32 y, i32 value, i32* outY) {
        return 0;
    }
    RVA(0x001613d0, 0x7)
    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outValue) {
        return m_width - 1;
    }
    RVA(0x001613e0, 0x5)
    virtual i32 ScanRightForValue(i32 x, i32 y, i32 value, i32* outX) {
        return 0;
    }
    RVA(0x001613f0, 0x7)
    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outValue) {
        return m_height - 1;
    }
    RVA(0x00161400, 0x5)
    virtual i32 ScanDownForValue(i32 x, i32 y, i32 value, i32* outY) {
        return 0;
    }
    CUniformTileImageSet() {
        m_width = 0;
    }
    void* operator new(size_t n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    i32 m_collisionValue;
};
struct CRectTileImageSet : public CTileImageSet {
    virtual ~CRectTileImageSet() OVERRIDE {}

    virtual i32 Parse(WwdTileImageRecord* record) OVERRIDE;
    RVA(0x00161420, 0x1)
    virtual void FreePixels() OVERRIDE {}
    RVA(0x00161430, 0x6)
    virtual i32 GetKind() OVERRIDE {
        return TILE_IMAGESET_RECT;
    }
    RVA(0x00161470, 0x2c)
    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y) OVERRIDE {
        if (x < m_left || x > m_right || y < m_top || y > m_bottom) {
            return static_cast<TileCollisionKind>(m_outsideValue);
        }
        return static_cast<TileCollisionKind>(m_insideValue);
    }
    RVA(0x001614a0, 0x6)
    virtual i32 GetStride() OVERRIDE {
        return offsetof(WwdTileImageRecord, m_fields) + 6 * sizeof(i32);
    }

    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outValue);
    virtual i32 ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX);
    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outValue);
    virtual i32 ScanUpForValue(i32 x, i32 y, i32 value, i32* outY);
    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outValue);
    virtual i32 ScanRightForValue(i32 x, i32 y, i32 value, i32* outX);
    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outValue);
    virtual i32 ScanDownForValue(i32 x, i32 y, i32 value, i32* outY);
    CRectTileImageSet() {
        m_width = 0;
    }
    void* operator new(size_t n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    i32 m_outsideValue;
    i32 m_insideValue;
    i32 m_left;
    i32 m_top;
    i32 m_right;
    i32 m_bottom;
};
struct CPixelTileImageSet : public CTileImageSet {
    virtual ~CPixelTileImageSet() OVERRIDE {
        if (m_pixels) {
            delete[] m_pixels;
        }
        m_pixels = NULL;
    }

    virtual i32 Parse(WwdTileImageRecord* record) OVERRIDE;
    virtual void FreePixels() OVERRIDE;
    virtual i32 GetKind() OVERRIDE;
    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y) OVERRIDE;
    virtual i32 GetStride() OVERRIDE;

    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outValue);

    virtual i32 ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX);

    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outValue);

    virtual i32 ScanUpForValue(i32 x, i32 y, i32 value, i32* outY);
    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outValue);
    virtual i32 ScanRightForValue(i32 x, i32 y, i32 value, i32* outX);
    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outValue);
    virtual i32 ScanDownForValue(i32 x, i32 y, i32 value, i32* outY);

    CPixelTileImageSet() {
        m_width = 0;
        m_pixels = NULL;
    }
    void* operator new(size_t n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    i32 m_heightLog2;
    i32 m_byteSize;

    u8* m_pixels;
};

#endif // GRUNTZ_IMAGESETS_H
