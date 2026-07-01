// PaletteBmp.cpp - load a 256-color palette from a BMP file (0x177480).
// __thiscall, ret 8. Opens the file through an engine binary reader (a
// destructible stack object -> /GX EH frame), reads the 14-byte BITMAPFILEHEADER
// + 40-byte BITMAPINFOHEADER + 1024-byte (256*4) palette, swaps each entry from
// the file's BGRx order to RGB0, then hands the converted table to the owner's
// ApplyPalette. The reader's ctor/dtor/Open/Read are reloc-masked engine fns.
// Field names are placeholders; offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <rva.h>

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

// ---------------------------------------------------------------------------
// Class metadata (SIZE sweep) - hosted at TU EOF; labels.py scans tree-wide.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CPalLoader);
SIZE_UNKNOWN(PalFile);
