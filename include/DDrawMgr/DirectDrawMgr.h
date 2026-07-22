#ifndef GRUNTZ_CDIRECTDRAWMGR_H
#define GRUNTZ_CDIRECTDRAWMGR_H

#include <rva.h>
#include <DDrawMgr/DDrawPtrCollections.h> // the ONE device/pool class

#include <Mfc.h> // POSITION (CDDPalette::m_pos, the pool-B cached CPtrList handle) from the
#include <DDrawMgr/DDSurface.h>

struct IDirectDraw;        // <ddraw.h>: the raw device (m_dd1)
struct IDirectDraw2;       // <ddraw.h>: the QI'd device (m_device)
struct IDirectDrawPalette; // <ddraw.h>: the held palette

void __cdecl DDrawLogLine(char* fmt, ...);

struct CDdMode {
    char _0[8];
    u32 m_8; // +0x08  key part A (height)
    u32 m_c; // +0x0c  key part B (width)
    char _10[0x54 - 0x10];
    u32 m_54; // +0x54  mode tag / bpp (Compare's tie-break is unsigned)
};
SIZE_UNKNOWN();

struct CDdModePair {
    i32 a, b;
};
SIZE_UNKNOWN();

typedef CDDrawPtrCollections CDirectDrawMgr;

struct CDDPalette {     // struct (PAUCDDPalette mangling); consistent with the fwd decls
public:
    // Pool-item construction (the CDDrawPtrCollections MakeB*/Create factories
    // inline these): zero the fields; class operator new is the Rez heap 0x38 alloc.
    CDDPalette() {
        m_palette = 0;
        m_pos = 0;
        m_8 = 0;
        m_cacheA = 0;
        m_cacheB = 0;
        m_active = 0;
        m_sourcePalette = 0;
        m_targetPalette = 0;
        m_firstColorIndex = 0;
        m_colorCount = 0;
    }
    void* operator new(u32) {
        return ::operator new(0x38);
    }

    i32 LoadFromFile(IDirectDraw2* dd, char* filename, u32 flags); // 0x147410
    i32 Create(IDirectDraw2* dd, void* entries, u32 flags);        // 0x147390
    i32 LoadBmp(IDirectDraw2* dd, char* filename, u32 flags);      // 0x147590
    i32 LoadPcx(IDirectDraw2* dd, char* filename, u32 flags);      // 0x147710
    i32 CreateRGB(IDirectDraw2* dd, void* rgb, u32 flags);         // 0x1474d0
    i32 CreateFromTrailing(IDirectDraw2* dd, void* data, u32 size,
                           u32 flags);                            // 0x147840
    i32 LoadPal(IDirectDraw2* dd, char* filename, u32 flags);     // 0x1478c0
    i32 LoadDefault(IDirectDraw2* dd, char* filename, u32 flags); // 0x1479e0
    void Destroy();                                               // 0x147530
    i32 GetEntries();                                             // 0x147c30
    i32 SetAndNotify(i32 start, i32 count, i32* data, i32 a4);    // 0x147aa0
    // Expand a dynamically-allocated block of source entries into PALETTEENTRYs
    // then SetAndNotify. Quad: 4-byte RGBQUAD source (R/B swapped). RGB: packed
    // 3-byte RGB source (straight). Both return the SetAndNotify HRESULT.
    i32 SetEntriesQuad(i32 start, i32 count, u8* quads, i32 a4); // 0x147b10
    i32 SetEntriesRGB(i32 start, i32 count, u8* rgb, i32 a4);    // 0x147ba0
    // Linear time-based BLOCKING fade of the [start,start+count) range toward
    // the solid color (r,g,b) over durationMs ms; finalizes with SetRange.
    void FadeRange(i32 start, i32 count, i32 r, i32 g, i32 b,
                   i32 durationMs); // 0x147d50
    // The NON-blocking per-frame fade machinery (the ex-PalCtx/PaletteLerp
    // views, folded wave3-J): StartFade* arms the +0x14..+0x34 fade state, Tick
    // lerps m_cacheA from m_sourcePalette toward the target each frame, Flush
    // snaps to the final target and retires the fade.
    void StartFadeToColor(i32 start, i32 count, char r, char g, char b,
                          i32 durationMs); // 0x147f30
    void StartFadeToPalette(i32 start, i32 count, u8* target,
                            i32 durationMs); // 0x147ff0
    i32 Tick();                              // 0x1480a0
    void Flush();                            // 0x148250
    // Blend the range pct% (0..100) toward the solid color (r,g,b) once and
    // push it to the DirectDraw palette (no cache/notify).
    void BlendRange(i32 pct, i32 start, i32 count, i32 r, i32 g,
                    i32 b); // 0x1482c0
    void Apply(i32 a1);     // 0x147c80 (a1 unused)
    i32 SetRange(i32 start, i32 count, u8 r, u8 g, u8 b,
                 u32 flags);    // 0x147cd0
    i32 CaptureSystemPalette(); // 0x1485b0 (system-reserved entries -> m_cacheA)

    // --- layout ---------------------------------------------------------------
    POSITION m_pos;                // +0x00  pool-B cached CPtrList POSITION (AddItemB
                                   //        stamps it; RemoveItemB unlinks by it);
                                   //        cleared by Destroy
    IDirectDrawPalette* m_palette; // +0x04  the held palette interface
    i32 m_8;                       // +0x08  cleared by Destroy
    u8* m_cacheA;                  // +0x0c  PALETTEENTRY cache A (0x400 bytes; the live palette)
    u8* m_cacheB;                  // +0x10  PALETTEENTRY cache B (0x400 bytes; GetEntries readback)
    u8* m_targetPalette;           // +0x14  fade target entries (0 => fade to the fixed color)
    u8* m_sourcePalette; // +0x18  captured fade source (lazy 0x400 RezAlloc; Destroy frees)
    u8 m_fixedR;         // +0x1c  fixed fade target R
    u8 m_fixedG;         // +0x1d  fixed fade target G
    u8 m_fixedB;         // +0x1e  fixed fade target B
    char m_pad1f[1];
    i32 m_durationMs;      // +0x20  fade duration (ms)
    i32 m_startTimeMs;     // +0x24  fade start timestamp (timeGetTime)
    i32 m_lastElapsedMs;   // +0x28  last applied elapsed (-1 = none yet)
    i32 m_firstColorIndex; // +0x2c  fade first color index
    i32 m_colorCount;      // +0x30  fade color count
    i32 m_active;          // +0x34  fade active/pending flag (cleared by Destroy/Flush)
};
SIZE(0x38); // measured: the pool factories RezAlloc 0x38-byte items

struct DDModeInfo {
    i32 width;  // +0x00
    i32 height; // +0x04
    i32 bpp;    // +0x08
};
SIZE_UNKNOWN();

struct CPageRec {
    u8* m_00; // +0x00  owned heap buffer (RemoveAt frees)
    char m_pad04[0x10 - 4];
    u8* m_10; // +0x10  owned heap buffer
    u8* m_14; // +0x14  owned heap buffer
};
SIZE_UNKNOWN();

class CMoviePlayer;
typedef CMoviePlayer CDDPageMgr;
extern i32 RestoreLostSurfaces(); // 0x1437f0 (BoundaryUpper2.cpp)


// TU-local thunk/table names this TU registers (moved from the .cpp; the
// addresses are ILT thunk VAs, reloc-masked at every use).
extern "C" void DdEnumModesCallback(); // 0x143390

#endif // GRUNTZ_CDIRECTDRAWMGR_H
