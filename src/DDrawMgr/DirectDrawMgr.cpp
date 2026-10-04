#include <StdAfx.h>
#include <Io/File.h>

#include <Ints.h>

#include <DDrawMgr/DirectDrawMgr.h>

#include <ComOutRef.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirPal.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/PixelShift.h>
#include <Dsndmgr/SoundBankLoad.h>
#include <Image/Image.h>
#include <Io/FileStream.h>
#include <SafeDelete.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

#define DDRAWMGR_FILE "C:\\Proj\\DDrawMgr\\DDRAWMGR.CPP"
#define DDRAWMGR_H_FILE "C:\\Proj\\DDrawMgr\\ddrawmgr.h"

CDDrawDeviceManager* g_directDrawMgr = NULL;

b32 g_ddLogEnabled = false;

b32 g_ddMsgBoxEnabled = false;

b32 g_ddBeepEnabled = false;

b32 g_ddThirdEnabled = false;

i32 (*g_restoreHandler)() = NULL;

HINSTANCE g_resModule;

IDirectDraw2* g_directDraw = NULL;

std::vector<DDSURFACEDESC*> g_modeArray;

GUID* g_ddCreateCtx = NULL;

void SetDDrawReportModes(b32 log, b32 msgBox, b32 beep, b32 third) {
    g_ddLogEnabled = log;
    g_ddMsgBoxEnabled = msgBox;
    g_ddBeepEnabled = beep;
    g_ddThirdEnabled = third;
}

