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

#include "../Io/FileStream.h"

// ---------------------------------------------------------------------------
// CImage - the image-resolution dispatcher.
// ---------------------------------------------------------------------------
class CImage {
public:
    int LoadFromRez(char *name, void *a2, void *a3);

    // Five per-extension loaders (external; bodies live elsewhere).
    int LoadBmp(char *name, void *a2, void *a3);
    int LoadPcx(char *name, void *a2, void *a3);
    int LoadRid(char *name, void *a2, void *a3);
    int LoadPid(char *name, void *a2, void *a3);
    int LoadDefault(char *name, void *a2, void *a3);
};

// ---------------------------------------------------------------------------
// CFileImage - the file-backed format loaders.
// Internal helpers (all __thiscall on CFileImage) listed here to define the
// call signature; they are implemented in Image.cpp. The three Decode*
// methods remain external (their bodies live in another TU) so objdiff
// reloc-masks the call from LoadBmp/LoadPcx/LoadPid.
// ---------------------------------------------------------------------------
class CFileImage {
public:
    // Public loaders (matched).
    void *LoadBmp(char *name, char *path);             // @0x144110, ret 8
    void *LoadPcx(char *name, char *path);             // @0x145110, ret 8
    void *LoadPid(char *name, char *path, void *a3);   // @0x145cd0, ret 0xc

    // Per-format decoders (EXTERNAL - defined in a sibling TU).  Keep as
    // no-body declarations so the calls from LoadBmp/LoadPcx/LoadPid produce
    // external-symbol relocs matching the delinked target.
    void *DecodeBmp(char *name, void *buf, unsigned int size);
    void *DecodePcx(char *name, void *buf, unsigned int size);
    void *DecodePid(char *name, void *buf, unsigned int size, void *a3);

    // ---- Vtable / external helpers called from the internal decoders ----------
    // These are declared as CFileImage members (i.e. __thiscall with this in
    // ecx) but are DEFINED in other TUs (DDrawMgr, Wap32, etc.).  Keeping them
    // here ensures the compiler emits `mov ecx, this; call xxx` for each call.
    int FUN_0053e0d0(void *a1, int a2, int a3, int a4, void *a5);
    int FUN_0053ece0(void *a1);
    int FUN_0053eb40(void *a1);
    int FUN_0053faa0(void *a1, int a2, void *a3);
    int FUN_00540aa0(void *a1);
    int FUN_00540c50(void *a1);

    // ---- Internal helpers (the 17 FUN_ functions in the cluster) ----------
    // These are __thiscall on CFileImage.  Field offsets are accessed
    // via pointer arithmetic in Image.cpp (no struct fields in the header).

    // @0x543cf0, ret 0x10 (4 stack args + this).
    int CreateDibFromData(void *data, void *a2, int a3, int a4);

    // @0x543e60, ret 0xc (3 stack args + this, EH).
    void *LoadRaw(char *name, char *path, void *a3);

    // @0x543fc0, ret 0xc.
    int DecodeBmpSurface(void *data, int a2, int a3);

    // @0x544270, ret 0xc.
    int CreateDibSurface(int width, int height);

    // @0x544350, ret 0xc.
    int DecodeImage(char *name, void *buf, unsigned int size);

    // @0x5443b0, ret 0xc (EH).
    int DecodeImage8(char *name, void *buf, unsigned int size);

    // @0x544640, ret 0xc (EH).
    int DecodeImage16(char *name, void *buf, unsigned int size);

    // @0x544900, ret 0xc (EH).
    int DecodeImage24(char *name, void *buf, unsigned int size);

    // @0x544b30, ret 0x10.
    int DecodePidImage(void *data, int a2, int a3, int a4);

    // @0x544d80, ret 0xc.
    int DecodePidImage2(void *data, int a2, int a3);

    // @0x544ee0, ret 0xc.
    int DecodePcxImage(void *data, int a2, int a3);

    // @0x545270, ret 0x10.
    int RleDecompress8(unsigned char *dst, unsigned char *src,
                       int rowBytes, int height);

    // @0x5453f0, ret 0x10.
    int RleDecompress24(unsigned char *dst, unsigned char *src,
                        int rowBytes, int height);

    // @0x5457a0, ret 0x14.
    int DecodePcxData(void *data, int a2, int a3, int a4, int a5);

    // @0x5459d0, ret 0x10 (EH).
    int DecodePcxEx(char *name, char *path, void *a3, void *a4);

    // @0x545b10, ret 0x10.
    int DecodePidImageEx(void *data, int a2, int a3, int a4);
};

// IsPowerOfTwo is a freestanding helper (no ecx/this). @0x545e00.
int IsPowerOfTwo(unsigned int value);

#endif // SRC_IMAGE_IMAGE_H
