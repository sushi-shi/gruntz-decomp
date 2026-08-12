#ifndef GRUNTZ_GRUNTZ_CDDRAWPTRCOLLECTIONS_H
#define GRUNTZ_GRUNTZ_CDDRAWPTRCOLLECTIONS_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/WorldInitError.h>
#include <Ints.h>

struct PidHeader;
struct CDDPalette;
struct IDirectDraw;
struct IDirectDraw2;

class CPoolItemA88 : public CDDSurface {
public:
    virtual ~CPoolItemA88() OVERRIDE;
    virtual DDSurfacePoolKind GetPoolKind() OVERRIDE;
    virtual i32 Blit7(CDDrawPtrCollections*, i32, i32, i32);

    virtual i32 UpdateOverlay(void* srcRect, CDDSurface* dest, void* destRect, u32 flags, void* fx);
};

class CPoolItemAB8 : public CDDSurface {
public:
    virtual ~CPoolItemAB8() OVERRIDE;
    virtual i32 CreateFromDesc(CDDrawPtrCollections*, const DDSURFACEDESC*) OVERRIDE;
    virtual DDSurfacePoolKind GetPoolKind() OVERRIDE;
    virtual i32 Setup(CDDrawPtrCollections*, i32, i32, i32);
    virtual i32 InstallColorFormat();
};

class CPoolItemAE8 : public CDDSurface {
public:
    virtual ~CPoolItemAE8() OVERRIDE;
    virtual i32 CreateFromDesc(CDDrawPtrCollections*, const DDSURFACEDESC*) OVERRIDE;
    virtual DDSurfacePoolKind GetPoolKind() OVERRIDE;

    virtual i32 Blit47(CDDrawPtrCollections*, i32, i32, i32, i32, i32, i32);
};

struct CDdModePair;

class CDDrawPtrCollections {
public:
    i32 CreateDevice(
        void* hwnd,
        void* driverGuid,
        i32 width,
        i32 height,
        ColorDepth bpp,
        u32 coopFlags
    );

    i32 GetDisplayMode(i32* pWidth, i32* pHeight, i32* pBpp);

    IDirectDrawSurface* GetGDISurface();

    i32 GetFreeVidMem();

    static void GetErrorString(char* file, i32 line, i32 hr);

    void SetupCaps();

    void* CreatePoolItem(void* srcSurface, i32 caps);

    i32 Compare(void* a, void* b);

    i32 FindIndex(i32 k0, i32 k1, ColorDepth colorDepth);
    i32 FindLast(u32 k0, u32 k1, i32 k2);
    CDdModePair FindFwd(i32 k0, i32 k1, ColorDepth colorDepth);
    CDdModePair FindBack(i32 k0, i32 k1, ColorDepth colorDepth);

    CDdModePair FindMatch(u32 k0, u32 k1, i32 k2);

    i32 Init(void* factory, void* hwnd, i32 width, i32 height, ColorDepth bpp, u32 coop);

    i32 GetAvailableVidMem(u32 caps, DWORD* total, DWORD* free);

    CDDrawPtrCollections();
    ~CDDrawPtrCollections();

    void Clear(i32 mode);
    void EmptyPoolA();
    void EmptyPoolB();
    void AddItemA(CDDSurface* item);
    void AddItemB(CDDPalette* item);
    void RemoveItemA(CDDSurface* item);
    void RemoveItemB(CDDPalette* item);
    CDDSurface* CreateSurfaceFromDesc(const DDSURFACEDESC* desc);

    CDDSurface*
    LoadSurfaceFromPid(PidHeader* hdr, FileImageFormat type, u32 size, i32 ctrl, i32 trans);
    CDDSurface* CreateKeyedSurface(i32 width, i32 height, ColorDepth bitDepth, i32 caps, i32 key);
    CDDSurface* CreateFileSurfaceFromDesc(const DDSURFACEDESC* desc);

    CDDSurface* LoadFileSurface(char* path, i32 caps, i32 colorKey);

    i32 CreateRange(
        CDDSurface** out,
        i32 start,
        i32 count,
        char* baseName,
        char* suffix,
        i32 caps,
        i32 colorKey
    );
    CDDSurface* CreateBlit7Surface(i32 a, i32 b, i32 c);
    CDDSurface* CreateBlit7SurfaceFromDesc(const DDSURFACEDESC* desc);
    CDDSurface* CreatePaletteSurface(i32 a, i32 b, i32 c);
    CDDSurface* CreatePaletteSurfaceFromDesc(const DDSURFACEDESC* desc);
    CDDSurface* Create24BitPaletteSurface(i32 a);
    CDDSurface* CreateBlit47Surface(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f);
    CDDSurface* CreateBlit47SurfaceFromDesc(const DDSURFACEDESC* desc);
    CDDSurface* MakeAndAddB(i32 width, i32 height, ColorDepth bitDepth, i32 caps, i32 key);
    CDDPalette* CreateRgbPalette(void* rgb, i32 flags);
    CDDPalette* CreatePaletteFromEntries(i32 a, i32 b);

    CDDPalette* LoadPaletteFromFile(char* path, i32 flags);
    CDDPalette* CreatePaletteFromTrailingData(void* a, u32 b, i32 c);

    CDDPalette* LoadTrailingRgbPalette(const char* path, i32 z);
    CDDPalette* LoadDisplayPaletteFromFile(const char* path, i32 z);
    CDDPalette* SetDisplayPaletteFromRgb(void* buf, i32 z);

    CDDPalette* SetDisplayPaletteFromTrailingRgb(u8* buf, i32 size, i32 tag);

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
    CPtrList m_poolA;
    CPtrList m_poolB;
    CPtrArray m_poolItems;
    char _pad4C8[0x534 - 0x4c8];
    i32 m_bltCaps;

    ColorDepth m_palBpp;

    PALETTEENTRY m_palette[0x100];
    i32 m_hasPalette;
    i32 m_paletteTag;
    DDrawDeviceError m_lastError;
};

void BuildColorChannelTables();

#endif // GRUNTZ_GRUNTZ_CDDRAWPTRCOLLECTIONS_H
