// DDScreen.h - THE canonical shape of CDDScreen, the big tiled-DirectDraw display
// manager (the CDDPageMgr bring-up class behind 0x17c040; members run out to
// ~0x86a4). Unifies the three formerly per-TU views (DDScreen.cpp's full class +
// PaletteReset.cpp's IPalSink/partial + PaletteCopy.cpp's partial).
//
// LAYOUT-ONLY header: the DirectDraw COM interfaces (IDirectDraw/IDirectDraw2/
// IDirectDrawSurface/IDirectDrawPalette) and the POINT/RECT descriptor structs come
// from the real SDK, so an includer must pull <Win32.h> (windows.h) then <ddraw.h>
// BEFORE this header (matching the established include order of the owning .cpp).
//
// Field names recovered from usage; offsets + emitted bytes are load-bearing. A few
// fields only touched once (m_20/m_28/m_50c/m_514/m_86a0) keep their offset names -
// role unproven from the matched methods.
#ifndef GRUNTZ_DDRAWMGR_DDSCREEN_H
#define GRUNTZ_DDRAWMGR_DDSCREEN_H

#include <Ints.h>
#include <rva.h>

// The tile/mode descriptor m_tileInfo (+0x10) points at: tile width/height at
// +0x4/+0x8 (read by Configure); the frame's 3-byte-per-entry RGB palette source
// sits at +0x6c (read by UploadPalette off the same pointer). unsigned width/height
// so the validation/divide lower to `ja`/`div`.
struct CTileInfo {
    i32 m_0;      // +0x0
    u32 m_width;  // +0x4  tile width
    u32 m_height; // +0x8  tile height
};

class CDDScreen {
public:
    void HandleError();       // 0x17cc80
    void ResetPalette();      // 0x17ca60 (body in PaletteReset.cpp; clears the +0x108 table)
    void Snapshot(HWND hWnd); // 0x17cd90 (PaletteSnapshot.cpp; system palette -> +0x108)
    i32 BlitRegion(i32 col, i32 row, i32 nCols, i32 nRows);        // 0x17cdf0
    i32 Configure(i32 mode, i32 flags, POINT* origin, RECT* rect); // 0x17cfc0
    i32 CheckGrid();                                               // 0x17cbe0 (sibling, external)
    void UploadPalette(); // 0x17ca10 (palette re-realize on 8bpp restore; body in PaletteCopy.cpp)
    // 0x17c3f0 (body in DDPageMgr.cpp): the borrowed-interface mode bring-up - snapshot
    // the system palette, create+attach the 8bpp palette (or validate 16bpp / reject
    // 24bpp), latch geometry+bpp and hide the cursor. Only p-args 4..30 (minus the
    // consumed ones) are dead pass-throughs the retail caller pushes.
    i32 InitMode(
        HWND wnd,
        IDirectDraw2* dd2,
        IDirectDrawSurface* primary,
        i32 p4,
        i32 p5,
        i32 height,
        i32 width,
        i32 p8,
        i32 p9,
        i32 p10,
        i32 p11,
        i32 p12,
        i32 p13,
        i32 p14,
        i32 p15,
        i32 p16,
        i32 p17,
        i32 p18,
        i32 p19,
        i32 p20,
        i32 p21,
        i32 p22,
        i32 p23,
        i32 p24,
        i32 bpp,
        i32 p26,
        i32 p27,
        i32 p28,
        i32 p29,
        i32 p30,
        i32 a31
    );

    HWND m_window;      // +0x00  owner window (InitMode stores it)
    i32 m_initialized;  // +0x04  "initialized" flag (InitMode sets 1; cf. CDDPageMgr +0x04)
    void* m_8;          // +0x08  cleared by InitMode (role unproven)
    i32 m_0c; // +0x0c   ==0 gates full DDraw-stack teardown (owns-vs-borrows, unproven)
    CTileInfo* m_tileInfo;         // +0x10
    IDirectDraw2* m_dd2;           // +0x14   IDirectDraw2
    IDirectDraw* m_dd;             // +0x18   IDirectDraw
    IDirectDrawSurface* m_primary; // +0x1c   primary surface
    IDirectDrawSurface* m_20;      // +0x20   surface (only Release'd; role unproven)
    IDirectDrawSurface* m_srcSurf; // +0x24   blit source surface
    IDirectDrawSurface* m_28;      // +0x28   surface (only Release'd; role unproven)
    IDirectDrawPalette* m_palette; // +0x2c
    char m_pad30[0x108 - 0x30];
    // +0x108  256 * 4-byte PALETTEENTRY slots (4th byte kept). Two views: ResetPalette/
    // UploadPalette walk it byte-wise (m_colorSlots); Snapshot fills it as a real
    // PALETTEENTRY[256] from GetSystemPaletteEntries (m_palEntries).
    union {
        u8 m_colorSlots[0x400];
        PALETTEENTRY m_palEntries[0x100];
    };
    i32 m_508; // +0x508   InitMode stores its a31 pass-through scalar (role unproven)
    i32 m_50c; // +0x50c   reset to 0 by Configure
    i32 m_510; // +0x510   cleared by InitMode after the 8bpp palette attach (role unproven)
    i32 m_514;            // +0x514  set in mode-2 fallback (unproven)
    u32 m_screenWidth;    // +0x518
    u32 m_screenHeight;   // +0x51c
    i32 m_bpp;            // +0x520
    i32 m_tilesAcross;    // +0x524
    i32 m_tilesDown;      // +0x528
    i32 m_originX;        // +0x52c
    i32 m_originY;        // +0x530
    RECT* m_destRect;     // +0x534  explicit dest rect (or 0)
    i32 m_forceSingleRow; // +0x538
    char m_pad53c[0x86a0 - 0x53c];
    i32 m_86a0; // +0x86a0   reset to 0 by Configure
};
SIZE_UNKNOWN(CDDScreen);
SIZE_UNKNOWN(CTileInfo);

#endif // GRUNTZ_DDRAWMGR_DDSCREEN_H