void CDDrawDeviceManager::ReportError(char* file, i32 line, i32 hr) {
    char szCode[64];
    char szMsg[256];
    char szLine[512];

    if (g_ddBeepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_ddLogEnabled && !g_ddMsgBoxEnabled && !g_ddThirdEnabled) {
        return;
    }

    i32 code = HRESULT_CODE(hr);

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, "");

    switch (hr) {
        case DDERR_UNSUPPORTED:
            strcpy(szCode, "DDERR_UNSUPPORTED");
            strcpy(szMsg, "Action not supported");
            break;
        case DDERR_GENERIC:
            strcpy(szCode, "DDERR_GENERIC");
            strcpy(szMsg, "Generic failure");
            break;
        case DDERR_OUTOFMEMORY:
            strcpy(szCode, "DDERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case DDERR_INVALIDPARAMS:
            strcpy(szCode, "DDERR_INVALIDPARAMS");
            strcpy(szMsg, "No message");
            break;
        case DDERR_INVALIDCAPS:
            strcpy(szCode, "DDERR_INVALIDCAPS");
            strcpy(szMsg, "One or more of the caps bits passed to the callback are incorrect");
            break;
        case DDERR_INVALIDMODE:
            strcpy(szCode, "DDERR_INVALIDMODE");
            strcpy(szMsg, "No message");
            break;
        case DDERR_INVALIDOBJECT:
            strcpy(szCode, "DDERR_INVALIDOBJECT");
            strcpy(szMsg, "No message");
            break;
        case DDERR_INVALIDPIXELFORMAT:
            strcpy(szCode, "DDERR_INVALIDPIXELFORMAT");
            strcpy(szMsg, "Pixel format was invalid as specified.");
            break;
        case DDERR_INVALIDRECT:
            strcpy(szCode, "DDERR_INVALIDRECT");
            strcpy(szMsg, "No message");
            break;
        case DDERR_LOCKEDSURFACES:
            strcpy(szCode, "DDERR_LOCKEDSURFACES");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NO3D:
            strcpy(szCode, "DDERR_NO3D");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOALPHAHW:
            strcpy(szCode, "DDERR_NOALPHAHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOCOLORCONVHW:
            strcpy(szCode, "DDERR_NOCOLORCONVHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOCOOPERATIVELEVELSET:
            strcpy(szCode, "DDERR_NOCOOPERATIVELEVELSET");
            strcpy(
                szMsg,
                "Create function called without DirectDraw object method SetCooperativeLevel being "
                "called"
            );
            break;
        case DDERR_NOEXCLUSIVEMODE:
            strcpy(szCode, "DDERR_NOEXCLUSIVEMODE");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOGDI:
            strcpy(szCode, "DDERR_NOGDI");
            strcpy(szMsg, "There is no GDI present");
            break;
        case DDERR_NOMIRRORHW:
            strcpy(szCode, "DDERR_NOMIRRORHW");
            strcpy(
                szMsg,
                "Operation could not be carried out because there is no hardware present or "
                "available."
            );
            break;
        case DDERR_NOTFOUND:
            strcpy(szCode, "DDERR_NOTFOUND");
            strcpy(szMsg, "Request item was not found");
            break;
        case DDERR_NOOVERLAYHW:
            strcpy(szCode, "DDERR_NOOVERLAYHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NORASTEROPHW:
            strcpy(szCode, "DDERR_NORASTEROPHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOROTATIONHW:
            strcpy(szCode, "DDERR_NOROTATEHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOSTRETCHHW:
            strcpy(szCode, "DDERR_NOSTRETCHHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOT8BITCOLOR:
            strcpy(szCode, "DDERR_NOT8BITCOLOR");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOTEXTUREHW:
            strcpy(szCode, "DDERR_NOTEXTUREHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOVSYNCHW:
            strcpy(szCode, "DDERR_NOVSYNCHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOZBUFFERHW:
            strcpy(szCode, "DDERR_NOZBUFFERHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_OUTOFCAPS:
            strcpy(szCode, "DDERR_OUTOFCAPS");
            strcpy(szMsg, "No message");
            break;
        case DDERR_OUTOFVIDEOMEMORY:
            strcpy(szCode, "DDERR_OUTOFVIDEOMEMORY");
            strcpy(szMsg, "No message");
            break;
        case DDERR_PALETTEBUSY:
            strcpy(szCode, "DDERR_PALETTEBUSY");
            strcpy(szMsg, "No message");
            break;
        case DDERR_SURFACEBUSY:
            strcpy(szCode, "DDERR_SURFACEBUSY");
            strcpy(szMsg, "No message");
            break;
        case DDERR_SURFACEISOBSCURED:
            strcpy(szCode, "DDERR_SURFACEISOBSCURED");
            strcpy(szMsg, "No message");
            break;
        case DDERR_SURFACELOST:
            strcpy(szCode, "DDERR_SURFACELOST");
            strcpy(szMsg, "No message");
            break;
        case DDERR_SURFACENOTATTACHED:
            strcpy(szCode, "DDERR_SURFACENOTATTACHED");
            strcpy(szMsg, "The requested surface is not attached");
            break;
        case DDERR_TOOBIGSIZE:
            strcpy(szCode, "DDERR_TOOBIGSIZE");
            strcpy(szMsg, "No message");
            break;
        case DDERR_TOOBIGWIDTH:
            strcpy(szCode, "DDERR_TOOBIGWIDTH");
            strcpy(szMsg, "No message");
            break;
        case DDERR_VERTICALBLANKINPROGRESS:
            strcpy(szCode, "DDERR_VERTICALBLANKINPROGRESS");
            strcpy(szMsg, "No message");
            break;
        case DDERR_WASSTILLDRAWING:
            strcpy(szCode, "DDERR_WASTILLDRAWING");
            strcpy(
                szMsg,
                "The previous Blt which is transfering information to or from this Surface is "
                "incomplete"
            );
            break;
        case DDERR_NODIRECTDRAWHW:
            strcpy(szCode, "DDERR_NODIRECTDRAWHW");
            strcpy(szMsg, "No message");
            break;
        case DDERR_DIRECTDRAWALREADYCREATED:
            strcpy(szCode, "DDERR_DIRECTDRAWALREADYCREATED");
            strcpy(szMsg, "No message");
            break;
        case DDERR_XALIGN:
            strcpy(szCode, "DDERR_XALIGN");
            strcpy(szMsg, "Rectangle provided was not horizontally aligned on a DWORD boundary");
            break;
        case DDERR_HWNDSUBCLASSED:
            strcpy(szCode, "DDERR_HWNDSUBCLASSED");
            strcpy(szMsg, "No message");
            break;
        case DDERR_HWNDALREADYSET:
            strcpy(szCode, "DDERR_HWNDALREADYSET");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOPALETTEHW:
            strcpy(szCode, "DDERR_NOPALETTEHW");
            strcpy(szMsg, "No hardware support for 16 or 256 color palettes");
            break;
        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            strcpy(szCode, "DDERR_PRIMARYSURFACEALREADYEXISTS");
            strcpy(szMsg, "This process already has created a primary surface");
            break;
        case DDERR_EXCLUSIVEMODEALREADYSET:
            strcpy(szCode, "DDERR_EXCLUSIVEMODEALREADYSET");
            strcpy(szMsg, "No message");
            break;
        case DDERR_NOTLOCKED:
            strcpy(szCode, "DDERR_LOCKEDSURFACES");
            strcpy(szMsg, "No message");
            break;
        case DD_OK:
            strcpy(szCode, "DD_OK");
            strcpy(szMsg, "No error");
            break;
        default:
            break;
    }

    if (g_ddLogEnabled) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
        DDrawLogLine(szLine);
    }
    if (g_ddMsgBoxEnabled) {
        if (file == NULL || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA(static_cast<HWND>(0), szLine, "DirectDrawMgr", MB_ICONEXCLAMATION);
    }
}

void __cdecl DDrawLogLine(char*, ...) {}

CDDrawDeviceManager::CDDrawDeviceManager() : m_surfaces(), m_palettes(), m_displayModes() {
    m_device = NULL;
    m_directDraw1 = NULL;
    m_bankSwitchedCaps = 0;
    m_displayColorDepth = BPP_UNSET;
    m_hasPalette = false;
    m_paletteTag = 0;
    m_lastError = DDRAWERR_NONE;
}

CDDrawDeviceManager::~CDDrawDeviceManager() {
    Clear(1);
}

i32 CDDrawDeviceManager::CreateDevice(
    HWND hwnd,
    GUID* driverGuid,
    i32 width,
    i32 height,
    ColorDepth bpp,
    u32 coopFlags
) {
    m_hasPalette = false;
    m_paletteTag = 0;
    IDirectDraw2* dd = g_directDraw;
    if (dd != NULL) {
        m_device = dd;
    } else {
        i32 chr = DirectDrawCreate(driverGuid, &m_directDraw1, NULL);
        if (chr != 0) {
            CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0x88, chr);
            if (m_lastError == DDRAWERR_NONE) {
                m_lastError = DDRAWERR_CREATE;
            }
            return 0;
        }
        ComOutRef<IDirectDraw2> devOut;
        devOut.m_asTyped = &m_device;
        chr = m_directDraw1->QueryInterface(IID_IDirectDraw2, devOut.m_asVoid);
        if (chr != 0) {
            CDDrawDeviceManager::ReportError(NULL, 0, chr);
            if (m_lastError == DDRAWERR_NONE) {
                m_lastError = DDRAWERR_QUERY_INTERFACE;
            }
            return 0;
        }
    }

    i32 hr = m_device->SetCooperativeLevel(hwnd, coopFlags);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(DDRAWMGR_H_FILE, 0x120, hr);
    }
    if (hr != 0) {
        if (m_lastError == DDRAWERR_NONE) {
            m_lastError = DDRAWERR_COOPERATIVE_LEVEL;
        }
        return 0;
    }

    memset(&m_driverCaps, 0, sizeof(m_driverCaps));
    memset(&m_helCaps, 0, sizeof(m_helCaps));
    m_driverCaps.dwSize = sizeof(DDCAPS);
    m_helCaps.dwSize = sizeof(DDCAPS);
    hr = m_device->GetCaps(&m_driverCaps, &m_helCaps);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0xad, hr);
    }
    m_bankSwitchedCaps = m_driverCaps.dwCaps & DDCAPS_BANKSWITCHED;
    EnumerateDisplayModes();

    if (width > 0 && height > 0) {
        hr = ConfigureSurface(width, height, bpp, 0, 0);
        if (hr != 0) {
            CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0xc2, hr);
            if (m_lastError == DDRAWERR_NONE) {
                m_lastError = DDRAWERR_DISPLAY_MODE;
            }
            return 0;
        }
        m_displayColorDepth = bpp;
    }

    if (bpp == BPP_UNSET) {
        DDSURFACEDESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = sizeof(desc);
        hr = m_device->GetDisplayMode(&desc);
        if (hr == 0) {
            m_displayColorDepth = static_cast<ColorDepth>(desc.ddpfPixelFormat.dwRGBBitCount);
        }
    }

    g_directDrawMgr = this;
    return 1;
}

i32 CDDrawDeviceManager::Init(
    void* factory,
    HWND hwnd,
    i32 width,
    i32 height,
    ColorDepth bpp,
    u32 coop
) {
    if (factory == NULL) {
        return 0;
    }
    g_ddCreateCtx = NULL;
    DdDriverEnumFn cb;
    cb.m_body = CreateDirectDrawVia;
    i32 hr = DirectDrawEnumerateA(cb.m_sdk, factory);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0xf4, hr);
        return 0;
    }
    return CreateDevice(hwnd, g_ddCreateCtx, width, height, bpp, coop);
}

void CDDrawDeviceManager::Clear(i32 restoreDisplayMode) {
    if (restoreDisplayMode && m_device) {
        m_device->RestoreDisplayMode();
    }
    FreeDisplayModes();
    ClearSurfaces();
    ClearPalettes();
    g_directDrawMgr = NULL;
    SAFE_RELEASE(m_device);
    SAFE_RELEASE(m_directDraw1);
    m_bankSwitchedCaps = 0;
}

void CDDrawDeviceManager::RegisterSurface(CDDSurface* item) {
    item->m_pos = m_surfaces.insert(m_surfaces.end(), item);
}

void CDDrawDeviceManager::ClearSurfaces() {
    std::list<CDDSurface*>::iterator pos = m_surfaces.begin();
    while (pos != m_surfaces.end()) {
        CDDSurface* item = static_cast<CDDSurface*>(*(pos++));
        delete item;
    }
    m_surfaces.clear();
}

void CDDrawDeviceManager::RemoveSurface(CDDSurface* item) {
    m_surfaces.erase(item->m_pos);
    delete item;
}

void CDDrawDeviceManager::NoOpSurfacePoolHook() {}

CDDSurface* CDDrawDeviceManager::CreateSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDSurface* item = new CDDSurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

CDDSurface* CDDrawDeviceManager::LoadSurfaceFromPid(
    PidHeader* hdr,
    FileImageFormat type,
    u32 size,
    i32 ctrl,
    i32 trans
) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->ResolveEx(this, hdr, type, size, ctrl, trans)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

CDDSurface* CDDrawDeviceManager::CreateKeyedSurface(
    i32 width,
    i32 height,
    ColorDepth bitDepth,
    i32 caps,
    i32 key
) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->LoadKeyed(this, width, height, bitDepth, caps, key)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

CDDSurface* CDDrawDeviceManager::CreateFileSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

CDDSurface* CDDrawDeviceManager::LoadFileSurface(char* path, i32 caps, i32 colorKey) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->LoadByExt(this, path, caps, colorKey)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

i32 CDDrawDeviceManager::LoadNumberedSurfaces(
    CDDSurface** out,
    i32 start,
    i32 count,
    char* baseName,
    char* suffix,
    i32 caps,
    i32 colorKey
) {
    i32 n = 0;
    i32 end = start + count;
    for (i32 i = start; i < end; i++) {
        char buf[32];
        sprintf(buf, "%s%i", baseName, i);
        if (suffix != NULL) {
            if (suffix[0] != '.') {
                strcat(buf, g_singleDot);
            }
            strcat(buf, suffix);
        }
        CDDSurface* item = LoadFileSurface(buf, caps, colorKey);
        if (item == NULL) {
            break;
        }
        out[n] = item;
        n++;
    }
    return n;
}

CDDSurface* CDDrawDeviceManager::CreateOverlaySurface(i32 width, i32 height, i32 caps) {
    CDDrawOverlaySurface* item = new CDDrawOverlaySurface;
    if (!item->CreateOverlay(this, width, height, caps)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

CDDSurface* CDDrawDeviceManager::CreateOverlaySurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDrawOverlaySurface* item = new CDDrawOverlaySurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

CDDSurface*
CDDrawDeviceManager::CreatePrimarySurface(i32 caps, i32 descFlags, i32 backBufferCount) {
    CDDrawPrimarySurface* item = new CDDrawPrimarySurface;
    if (!item->CreatePrimary(this, caps, descFlags, backBufferCount)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    m_displayColorDepth = item->GetBitDepth();
    return item;
}

CDDSurface* CDDrawDeviceManager::CreatePrimarySurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDrawPrimarySurface* item = new CDDrawPrimarySurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    m_displayColorDepth = item->GetBitDepth();
    return item;
}

CDDSurface* CDDrawDeviceManager::Create24BitPrimarySurface(i32 backBufferCount) {
    CDDrawPrimarySurface* item = new CDDrawPrimarySurface;
    if (!item->CreatePrimary(
            this,
            DDSCAPS_COMPLEX | DDSCAPS_FLIP,
            DDSD_CAPS | DDSD_BACKBUFFERCOUNT,
            backBufferCount
        )) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    m_displayColorDepth = item->GetBitDepth();
    return item;
}

CDDSurface* CDDrawDeviceManager::CreateZBufferSurface(
    i32 width,
    i32 height,
    i32 caps,
    i32 extraCaps,
    i32 unused,
    i32 zBufferBitDepth
) {
    CDDrawZBufferSurface* item = new CDDrawZBufferSurface;
    if (!item->CreateZBuffer(this, width, height, caps, extraCaps, unused, zBufferBitDepth)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

CDDSurface* CDDrawDeviceManager::CreateZBufferSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDrawZBufferSurface* item = new CDDrawZBufferSurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

CDDSurface* CDDrawDeviceManager::CreateOffscreenSurface(
    i32 width,
    i32 height,
    ColorDepth bitDepth,
    i32 caps,
    i32 key
) {
    return CreateKeyedSurface(
        width,
        height,
        bitDepth,
        caps | DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN,
        key
    );
}

CDDSurface* CDDrawDeviceManager::LoadSystemMemorySurface(char* path, i32 caps, i32 colorKey) {
    return LoadFileSurface(path, caps | DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN, colorKey);
}

void CDDrawDeviceManager::RegisterPalette(CDDPalette* item) {
    item->m_pos = m_palettes.insert(m_palettes.end(), item);
}

void CDDrawDeviceManager::ClearPalettes() {
    std::list<CDDPalette*>::iterator pos = m_palettes.begin();
    while (pos != m_palettes.end()) {
        CDDPalette* item = static_cast<CDDPalette*>(*(pos++));
        if (item) {
            item->Destroy();
            delete item;
        }
    }
    m_palettes.clear();
}

void CDDrawDeviceManager::RemovePalette(CDDPalette* item) {
    m_palettes.erase(item->m_pos);
    if (item) {
        item->Destroy();
        delete item;
    }
}

CDDPalette* CDDrawDeviceManager::LoadPaletteFromFile(char* path, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->LoadFromFile(m_device, path, flags)) {
        if (item) {
            item->Destroy();
            delete item;
        }
        return NULL;
    }
    RegisterPalette(item);
    return item;
}

CDDPalette* CDDrawDeviceManager::CreateRgbPalette(u8* rgb, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateRGB(m_device, rgb, flags)) {
        if (item) {
            item->Destroy();
            delete item;
        }
        return NULL;
    }
    RegisterPalette(item);
    return item;
}

CDDPalette* CDDrawDeviceManager::CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flags) {
    CDDPalette* item = new CDDPalette;

    if (!item->Create(m_device, entries, flags)) {
        if (item) {
            item->Destroy();
            delete item;
        }
        return NULL;
    }
    RegisterPalette(item);
    return item;
}

CDDPalette* CDDrawDeviceManager::CreatePaletteFromTrailingData(void* data, u32 size, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateFromTrailing(m_device, data, size, flags)) {
        if (item) {
            item->Destroy();
            delete item;
        }
        return NULL;
    }
    RegisterPalette(item);
    return item;
}

CDDPalette* CDDrawDeviceManager::LoadTrailingRgbPalette(const char* path, i32 z) {
    io::File file;
    if (!file.open(path, io::ReadOnly)) {
        return NULL;
    }
    file.seek(-PALETTE_RGB_BYTE_COUNT, io::End);
    u8 buf[PALETTE_RGB_BYTE_COUNT];
    if (file.read(buf, PALETTE_RGB_BYTE_COUNT) != PALETTE_RGB_BYTE_COUNT) {
        return NULL;
    }
    return CreateRgbPalette(buf, z);
}

void CDDrawDeviceManager::EnumerateDisplayModes() {
    FreeDisplayModes();
    g_modeArray.clear();
    DdModeEnumFn modeCb;
    modeCb.m_body = DdEnumModesCallback;
    i32 hr = m_device->EnumDisplayModes(0, NULL, NULL, modeCb.m_sdk);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0x507, hr);
    }

    for (i32 j = 0; j < static_cast<i32>(g_modeArray.size()); j++) {
        m_displayModes.push_back(g_modeArray[j]);
    }
    g_modeArray.clear();
    i32 modeCount = static_cast<i32>(m_displayModes.size());
    if (modeCount > 1) {
        for (i32 firstIndex = 0; firstIndex < modeCount - 1; firstIndex++) {
            for (i32 secondIndex = firstIndex + 1; secondIndex < modeCount; secondIndex++) {

                DDSURFACEDESC* first = GetModeDesc(firstIndex);
                DDSURFACEDESC* second = GetModeDesc(secondIndex);
                if (ShouldSwapDisplayModes(first, second)) {
                    m_displayModes[firstIndex] = second;
                    m_displayModes[secondIndex] = first;
                }
            }
        }
    }
}

