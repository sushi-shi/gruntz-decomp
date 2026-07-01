// CScanlineSurfaceSave.cpp - CScanlineSurface::SaveBmp (0x176b30), the 8bpp software
// surface's "write me out as an 8-bit BMP" path. Split into its own /GX EH TU (the
// rest of CScanlineSurface.cpp is FPO `base`): a destructible stack CFile temp forces
// the exception frame (push -1 / handler / fs:0).
//
// Pass: build a BITMAPFILEHEADER (copied from the 14-byte template @0x61aabc, then the
// bfSize/bfOffBits slots patched) + a zeroed BITMAPINFOHEADER and 256-entry colour table
// (de-interleaved from the source palette object's RGBQUADs), open the file and Write the
// two headers then the scanlines bottom-up (m_pixels + m_scanlineOffsets[row], width bytes each).
//
// The CFile ctor/Open/Write/dtor are reloc-masked __thiscall engine bodies; the surface
// geometry fields are named (see the header), only offsets + emitted bytes are load-bearing.
#include <Mfc.h>

#include <Image/CScanlineSurface.h>
#include <rva.h>

// The 14-byte "BM" BITMAPFILEHEADER template copied up front ($SG .rdata).
extern char g_bmpHeaderTemplate[]; // 0x61aabc

// The stack CFile temp: ctor/dtor act on the object base; the engine writer's Open lives
// on the +0xc sub-object and Write on the +0x8 sub-object (the embedded stream bases), so
// the `lea ecx,[file+N]` adjustments fall out. All reloc-masked.
struct BmpFile {
    struct Stream {
        void Write(const void* buf, i32 len); // 0x1bf362
    };
    struct Opener {
        i32 Open(const char* path, i32 mode, i32 share); // 0x1bf200
    };
    BmpFile();  // 0x1befd7
    ~BmpFile(); // 0x1bf121
    char p0[0x8];
    Stream m_8; // +0x08
    char p9[0xc - 0x9];
    Opener m_c; // +0x0c
    char pad[0x440 - 0x10];
};

// CScanlineSurface::SaveBmp(filename, paletteObj) - 0x176b30, __thiscall (ret 8).
// @early-stop
// /GX-EH + stack-layout wall: the logic (header build, the RGBQUAD de-interleave, the
// bottom-up scanline Write loop) and the reloc-masked CFile calls are faithful, but the
// BMP header/info temporaries and the de-interleave's base-relative addressing land in
// different stack slots than retail, and the EH frame shifts the whole allocation. A
// complete, correct reconstruction parked on the documented EH/stack-slot wall. topic:wall.
RVA(0x00176b30, 0x1e5)
i32 CScanlineSurface::SaveBmp(const char* filename, void* paletteObj) {
    void* obj = paletteObj;
    if (obj == 0) {
        obj = m_paletteObj; // +0x458 default palette object
        if (obj == 0) {
            return 0;
        }
    }

    char fileHdr[0xe]; // BITMAPFILEHEADER  ([esp+0x10])
    char info[0x428];  // BITMAPINFOHEADER + 256-entry colour table ([esp+0x34])
    for (i32 z = 0; z < 0x428 / 4; z++) {
        *(i32*)(info + z * 4) = 0;
    }
    *(i32*)(info + 0x00) = 0x28;     // biSize
    *(i32*)(info + 0x04) = m_width;  // biWidth
    *(i32*)(info + 0x08) = m_height; // biHeight
    *(i16*)(info + 0x0c) = 1;        // biPlanes
    *(i16*)(info + 0x0e) = 8;        // biBitCount
    *(i32*)(info + 0x10) = 0;        // biCompression
    *(i32*)(info + 0x14) = 0;        // biSizeImage

    u8* pal = (u8*)obj + 8;
    if (pal == 0) {
        return 0;
    }
    // De-interleave the source RGBQUADs into the colour table (BMP BGR order).
    u8* ct = (u8*)info + 0x28;
    for (i32 i = 0x100; i != 0; i--) {
        ct[0] = pal[0];
        *(ct - 1) = pal[1];
        *(ct - 2) = pal[2];
        ct += 4;
        pal += 4;
    }

    // Copy the 14-byte file-header template, then patch bfSize / bfOffBits.
    for (i32 b = 0; b < 0xe; b++) {
        fileHdr[b] = g_bmpHeaderTemplate[b];
    }
    *(i32*)(fileHdr + 2) = m_width * m_height + 0x436; // bfSize
    *(i32*)(fileHdr + 0xa) = 0x436;                    // bfOffBits

    BmpFile file;
    if (file.m_c.Open(filename, 0x1001, 0) == 0) {
        return 0;
    }
    file.m_8.Write(fileHdr, 0xe);
    file.m_8.Write(info, 0x428);
    for (i32 row = m_height - 1; row >= 0; row--) {
        file.m_8.Write((char*)m_pixels + m_scanlineOffsets[row], m_width);
    }
    return 1;
}
