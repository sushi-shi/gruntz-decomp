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

#include <DDrawMgr/CDDSurface.h> // IDirectDrawSurfaceZ (the held COM surface interface) + the
                                 // CDDSurface wrapper's foreign vtable (IsValid/v20 dispatch)
#include <Io/FileStream.h>

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

    // Layout. The object opens with a BITMAPINFOHEADER (this+0,
    // biSize..biClrImportant) and a 256-entry WORD color table (this+0x28);
    // DecodeBmpHeader fills these, CreateDIBSections the plane and builds the
    // bottom-up per-row offset table. The OFFSETS are load-bearing.
    BITMAPINFOHEADER m_bih;       // +0x000  biSize/biWidth/biHeight/... (filled by setup)
    u16 m_pal[256];               // +0x28  DIB_PAL_COLORS index table
    char m_pad228[0x428 - 0x228]; // +0x228
    HBITMAP m_dibSection;         // +0x428  HBITMAP from CreateDIBSection (the DIB section)
    void* m_pixels;               // +0x42c  decoded pixel plane (CreateDIBSection's bits)
    i32* m_rowOffsets;            // +0x430  bottom-up per-row byte-offset table (operator new'd)
    i32 m_434;                    // +0x434  0 (write-only; role unproven, decoded by DecodeBlit)
    i32 m_width;                  // +0x438  image width (bytes/row of the source)
    i32 m_height;                 // +0x43c  abs(height): number of rows
    i32 m_bitCount;               // +0x440  bits per pixel
    i32 m_stride;                 // +0x444  aligned destination row stride (bytes per row)
    i32 m_rowPad;                 // +0x448  destination padding = m_stride - m_width
    char m_pad44c[0x4];           // +0x44c
    i32 m_transparent;            // +0x450  flag (1 = transparent/RLE plane)
    i32 m_454;                    // +0x454  0 (write-only; role unproven)
    i32 m_458;                    // +0x458  0 (write-only; role unproven)
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
// The OFFSETS + code bytes are load-bearing. The decoders touch the surface
// geometry (m_height, m_width) and the palette context (m_palBitCount,
// m_palette 256-entry table, m_hasPalette flag).
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
class CFileImageSurface {
public:
    void* ScalarDelete(u32 flags); // 0x142340 (`??_G` scalar-deleting destructor)
    virtual ~CFileImageSurface();  // 0x142360 (virtual: the implicit vptr stamp lands stamp-first)
    void FreeSurfaces();           // 0x13e4d0 (shared teardown, external/no-body)

    // vptr @+0x00 (implicit, polymorphic; the compiler emits the dtor's stamp).
    char m_pad04[0x94 - 0x04]; // +0x04
    CPtrArray m_elements;      // +0x94  owned element array (auto member-dtor)
};

// CFileImage's own (foreign, shared) surface vtable lives at 0x5ef7f0 - a hand-rolled
// vtable a cl-emitted ??_7 would diverge from (the vtable-realization boundary). So
// CFileImage carries only its dtor virtual; the two surface virtuals it dispatches
// (IsValid @0x14, v20 @0x20) go through this pointer-only dispatch interface. It holds
// NO data (not a second view of the struct) - only the vtable slots, so the call lowers
// to the exact `mov eax,[this]; call [eax+slot]` the foreign vtable expects.
//
// Why not fold this into a polymorphic CFileByte hierarchy (final-sweep worklist):
// 0x5ef7f0 is the SHARED base vtable of a 4+ class family (bound by address in
// CDirectDrawMgr.cpp as g_poolItemVtbl; modeled as CPoolItemBase in
// CDDrawPtrCollections.cpp, CPoolItemA @0x5efa58, CDDSurface, ...). Its slot bodies
// are CFileImage methods referenced by their Q-mangled (non-virtual) names ACROSS
// FIVE TUs: e.g. FreeSurfaces (slot 4, 0x13e4d0) is direct-called from the dtors in
// image / cddrawptrcollections / cddsurfacedtor, and BlitSurf (slot 3, 0x13e0d0) has
// three direct by-name callers. Virtualizing them here (Q->U mangling) would break
// every cross-TU by-name reference. The real fix is unifying CFileImage with the
// DDrawMgr CPoolItemBase base across all 5 TUs - a dedicated multi-TU pass; until
// then this view is the language-forced device (BlitSurf/SaveFile already byte-exact
// through it).
SIZE_UNKNOWN(CFileImageVtblView);
struct CFileImageVtblView {
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual i32 BeginDecode(void* info, i32 w, i32 h, i32 z, i32 mode); // slot 3, @0x0c
    virtual void v10();
    virtual i32 IsValid(); // slot 5, @0x14
    virtual void v18();
    virtual void v1c();
    virtual i32 v20(void* a); // slot 8, @0x20
};

