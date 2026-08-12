#ifndef GRUNTZ_IMAGESETS_H
#define GRUNTZ_IMAGESETS_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/TileCollisionKind.h>
#include <Ints.h>
#include <Wap32/Object.h>

#include <stddef.h>

// Serialized tile-collision image encodings. Their payload layouts prove the
// names: one value for the whole image, inside/outside values for a rectangle,
// or a full byte-per-tile pixel map.
GZ_ENUM_CONST_BEGIN(TileImageSetKind)
    TILE_IMAGESET_UNIFORM = 1,
    TILE_IMAGESET_RECT = 2,
    TILE_IMAGESET_PIXELS = 3
GZ_ENUM_CONST_END(TileImageSetKind)

struct WwdTileImageRecord {
    char m_header[8];
    i32 m_fields[1];
};

VTBL_ABSENT(CTileImageSet);
class CTileImageSet : public CObject {
public:
    virtual i32 Parse(void* record);
    virtual void FreePixels();
    virtual i32 GetKind();

    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y);
    virtual i32 GetStride();

    i32 m_width;
};

struct CImageSet1 : public CTileImageSet {
    virtual ~CImageSet1() OVERRIDE;

    virtual i32 Parse(void* record) OVERRIDE;
    virtual void FreePixels() OVERRIDE;

    virtual i32 GetKind() OVERRIDE;

    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y) OVERRIDE;
    virtual i32 GetStride() OVERRIDE;

    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outVal);
    virtual i32 ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX);
    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outVal);
    virtual i32 ScanUpForValue(i32 x, i32 y, i32 value, i32* outY);
    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outVal);
    virtual i32 ScanRightForValue(i32 x, i32 y, i32 value, i32* outX);
    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outVal);
    virtual i32 ScanDownForValue(i32 x, i32 y, i32 value, i32* outY);
    CImageSet1() {
        m_width = 0;
    }
    void* operator new(size_t n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    i32 m_height;
    i32 m_collisionValue;
};
struct CImageSet2 : public CTileImageSet {
    virtual ~CImageSet2() OVERRIDE;

    virtual i32 Parse(void* record) OVERRIDE;
    virtual void FreePixels() OVERRIDE;
    virtual i32 GetKind() OVERRIDE;
    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y) OVERRIDE;
    virtual i32 GetStride() OVERRIDE;

    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outVal);
    virtual i32 ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX);
    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outVal);
    virtual i32 ScanUpForValue(i32 x, i32 y, i32 value, i32* outY);
    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outVal);
    virtual i32 ScanRightForValue(i32 x, i32 y, i32 value, i32* outX);
    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outVal);
    virtual i32 ScanDownForValue(i32 x, i32 y, i32 value, i32* outY);
    CImageSet2() {
        m_width = 0;
    }
    void* operator new(size_t n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    i32 m_height;
    i32 m_outsideValue;
    i32 m_insideValue;
    i32 m_left;
    i32 m_top;
    i32 m_right;
    i32 m_bottom;
};
struct CImageSet3 : public CTileImageSet {
    virtual ~CImageSet3() OVERRIDE;

    virtual i32 Parse(void* record) OVERRIDE;
    virtual void FreePixels() OVERRIDE;
    virtual i32 GetKind() OVERRIDE;
    virtual TileCollisionKind GetCollisionAt(i32 x, i32 y) OVERRIDE;
    virtual i32 GetStride() OVERRIDE;

    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outVal);

    virtual i32 ScanRunLeftForValue(i32 x, i32 y, i32 value, i32* outX);

    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outVal);

    virtual i32 ScanUpForValue(i32 x, i32 y, i32 value, i32* outY);
    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outVal);
    virtual i32 ScanRightForValue(i32 x, i32 y, i32 value, i32* outX);
    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outVal);
    virtual i32 ScanDownForValue(i32 x, i32 y, i32 value, i32* outY);

    CImageSet3() {
        m_width = 0;
        m_pixels = NULL;
    }
    void* operator new(size_t n) {
        return ::operator new(n);
    }
    void operator delete(void* p) {
        ::operator delete(p);
    }

    i32 m_height;
    i32 m_heightLog2;
    i32 m_byteSize;

    u8* m_pixels;
};

#endif // GRUNTZ_IMAGESETS_H
