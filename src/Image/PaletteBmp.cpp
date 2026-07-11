// PaletteBmp.cpp - the two tail methods of the CImagePaletteNode palette-load obj
// (retail obj [0x177480..0x177652], the run right after the ImagePool.cpp obj ends
// at 0x177476). Both are really CImagePaletteNode methods (ImagePool.cpp declares
// them declared-only: LoadBmpPalette IS CImagePaletteNode::LoadBmpFile @0x177480 and
// Apply IS CImagePaletteNode::Apply @0x1775f0); the local CPalLoader stands in for
// CImagePaletteNode (whose canonical view lives in ImagePool.cpp's ApiCallerStubs
// namespace - dissolving CPalLoader onto it needs a shared header, deferred).
//
//   LoadBmpPalette (0x177480, __thiscall ret 8): open a BMP file through an engine
//     binary reader (a destructible stack object -> /GX EH frame), read the 14-byte
//     BITMAPFILEHEADER + 40-byte BITMAPINFOHEADER + 1024-byte (256*4) palette, swap
//     each entry from the file's BGRx order to RGB0, then hand the converted table
//     to the owner's ApplyPalette.
//   Apply (0x1775f0, __thiscall ret 8): find/load/lock a PALETTE resource from the
//     app resource module, then hand it to ProcessPal (0x176e70) - re-homed from the
//     ResourceLoaders.cpp holding TU (was ResLoaders::PalHost_1775f0::Apply).
//
// The reader's ctor/dtor/Open/Read and ProcessPal are reloc-masked engine fns.
// Field names are placeholders; offsets + code bytes are the load-bearing fact.
#include <Win32.h> // FindResourceA / LoadResource / LockResource + HINSTANCE
#include <Ints.h>
#include <rva.h>

// The PALETTE-resource source module (DAT_006bf6e0; the DATA(0x2bf6e0) binding is
// canonical in ImagePool.cpp as g_hResModule - extern-only pin here, reloc-masked).
extern "C" HINSTANCE g_hResModule; // 0x6bf6e0

// The engine binary file reader (destructible stack local -> the /GX frame).
struct PalFile {
    PalFile();                                     // 0x1befd7
    ~PalFile();                                    // 0x1bf121
    i32 Open(const char* name, i32 mode, i32 err); // 0x1bf200
    i32 Read(void* buf, i32 len);                  // 0x1bf328
};

struct CPalLoader {
    i32 LoadBmpPalette(const char* name, i32 unused); // 0x177480
    i32 ApplyPalette(void* pal, i32 a);               // 0x176df0
    i32 Apply(const char* name, i32 arg);             // 0x1775f0
    i32 ProcessPal(void* data, i32 arg);              // 0x176e70 (= CImagePaletteNode::ProcessPal)
};

// @early-stop
// /GX CFile-frame plateau: the BMP read + BGR->RGB swap + ApplyPalette hand-off
// are faithful; the exact stack-buffer offset assignment under the EH frame is
// not source-steerable.
RVA(0x00177480, 0x169)
i32 CPalLoader::LoadBmpPalette(const char* name, i32 unused) {
    PalFile f;
    if (f.Open(name, 0, 0) == 0) {
        return 0;
    }

    char fileHdr[14];
    if (f.Read(fileHdr, 0xe) == 0) {
        return 0;
    }
    char infoHdr[40];
    if (f.Read(infoHdr, 0x28) == 0) {
        return 0;
    }
    u8 raw[0x400];
    if (f.Read(raw, 0x400) == 0) {
        return 0;
    }

    u8 out[0x400];
    for (i32 i = 0; i < 0x400; i += 4) {
        out[i + 0] = raw[i + 2];
        out[i + 1] = raw[i + 1];
        out[i + 2] = raw[i + 0];
        out[i + 3] = 0;
    }
    return ApplyPalette(out, 0);
}

// __thiscall(name, arg): find/load/lock a PALETTE resource from the app resource
// module, then hand the locked data on to ProcessPal (0x176e70).
RVA(0x001775f0, 0x62)
i32 CPalLoader::Apply(const char* name, i32 arg) {
    HINSTANCE mod = g_hResModule;
    if (!mod) {
        return 0;
    }
    HRSRC hRsrc = FindResourceA(mod, name, "PALETTE");
    if (!hRsrc) {
        return 0;
    }
    HGLOBAL hRes = LoadResource(mod, hRsrc);
    if (!hRes) {
        return 0;
    }
    void* data = LockResource(hRes);
    if (!data) {
        return 0;
    }
    return ProcessPal(data, arg);
}
SIZE_UNKNOWN(CPalLoader);
SIZE_UNKNOWN(PalFile);