// The held DirectDraw surface at +0x08, dispatched THISCALL for the CFileImage.cpp
// save/flip Unlock call sites (vtable slot +0x80 = Unlock(rect)). Same physical held
// surface as m_8 (IDirectDrawSurfaceZ*, the DDraw COM/stdcall view); this is the
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

// The 16-byte rect/clip record DecodeThunk builds on the stack and passes by value to
// the inner blit/decode worker (Run, 0x1471d0).
struct ClipRect16 {
    i32 a, b, c, d;
};

// The CFileImage.cpp save/decode data records (DecodeSrc / BmpFileHeader / TgaHeader)
// live in <Image/CFileImageRecords.h> - a CFileImage.cpp-only header, kept out of this
// widely-included one so the other Image TUs are not perturbed (MSVC codegen leak).

class CFileImage {
public:
    void* LoadBmp(char* name, char* path);
    void* LoadPcx(char* name, char* path);
    void* LoadPid(char* name, char* path, void* a3);
    i32 DecodePcxEx(
        void* surf,
        char* path,
        void* a3,
        void* a4
    ); // arg1 = decode-target surface (-> DecodePcxData surf)

    // The surface SAVE/export path (DIRSURF.CPP). SaveFile validates the surface +
    // arguments, then SaveDispatch picks the per-bit-depth writer by m_a8 (8/16/24).
    // Clear blanks the surface, LoadKeyed blits + installs a colour key.
    i32 SaveFile(char* buf, i32 type, void* a3, void* a4); // 0x13f910 (ret 0x10)
    i32 SaveDispatch(char* a1, void* a2, void* a3);        // 0x144350 (ret 0xc)
    void Clear(i32 white);                                 // 0x13edb0 (ret 4)
    i32 LoadKeyed(
        void* surf,
        i32 width,
        i32 height,
        i32 a4,
        i32 a5,
        i32 key
    ); // 0x148840 (ret 0x18)

    // The per-bit-depth file writers SaveDispatch delegates to (ret 0xc = 3 args).
    // SaveBmp (0x1443b0) writes the 8bpp palettized BMP, SaveTga (0x144900) the 24bpp
    // TGA, SaveRle16 (0x144640) the 16bpp->24bpp BMP. SaveBmp/SaveTga are reconstructed
    // in CFileImage.cpp; the calls reloc-mask. (Former swapped names Save8@0x144900 /
    // Save24@0x1443b0 were the caller-side placeholders - the bodies prove 0x1443b0 is
    // the 8bpp writer, 0x144900 the 24bpp writer.)
    i32 SaveBmp(const char* path, void* pal, i32 mode); // 0x1443b0 (8bpp)
    i32 SaveRle16(void* a1, void* a2, void* a3);        // 0x144640 (16bpp)
    i32 SaveTga(const char* path, void* pal, i32 mode); // 0x144900 (24bpp)

    // Format dispatchers (reconstructed in Image.cpp). __thiscall on CFileImage.
    // Resolve picks the BMP/PCX/PID decoder by `type` (1/2/4) for the file path;
    // ResolveEx is the surface-blit variant that ORs the control word with 0x40,
    // runs the *Data decoders and installs the transparency colour after.
    i32 Resolve(void* surf, void* buf, i32 type, u32 size, void* surf2);
    i32 ResolveEx(void* surf, void* buf, i32 type, u32 size, i32 ctrl, i32 trans);
    i32 Fill(u32 color); // colour-fill blt (0x13e760)

