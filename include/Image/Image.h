#ifndef SRC_IMAGE_IMAGE_H
#define SRC_IMAGE_IMAGE_H

#include <rva.h>

#include <Mfc.h> // POSITION (CRezImage::m_listPosition, the pool's cached list-node handle)
#include <DDrawMgr/DDSurface.h> // IDirectDrawSurface (the held COM surface interface) + the
#include <Io/FileStream.h>

class CDDrawPtrCollections;

namespace ApiCallerStubs {
    struct CImagePaletteNode;
}

struct CRezFillRect {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};
SIZE(0x10);

struct ScanlinePalette {
    char m_pad0[8];    // +0x00  header
    u32 m_colors[256]; // +0x08  RGB table (indexed by the 8bpp pixel)
};
SIZE_UNKNOWN();

class CRezImage {
public:
    i32 LoadFromRez(char* name, void* a2, void* a3);

    // The five per-extension loaders (bodies in Image.cpp). All __thiscall,
    // ret 0xc, taking the same (name, a2, a3) triple.
    i32 LoadBmp(char* name, void* a2, void* a3);
    i32 LoadPcx(char* name, void* a2, void* a3);
    i32 LoadRid(char* name, void* a2, void* a3);
    i32 LoadPid(char* name, void* a2, void* a3);
    i32 LoadDefault(char* name, void* a2, void* a3);

    // Per-format decode helpers (bodies in Image.cpp). All __thiscall on CRezImage,
    // invoked by the loaders above with the decoded header fields / raw file
    // buffer / resource pointer. Names are placeholders (the FUN_* labels carry
    // no real name). DecodeBmpHeader is the allocator/setup: it fills the
    // BITMAPINFOHEADER at this+0, CreateDIBSections the plane (HBITMAP @+0x428,
    // bits @+0x42c) and operator-new's the per-row offset table @+0x430. The
    // Pcx/Rid/Pid/Res decoders call it, then blit the decoded pixels.
    i32 DecodeBmpHeader(void* a2, i32 width, i32 height, i32 bitcount, void* a3);
    i32 DecodePcxData(void* buf, void* a2, void* a3);
    i32 DecodeRidData(void* buf, void* a2, void* a3);
    i32 DecodePidData(void* buf, void* a2, void* a3);
    i32 DecodeResData(void* buf, void* a2, void* a3);

    // The shared plane blitter (FUN_00575930): a __thiscall CRezImage method that
    // (re)allocates/decodes via DecodeBmpHeader then copies `src` into the plane
    // (flat rep-movs when m_rowPad==0, else row-by-row through m_rowOffsets).
    // External/no-body so its call reloc-masks. ret 0x18 = 6 stack args.
    i32 DecodeBlit(void* src, void* a2, i32 width, i32 height, i32 bitcount, void* a3);

    // The rest of the DIB-surface class (bodies in CScanlineSurface.cpp /
    // CScanlineSurfaceSave.cpp / ImagePool.cpp) - formerly the fragmented views
    // CScanlineSurface and ApiCallerStubs::CImageSurfaceNode, now folded onto this
    // one class. DispatchDecode (0x175a00) is the format-decoder selector: __thiscall,
    // reads `kind` (the 2nd stack arg, 2..5) and forwards (this, buf, a2, a3) to one
    // of the four Decode*Data decoders keeping `this` in ecx (the retail 2-register
    // arg-forward reserves ecx for the pass-through). EnsureSize/Convert8To16 reallocate
    // via DecodeBmpHeader (the "Create" entry @0x1757c0); Free (0x175c90, == the pool's
    // node Cleanup) releases the DIB object + the operator-new'd row table; SetPalette
    // (0x176ad0) latches the associated palette node; SaveBmp writes the 8bpp plane out.
    i32 DispatchDecode(void* buf, i32 kind, void* dc, void* ctrl);    // 0x175a00
    i32 Convert8To16(void* dc, CRezImage* src, void* pal);            // 0x175b80
    i32 EnsureSize(void* dc, i32 w, i32 h, i32 bitCount, void* flag); // 0x175ce0
    void Fill(i32 value);                                             // 0x175d50
    void Free();                                                      // 0x175c90 (pool: Cleanup)
    void SetPalette(void* paletteNode, i32 scalar);                   // 0x176ad0
    i32 Save(const char* filename, void* paletteObj);    // 0x176b00 (8bpp-only dispatch to SaveBmp)
    i32 SaveBmp(const char* filename, void* paletteObj); // 0x176b30
    void FillRect(CRezFillRect* r, i32 color);           // 0x176d20 (8bpp scanline rect fill)
    void FillRectAt(
        i32 dx,
        i32 dy,
        CRezFillRect* src,
        i32 color
    );                   // 0x176da0 (translate src rect to dx,dy, then FillRect)
    void FlipVertical(); // 0x176840 (top-bottom row swap via scratch)

