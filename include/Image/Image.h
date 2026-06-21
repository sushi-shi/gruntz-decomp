// Image.h - the engine's image-resolution surface (the REZ -> image load path).
//
// Two classes live here, both reconstructed only as deeply as needed to
// byte-match their leaf methods. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS + code bytes are load-bearing (campaign doctrine).
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
    int LoadFromRez(char* name, void* a2, void* a3);

    // The five per-extension loaders (bodies in Image.cpp). All __thiscall,
    // ret 0xc, taking the same (name, a2, a3) triple.
    int LoadBmp(char* name, void* a2, void* a3);
    int LoadPcx(char* name, void* a2, void* a3);
    int LoadRid(char* name, void* a2, void* a3);
    int LoadPid(char* name, void* a2, void* a3);
    int LoadDefault(char* name, void* a2, void* a3);

    // Per-format decode helpers (bodies in Image.cpp). All __thiscall on CImage,
    // invoked by the loaders above with the decoded header fields / raw file
    // buffer / resource pointer. Names are placeholders (the FUN_* labels carry
    // no real name). DecodeBmpHeader is the allocator/setup: it fills the
    // BITMAPINFOHEADER at this+0, CreateDIBSections the plane (HBITMAP @+0x428,
    // bits @+0x42c) and operator-new's the per-row offset table @+0x430. The
    // Pcx/Rid/Pid/Res decoders call it, then blit the decoded pixels.
    int DecodeBmpHeader(void* a2, int width, int height, int bitcount, void* a3);
    int DecodePcxData(void* buf, void* a2, void* a3);
    int DecodeRidData(void* buf, void* a2, void* a3);
    int DecodePidData(void* buf, void* a2, void* a3);
    int DecodeResData(void* buf, void* a2, void* a3);

    // The shared plane blitter (FUN_00575930): a __thiscall CImage method that
    // (re)allocates/decodes via DecodeBmpHeader then copies `src` into the plane
    // (flat rep-movs when m_448==0, else row-by-row through the +0x430 table).
    // External/no-body so its call reloc-masks. ret 0x18 = 6 stack args.
    int DecodeBlit(void* src, void* a2, int width, int height, int bitcount, void* a3);

    // Layout. Field names are placeholders; the OFFSETS are load-bearing. The
    // object opens with a BITMAPINFOHEADER (this+0, biSize..biClrImportant) and a
    // 256-entry WORD color table (this+0x28). DecodeBmpHeader fills these.
    BITMAPINFOHEADER m_bih;       // +0x000  biSize/biWidth/biHeight/... (filled by setup)
    unsigned short m_pal[256];    // +0x28  DIB_PAL_COLORS index table
    char m_pad228[0x428 - 0x228]; // +0x228
    void* m_428;                  // +0x428  HBITMAP from CreateDIBSection (the DIB section)
    void* m_42c;                  // +0x42c  decoded pixel buffer (CreateDIBSection's bits)
    int* m_430;                   // +0x430  per-row byte-offset table (operator new'd)
    int m_434;                    // +0x434  0
    int m_438;                    // +0x438  source width param (bytes/row of the source)
    int m_43c;                    // +0x43c  abs(height): number of rows
    int m_440;                    // +0x440  bit count
    int m_444;                    // +0x444  aligned destination row stride (bytes per row)
    int m_448;                    // +0x448  destination padding = m_444 - m_438
    char m_pad44c[0x4];           // +0x44c
    int m_450;                    // +0x450  flag (1 = transparent/RLE plane)
    int m_454;                    // +0x454  0
    int m_458;                    // +0x458  0
};

// ---------------------------------------------------------------------------
// CFileImage - the file-backed format loaders (the REZ payload consumers).
// Each constructs a stack CFileIO, opens the file named by its second arg,
// reads GetLength() bytes into an `operator new` buffer, then calls a per-format
// decode helper on `this` and returns the decoder's result; the buffer is freed
// and the stream closed on every exit. The decode helpers are external/no-body.
// ---------------------------------------------------------------------------
class CFileImage {
public:
    void* LoadBmp(char* name, char* path);
    void* LoadPcx(char* name, char* path);
    void* LoadPid(char* name, char* path, void* a3);
    int DecodePcxEx(char* name, char* path, void* a3, void* a4);

    // Per-format decoders (external; reloc-masked). __thiscall on CFileImage.
    void* DecodeBmp(char* name, void* buf, unsigned int size);
    void* DecodePcx(char* name, void* buf, unsigned int size);
    void* DecodePid(char* name, void* buf, unsigned int size, void* a3);
    int DecodePcxData(char* name, void* buf, unsigned int len, void* a3, void* a4); // external
};

#endif // SRC_IMAGE_IMAGE_H
