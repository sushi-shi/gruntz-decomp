#ifndef GRUNTZ_IMAGESETS_H
#define GRUNTZ_IMAGESETS_H
// ImageSets.h - the three CImageSet collision-record variants CGameLevel::
// ReadImageSet allocates from a WWD tile-descriptor record (the first int of the
// record selects the kind). Split out of the GameLevel god-TU: the class defs live
// here (shared by the gamelevel factory + the imageset1/2/3 method TUs); the
// out-of-line method bodies live in src/Gruntz/ImageSet{1,2,3}.cpp.
//
// REAL-POLYMORPHIC: each is an 18-slot class deriving the CObject grand-base
// (slots 0-4 inherited, slot 1 = its virtual dtor), so cl emits its
// ??_7CImageSetN@@6B@ (VTBL-bound in GameLevel.cpp, where ReadImageSet's
// `new CImageSetN` instantiates the inline ctor) and AUTO-stamps the vptr in that
// INLINE ctor - the base-subobject stamp dead-store-elides, lowering `new
// CImageSetN` to exactly the retail `RezAlloc(size); if (p) { stamp vptr; zero
// fields }` shape. Only the matched slots carry bodies; the inherited base thunks +
// engine slots are declared-only (their vtable entries reloc-mask). The vptr sits
// at +0x00 (implicit); the padding pins each size: kind 1 = 0x10, kind 2 = 0x24,
// kind 3 = 0x18. Slot RVAs (from retail 0x5f0198/01e0/0228) noted per class.
#include <Wap32/Object.h> // CObject grand-base (slots 0-4)
#include <Ints.h>
#include <rva.h>

// The engine routes object allocation through the Rez heap (RezAlloc @0x1b9b46 =
// nothrow operator new / RezFree @0x1b9b82). ReadImageSet `new`s its variants
// through RezAlloc, so each class models it as the class allocator: `new CImageSetN`
// emits a direct `push size; call RezAlloc` instead of the global `??2`.
extern "C" void* RezAlloc(u32 size); // 0x1b9b46
extern "C" void RezFree(void* p);    // 0x1b9b82

