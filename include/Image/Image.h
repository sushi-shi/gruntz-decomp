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
//     resource and decodes it. The per-format decode helpers are themselves
//     CImage __thiscall methods, declared external/no-body so their calls
//     reloc-mask.
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
// The five sibling loaders are external/no-body so their calls reloc-mask.
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

    // Per-format decode helpers (external/no-body; reloc-masked). All __thiscall
    // on CImage, invoked by the loaders above with the decoded header fields /
    // raw file buffer / resource pointer. The signatures mirror the loaders'
    // push order; names are placeholders (the FUN_* labels carry no real name).
    int DecodeBmpHeader(void* a2, int width, int height, int bitcount, void* a3);
    int DecodePcxData(void* buf, void* a2, void* a3);
    int DecodeRidData(void* buf, void* a2, void* a3);
    int DecodePidData(void* buf, void* a2, void* a3);
    int DecodeResData(void* buf, void* a2, void* a3);

    // Layout (only the fields LoadBmp touches are named; the decode helpers fill
    // the rest and are external). Field names are placeholders; the OFFSETS are
    // load-bearing. m_42c is the decoded pixel plane buffer the decode helper
    // allocates; m_444 is the aligned row stride (bytes per row).
    char m_pad0[0x42c]; // +0x000
    void* m_42c;        // +0x42c  decoded pixel buffer (operator new'd by helper)
    char m_pad430[0x14];// +0x430
    int m_444;          // +0x444  aligned row stride
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
