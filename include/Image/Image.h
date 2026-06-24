// Image.h - the engine's image-resolution surface (the REZ -> image load path).
//
// Two classes live here, both reconstructed only as deeply as needed to
// byte-match their leaf methods. Fields are named from their use across the
// matched methods; the OFFSETS + code bytes stay load-bearing. A few write-only
// fields with no proven role keep their m_<hexoffset> placeholder.
//
//   class CImage  - the extension DISPATCHER class plus its five per-extension
//     loaders. LoadFromRez(name,a2,a3) does `ext = strrchr(name,'.')` then a
//     stricmp ladder on ".BMP"/".PCX"/".RID"/".PID" and hands off (all args
//     forwarded) to one of five sibling __thiscall loaders (LoadBmp/LoadPcx/
//     LoadRid/LoadPid/LoadDefault), each `ret 0xc`. The loaders are the real
//     file/resource consumers: LoadBmp opens a CFileIO, parses the BMP file +
//     info headers, builds the CImage via a decode helper and reads the pixel
//     bytes; LoadPcx/Rid/Pid slurp the whole file into an `operator new` buffer
//     and run a per-format decode helper; LoadDefault loads a Win32 RT_BITMAP
//     resource and decodes it. The per-format decode helpers are also CImage
//     __thiscall methods, reconstructed in Image.cpp; they share the plane
//     allocator DecodeBmpHeader and the blitter DecodeBlit (external/no-body).
//
//   class CFileImage - the file-backed BMP/PCX/PID loaders that actually open a
//     file via CFileIO, slurp its bytes into an `operator new` buffer and hand
//     them to a per-format decode helper. Matched here: LoadBmp/LoadPcx (ret 8)
//     and LoadPid (ret 0xc). The CFileIO stack object forces a C++ EH frame -> /GX.
#ifndef SRC_IMAGE_IMAGE_H
#define SRC_IMAGE_IMAGE_H

#include <Ints.h>

#include <Io/FileStream.h>

// ---------------------------------------------------------------------------
// CImage - the image-resolution dispatcher.
// LoadFromRez is __thiscall, ret 0xc (this + name + two opaque pass-through
// args). It forwards (name, a2, a3) verbatim to the matching format loader.
// The five sibling loaders and the per-format decoders are reconstructed in
// Image.cpp; only the shared blitter DecodeBlit stays external/no-body.
// ---------------------------------------------------------------------------
class CImage {
public:
    i32 LoadFromRez(char* name, void* a2, void* a3);

    // The five per-extension loaders (bodies in Image.cpp). All __thiscall,
    // ret 0xc, taking the same (name, a2, a3) triple.
    i32 LoadBmp(char* name, void* a2, void* a3);
    i32 LoadPcx(char* name, void* a2, void* a3);
    i32 LoadRid(char* name, void* a2, void* a3);
    i32 LoadPid(char* name, void* a2, void* a3);
    i32 LoadDefault(char* name, void* a2, void* a3);

    // Per-format decode helpers (bodies in Image.cpp). All __thiscall on CImage,
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

    // The shared plane blitter (FUN_00575930): a __thiscall CImage method that
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
class CFileImage {
public:
    void* LoadBmp(char* name, char* path);
    void* LoadPcx(char* name, char* path);
    void* LoadPid(char* name, char* path, void* a3);
    i32 DecodePcxEx(char* name, char* path, void* a3, void* a4);

    // Format dispatchers (reconstructed in Image.cpp). __thiscall on CFileImage.
    // Resolve picks the BMP/PCX/PID decoder by `type` (1/2/4) for the file path;
    // ResolveEx is the surface-blit variant that ORs the control word with 0x40,
    // runs the *Data decoders and installs the transparency colour after.
    i32 Resolve(void* surf, void* buf, i32 type, u32 size, void* surf2);
    i32 ResolveEx(void* surf, void* buf, i32 type, u32 size, i32 ctrl, i32 trans);
    i32 Fill(u32 color); // colour-fill blt (0x13e760)
    ~CFileImage();       // 0x141350

    // The shared surface-teardown helper the destructor calls before destroying
    // its CByteArray member (external no-body, reloc-masked): releases the held
    // DirectDraw surfaces (m_8/m_c), empties the +0x94 CByteArray, and walks the
    // +0x98/+0x9c object array calling each element's slot-0 destructor.
    void FreeSurfaces(); // 0x13e4d0

    // Per-format decoders (reconstructed in Image.cpp). __thiscall on CFileImage.
    void* DecodeBmp(char* surf, void* buf, u32 size);
    void* DecodePcx(char* surf, void* buf, u32 size);
    void* DecodePid(char* surf, void* buf, u32 size, void* surf2);
    i32 DecodePcxData(void* surf, void* buf, i32 size, i32 a4, i32 a5);

    // The surface-blit decoder variants ResolveEx dispatches to (external no-body,
    // reloc-masked; ret 0x10 = 4 args). DecodeBmpData/DecodePcxData2 take
    // (surf, buf, size, ctrl).
    i32 DecodeBmpData(void* surf, void* buf, u32 size, i32 ctrl);  // 0x143cf0
    i32 DecodePcxData2(void* surf, void* buf, u32 size, i32 ctrl); // 0x144b30

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

    // Layout. The OFFSETS are load-bearing. This is the same physical object the
    // CDDSurface wrapper holds (DIRSURF.CPP); the decoders touch only the geometry
    // (height/width) and the palette context (bitcount/palette/have-palette flag).
    char m_pad00[0x18];         // +0x00  (vptr @0, manually stamped by ~CFileImage)
    i32 m_height;               // +0x18  surface height (compared vs decoded height)
    i32 m_width;                // +0x1c  surface width  (compared vs decoded width)
    char m_pad20[0x94 - 0x20];  // +0x20
    CByteArray m_byteBuffer;    // +0x94  owned byte buffer (destroyed by ~CFileImage)
    char m_pada8[0x538 - 0xa8]; // +0xa8
    i32 m_palBitCount;          // +0x538  bits per pixel of the palette context
    i32 m_palette[0x100];       // +0x53c  256-entry palette (ends at 0x93c)
    i32 m_hasPalette;           // +0x93c  have-palette flag
};

#endif // SRC_IMAGE_IMAGE_H