    // Layout. The object opens with a BITMAPINFOHEADER (this+0,
    // biSize..biClrImportant) and a 256-entry WORD color table (this+0x28);
    // DecodeBmpHeader fills these, CreateDIBSections the plane and builds the
    // bottom-up per-row offset table. The OFFSETS are load-bearing.
    BITMAPINFOHEADER m_bih;       // +0x000  biSize/biWidth/biHeight/... (filled by setup)
    u16 m_pal[256];               // +0x28  DIB_PAL_COLORS index table
    char m_pad228[0x428 - 0x228]; // +0x228
    HBITMAP m_dibSection;         // +0x428  HBITMAP from CreateDIBSection (the DIB section)
    u8* m_pixels;                 // +0x42c  decoded pixel plane (CreateDIBSection's bits)
    i32* m_rowOffsets;            // +0x430  bottom-up per-row byte-offset table (operator new'd)
    i32 m_434;                    // +0x434  0 (write-only; role unproven, decoded by DecodeBlit)
    i32 m_width;                  // +0x438  image width (bytes/row of the source)
    i32 m_height;                 // +0x43c  abs(height): number of rows
    i32 m_bitCount;               // +0x440  bits per pixel
    i32 m_stride;                 // +0x444  aligned destination row stride (bytes per row)
    i32 m_rowPad;                 // +0x448  destination padding = m_stride - m_width
    POSITION m_listPosition;      // +0x44c  pool: cached AddTail POSITION (surface list node)
    i32 m_transparent;            // +0x450  flag (1 = transparent/RLE plane)
    i32 m_paletteScalar;          // +0x454  associated palette scalar (SetPalette 2nd arg)
    ApiCallerStubs::CImagePaletteNode* m_paletteNode; // +0x458  the pool's palette list node
};
SIZE_UNKNOWN();

// VTBL_ABSENT: never-constructed 1-slot dtor-dispatch facet over the CDDSurface
// m_elements pool entries (slot-0-dtor family scheme). @identity-TODO: no
// reconstructed producer adds to m_elements yet - recover the element class from
// the filler once it lands.
VTBL_ABSENT(CFileImageElement);
class CFileImageElement {
public:
    virtual ~CFileImageElement(); // slot 0, @0x00 (scalar-deleting dtor ??_G)
};
SIZE_UNKNOWN();

class CFileImageSurface : public CDDSurface {
public:
    virtual ~CFileImageSurface() OVERRIDE; // slot 0  0x142360
    virtual i32 GetPoolKind() OVERRIDE;    // slot 6  0x143cc0 (POOLKIND_FILEIMAGE)
    // The three init-tail slots. xref-proven REAL virtuals (zero direct callers -
    // each body is reached ONLY through this vtable); bodies in DDrawPtrCollections.cpp
    // (were misbound as CDDSurface:: non-virtual base methods).
    virtual i32 ResolveEx(
        void* surf,
        void* buf,
        i32 type,
        u32 size,
        i32 ctrl,
        i32 trans
    ); // slot 9  0x148890 (decode buf into the surface)
    virtual i32 LoadByExt(
        CDDrawPtrCollections* info,
        char* path,
        i32 flags,
        i32 key
    ); // slot 10 0x148940 (pick loader by file extension)
    virtual i32 LoadKeyed(
        void* surf,
        i32 width,
        i32 height,
        i32 a4,
        i32 a5,
        i32 key
    ); // slot 11 0x148840 (blit + install colour key)
};
SIZE(0xc0);

class CFileImageSrc {
public:
    char _00[0x04];
    i16 m_boxTop;    // +0x04  box top
    i16 m_boxLeft;   // +0x06  box left
    i16 m_boxBottom; // +0x08  box bottom
    i16 m_boxRight;  // +0x0a  box right
    char _0c[0x41 - 0x0c];
    u8 m_format; // +0x41  format (1 = 8-bit, 3 = 24-bit)
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

class CFileImagePal {
public:
    char _00[0x0c];
    u8* m_srcPalette; // +0x0c  source palette (4 bytes/entry)
};
SIZE_UNKNOWN();

#endif // SRC_IMAGE_IMAGE_H