    // The held-surface COM thunks (DIRSURF.CPP), external no-body/reloc-masked; the
    // same object exposes them so the decoders/blitters reach the DirectDraw surface
    // without a separate wrapper-class view. __thiscall.
    i32 Lock(void* rect);                                                    // 0x13e6d0
    i32 BltEx(void* dstRect, void* src, void* srcRect, u32 flags, void* fx); // 0x13eef0
    i32 SetColorKey(u32 flags, void* key);                                   // 0x13eaa0

    // The RLE row-decoders' surface accessors (external no-body/reloc-masked leaf
    // __thiscall): width/height getters, the pitch-scale (m_pitch * n) row base, and
    // the slot-0x80 Unlock thunk. Same object => plain methods, no wrapper-class view.
    i32 GetWidth();     // 0x141310  (returns m_width)
    i32 GetHeight();    // 0x141320  (returns m_height)
    i32 Scale(i32 n);   // 0x1413c0  (returns m_pitch * n)
    void UnlockThunk(); // 0x1413b0  (m_8->vtbl[0x80](m_8, 0))

    virtual ~CFileImage(); // 0x141350  (virtual: the implicit vptr stamp lands stamp-first)

    // The shared surface-teardown helper the destructor calls before destroying
    // its CByteArray member (external no-body, reloc-masked): releases the held
    // DirectDraw surfaces (m_8/m_c), empties the +0x94 CByteArray, and walks the
    // +0x98/+0x9c object array calling each element's slot-0 destructor.
    void FreeSurfaces(); // 0x13e4d0

    // Per-format decoders (reconstructed in Image.cpp). __thiscall on CFileImage.
    // arg1 is the source-palette surface (downcast to CFileImage* in each body); the
    // class passes surfaces as void* (Resolve/LoadBmp both hand it in untyped).
    void* DecodeBmp(void* surf, void* buf, u32 size);
    void* DecodePcx(void* surf, void* buf, u32 size);
    void* DecodePid(void* surf, void* buf, u32 size, void* surf2);
    i32 DecodePcxData(void* surf, void* buf, i32 size, i32 a4, i32 a5);

    // The surface-blit decoders ResolveEx dispatches to (ret 0x10 = 4 args). Same
    // functions the .BMP/.PCX file-load path uses (DecodeRun == the former caller-side
    // DecodeBmpData @0x143cf0; Decode == DecodePcxData2 @0x144b30). Reconstructed in
    // CFileImage.cpp; `info` is a 2nd CFileImage instance (the decode target/config).
    i32 DecodeRun(CFileImage* info, void* src, i32 a, i32 b);            // 0x143cf0 (BMP run)
    i32 Decode(CFileImage* info, CFileImageSrc* src, i32 len, i32 mode); // 0x144b30 (PCX run)

    // The file-load + export path reconstructed in CFileImage.cpp (== the DIRSURF.CPP
    // surface). LoadFile2/LoadFile slurp a .BMP/.PCX file into a heap buffer then run
    // DecodeRun/Decode; LoadByExt (CFileImageLoadByExt.cpp) picks the loader by file
    // extension; Load (0x144270, ApiCallers TU) is the default RT_BITMAP loader.
    // FlipVertical row-flips the locked surface; DecodeThunk forwards to Run (the inner
    // blit/decode worker, external/reloc-masked, taking a by-value ClipRect16).
    void FlipVertical(); // 0x13ebb0
    void DecodeThunk(
        i32 a1,
        i32 a2,
        i32 a3,
        i32 a4,
        i32 a5,
        i32 a6,
        i32 r0,
        i32 r1,
        i32 r2,
        i32 r3
    );                                                                        // 0x141280
    i32 Run(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, ClipRect16 clip); // 0x1471d0
    i32 LoadFile2(CFileImage* info, const char* path, i32 mode);              // 0x143e60
    i32 LoadFile(CFileImage* info, const char* path, i32 mode);               // 0x144d80
    i32 LoadByExt(CFileImage* info, char* path, i32 flags, i32 a4);           // 0x148940
    i32 Load(i32 a, char* name, i32 c);                                       // 0x144270

