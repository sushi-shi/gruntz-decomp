// PaletteBmp.cpp - the two BMP/PALETTE loader tail methods of CImagePaletteNode
// (retail obj [0x177480..0x177652], the run right after the ImagePool.cpp obj ends
// at 0x177476). Both are ApiCallerStubs::CImagePaletteNode methods (declared in the
// shared Image/ImagePaletteNode.h, whose pool-owned builders/front-ends live in
// ImagePool.cpp); their bodies were formerly mis-named onto a per-TU CPalLoader view,
// now deleted so LoadBmpFile->Build (0x176df0) and Apply->ProcessPal (0x176e70) bind
// to the real pool bodies instead of reloc-masking through a fake view.
//
//   LoadBmpFile (0x177480, __thiscall ret 8): open a BMP file through the engine
//     CFileIO reader (a destructible stack object -> /GX EH frame), read the 14-byte
//     BITMAPFILEHEADER + 40-byte BITMAPINFOHEADER + 1024-byte (256*4) palette, swap
//     each entry from the file's BGRx order to RGB0, then hand the converted table to
//     the node's Build (0x176df0).
//   Apply (0x1775f0, __thiscall ret 8): find/load/lock a PALETTE resource from the
//     app resource module, then hand it to ProcessPal (0x176e70).
//
// Field names are placeholders; offsets + code bytes are the load-bearing fact.
#include <Image/ImagePaletteNode.h> // ApiCallerStubs::CImagePaletteNode (Build/ProcessPal)
#include <Io/FileStream.h>          // CFileIO - the engine KERNEL32 file reader (0x1befd7..)
#include <Ints.h>
#include <rva.h>

// The PALETTE-resource source module (DAT_006bf6e0; the DATA(0x2bf6e0) binding is
// canonical in ImagePool.cpp as g_hResModule - extern-only pin here, reloc-masked).
extern "C" HINSTANCE g_hResModule; // 0x6bf6e0

namespace ApiCallerStubs {

    // @early-stop
    // /GX CFile-frame plateau: the BMP read + BGR->RGB swap + Build hand-off are
    // faithful; the exact stack-buffer offset assignment under the EH frame is not
    // source-steerable.
    RVA(0x00177480, 0x169)
    i32 CImagePaletteNode::LoadBmpFile(char* path, i32 arg) {
        CFileIO f;
        if (f.Open(path, 0, 0) == 0) {
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
        return Build((PALETTEENTRY*)out, 0);
    }

    // __thiscall(path, arg): find/load/lock a PALETTE resource from the app resource
    // module, then hand the locked data on to ProcessPal (0x176e70).
    RVA(0x001775f0, 0x62)
    i32 CImagePaletteNode::Apply(char* path, i32 arg) {
        HINSTANCE mod = g_hResModule;
        if (!mod) {
            return 0;
        }
        HRSRC hRsrc = FindResourceA(mod, path, "PALETTE");
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

} // namespace ApiCallerStubs
