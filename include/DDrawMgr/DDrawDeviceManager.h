#ifndef GRUNTZ_DDRAWMGR_DDRAWDEVICEMANAGER_H
#define GRUNTZ_DDRAWMGR_DDRAWDEVICEMANAGER_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/WorldInitError.h>
#include <Ints.h>

struct PidHeader;
struct CDDPalette;
struct IDirectDraw;
struct IDirectDraw2;

class CDDrawOverlaySurface : public CDDSurface {
public:
    virtual ~CDDrawOverlaySurface() OVERRIDE;
    virtual DDSurfacePoolKind GetPoolKind() OVERRIDE;
    virtual i32 CreateOverlay(CDDrawDeviceManager* manager, i32 width, i32 height, i32 caps);

    virtual i32
    UpdateOverlay(RECT* srcRect, CDDSurface* dest, RECT* destRect, u32 flags, DDOVERLAYFX* fx);
};

class CDDrawPrimarySurface : public CDDSurface {
public:
    virtual ~CDDrawPrimarySurface() OVERRIDE;
    virtual i32 CreateFromDesc(CDDrawDeviceManager*, const DDSURFACEDESC*) OVERRIDE;
    virtual DDSurfacePoolKind GetPoolKind() OVERRIDE;
    virtual i32
    CreatePrimary(CDDrawDeviceManager* manager, i32 caps, i32 descFlags, i32 backBufferCount);
    virtual i32 InstallColorFormat();
};

class CDDrawZBufferSurface : public CDDSurface {
public:
    virtual ~CDDrawZBufferSurface() OVERRIDE;
    virtual i32 CreateFromDesc(CDDrawDeviceManager*, const DDSURFACEDESC*) OVERRIDE;
    virtual DDSurfacePoolKind GetPoolKind() OVERRIDE;

    virtual i32 CreateZBuffer(
        CDDrawDeviceManager* manager,
        i32 width,
        i32 height,
        i32 caps,
        i32 extraCaps,
        i32 unused,
        i32 zBufferBitDepth
    );
};

struct DisplayResolution;

class CDDrawDeviceManager {
public:
    i32
    CreateDevice(HWND hwnd, GUID* driverGuid, i32 width, i32 height, ColorDepth bpp, u32 coopFlags);

    i32 GetDisplayMode(i32* pWidth, i32* pHeight, i32* pBpp);

    IDirectDrawSurface* GetGDISurface();

    i32 GetFreeVidMem();

    static void ReportError(char* file, i32 line, i32 hr);

    void EnumerateDisplayModes();

    CDDSurface* WrapAttachedSurface(CDDSurface* srcSurface, i32 caps);

    i32 ShouldSwapDisplayModes(DDSURFACEDESC* first, DDSURFACEDESC* second);

    i32 FindResolutionIndex(i32 width, i32 height, ColorDepth colorDepth);
    i32 FindFirstFittingResolutionIndex(u32 minWidth, u32 minHeight, i32 colorDepth);
    DisplayResolution FindNextResolution(i32 width, i32 height, ColorDepth colorDepth);
    DisplayResolution FindPreviousResolution(i32 width, i32 height, ColorDepth colorDepth);
    DDSURFACEDESC* ResetSurfaceDesc();

    DisplayResolution FindSmallestFittingResolution(u32 minWidth, u32 minHeight, i32 colorDepth);

    i32 Init(void* factory, HWND hwnd, i32 width, i32 height, ColorDepth bpp, u32 coop);

    i32 GetAvailableVidMem(u32 caps, DWORD* total, DWORD* free);

    CDDrawDeviceManager();
    ~CDDrawDeviceManager();

    void Clear(i32 restoreDisplayMode);
    void ClearSurfaces();
    void ClearPalettes();
    void RegisterSurface(CDDSurface* item);
    void RegisterPalette(CDDPalette* item);
    void RemoveSurface(CDDSurface* item);
    void NoOpSurfacePoolHook();
    void RemovePalette(CDDPalette* item);
    CDDSurface* CreateSurfaceFromDesc(const DDSURFACEDESC* desc);

    CDDSurface*
    LoadSurfaceFromPid(PidHeader* hdr, FileImageFormat type, u32 size, i32 ctrl, i32 trans);
    CDDSurface* CreateKeyedSurface(i32 width, i32 height, ColorDepth bitDepth, i32 caps, i32 key);
    CDDSurface* CreateFileSurfaceFromDesc(const DDSURFACEDESC* desc);

    CDDSurface* LoadFileSurface(char* path, i32 caps, i32 colorKey);
    CDDSurface* LoadSystemMemorySurface(char* path, i32 caps, i32 colorKey);

    i32 LoadNumberedSurfaces(
        CDDSurface** out,
        i32 start,
        i32 count,
        char* baseName,
        char* suffix,
        i32 caps,
        i32 colorKey
    );
    CDDSurface* CreateOverlaySurface(i32 width, i32 height, i32 caps);
    CDDSurface* CreateOverlaySurfaceFromDesc(const DDSURFACEDESC* desc);
    CDDSurface* CreatePrimarySurface(i32 caps, i32 descFlags, i32 backBufferCount);
    CDDSurface* CreatePrimarySurfaceFromDesc(const DDSURFACEDESC* desc);
    CDDSurface* Create24BitPrimarySurface(i32 backBufferCount);
    CDDSurface* CreateZBufferSurface(
        i32 width,
        i32 height,
        i32 caps,
        i32 extraCaps,
        i32 unused,
        i32 zBufferBitDepth
    );
    CDDSurface* CreateZBufferSurfaceFromDesc(const DDSURFACEDESC* desc);
    CDDSurface*
    CreateOffscreenSurface(i32 width, i32 height, ColorDepth bitDepth, i32 caps, i32 key);
    CDDPalette* CreateRgbPalette(u8* rgb, i32 flags);
    CDDPalette* CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flags);

    CDDPalette* LoadPaletteFromFile(char* path, i32 flags);
    CDDPalette* CreatePaletteFromTrailingData(void* data, u32 size, i32 flags);

    CDDPalette* LoadTrailingRgbPalette(const char* path, i32 z);
    i32 LoadDisplayPaletteFromFile(const char* path, i32 z);
    i32 SetDisplayPaletteFromRgb(u8* buf, i32 z);

    i32 SetDisplayPaletteFromTrailingRgb(u8* buf, i32 size, i32 tag);

    i32 SetDisplayPaletteFrom(CDDPalette* pal, i32 tag);
    i32 SetDisplayPaletteDirect(PALETTEENTRY* entries, i32 tag);

    i32 ComputeColorMasks();

    i32 ConfigureSurface(i32 width, i32 height, ColorDepth bpp, i32 refreshRate, i32 flags);

    i32 GetCapsChecked();

    IDirectDraw2* m_device;

    IDirectDraw* m_directDraw1;

    DDCAPS m_driverCaps;
    DDCAPS m_helCaps;
    char _pad300[0x47c - 0x300];
    CPtrList m_surfaces;
    CPtrList m_palettes;
    CPtrArray m_displayModes;
    DDSURFACEDESC m_surfaceDesc;
    i32 m_bankSwitchedCaps;

    ColorDepth m_displayColorDepth;

    PALETTEENTRY m_palette[PALETTE_ENTRY_COUNT];
    i32 m_hasPalette;
    i32 m_paletteTag;
    DDrawDeviceError m_lastError;
};

void BuildColorChannelTables();

#endif // GRUNTZ_DDRAWMGR_DDRAWDEVICEMANAGER_H