i32 __stdcall DdEnumModesCallback(DDSURFACEDESC* mode, i32 unused) {
    DDSURFACEDESC* copy = new DDSURFACEDESC;
    memcpy(copy, mode, sizeof(DDSURFACEDESC));
    g_modeArray.push_back(copy);
    return DDENUMRET_OK;
}

i32 CDDrawDeviceManager::ShouldSwapDisplayModes(DDSURFACEDESC* first, DDSURFACEDESC* second) {
    if (first->dwWidth > second->dwWidth) {
        return 1;
    }
    if (first->dwWidth < second->dwWidth) {
        return 0;
    }
    if (first->dwHeight > second->dwHeight) {
        return 1;
    }
    if (first->dwHeight < second->dwHeight) {
        return 0;
    }
    return first->ddpfPixelFormat.dwRGBBitCount > second->ddpfPixelFormat.dwRGBBitCount;
}

DisplayResolution
CDDrawDeviceManager::FindSmallestFittingResolution(u32 minWidth, u32 minHeight, i32 colorDepth) {
    i32 idx = FindFirstFittingResolutionIndex(minWidth, minHeight, colorDepth);
    if (idx == -1) {
        DisplayResolution none;
        none.m_width = -1;
        none.m_height = -1;
        return none;
    }
    DDSURFACEDESC* mode = GetModeDesc(idx);
    DisplayResolution resolution;
    resolution.m_width = mode->dwWidth;
    resolution.m_height = mode->dwHeight;
    return resolution;
}