    // The surface blitters + raw run-decoders the decoders delegate to (external
    // no-body, reloc-masked). Blit does a palette-remap copy (ret 0x10 = 4 args),
    // BlitDirect a straight copy (ret 8 = 2 args), BlitSurf the DecodePcxData
    // setup (ret 0x14 = 5 args); DecodeRun8/DecodeRun24 RLE-expand one plane (ret
    // 4 = 1 arg); RunDecode1/RunDecode3 emit a decoded scanline run (ret 0x10 = 4
    // args); FillPalette installs the transparency colour (ret 4 = 1 arg).
    i32 Blit(void* src, i32 bitcount, void* palette, i32 mode);      // 0x13faa0
    i32 BlitDirect(void* src, i32 mode);                             // 0x13ece0
    i32 BlitSurf(void* surf, i32 width, i32 height, i32 a4, i32 a5); // 0x13e0d0
    i32 DecodeRun8(void* dst);                                       // 0x140aa0
    i32 DecodeRun24(void* dst);                                      // 0x140c50
    i32 RunDecode1(void* dst, void* src, i32 width, i32 height);     // 0x145270
    i32 RunDecode3(void* dst, void* src, i32 width, i32 height);     // 0x1453f0
    void FillPalette(void* arg);                                     // 0x13eb40

    // The per-(dest-bpp,src-bpp) blit specializations Blit dispatches to (external
    // no-body, reloc-masked; stubbed in src/Stub). The trailing digits encode
    // dest_src bit depths: 248 = dest 24bpp / src 8bpp, etc.
    i32 Blit248(void* src, void* palette, i32 mode); // 0x13fe60 (ret 0xc)
    i32 Blit2416(void* src, i32 mode);               // 0x13ff80 (ret 8)
    i32 Blit1624(void* src, i32 mode);               // 0x13fce0 (ret 8)
    i32 Blit168(void* src, void* palette, i32 mode); // 0x13fbb0 (ret 0xc)
    i32 Blit824(void* src, void* palette, i32 mode); // 0x140110 (ret 0xc)
    i32 Blit816(void* src, void* palette, i32 mode); // 0x140420 (ret 0xc)

    // Layout. The OFFSETS are load-bearing. This IS the DIRSURF.CPP surface object
    // (the CDDSurface wrapper is the same physical struct) - the full surface layout
    // is modeled here directly rather than viewed through a separate wrapper class.
    // vptr @+0x00 (implicit, polymorphic; the compiler emits the dtor's stamp).
    char m_pad04[0x08 - 0x04]; // +0x04
    IDirectDrawSurfaceZ* m_8;  // +0x08  held DirectDraw surface (released via Release)
    IDirectDrawSurfaceZ* m_c;  // +0x0c  held back/secondary surface (also released)
    union {                    // +0x10  DDSURFACEDESC scratch (m_desc-relative accessors)
        char m_desc[0x24];
        struct {
            char m_descpad10[0x18 - 0x10];
            i32 m_height; // +0x18  surface height (compared vs decoded height)
            i32 m_width;  // +0x1c  surface width  (compared vs decoded width)
            i32 m_pitch;  // +0x20  row stride
        };
    };
    i32 m_34;                   // +0x34  desc lPitch (returned by Lock)
    char m_pad38[0x64 - 0x38];  // +0x38
    i32 m_64;                   // +0x64  pixel-format bit depth / colour-key colour
    char m_pad68[0x7c - 0x68];  // +0x68
    i32 m_7c;                   // +0x7c  don't-own flag (bit0 => surfaces not released)
    char m_pad80[0x94 - 0x80];  // +0x80
    CPtrArray m_elements;       // +0x94  owned element array (m_pData@0x98 / m_nSize@0x9c);
                                //        FreeSurfaces scalar-dtor-deletes each then RemoveAll
    i32 m_a8;                   // +0xa8  raw bit depth (8/16/24; the SaveDispatch selector)
    i32 m_ac;                   // +0xac
    i32 m_b0;                   // +0xb0
    i32 m_b4;                   // +0xb4
    i32 m_b8;                   // +0xb8  cleared by the surface teardown
    i32 m_bc;                   // +0xbc
    char m_padc0[0x538 - 0xc0]; // +0xc0
    i32 m_palBitCount;          // +0x538  bits per pixel of the palette context
    i32 m_palette[0x100];       // +0x53c  256-entry palette (ends at 0x93c)
    i32 m_hasPalette;           // +0x93c  have-palette flag
};

#endif // SRC_IMAGE_IMAGE_H