struct CImageSet1 : CObject {
    virtual ~CImageSet1() OVERRIDE; // slot 1 (CObject dtor)
    // slots 0-4 inherited from CObject (slot 1 = its virtual dtor; cl auto-
    // stamps this vptr in the inline ctor, the base stamp dead-store-elides).
    virtual i32 Parse(void* record); // [5]  +0x14  0x166d40
    virtual void s18();              // [6]  0x161330
    virtual void s1c();              // [7]  0x161340
    virtual i32
    GetCollisionAt(i32 x, i32 y); // [8]  +0x20  0x161380  per-pixel collision-kind query
    virtual i32 GetStride();      // [9]  +0x24  0x161410  record byte length (cursor advance)
    virtual void s28();           // [10] 0x161390
    virtual void s2c();           // [11] 0x1613a0
    virtual void s30();           // [12] 0x1613b0
    virtual void s34();           // [13] 0x1613c0
    virtual void s38();           // [14] 0x1613d0
    virtual void s3c();           // [15] 0x1613e0
    virtual void s40();           // [16] 0x1613f0
    virtual void s44();           // [17] 0x161400
    CImageSet1() {
        m_04 = 0; // cl auto-stamps &??_7CImageSet1 first
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    // ~CImageSet1 (0x161370) is the real virtual dtor: /O2 dead-store-elides the
    // derived vptr stamp under the immediate base ~CObject stamp, lowering to the
    // single `mov [ecx], &??_7CObject; ret` retail carries. Body in ImageSet1.cpp.
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
};
struct CImageSet2 : CObject {
    virtual ~CImageSet2() OVERRIDE; // slot 1 (CObject dtor)
    // slots 0-4 inherited from CObject (slot 1 = its virtual dtor).
    virtual i32 Parse(void* record); // [5]  +0x14  0x166990
    virtual void s18();              // [6]  0x161420
    virtual void s1c();              // [7]  0x161430
    virtual i32
    GetCollisionAt(i32 x, i32 y); // [8]  +0x20  0x161470  per-pixel collision-kind query
    virtual i32 GetStride();      // [9]  +0x24  0x1614a0  record byte length (cursor advance)
    // [10] 0x1669e0: bounds-clamp query - if (a,b) is inside the {m_14..m_1c}x{m_18..m_20}
    // box, report the near x-edge coord + its paired value in *outA/*outB (1), else 0.
    virtual i32 Query_1669e0(i32 a, i32 b, i32* outA, i32* outB);
    // slots 11-17: directional edge-finder variants (near/far x/y edge; the value-check
    // forms take an i32 to match against the paired cell before reporting).
    virtual i32 Query_166a40(i32 a, i32 b, i32 val, i32* out);    // [11] 0x166a40
    virtual i32 Query_166b90(i32 a, i32 b, i32* outA, i32* outB); // [12] 0x166b90
    virtual i32 Query_166bf0(i32 a, i32 b, i32 val, i32* out);    // [13] 0x166bf0
    virtual i32 Query_166ab0(i32 a, i32 b, i32* outA, i32* outB); // [14] 0x166ab0
    virtual i32 Query_166b20(i32 a, i32 b, i32 val, i32* out);    // [15] 0x166b20
    virtual i32 Query_166c60(i32 a, i32 b, i32* outA, i32* outB); // [16] 0x166c60
    virtual i32 Query_166cd0(i32 a, i32 b, i32 val, i32* out);    // [17] 0x166cd0
    CImageSet2() {
        m_04 = 0; // cl auto-stamps &??_7CImageSet2 first
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    i32 m_04; // +0x04
    i32 m_08; // +0x08
    i32 m_0c; // +0x0c
    i32 m_10; // +0x10
    i32 m_14; // +0x14
    i32 m_18; // +0x18
    i32 m_1c; // +0x1c
    i32 m_20; // +0x20
};
SIZE_UNKNOWN(CImageSet3);
struct CImageSet3 : CObject {
    virtual ~CImageSet3() OVERRIDE; // slot 1 (CObject dtor)
    // slots 0-4 inherited from CObject (slot 1 = its virtual dtor).
    virtual i32 Parse(void* record); // [5]  +0x14  0x166d70
    virtual void FreePixels();       // [6]  0x1614b0  release the owned +0x14 pixel buffer
    virtual void s1c();              // [7]  0x1614d0
    virtual i32
    GetCollisionAt(i32 x, i32 y); // [8]  +0x20  0x161570  per-pixel collision-kind query
    virtual i32 GetStride();      // [9]  +0x24  0x161590  record byte length (cursor advance)
    // [10] 0x166e00: scan LEFT from (x,y) along the row for the first pixel that differs
    // from the pixel at (x,y); report its column + value. Was a body-less `s28` placeholder
    // here while the REAL body sat in src/Image/ImageSet3.cpp as a non-virtual on a
    // 5-slot local view of this class - so the vtable pointed at a symbol nothing defined.
    virtual i32 ScanRunLeft_166e00(i32 x, i32 y, i32* outX, i32* outVal); // [10] 0x166e00
    virtual void s2c(); // [11] 0x166e60 (recovery gap, not a stub)
    // [12] 0x166eb0: vertical run-scan UP from (x,y) - walk to the first row whose pixel
    // at column x differs from (x,y)'s; report that row + its value.
    virtual i32 ScanUp_166eb0(i32 x, i32 y, i32* outY, i32* outVal);
    // [13-17]: the directional scan family - UP/RIGHT/DOWN run scans (the *Gate forms
    // walk to the first pixel that EQUALS `val`, reporting only the coord).
    virtual i32 ScanUpGate_166f20(i32 x, i32 y, i32 val, i32* outY);    // [13] 0x166f20
    virtual i32 ScanRight_166f80(i32 x, i32 y, i32* outX, i32* outVal); // [14] 0x166f80
    virtual i32 ScanRightGate_166ff0(i32 x, i32 y, i32 val, i32* outX); // [15] 0x166ff0
    virtual i32 ScanDown_167050(i32 x, i32 y, i32* outY, i32* outVal);  // [16] 0x167050
    virtual i32 ScanDownGate_1670d0(i32 x, i32 y, i32 val, i32* outY);  // [17] 0x1670d0
    // (GetSize_1633e0 moved to its REAL owner CDDrawWorkerHost - the 0x1633e0 body
    // reads +0xb0, which this 0x18-byte record cannot hold; Play.cpp's grid-owner
    // casts were a mis-attribution and now target CDDrawWorkerHost.)

    CImageSet3() {
        m_width = 0; // cl auto-stamps &??_7CImageSet3 first
        m_pixels = 0;
    }
    void* operator new(size_t n) {
        return RezAlloc(n);
    }
    void operator delete(void* p) {
        RezFree(p);
    }
    i32 m_width;      // +0x04  tile width
    i32 m_height;     // +0x08  tile height
    i32 m_heightLog2; // +0x0c  log2(height)
    i32 m_byteSize;   // +0x10  width*height (byte size)
    void* m_pixels;   // +0x14  owned pixel buffer
};

#endif // GRUNTZ_IMAGESETS_H
