// CFileImage.h - the file-backed surface image in the DDrawMgr image cluster
// (0x13e6d0..0x1453f0). A surface-backed image element (vtable slot +0x14 =
// IsValid, slot +0x0c = a decode helper, slot +0x80 on its held surface = Unlock)
// that can save its locked pixels to a BMP/TGA file (the two export methods) and
// load+decode a run-length image back (the LoadFile + Decode pair). Placeholder
// RTTI name (no class descriptor in the binary); only the OFFSETS + emitted code
// bytes are load-bearing (campaign doctrine).
//
// Layout (the offsets these methods touch):
//   +0x00  vtable           (slot +0x14 IsValid; the object is polymorphic)
//   +0x08  m_surface        the held DDraw surface (Unlock is its vtable slot +0x80)
//   +0x18  m_height         rows
//   +0x1c  m_width          columns
//   +0x20  m_pitch          row stride (bytes)
//   +0xa8  m_bpp            bit depth (8 = palettized, 0x18 = 24-bit BGR)
//   +0x538 m_fmt            current decode format (8 / 0x18)
//   +0x53c m_palette        inline 0x300-byte (256*3) palette
//   +0x93c m_hasPalette     non-zero when m_palette holds a valid palette
#ifndef SRC_IMAGE_CFILEIMAGE_H
#define SRC_IMAGE_CFILEIMAGE_H

#include <Mfc.h> // CFile (the export/load methods slurp through the real MFC CFile)

#include <Ints.h>
#include <rva.h>

// The held DDraw surface at +0x08: its vtable slot +0x80 is Unlock(rect). The
// surface's own vtable contents are external engine code; only the slot offset
// matters here. Modeled as a tiny polymorphic view (vptr @+0) so the dispatch
// lowers to `mov eax,[surface]; call [eax+0x80]`.
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

// The decode-source descriptor passed to the export/decode methods: a run-length
// image header with a +0x04/+0x06/+0x08/+0x0a int16 bounding box, a +0x41 format
// byte (1 = 8-bit, 3 = 24-bit), and the encoded run data at +0x80.
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

// The palette source the BMP export reads (arg2): its +0x0c is the 256-entry source
// palette (4 bytes per entry; the export copies bytes 0/1/2 into the BMP RGBQUADs).
class CFileImagePal {
public:
    char _00[0x0c];
    u8* m_0c; // +0x0c  source palette (4 bytes/entry)
};

// The palette/format config Decode resolves against (LoadFile's `src` arg): a
// current-format word at +0x538, an inline 0x300-byte palette at +0x53c (only used
// when the source format differs), and a has-palette flag at +0x93c.
class CFileImageInfo {
public:
    char _00[0x538];
    i32 m_538;       // +0x538  current decode format (8 / 0x18)
    u8 m_53c[0x300]; // +0x53c  inline 256*3 palette
    char _83c[0x93c - 0x83c];
    i32 m_93c; // +0x93c  has-palette flag
};

// The 16-byte rect/clip record DecodeThunk builds on the stack and passes by value to
// the inner blit/decode worker (0x1471d0).
struct ClipRect16 {
    i32 a, b, c, d;
};

class CFileImage {
public:
    // slot +0x14 (IsValid) and slot +0x0c (BeginDecode: lock + prep) of this object's
    // vtable, and Lock (0x13e6d0) which returns the locked pixel buffer; all external
    // in this TU (their call sites reloc-mask).
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual i32 BeginDecode(CFileImageInfo* info, i32 w, i32 h, i32 z, i32 mode); // +0x0c
    virtual void v10();
    virtual i32 IsValid(); // +0x14

    // Lock the held surface (returns the locked pixel buffer; 0 on failure). The
    // cluster's own 0x13e6d0; reloc-masked __thiscall.
    void* Lock(void* rect); // 0x13e6d0

    // The four run-length decoders Decode dispatches into (all __thiscall methods on
    // this image; reloc-masked). 8/24-bit straight decode (0x140aa0/0x140c50) and the
    // format-convert variants (0x145270/0x1453f0).
    i32 DecodeRun8(void* run);                            // 0x140aa0
    i32 DecodeRun24(void* run);                           // 0x140c50
    i32 RunDecode1(void* dst, void* run, i32 w, i32 h);   // 0x145270
    i32 RunDecode3(void* dst, void* run, i32 w, i32 h);   // 0x1453f0
    i32 Blit(void* run, i32 fmt, void* pal, i32 one);     // 0x13faa0
    i32 BlitSurf(void* info, i32 a, i32 b, i32 c, i32 d); // 0x13e0d0
    i32 BlitDirect(void* run, i32 mode);                  // 0x13ece0

    // A second decode/load pair (alongside Decode/LoadFile): DecodeRun reads a run-length
    // image described by `src` (its own descriptor shape - format word @+0x1c, dims
    // @+0x12/+0x16, run-data offset @+0x0a) into `info`; LoadFile2 slurps a file (engine
    // CFileIO) then calls DecodeRun.
    i32 DecodeRun(CFileImageInfo* info, void* src, i32 a, i32 b);    // 0x143cf0
    i32 LoadFile2(CFileImageInfo* info, const char* path, i32 mode); // 0x143e60

    // The six methods reconstructed in this TU (retail-RVA order):
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
    ); // 0x141280
    // The inner blit/decode worker DecodeThunk tail-calls (same __thiscall `this`;
    // external no-body/reloc-masked - the body lives in another TU).
    i32 Run(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, ClipRect16 clip); // 0x1471d0
    i32 SaveBmp(const char* path, void* pal, i32 mode);                       // 0x1443b0
    i32 SaveTga(const char* path, void* pal, i32 mode);                       // 0x144900
    i32 Decode(CFileImageInfo* info, CFileImageSrc* src, i32 len, i32 mode);  // 0x144b30
    i32 LoadFile(CFileImageInfo* info, const char* path, i32 mode);           // 0x144d80

    // vptr @+0x00
    char _04[0x08 - 0x04];
    CFileImageHeldSurface* m_surface; // +0x08
    char _0c[0x18 - 0x0c];
    i32 m_height; // +0x18
    i32 m_width;  // +0x1c
    i32 m_pitch;  // +0x20
    char _24[0xa8 - 0x24];
    i32 m_bpp; // +0xa8
    char _ac[0x538 - 0xac];
    i32 m_fmt;           // +0x538
    u8 m_palette[0x300]; // +0x53c
    char _83c[0x93c - 0x83c];
    i32 m_hasPalette; // +0x93c
};

#endif // SRC_IMAGE_CFILEIMAGE_H