i32 CDDrawDeviceManager::FindFirstFittingResolutionIndex(
    u32 minWidth,
    u32 minHeight,
    i32 colorDepth
) {
    i32 result = -1;
    for (i32 i = (static_cast<i32>(m_displayModes.size()) - 1); i >= 0; i--) {
        DDSURFACEDESC* mode = GetModeDesc(i);
        if (mode->dwWidth >= minWidth && mode->dwHeight >= minHeight
            && mode->ddpfPixelFormat.dwRGBBitCount == colorDepth) {
            result = i;
        }
    }
    return result;
}

i32 CDDrawDeviceManager::FindResolutionIndex(i32 width, i32 height, ColorDepth colorDepth) {
    for (i32 i = 0; i < static_cast<i32>(m_displayModes.size()); i++) {
        DDSURFACEDESC* mode = GetModeDesc(i);
        if (mode->dwWidth == static_cast<u32>(width) && mode->dwHeight == static_cast<u32>(height)
            && mode->ddpfPixelFormat.dwRGBBitCount == IDX(colorDepth)) {
            return i;
        }
    }
    return -1;
}

DisplayResolution
CDDrawDeviceManager::FindNextResolution(i32 width, i32 height, ColorDepth colorDepth) {
    DisplayResolution resolution;
    i32 idx = FindResolutionIndex(width, height, colorDepth);
    if (idx != -1 && idx < static_cast<i32>(m_displayModes.size())) {
        idx++;
        if (idx < static_cast<i32>(m_displayModes.size())) {
            for (; idx < static_cast<i32>(m_displayModes.size()); idx++) {
                DDSURFACEDESC* mode = GetModeDesc(idx);
                if (mode->ddpfPixelFormat.dwRGBBitCount == IDX(colorDepth)) {
                    resolution.m_width = mode->dwWidth;
                    resolution.m_height = mode->dwHeight;
                    return resolution;
                }
            }
        }
    }
    resolution.m_width = -1;
    resolution.m_height = -1;
    return resolution;
}

