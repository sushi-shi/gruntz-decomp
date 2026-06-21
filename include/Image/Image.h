// Image.h - the engine's image-resolution surface (the REZ -> image load path).
//
// Two classes live here, both reconstructed only as deeply as needed to
// byte-match their leaf methods. Field names are placeholders (m_<hexoffset>);
// only the OFFSETS + code bytes are load-bearing (campaign doctrine).
//
//   class CImage  - the extension DISPATCHER class. Its LoadFromRez(name,a2,a3)
//     does `ext = strrchr(name,'.')` then a stricmp ladder on
//     ".BMP"/".PCX"/".RID"/".PID" and hands off (all args forwarded) to one of
//     five sibling __thiscall loader methods (LoadBmp/LoadPcx/LoadRid/LoadPid/
//     LoadDefault), each `ret 0xc`. The five siblings are declared external
//     (no body) so their `call rel32` displacements are reloc-masked in objdiff.
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
    int LoadFromRez(char *name, void *a2, void *a3);

    // The five per-extension loaders (external; bodies live elsewhere). All
    // __thiscall, ret 0xc, taking the same (name, a2, a3) triple.
    int LoadBmp(char *name, void *a2, void *a3);
    int LoadPcx(char *name, void *a2, void *a3);
    int LoadRid(char *name, void *a2, void *a3);
    int LoadPid(char *name, void *a2, void *a3);
    int LoadDefault(char *name, void *a2, void *a3);
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
    void *LoadBmp(char *name, char *path);
    void *LoadPcx(char *name, char *path);
    void *LoadPid(char *name, char *path, void *a3);
    int   DecodePcxEx(char *name, char *path, void *a3, void *a4);

    // Per-format decoders (external; reloc-masked). __thiscall on CFileImage.
    void *DecodeBmp(char *name, void *buf, unsigned int size);
    void *DecodePcx(char *name, void *buf, unsigned int size);
    void *DecodePid(char *name, void *buf, unsigned int size, void *a3);
    int   DecodePcxData(char *name, void *buf, unsigned int len, void *a3, void *a4);  // external
};

#endif // SRC_IMAGE_IMAGE_H
