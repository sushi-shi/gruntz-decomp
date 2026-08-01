#ifndef GRUNTZ_IMAGESETS_H
#define GRUNTZ_IMAGESETS_H
#include <Wap32/Object.h>
#include <Ints.h>
#include <rva.h>

#include <Rez/RezAlloc.h>

struct WwdTileImageRecord {
    char m_header[8];
    i32 m_fields[1];
};
SIZE_UNKNOWN();

VTBL_ABSENT(CTileImageSet);
class CTileImageSet : public CObject {
public:
    virtual i32 Parse(void* record);
    virtual void FreePixels();
    virtual i32 GetKind();

    virtual i32 GetCollisionAt(i32 x, i32 y);
    virtual i32 GetStride();

    i32 m_width;
};
SIZE_UNKNOWN();

struct CImageSet1 : public CTileImageSet {
    virtual ~CImageSet1() OVERRIDE;

    virtual i32 Parse(void* record) OVERRIDE;
    virtual void FreePixels() OVERRIDE;

    virtual i32 GetKind() OVERRIDE;

    virtual i32 GetCollisionAt(i32 x, i32 y) OVERRIDE;
    virtual i32 GetStride() OVERRIDE;

    virtual i32 Query_161390(i32 a, i32 b, i32* outA, i32* outB);
    virtual i32 Query_1613a0(i32 a, i32 b, i32 val, i32* out);
    virtual i32 Query_1613b0(i32 a, i32 b, i32* outA, i32* outB);
    virtual i32 Query_1613c0(i32 a, i32 b, i32 val, i32* out);
    virtual i32 Query_1613d0(i32 a, i32 b, i32* outA, i32* outB);
    virtual i32 Query_1613e0(i32 a, i32 b, i32 val, i32* out);
    virtual i32 Query_1613f0(i32 a, i32 b, i32* outA, i32* outB);
    virtual i32 Query_161400(i32 a, i32 b, i32 val, i32* out);
    CImageSet1() {
        m_width = 0;
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }

    i32 m_08;
    i32 m_0c;
};
SIZE(0x10);
struct CImageSet2 : public CTileImageSet {
    virtual ~CImageSet2() OVERRIDE;

    virtual i32 Parse(void* record) OVERRIDE;
    virtual void FreePixels() OVERRIDE;
    virtual i32 GetKind() OVERRIDE;
    virtual i32 GetCollisionAt(i32 x, i32 y) OVERRIDE;
    virtual i32 GetStride() OVERRIDE;

    virtual i32 Query_1669e0(i32 a, i32 b, i32* outA, i32* outB);

    virtual i32 Query_166a40(i32 a, i32 b, i32 val, i32* out);
    virtual i32 Query_166b90(i32 a, i32 b, i32* outA, i32* outB);
    virtual i32 Query_166bf0(i32 a, i32 b, i32 val, i32* out);
    virtual i32 Query_166ab0(i32 a, i32 b, i32* outA, i32* outB);
    virtual i32 Query_166b20(i32 a, i32 b, i32 val, i32* out);
    virtual i32 Query_166c60(i32 a, i32 b, i32* outA, i32* outB);
    virtual i32 Query_166cd0(i32 a, i32 b, i32 val, i32* out);
    CImageSet2() {
        m_width = 0;
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }

    i32 m_08;
    i32 m_0c;
    i32 m_10;
    i32 m_14;
    i32 m_18;
    i32 m_1c;
    i32 m_20;
};
SIZE(0x24);
struct CImageSet3 : public CTileImageSet {
    virtual ~CImageSet3() OVERRIDE;

    virtual i32 Parse(void* record) OVERRIDE;
    virtual void FreePixels() OVERRIDE;
    virtual i32 GetKind() OVERRIDE;
    virtual i32 GetCollisionAt(i32 x, i32 y) OVERRIDE;
    virtual i32 GetStride() OVERRIDE;

    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outVal);

    virtual i32 ScanRunLeftGate_166e60(i32 x, i32 y, i32 val, i32* outX);

    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outVal);

    virtual i32 ScanUpGate(i32 x, i32 y, i32 val, i32* outY);
    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outVal);
    virtual i32 ScanRightGate(i32 x, i32 y, i32 val, i32* outX);
    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outVal);
    virtual i32 ScanDownGate(i32 x, i32 y, i32 val, i32* outY);

    CImageSet3() {
        m_width = 0;
        m_pixels = 0;
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }

    i32 m_height;
    i32 m_heightLog2;
    i32 m_byteSize;

    u8* m_pixels;
};
SIZE_UNKNOWN();
SIZE(0x18);

#endif // GRUNTZ_IMAGESETS_H