DisplayResolution
CDDrawDeviceManager::FindPreviousResolution(i32 width, i32 height, ColorDepth colorDepth) {
    DisplayResolution resolution;
    i32 idx = FindResolutionIndex(width, height, colorDepth);
    if (idx != -1 && idx < static_cast<i32>(m_displayModes.size())) {
        idx--;
        if (idx >= 0) {
            for (; idx >= 0; idx--) {
                DDSURFACEDESC* mode = GetModeDesc(idx);
                if (mode->ddpfPixelFormat.dwRGBBitCount == IDX(colorDepth)) {
                    resolution.m_width = mode->dwWidth;
                    resolution.m_height = mode->dwHeight;
                    return resolution;
                }
            }
        }
    }
    resolution.m_width = -1;
    resolution.m_height = -1;
    return resolution;
}

DDSURFACEDESC* CDDrawDeviceManager::ResetSurfaceDesc() {
    memset(&m_surfaceDesc, 0, sizeof(m_surfaceDesc));
    m_surfaceDesc.dwSize = sizeof(m_surfaceDesc);
    return &m_surfaceDesc;
}

CDDSurface* CDDrawDeviceManager::WrapAttachedSurface(CDDSurface* srcSurface, i32 caps) {
    IDirectDrawSurface* attached = NULL;
    DDSCAPS want;
    want.dwCaps = caps;
    i32 hr = srcSurface->GetDirectDrawSurface()->GetAttachedSurface(&want, &attached);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0x6ae, hr);
        return NULL;
    }

    CDDSurface* item = new CDDSurface;
    if (item->Refresh(attached) == 0) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

