#ifndef GRUNTZ_CDIRECTDRAWMGR_H
#define GRUNTZ_CDIRECTDRAWMGR_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDSurface.h>

struct IDirectDraw;
struct IDirectDraw2;
struct IDirectDrawPalette;

void __cdecl DDrawLogLine(char* fmt, ...);

struct CDdModePair {
    i32 a, b;
};
SIZE_UNKNOWN();

struct CDDPalette {
public:
    CDDPalette() {
        m_palette = NULL;
        m_pos = NULL;
        m_reserved = 0;
        m_cacheA = NULL;
        m_cacheB = NULL;
        m_active = 0;
        m_sourcePalette = NULL;
        m_targetPalette = NULL;
        m_firstColorIndex = 0;
        m_colorCount = 0;
    }

    i32 LoadFromFile(IDirectDraw2* dd, char* filename, u32 flags);

    i32 Create(IDirectDraw2* dd, PALETTEENTRY* entries, u32 flags);
    i32 LoadBmp(IDirectDraw2* dd, char* filename, u32 flags);
    i32 LoadPcx(IDirectDraw2* dd, char* filename, u32 flags);
    i32 CreateRGB(IDirectDraw2* dd, void* rgb, u32 flags);
    i32 CreateFromTrailing(IDirectDraw2* dd, void* data, u32 size, u32 flags);
    i32 LoadPal(IDirectDraw2* dd, char* filename, u32 flags);
    i32 LoadDefault(IDirectDraw2* dd, char* filename, u32 flags);
    void Destroy();
    void GetEntries();

    i32 SetAndNotify(u32 start, u32 count, PALETTEENTRY* data, i32 unused);

    i32 SetEntriesQuad(i32 start, i32 count, RGBQUAD* quads, i32 unused);
    i32 SetEntriesRGB(i32 start, i32 count, u8* rgb, i32 unused);

    void FadeRange(i32 start, i32 count, i32 r, i32 g, i32 b, i32 durationMs);

    void StartFadeToColor(i32 start, i32 count, char r, char g, char b, i32 durationMs);
    void StartFadeToPalette(i32 start, i32 count, PALETTEENTRY* target, i32 durationMs);
    i32 Tick();
    void Flush();

    void BlendRange(i32 pct, i32 start, i32 count, u8 r, u8 g, u8 b);
    void Apply(i32 unused);
    i32 SetRange(i32 start, i32 count, u8 r, u8 g, u8 b, u32 flags);
    i32 CaptureSystemPalette();

    POSITION m_pos;

    IDirectDrawPalette* m_palette;
    i32 m_reserved;

    PALETTEENTRY* m_cacheA;
    PALETTEENTRY* m_cacheB;
    PALETTEENTRY* m_targetPalette;
    PALETTEENTRY* m_sourcePalette;

    PALETTEENTRY m_fixedColor;
    i32 m_durationMs;
    i32 m_startTimeMs;
    i32 m_lastElapsedMs;
    i32 m_firstColorIndex;
    i32 m_colorCount;
    i32 m_active;
};
SIZE(0x38);

struct DDModeInfo {
    i32 width;
    i32 height;
    ColorDepth bpp;
};
SIZE_UNKNOWN();

class CMoviePlayer;

extern i32 RestoreLostSurfaces();

extern "C" i32 __stdcall DdEnumModesCallback(void* mode, i32 unused);

#include <ddraw.h>
#include <stdio.h>

extern i32 (*g_restoreHandler)();
class CDDrawPtrCollections;

extern "C" CDDrawPtrCollections* g_DirectDrawMgr;

void BuildColorChannelTables();
i32 __stdcall CreateDirectDrawVia(
    void* ctx,
    i32 driverDesc,
    i32 driverName,
    IDirectDraw2*(__cdecl* factory)(void*, i32, i32)
);

union DdDriverEnumFn {
    LPDDENUMCALLBACKA m_sdk;
    i32(__stdcall* m_body)(void*, i32, i32, IDirectDraw2*(__cdecl*)(void*, i32, i32));
};
union DdModeEnumFn {
    LPDDENUMMODESCALLBACK m_sdk;
    i32(__stdcall* m_body)(void*, i32);
};

#endif // GRUNTZ_CDIRECTDRAWMGR_H
