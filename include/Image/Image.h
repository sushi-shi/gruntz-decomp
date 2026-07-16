// Image.h - the engine's image-resolution surface (the REZ -> image load path).
//
// Two classes live here, both reconstructed only as deeply as needed to
// byte-match their leaf methods. Fields are named from their use across the
// matched methods; the OFFSETS + code bytes stay load-bearing. A few write-only
// fields with no proven role keep their m_<hexoffset> placeholder.
//
//   class CRezImage  - the extension DISPATCHER class plus its five per-extension
//     loaders. LoadFromRez(name,a2,a3) does `ext = strrchr(name,'.')` then a
//     stricmp ladder on ".BMP"/".PCX"/".RID"/".PID" and hands off (all args
//     forwarded) to one of five sibling __thiscall loaders (LoadBmp/LoadPcx/
//     LoadRid/LoadPid/LoadDefault), each `ret 0xc`. The loaders are the real
//     file/resource consumers: LoadBmp opens a CFileIO, parses the BMP file +
//     info headers, builds the CRezImage via a decode helper and reads the pixel
//     bytes; LoadPcx/Rid/Pid slurp the whole file into an `operator new` buffer
//     and run a per-format decode helper; LoadDefault loads a Win32 RT_BITMAP
//     resource and decodes it. The per-format decode helpers are also CRezImage
//     __thiscall methods, reconstructed in Image.cpp; they share the plane
//     allocator DecodeBmpHeader and the blitter DecodeBlit (external/no-body).
//
//   class CFileImage - the file-backed BMP/PCX/PID loaders that actually open a
//     file via CFileIO, slurp its bytes into an `operator new` buffer and hand
//     them to a per-format decode helper. Matched here: LoadBmp/LoadPcx (ret 8)
//     and LoadPid (ret 0xc). The CFileIO stack object forces a C++ EH frame -> /GX.
#ifndef SRC_IMAGE_IMAGE_H
#define SRC_IMAGE_IMAGE_H

#include <rva.h>

#include <Mfc.h> // POSITION (CRezImage::m_listPosition, the pool's cached list-node handle)
                 // + <windows.h> (BITMAPINFOHEADER / HBITMAP the DIB fields use)
#include <DDrawMgr/DDSurface.h> // IDirectDrawSurface (the held COM surface interface) + the
                                // CDDSurface wrapper vtable (IsValid/BlitIntoDesc dispatch)
#include <Io/FileStream.h>

// The DDraw surface/palette pool host (<DDrawMgr/DDrawPtrCollections.h>) - the display
// manager the file-image decoders receive as their palette-context argument (m_palBpp /
// m_palette / m_hasPalette). Pointer/param-only here, so a forward decl keeps the manager
// (and its MFC container members) out of the widely-included consumers of this header.
class CDDrawPtrCollections;

// The palette list node CRezImage::m_paletteNode holds (+0x458). Pointer-only here, so a
// forward decl in its home namespace (the full shape + bodies are <Image/ImagePaletteNode.h>,
// owned by ImagePool.cpp). CImagePool::Free reads m_paletteNode as this type (it was a
// `void*` cast to CImagePaletteNode* at every read).
namespace ApiCallerStubs {
    struct CImagePaletteNode;
}

// ---------------------------------------------------------------------------
// CRezImage - the image-resolution dispatcher.
// LoadFromRez is __thiscall, ret 0xc (this + name + two opaque pass-through
// args). It forwards (name, a2, a3) verbatim to the matching format loader.
// The five sibling loaders and the per-format decoders are reconstructed in
// Image.cpp; only the shared blitter DecodeBlit stays external/no-body.
// ---------------------------------------------------------------------------
// The {left,top,right,bottom} fill rectangle FillRect scan-fills and FillRectAt builds
// (a plain rect record, not a class view).
struct CRezFillRect {
    i32 left;   // +0x00
    i32 top;    // +0x04
    i32 right;  // +0x08
    i32 bottom; // +0x0c
};
SIZE(CRezFillRect, 0x10);

SIZE_UNKNOWN(CRezImage);
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

// ---------------------------------------------------------------------------
// CFileImage - the file-backed format loaders (the REZ payload consumers) AND
// their per-format pixel decoders. The Load{Bmp,Pcx,Pid} entry points construct
// a stack CFileIO, open the file, slurp it into an `operator new` buffer and call
// the matching per-format decoder; the buffer is freed + the stream closed on
// every exit.
//
// The decoders (DecodeBmp/DecodePcx/DecodePid + the low-level DecodePcxData) are
// reconstructed in Image.cpp. They read the format header out of `buf`, validate
// the geometry against the destination surface fields, optionally build a
// 256-entry RGBQUAD palette into a per-decoder file-scope buffer (from the BMP
// in-file RGBQUADs / the PCX|PID trailing 768-byte VGA palette) and hand the
// pixel bytes to one of the surface blitters (Blit / BlitDirect, ret 0x10/8 -
// external no-body) which copy into the destination plane.
//
// The OFFSETS + code bytes are load-bearing. The decoders touch only the 0xc0
// surface geometry (m_height, m_width) on `this`; the palette context they read
// (m_palBpp / m_palette / m_hasPalette at +0x538/+0x53c/+0x93c) lives on the
// CDDrawPtrCollections display manager passed in as the `surf`/`info` ARGUMENT -
// NOT on this 0xc0 surface (both surface ctors 0x13e9a0/0x1421a0 operator-new(0xc0)).
// The former conflated palette fields were retired to <DDrawMgr/DDrawPtrCollections.h>.
// ---------------------------------------------------------------------------