i32 CDDrawDeviceManager::GetDisplayMode(i32* pWidth, i32* pHeight, i32* pBpp) {
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    i32 hr = m_device->GetDisplayMode(&desc);
    if (hr != 0) {
        *pWidth = 0;
        *pHeight = 0;
        *pBpp = 0;
        CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0x6e5, hr);
        return 0;
    }
    *pWidth = desc.dwWidth;
    *pHeight = desc.dwHeight;
    *pBpp = desc.ddpfPixelFormat.dwRGBBitCount;
    return 1;
}

void SetSurfaceRestoreHandler(SurfaceRestoreFn handler) {
    g_restoreHandler = handler;
}

i32 RestoreLostSurfaces() {
    if (g_restoreHandler) {
        return g_restoreHandler();
    }
    DDrawLogLine("WARNING - Surface(s) lost but no restore handler is available\n");
    return 0;
}

i32 CDDrawDeviceManager::GetAvailableVidMem(u32 caps, DWORD* total, DWORD* free) {
    DDSCAPS ddsCaps;
    ddsCaps.dwCaps = caps;
    HRESULT hr = m_device->GetAvailableVidMem(&ddsCaps, total, free);
    return hr == 0;
}

i32 CDDrawDeviceManager::GetFreeVidMem() {
    DDSCAPS caps;
    DWORD total;
    DWORD freeMem;
    caps.dwCaps = DDSCAPS_TEXTURE;
    i32 hr = m_device->GetAvailableVidMem(&caps, &total, &freeMem);
    return hr == 0 ? freeMem : 0;
}

