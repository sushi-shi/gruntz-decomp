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

#include <DDrawMgr/DDSurface.h> // IDirectDrawSurface (the held COM surface interface) + the
                                // CDDSurface wrapper's foreign vtable (IsValid/v20 dispatch)
#include <Io/FileStream.h>

// The DDraw surface/palette pool host (<DDrawMgr/DDrawPtrCollections.h>) - the display
// manager the file-image decoders receive as their palette-context argument (m_palBpp /
// m_palette / m_hasPalette). Pointer/param-only here, so a forward decl keeps the manager
// (and its MFC container members) out of the widely-included consumers of this header.
class CDDrawPtrCollections;

// ---------------------------------------------------------------------------
// CRezImage - the image-resolution dispatcher.
// LoadFromRez is __thiscall, ret 0xc (this + name + two opaque pass-through
// args). It forwards (name, a2, a3) verbatim to the matching format loader.
// The five sibling loaders and the per-format decoders are reconstructed in
// Image.cpp; only the shared blitter DecodeBlit stays external/no-body.
// ---------------------------------------------------------------------------
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
    RVA(0x00176ad0, 0x17)
    void SetPalette(void* paletteNode, i32 scalar) {
        m_paletteNode = paletteNode;
        m_paletteScalar = scalar;
    }
    i32 Save(const char* filename, void* paletteObj);                // 0x176b00 (8bpp-only dispatch to SaveBmp)
    i32 SaveBmp(const char* filename, void* paletteObj);              // 0x176b30

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
    void* m_listPosition;         // +0x44c  pool: cached AddTail POSITION (surface list node)
    i32 m_transparent;            // +0x450  flag (1 = transparent/RLE plane)
    i32 m_paletteScalar;          // +0x454  associated palette scalar (SetPalette 2nd arg)
    void* m_paletteNode;          // +0x458  palette node / SaveBmp default palette object
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
// CFileImageSurface - the derived surface wrapper whose vtable lives at 0x5efa58
// (adds LoadKeyed/Refresh etc. on top of CFileImage's slots; shares CFileImage's
// teardown). The class adds no destructible members of its own, so ~CFileImageSurface
// (0x142360) is byte-identical to ~CFileImage: it re-stamps the BASE vptr
// (the shared surface vtable @0x5ef7f0), runs the shared FreeSurfaces teardown and destroys the owned
// CPtrArray at +0x94. Modeled as a standalone shape (NOT deriving from CFileImage)
// so its dtor inlines that teardown the way retail's second compiled copy does,
// without disturbing the already-matched CFileImage dtor. ScalarDelete is its `??_G`
// (0x142340): destroy + conditional RezFree.
SIZE_UNKNOWN(CFileImageSurface);
VTBL(CFileImageSurface, 0x001efa58); // ??_7CFileImageSurface@@6B@ (12-slot surface vtable)
class CFileImageSurface {
public:
    void* ScalarDelete(u32 flags); // 0x142340 (`??_G` scalar-deleting destructor)
    virtual ~CFileImageSurface(); // slot 0 0x142360 (virtual: the implicit vptr stamp lands stamp-first)
    // The 12-slot 0x5efa58 table (CDDSurface-family surface interface). Slots 1-11
    // declared-only (reloc-masked, matching-neutral) so ??_7CFileImageSurface is sized.
    virtual i32 Refresh();        // slot 1  @0x13e140
    virtual i32 Apply();          // slot 2  @0x13e0a0
    virtual i32 BlitSurf();       // slot 3  @0x13e0d0
    virtual void FreeSurfaces2(); // slot 4  @0x13e4d0
    virtual i32 Slot05();         // slot 5  @0x1412d0
    virtual void ConfigureSurf(); // slot 6 @0x143cc0
    virtual i32 Slot07();         // slot 7  @0x13f960
    virtual void Slot08();        // slot 8  @0x13e2e0
    virtual i32 ResolveEx();      // slot 9  @0x148890
    virtual i32 LoadByExt();      // slot 10 @0x148940
    virtual i32 LoadKeyed();      // slot 11 @0x148840
    void FreeSurfaces();          // 0x13e4d0 (shared teardown, external/no-body)

    // vptr @+0x00 (implicit, polymorphic; the compiler emits the dtor's stamp).
    char m_pad04[0x94 - 0x04]; // +0x04
    CPtrArray m_elements;      // +0x94  owned element array (auto member-dtor)
};

// CFileImage's own surface vtable lives at VA 0x5ef7f0 - the SHARED 9-slot base vtable
// of the pool-item surface family (bound by address in CDirectDrawMgr.cpp as
// g_poolItemVtbl; the sibling derived classes CPoolItemA @0x5efa58 / CDDSurface / ... in
// CDDrawPtrCollections.cpp / CDDSurfaceDtor.cpp derive from this same base). CFileImage
// declares the full 9-slot polymorphic layout above (dtor / Refresh / Init1 / BlitSurf /
// FreeSurfaces / IsValid / v18 / v1c / v20), so the two surface virtuals it dispatches
// (IsValid @0x14 slot 5, v20 @0x20 slot 8) and the slot-3 BlitSurf dispatch are genuine
// virtual calls - the former pointer-only CFileImageVtblView is retired.

// The held DirectDraw surface at +0x08, dispatched THISCALL for the CFileImage.cpp
// save/flip Unlock call sites (vtable slot +0x80 = Unlock(rect)). Same physical held
// surface as m_8 (IDirectDrawSurface*, the DDraw COM/stdcall view); this is the
// pointer-only dispatch interface for the thiscall Unlock sites - it holds NO data.
// [The held-surface convention (thiscall here vs stdcall COM elsewhere) is a re-match
// reconcile item - see docs/multi-view-worklist.md.]
class CFileImageHeldSurface {
public:
    virtual void s00();
    virtual void s04();
    virtual void s08();
    virtual void s0c();
    virtual void s10();
    virtual void s14();
    virtual void s18();
    virtual void s1c();
    virtual void s20();
    virtual void s24();
    virtual void s28();
    virtual void s2c();
    virtual void s30();
    virtual void s34();
    virtual void s38();
    virtual void s3c();
    virtual void s40();
    virtual void s44();
    virtual void s48();
    virtual void s4c();
    virtual void s50();
    virtual void s54();
    virtual void s58();
    virtual void s5c();
    virtual void s60();
    virtual void s64();
    virtual void s68();
    virtual void s6c();
    virtual void s70();
    virtual void s74();
    virtual void s78();
    virtual void s7c();
    virtual i32 Unlock(void* rect); // +0x80
};

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
