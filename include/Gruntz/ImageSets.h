#ifndef GRUNTZ_IMAGESETS_H
#define GRUNTZ_IMAGESETS_H
#include <Wap32/Object.h> // CObject grand-base (slots 0-4)
#include <Ints.h>
#include <rva.h>

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

struct CImageSet1 : CObject {
    virtual ~CImageSet1() OVERRIDE; // slot 1 (CObject dtor)
    // slots 0-4 inherited from CObject (slot 1 = its virtual dtor; cl auto-
    // stamps this vptr in the inline ctor, the base stamp dead-store-elides).
    virtual i32 Parse(void* record); // [5]  +0x14  0x166d40
    virtual void FreePixels();       // [6]  0x161330  `ret` (the family's pixel-release
                                     //      slot; the kind-1 set owns no pixel buffer)
    virtual i32 GetKind();           // [7]  0x161340  `return 1` - the set-format tag
                                     //      (kind 2/3 siblings return 2/3 at this slot)
    virtual i32
    GetCollisionAt(i32 x, i32 y); // [8]  +0x20  0x161380  per-pixel collision-kind query
    virtual i32 GetStride();      // [9]  +0x24  0x161410  record byte length (cursor advance)
    // [10-17]: the edge-query family (CImageSet2's Query_* slots). The kind-1 set
    // has no collision box, so six report 0 (`xor eax,eax; ret 0x10`) and the two
    // far-edge forms report the extent minus one (m_04-1 / m_08-1).
    virtual i32 Query_161390(i32 a, i32 b, i32* outA, i32* outB); // [10] return 0
    virtual i32 Query_1613a0(i32 a, i32 b, i32 val, i32* out);    // [11] return 0
    virtual i32 Query_1613b0(i32 a, i32 b, i32* outA, i32* outB); // [12] return 0
    virtual i32 Query_1613c0(i32 a, i32 b, i32 val, i32* out);    // [13] return 0
    virtual i32 Query_1613d0(i32 a, i32 b, i32* outA, i32* outB); // [14] return m_04 - 1
    virtual i32 Query_1613e0(i32 a, i32 b, i32 val, i32* out);    // [15] return 0
    virtual i32 Query_1613f0(i32 a, i32 b, i32* outA, i32* outB); // [16] return m_08 - 1
    virtual i32 Query_161400(i32 a, i32 b, i32 val, i32* out);    // [17] return 0
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
    virtual void FreePixels();       // [6]  0x161420  `ret` (owns no pixel buffer)
    virtual i32 GetKind();           // [7]  0x161430  `return 2` - the set-format tag
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
    virtual i32 GetKind();           // [7]  0x1614d0  `return 3` - the set-format tag
    virtual i32
    GetCollisionAt(i32 x, i32 y); // [8]  +0x20  0x161570  per-pixel collision-kind query
    virtual i32 GetStride();      // [9]  +0x24  0x161590  record byte length (cursor advance)
    // [10] 0x166e00: scan LEFT from (x,y) along the row for the first pixel that differs
    // from the pixel at (x,y); report its column + value. Was a body-less `s28` placeholder
    // here while the REAL body sat in src/Image/ImageSet3.cpp as a non-virtual on a
    // 5-slot local view of this class - so the vtable pointed at a symbol nothing defined.
    virtual i32 ScanRunLeft(i32 x, i32 y, i32* outX, i32* outVal); // [10] 0x166e00
    // [11] 0x166e60: the val-gated form of ScanRunLeft (the family pairs each scan
    // with a gate variant; body is a Ghidra recovery gap, unreconstructed).
    virtual i32 ScanRunLeftGate_166e60(i32 x, i32 y, i32 val, i32* outX);
    // [12] 0x166eb0: vertical run-scan UP from (x,y) - walk to the first row whose pixel
    // at column x differs from (x,y)'s; report that row + its value.
    virtual i32 ScanUp(i32 x, i32 y, i32* outY, i32* outVal);
    // [13-17]: the directional scan family - UP/RIGHT/DOWN run scans (the *Gate forms
    // walk to the first pixel that EQUALS `val`, reporting only the coord).
    virtual i32 ScanUpGate(i32 x, i32 y, i32 val, i32* outY);    // [13] 0x166f20
    virtual i32 ScanRight(i32 x, i32 y, i32* outX, i32* outVal); // [14] 0x166f80
    virtual i32 ScanRightGate(i32 x, i32 y, i32 val, i32* outX); // [15] 0x166ff0
    virtual i32 ScanDown(i32 x, i32 y, i32* outY, i32* outVal);  // [16] 0x167050
    virtual i32 ScanDownGate(i32 x, i32 y, i32 val, i32* outY);  // [17] 0x1670d0
    // (GetSize moved to its REAL owner CDDrawWorkerHost - the 0x1633e0 body
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
    // +0x14 the owned pixel plane. Typed u8* on the binary's own evidence, not a
    // guess: EVERY reader indexes it as bytes - GetCollisionAt does
    // `reinterpret_cast<u8*>(m_pixels) + ((y << m_heightLog2) + x)` and the whole scan family does
    // `((u8*)m_pixels)[off]` (18 sites, not one of them casting to anything else),
    // and m_byteSize is width*height, i.e. ONE byte per pixel. The void* was what
    // forced all 18 casts.
    u8* m_pixels; // +0x14  owned pixel plane (1 byte/pixel; m_byteSize = w*h)
};

#endif // GRUNTZ_IMAGESETS_H