i32 __stdcall

CreateDirectDrawVia(
    GUID* lpGuid,
    i32 driverDesc,
    i32 driverName,
    IDirectDraw2*(__cdecl* factory)(void*, i32, i32)
) {
    if (factory != NULL) {
        IDirectDraw2* dd = factory(lpGuid, driverDesc, driverName);
        if (dd != NULL) {
            g_directDraw = dd;
            g_ddCreateCtx = lpGuid;
            return DDENUMRET_CANCEL;
        }
    }
    return DDENUMRET_OK;
}

IDirectDrawSurface* CDDrawDeviceManager::GetGDISurface() {
    IDirectDrawSurface* surf = NULL;
    i32 hr = m_device->GetGDISurface(&surf);
    if (hr != 0) {
        DDrawLogLine(
            const_cast<char*>("CDirectDrawMgr::GetGDISurface() - Cannot get the GDI surface!\r\n")
        );
        return NULL;
    }
    return surf;
}

i32 CDDrawDeviceManager::SetDisplayPaletteFrom(CDDPalette* pal, i32 tag) {

    if (pal == NULL) {
        return 0;
    }
    PALETTEENTRY* src = pal->m_entries;
    if (src == NULL) {
        return 0;
    }
    PALETTEENTRY* dst = m_palette;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        *dst++ = *src++;
    }
    m_hasPalette = true;
    m_paletteTag = tag;
    return 1;
}