// A heap element held in CFileImage::m_elements (the +0x94 CPtrArray). FreeSurfaces
// walks the array `delete`-ing each element, which lowers to its slot-0
// scalar-deleting destructor (`??_G`, vtbl[0](1)). Real polymorphic stand-in: a
// real virtual destructor (declared-only) so `delete e` emits the canonical
// null-guarded slot-0 dispatch. The element type itself lives in another
// (unmatched) TU, so no ??_7/dtor body is emitted here - address never taken.
class CFileImageElement {
public:
    virtual ~CFileImageElement(); // slot 0, @0x00 (scalar-deleting dtor ??_G)
};

// ---------------------------------------------------------------------------
// CFileImageSurface - a CDDSurface-derived pool-item surface (the "a58" subclass,
// vtable 0x5efa58: overrides the dtor + slot 6 GetPoolKind, adds slots 9/10/11).
// It is the SAME retail class DDrawPtrCollections.h models as CPoolItemA; its ??_G/~
// COMDAT (0x142340/0x142360) is emitted under the CFileImageSurface name from
// DirectDrawMgr.cpp (CPoolItemA there is the declared-only RELOC_VTBL alias).
//
// It adds NO destructible members of its own (the +0x94 CPtrArray lives in the
// CDDSurface base), so ~CFileImageSurface is the /O2-inlined base teardown: MSVC5
// elides the derived vptr stamp and emits ONLY the base ??_7CDDSurface (0x5ef7f0)
// stamp, then FreeSurfaces + the base's ~m_elements - byte-identical to ~CDDSurface
// (0x141350) and matching retail. That is why the dtor's vtable stamp binds to
// ??_7CDDSurface @0x1ef7f0 (reloc-fidelity), not this class's own 0x1efa58 datum.
// ScalarDelete is its `??_G` (0x142340): destroy + conditional RezFree.
SIZE(CFileImageSurface, 0xc0);
VTBL(CFileImageSurface, 0x001efa58); // ??_7CFileImageSurface@@6B@ (12-slot a58 surface vtable)
class CFileImageSurface : public CDDSurface {
public:
    void* ScalarDelete(u32 flags);         // 0x142340 (`??_G` scalar-deleting destructor)
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

// CFileImage's own surface vtable lives at VA 0x5ef7f0 - the SHARED 9-slot base vtable
// of the pool-item surface family (bound by address in CDirectDrawMgr.cpp as
// g_poolItemVtbl; the sibling derived classes CPoolItemA @0x5efa58 / CDDSurface / ... in
// CDDrawPtrCollections.cpp / CDDSurfaceDtor.cpp derive from this same base). CFileImage
// declares the full 9-slot polymorphic layout above (dtor / Refresh / Init1 / BlitSurf /
// FreeSurfaces / IsValid / GetPoolKind / RestoreLost / BlitIntoDesc), so the virtuals it dispatches
// (IsValid @0x14 slot 5, BlitIntoDesc @0x20 slot 8) and the slot-3 BlitSurf dispatch are genuine
// virtual calls - the former pointer-only CFileImageVtblView is retired.

// [The former CFileImageHeldSurface (a 33-slot thiscall placeholder view of the held
// surface, Unlock @+0x80) is retired: retail's Unlock sites are genuine COM stdcall
// (push arg / push surf / call [vtbl+0x80]) on m_8 (IDirectDrawSurface*), slot 32.]

// The run-length source header handed to Decode (arg `src`): a +0x04/+0x06/+0x08/+0x0a
// int16 bounding box and a +0x41 format byte (1 = 8-bit, 3 = 24-bit); run data at +0x80.
class CFileImageSrc {
public:
    char _00[0x04];
    i16 m_04; // +0x04  box top
    i16 m_06; // +0x06  box left
    i16 m_08; // +0x08  box bottom
    i16 m_0a; // +0x0a  box right
    char _0c[0x41 - 0x0c];
    u8 m_41; // +0x41  format (1 = 8-bit, 3 = 24-bit)
};

// The palette source SaveBmp reads (arg2): +0x0c = the 256-entry source palette
// (4 bytes/entry; the export copies bytes 0/1/2 into the BMP RGBQUADs).
class CFileImagePal {
public:
    char _00[0x0c];
    u8* m_0c; // +0x0c  source palette (4 bytes/entry)
};

// ClipRect16 (the 16-byte rect/clip record Run takes by value) now lives on the unified
// surface class's header, <DDrawMgr/DDSurface.h>.

// The CFileImage.cpp save/decode data records (DecodeSrc / BmpFileHeader / TgaHeader)
// live in <Image/FileImageRecords.h> - a CFileImage.cpp-only header, kept out of this
// widely-included one so the other Image TUs are not perturbed (MSVC codegen leak).

// The former CFileImage class (the DIRSURF.CPP 0xc0 surface: BMP/PCX/PID loaders +
// surface blitters + DirectDraw thunks + 9-slot vtable) is UNIFIED with CDDSurface -
// it was one physical class under two names. Its full definition now lives in
// <DDrawMgr/DDSurface.h> (included above); the method bodies below in this module
// (Image.cpp / FileImage*.cpp / LutShadeRect.cpp) are defined on CDDSurface.

// --- vtable catalog (reduced-view classes share their base vtable rva) ---

#endif // SRC_IMAGE_IMAGE_H
