#ifndef GRUNTZ_IMAGESETS_H
#define GRUNTZ_IMAGESETS_H

#include <Ints.h>

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
    i32 m_collisionData[1];
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
    virtual i32 Parse(WwdTileImageRecord* record)  ;

    virtual void FreePixels()   {}

    virtual i32 GetKind()   {
        return TILE_IMAGESET_UNIFORM;
    }

    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y)   {
        return static_cast<TileCollisionKind>(m_collisionValue);
    }

    virtual i32 GetStride()   {
        return sizeof(WwdTileImageRecord);
    }

    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outValue) {
        return 0;
    }

    virtual i32 ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX) {
        return 0;
    }

    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outValue) {
        return 0;
    }

    virtual i32 ScanUpForValue(i32 x, i32 y, i32 value, i32* outY) {
        return 0;
    }

    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outValue) {
        return m_width - 1;
    }

    virtual i32 ScanRightForValue(i32 x, i32 y, i32 value, i32* outX) {
        return 0;
    }

    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outValue) {
        return m_height - 1;
    }

    virtual i32 ScanDownForValue(i32 x, i32 y, i32 value, i32* outY) {
        return 0;
    }
    CUniformTileImageSet() {
        m_width = 0;
    }

    i32 m_collisionValue;
};
struct CRectTileImageSet : public CTileImageSet {
    virtual i32 Parse(WwdTileImageRecord* record)  ;

    virtual void FreePixels()   {}

    virtual i32 GetKind()   {
        return TILE_IMAGESET_RECT;
    }

    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y)   {
        if (x < m_left || x > m_right || y < m_top || y > m_bottom) {
            return static_cast<TileCollisionKind>(m_outsideValue);
        }
        return static_cast<TileCollisionKind>(m_insideValue);
    }

    virtual i32 GetStride()   {
        return offsetof(WwdTileImageRecord, m_collisionData) + 6 * sizeof(i32);
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

    i32 m_outsideValue;
    i32 m_insideValue;
    i32 m_left;
    i32 m_top;
    i32 m_right;
    i32 m_bottom;
};
struct CPixelTileImageSet : public CTileImageSet {
    virtual ~CPixelTileImageSet()   {
        if (m_pixels) {
            delete[] m_pixels;
        }
        m_pixels = NULL;
    }

    virtual i32 Parse(WwdTileImageRecord* record)  ;
    virtual void FreePixels()  ;
    virtual i32 GetKind()  ;
    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y)  ;
    virtual i32 GetStride()  ;

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

    i32 m_heightLog2;
    i32 m_byteSize;

    u8* m_pixels;
};

#endif