i32 CDDrawDeviceManager::SetDisplayPaletteFromRgb(u8* buf, i32 z) {
    if (buf == NULL) {
        return 0;
    }
    const u8* src = buf;
    COPY_RGB_PALETTE(m_palette, src, i, PALETTE_ENTRY_COUNT)
    m_hasPalette = true;
    m_paletteTag = z;
    return 1;
}

i32 CDDrawDeviceManager::SetDisplayPaletteDirect(PALETTEENTRY* entries, i32 tag) {
    if (entries == NULL) {
        return 0;
    }
    PALETTEENTRY* src = entries;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        m_palette[i] = *src++;
    }
    m_hasPalette = true;
    m_paletteTag = tag;
    return 1;
}

i32 CDDrawDeviceManager::SetDisplayPaletteFromTrailingRgb(u8* buf, i32 size, i32 tag) {
    if (buf == NULL) {
        return 0;
    }
    if (static_cast<u32>(size) < 0x3e8) {
        return 0;
    }
    return SetDisplayPaletteFromRgb(buf + size - PALETTE_RGB_BYTE_COUNT, tag);
}

i32 CDDrawDeviceManager::LoadDisplayPaletteFromFile(const char* path, i32 z) {
    io::File file;
    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }
    file.seek(-PALETTE_RGB_BYTE_COUNT, io::End);
    u8 buf[PALETTE_RGB_BYTE_COUNT];
    if (file.read(buf, PALETTE_RGB_BYTE_COUNT) != PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    return SetDisplayPaletteFromRgb(buf, z);
}

i32 CDDrawDeviceManager::ComputeColorMasks() {
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    i32 hr = m_device->GetDisplayMode(&desc);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0x82c, hr);
        return 0;
    }

    u32 m = desc.ddpfPixelFormat.dwRBitMask;
    i32 count = 0;
    i32 shift = -1;
    for (i32 redBit = 0; redBit < 0x20; redBit++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = redBit;
            }
            count++;
        }
        m >>= 1;
    }
    g_rUp = shift;
    g_rDown = 8 - count;

    m = desc.ddpfPixelFormat.dwGBitMask;
    count = 0;
    shift = -1;
    for (i32 greenBit = 0; greenBit < 0x20; greenBit++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = greenBit;
            }
            count++;
        }
        m >>= 1;
    }
    g_gUp = shift;
    g_gDown = 8 - count;

    m = desc.ddpfPixelFormat.dwBBitMask;
    count = 0;
    shift = -1;
    for (i32 blueBit = 0; blueBit < 0x20; blueBit++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = blueBit;
            }
            count++;
        }
        m >>= 1;
    }
    g_bUp = shift;
    g_bDown = 8 - count;

    BuildColorChannelTables();
    return 1;
}

i32 CDDrawDeviceManager::ConfigureSurface(
    i32 width,
    i32 height,
    ColorDepth bpp,
    i32 refreshRate,
    i32 flags
) {
    i32 hr = m_device->SetDisplayMode(width, height, IDX(bpp), refreshRate, flags);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0x8a2, hr);
        if (m_lastError == DDRAWERR_NONE) {
            m_lastError = DDRAWERR_DISPLAY_MODE;
        }
        return hr;
    }
    if (ComputeColorMasks() == 0) {
        hr = DDERR_GENERIC;
        if (m_lastError == DDRAWERR_NONE) {
            m_lastError = DDRAWERR_COLOR_MASKS;
        }
    }
    return hr;
}

DDSurfacePoolKind CDDrawOverlaySurface::GetPoolKind() {
    return POOLKIND_OVERLAY;
}

DDSurfacePoolKind CFileImageSurface::GetPoolKind() {
    return POOLKIND_FILEIMAGE;
}

DDSurfacePoolKind CDDrawPrimarySurface::GetPoolKind() {
    return POOLKIND_MODE;
}

DDSurfacePoolKind CDDrawZBufferSurface::GetPoolKind() {
    return POOLKIND_ZBUFFER;
}
