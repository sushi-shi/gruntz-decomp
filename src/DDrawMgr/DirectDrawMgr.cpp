#include <rva.h>

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

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

#define DDRAWMGR_FILE "C:\\Proj\\DDrawMgr\\DDRAWMGR.CPP"
#define DDRAWMGR_H_FILE "C:\\Proj\\DDrawMgr\\ddrawmgr.h"

DATA(0x002bed00)
CDDrawDeviceManager* g_DirectDrawMgr = NULL;

DATA(0x00283eb8)
b32 g_ddLogEnabled = false;
DATA(0x00283ebc)
b32 g_ddMsgBoxEnabled = false;
DATA(0x00283ec0)
b32 g_ddBeepEnabled = false;
DATA(0x00283ec4)
b32 g_ddThirdEnabled = false;

DATA(0x00283edc)
i32 (*g_restoreHandler)() = NULL;
DATA(0x00283ee0)
HINSTANCE g_resModule;

DATA(0x00283ee8)
IDirectDraw2* g_DirectDraw = NULL;
RVA_DYNINIT(0x00141c70, 0xa, g_modeArray)
RVA_DYNINIT(0x00141c80, 0xa, g_modeArray)
RVA_DYNINIT(0x00141c90, 0xe, g_modeArray)
RVA_DYNINIT(0x00141ca0, 0xa, g_modeArray)
DATA(0x00283ec8)
CPtrArray g_modeArray;
DATA(0x00283ee4)
GUID* g_ddCreateCtx = NULL;

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001413d0, 0x27)
void SetDDrawReportModes(b32 log, b32 msgBox, b32 beep, b32 third) {
    g_ddLogEnabled = log;
    g_ddMsgBoxEnabled = msgBox;
    g_ddBeepEnabled = beep;
    g_ddThirdEnabled = third;
}

RVA(0x00141400, 0x870)
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

    i32 code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, "");

    switch (hr) {
        case static_cast<i32>(DDERR_UNSUPPORTED):
            strcpy(szCode, "DDERR_UNSUPPORTED");
            strcpy(szMsg, "Action not supported");
            break;
        case static_cast<i32>(DDERR_GENERIC):
            strcpy(szCode, "DDERR_GENERIC");
            strcpy(szMsg, "Generic failure");
            break;
        case static_cast<i32>(DDERR_OUTOFMEMORY):
            strcpy(szCode, "DDERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_INVALIDPARAMS):
            strcpy(szCode, "DDERR_INVALIDPARAMS");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_INVALIDCAPS):
            strcpy(szCode, "DDERR_INVALIDCAPS");
            strcpy(szMsg, "One or more of the caps bits passed to the callback are incorrect");
            break;
        case static_cast<i32>(DDERR_INVALIDMODE):
            strcpy(szCode, "DDERR_INVALIDMODE");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_INVALIDOBJECT):
            strcpy(szCode, "DDERR_INVALIDOBJECT");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_INVALIDPIXELFORMAT):
            strcpy(szCode, "DDERR_INVALIDPIXELFORMAT");
            strcpy(szMsg, "Pixel format was invalid as specified.");
            break;
        case static_cast<i32>(DDERR_INVALIDRECT):
            strcpy(szCode, "DDERR_INVALIDRECT");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_LOCKEDSURFACES):
            strcpy(szCode, "DDERR_LOCKEDSURFACES");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887600aa):
            strcpy(szCode, "DDERR_NO3D");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOALPHAHW):
            strcpy(szCode, "DDERR_NOALPHAHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOCOLORCONVHW):
            strcpy(szCode, "DDERR_NOCOLORCONVHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOCOOPERATIVELEVELSET):
            strcpy(szCode, "DDERR_NOCOOPERATIVELEVELSET");
            strcpy(
                szMsg,
                "Create function called without DirectDraw object method SetCooperativeLevel being "
                "called"
            );
            break;
        case static_cast<i32>(DDERR_NOEXCLUSIVEMODE):
            strcpy(szCode, "DDERR_NOEXCLUSIVEMODE");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOGDI):
            strcpy(szCode, "DDERR_NOGDI");
            strcpy(szMsg, "There is no GDI present");
            break;
        case static_cast<i32>(DDERR_NOMIRRORHW):
            strcpy(szCode, "DDERR_NOMIRRORHW");
            strcpy(
                szMsg,
                "Operation could not be carried out because there is no hardware present or "
                "available."
            );
            break;
        case static_cast<i32>(DDERR_NOTFOUND):
            strcpy(szCode, "DDERR_NOTFOUND");
            strcpy(szMsg, "Request item was not found");
            break;
        case static_cast<i32>(DDERR_NOOVERLAYHW):
            strcpy(szCode, "DDERR_NOOVERLAYHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NORASTEROPHW):
            strcpy(szCode, "DDERR_NORASTEROPHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOROTATIONHW):
            strcpy(szCode, "DDERR_NOROTATEHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOSTRETCHHW):
            strcpy(szCode, "DDERR_NOSTRETCHHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760140):
            strcpy(szCode, "DDERR_NOT8BITCOLOR");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOTEXTUREHW):
            strcpy(szCode, "DDERR_NOTEXTUREHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOVSYNCHW):
            strcpy(szCode, "DDERR_NOVSYNCHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOZBUFFERHW):
            strcpy(szCode, "DDERR_NOZBUFFERHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_OUTOFCAPS):
            strcpy(szCode, "DDERR_OUTOFCAPS");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_OUTOFVIDEOMEMORY):
            strcpy(szCode, "DDERR_OUTOFVIDEOMEMORY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_PALETTEBUSY):
            strcpy(szCode, "DDERR_PALETTEBUSY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_SURFACEBUSY):
            strcpy(szCode, "DDERR_SURFACEBUSY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_SURFACEISOBSCURED):
            strcpy(szCode, "DDERR_SURFACEISOBSCURED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_SURFACELOST):
            strcpy(szCode, "DDERR_SURFACELOST");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_SURFACENOTATTACHED):
            strcpy(szCode, "DDERR_SURFACENOTATTACHED");
            strcpy(szMsg, "The requested surface is not attached");
            break;
        case static_cast<i32>(DDERR_TOOBIGSIZE):
            strcpy(szCode, "DDERR_TOOBIGSIZE");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_TOOBIGWIDTH):
            strcpy(szCode, "DDERR_TOOBIGWIDTH");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_VERTICALBLANKINPROGRESS):
            strcpy(szCode, "DDERR_VERTICALBLANKINPROGRESS");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_WASSTILLDRAWING):
            strcpy(szCode, "DDERR_WASTILLDRAWING");
            strcpy(
                szMsg,
                "The previous Blt which is transfering information to or from this Surface is "
                "incomplete"
            );
            break;
        case static_cast<i32>(DDERR_NODIRECTDRAWHW):
            strcpy(szCode, "DDERR_NODIRECTDRAWHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_DIRECTDRAWALREADYCREATED):
            strcpy(szCode, "DDERR_DIRECTDRAWALREADYCREATED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_XALIGN):
            strcpy(szCode, "DDERR_XALIGN");
            strcpy(szMsg, "Rectangle provided was not horizontally aligned on a DWORD boundary");
            break;
        case static_cast<i32>(DDERR_HWNDSUBCLASSED):
            strcpy(szCode, "DDERR_HWNDSUBCLASSED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_HWNDALREADYSET):
            strcpy(szCode, "DDERR_HWNDALREADYSET");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOPALETTEHW):
            strcpy(szCode, "DDERR_NOPALETTEHW");
            strcpy(szMsg, "No hardware support for 16 or 256 color palettes");
            break;
        case static_cast<i32>(DDERR_PRIMARYSURFACEALREADYEXISTS):
            strcpy(szCode, "DDERR_PRIMARYSURFACEALREADYEXISTS");
            strcpy(szMsg, "This process already has created a primary surface");
            break;
        case static_cast<i32>(DDERR_EXCLUSIVEMODEALREADYSET):
            strcpy(szCode, "DDERR_EXCLUSIVEMODEALREADYSET");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(DDERR_NOTLOCKED):
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

RVA(0x00141cb0, 0x1)
void __cdecl DDrawLogLine(char*, ...) {}

RVA(0x00141cc0, 0x84)
CDDrawDeviceManager::CDDrawDeviceManager() : m_surfaces(0xa), m_palettes(0xa), m_displayModes() {
    m_device = NULL;
    m_directDraw1 = NULL;
    m_bankSwitchedCaps = 0;
    m_displayColorDepth = BPP_UNSET;
    m_hasPalette = false;
    m_paletteTag = 0;
    m_lastError = DDRAWERR_NONE;
}

RVA(0x00141d50, 0x6f)
CDDrawDeviceManager::~CDDrawDeviceManager() {
    Clear(1);
}

RVA(0x00141dc0, 0x224)
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
    IDirectDraw2* dd = g_DirectDraw;
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

    g_DirectDrawMgr = this;
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00141ff0, 0x6c)
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

RVA(0x00142060, 0x9d)
void CDDrawDeviceManager::Clear(i32 restoreDisplayMode) {
    if (restoreDisplayMode && m_device) {
        m_device->RestoreDisplayMode();
    }
    for (i32 i = 0; i < m_displayModes.GetSize(); i++) {
        delete static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[i]);
    }
    m_displayModes.SetSize(0, -1);
    ClearSurfaces();
    ClearPalettes();
    g_DirectDrawMgr = NULL;
    if (m_device) {
        m_device->Release();
        m_device = NULL;
    }
    if (m_directDraw1) {
        m_directDraw1->Release();
        m_directDraw1 = NULL;
    }
    m_bankSwitchedCaps = 0;
}

RVA(0x00142100, 0x18)
void CDDrawDeviceManager::RegisterSurface(CDDSurface* item) {
    item->m_pos = m_surfaces.AddTail(item);
}

RVA(0x00142120, 0x31)
void CDDrawDeviceManager::ClearSurfaces() {
    POSITION pos = m_surfaces.GetHeadPosition();
    while (pos) {
        CDDSurface* item = static_cast<CDDSurface*>(m_surfaces.GetNext(pos));
        delete item;
    }
    m_surfaces.RemoveAll();
}

RVA(0x00142160, 0x24)
void CDDrawDeviceManager::RemoveSurface(CDDSurface* item) {
    m_surfaces.RemoveAt(item->m_pos);
    delete item;
}

// @identity-TODO: owner and no-op behavior are proven; the method identity is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142190, 0x1)
void CDDrawDeviceManager::NoOpSurfacePoolHook() {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001421a0, 0xbe)
CDDSurface* CDDrawDeviceManager::CreateSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDSurface* item = new CDDSurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

RVA(0x00142260, 0xd2)
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

RVA_COMPGEN(0x00142340, 0x1e, ??_GCFileImageSurface@@UAEPAXI@Z)

RVA(0x00142360, 0x53)
CFileImageSurface::~CFileImageSurface() {}

RVA(0x001423c0, 0xd2)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001424a0, 0xbe)
CDDSurface* CDDrawDeviceManager::CreateFileSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

RVA(0x00142560, 0xc8)
CDDSurface* CDDrawDeviceManager::LoadFileSurface(char* path, i32 caps, i32 colorKey) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->LoadByExt(this, path, caps, colorKey)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142630, 0xfe)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142730, 0xc8)
CDDSurface* CDDrawDeviceManager::CreateOverlaySurface(i32 width, i32 height, i32 caps) {
    CDDrawOverlaySurface* item = new CDDrawOverlaySurface;
    if (!item->CreateOverlay(this, width, height, caps)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

RVA_COMPGEN(0x00142800, 0x1e, ??_GCDDrawOverlaySurface@@UAEPAXI@Z)
RVA(0x00142820, 0x53)
CDDrawOverlaySurface::~CDDrawOverlaySurface() {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142880, 0xbe)
CDDSurface* CDDrawDeviceManager::CreateOverlaySurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDrawOverlaySurface* item = new CDDrawOverlaySurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142940, 0xd4)
CDDSurface*
CDDrawDeviceManager::CreatePrimarySurface(i32 caps, i32 descFlags, i32 backBufferCount) {
    CDDrawPrimarySurface* item = new CDDrawPrimarySurface;
    if (!item->CreatePrimary(this, caps, descFlags, backBufferCount)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    m_displayColorDepth = item->m_bitDepth;
    return item;
}

RVA_COMPGEN(0x00142a20, 0x1e, ??_GCDDrawPrimarySurface@@UAEPAXI@Z)
RVA(0x00142a40, 0x53)
CDDrawPrimarySurface::~CDDrawPrimarySurface() {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142aa0, 0xca)
CDDSurface* CDDrawDeviceManager::CreatePrimarySurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDrawPrimarySurface* item = new CDDrawPrimarySurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    m_displayColorDepth = item->m_bitDepth;
    return item;
}

RVA(0x00142b70, 0xce)
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
    m_displayColorDepth = item->m_bitDepth;
    return item;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142c40, 0xd7)
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

RVA_COMPGEN(0x00142d20, 0x1e, ??_GCDDrawZBufferSurface@@UAEPAXI@Z)
RVA(0x00142d40, 0x53)
CDDrawZBufferSurface::~CDDrawZBufferSurface() {}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142da0, 0xbe)
CDDSurface* CDDrawDeviceManager::CreateZBufferSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDrawZBufferSurface* item = new CDDrawZBufferSurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return NULL;
    }
    RegisterSurface(item);
    return item;
}

RVA(0x00142e60, 0x27)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00142e90, 0x1d)
CDDSurface* CDDrawDeviceManager::LoadSystemMemorySurface(char* path, i32 caps, i32 colorKey) {
    return LoadFileSurface(path, caps | DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN, colorKey);
}

RVA(0x00142eb0, 0x17)
void CDDrawDeviceManager::RegisterPalette(CDDPalette* item) {
    item->m_pos = m_palettes.AddTail(item);
}

RVA(0x00142ed0, 0x3d)
void CDDrawDeviceManager::ClearPalettes() {
    POSITION pos = m_palettes.GetHeadPosition();
    while (pos) {
        CDDPalette* item = static_cast<CDDPalette*>(m_palettes.GetNext(pos));
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
    }
    m_palettes.RemoveAll();
}

RVA(0x00142f10, 0x2b)
void CDDrawDeviceManager::RemovePalette(CDDPalette* item) {
    m_palettes.RemoveAt(item->m_pos);
    if (item) {
        item->Destroy();
        ::operator delete(item);
    }
}

RVA(0x00142f40, 0x7c)
CDDPalette* CDDrawDeviceManager::LoadPaletteFromFile(char* path, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->LoadFromFile(m_device, path, flags)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return NULL;
    }
    RegisterPalette(item);
    return item;
}

RVA(0x00142fc0, 0x7c)
CDDPalette* CDDrawDeviceManager::CreateRgbPalette(u8* rgb, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateRGB(m_device, rgb, flags)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return NULL;
    }
    RegisterPalette(item);
    return item;
}

RVA(0x00143040, 0x7c)
CDDPalette* CDDrawDeviceManager::CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flags) {
    CDDPalette* item = new CDDPalette;

    if (!item->Create(m_device, entries, flags)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return NULL;
    }
    RegisterPalette(item);
    return item;
}

RVA(0x001430c0, 0x81)
CDDPalette* CDDrawDeviceManager::CreatePaletteFromTrailingData(void* data, u32 size, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateFromTrailing(m_device, data, size, flags)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return NULL;
    }
    RegisterPalette(item);
    return item;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00143150, 0xe9)
CDDPalette* CDDrawDeviceManager::LoadTrailingRgbPalette(const char* path, i32 z) {
    CFile file;
    if (!file.Open(path, CFile::modeRead, NULL)) {
        return NULL;
    }
    file.Seek(-PALETTE_RGB_BYTE_COUNT, CFile::end);
    u8 buf[PALETTE_RGB_BYTE_COUNT];
    if (file.Read(buf, PALETTE_RGB_BYTE_COUNT) != PALETTE_RGB_BYTE_COUNT) {
        return NULL;
    }
    return CreateRgbPalette(buf, z);
}

RVA(0x00143240, 0x143)
void CDDrawDeviceManager::EnumerateDisplayModes() {
    for (i32 i = 0; i < m_displayModes.GetSize(); i++) {
        delete static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[i]);
    }
    m_displayModes.SetSize(0, -1);
    g_modeArray.SetSize(0, -1);
    DdModeEnumFn modeCb;
    modeCb.m_body = DdEnumModesCallback;
    i32 hr = m_device->EnumDisplayModes(0, NULL, NULL, modeCb.m_sdk);
    if (hr != 0) {
        CDDrawDeviceManager::ReportError(DDRAWMGR_FILE, 0x507, hr);
    }

    CPtrArray* modes = &m_displayModes;
    for (i32 j = 0; j < g_modeArray.GetSize(); j++) {
        modes->SetAtGrow(modes->GetSize(), g_modeArray.GetData()[j]);
    }
    g_modeArray.SetSize(0, -1);
    i32 modeCount = m_displayModes.GetSize();
    if (modeCount > 1) {
        for (i32 firstIndex = 0; firstIndex < modeCount - 1; firstIndex++) {
            for (i32 secondIndex = firstIndex + 1; secondIndex < modeCount; secondIndex++) {

                DDSURFACEDESC* first =
                    static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[firstIndex]);
                DDSURFACEDESC* second =
                    static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[secondIndex]);
                if (ShouldSwapDisplayModes(first, second)) {
                    m_displayModes.GetData()[firstIndex] = static_cast<void*>(second);
                    m_displayModes.GetData()[secondIndex] = static_cast<void*>(first);
                }
            }
        }
    }
}

RVA(0x00143390, 0x35)
i32 __stdcall DdEnumModesCallback(DDSURFACEDESC* mode, i32 unused) {
    DDSURFACEDESC* copy = new DDSURFACEDESC;
    memcpy(copy, mode, sizeof(DDSURFACEDESC));
    g_modeArray.SetAtGrow(g_modeArray.GetSize(), copy);
    return 1;
}

RVA(0x001433d0, 0x4f)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00143420, 0x4b)
DisplayResolution
CDDrawDeviceManager::FindSmallestFittingResolution(u32 minWidth, u32 minHeight, i32 colorDepth) {
    i32 idx = FindFirstFittingResolutionIndex(minWidth, minHeight, colorDepth);
    if (idx == -1) {
        DisplayResolution none;
        none.m_width = -1;
        none.m_height = -1;
        return none;
    }
    DDSURFACEDESC* mode = static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[idx]);
    DisplayResolution resolution;
    resolution.m_width = mode->dwWidth;
    resolution.m_height = mode->dwHeight;
    return resolution;
}

RVA(0x00143470, 0x47)
i32 CDDrawDeviceManager::FindFirstFittingResolutionIndex(
    u32 minWidth,
    u32 minHeight,
    i32 colorDepth
) {
    i32 result = -1;
    for (i32 i = m_displayModes.GetSize() - 1; i >= 0; i--) {
        DDSURFACEDESC* mode = static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[i]);
        if (mode->dwWidth >= minWidth && mode->dwHeight >= minHeight
            && mode->ddpfPixelFormat.dwRGBBitCount == colorDepth) {
            result = i;
        }
    }
    return result;
}

RVA(0x001434c0, 0x45)
i32 CDDrawDeviceManager::FindResolutionIndex(i32 width, i32 height, ColorDepth colorDepth) {
    for (i32 i = 0; i < m_displayModes.GetSize(); i++) {
        DDSURFACEDESC* mode = static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[i]);
        if (mode->dwWidth == static_cast<u32>(width) && mode->dwHeight == static_cast<u32>(height)
            && mode->ddpfPixelFormat.dwRGBBitCount == IDX(colorDepth)) {
            return i;
        }
    }
    return -1;
}

RVA(0x00143510, 0x71)
DisplayResolution
CDDrawDeviceManager::FindNextResolution(i32 width, i32 height, ColorDepth colorDepth) {
    DisplayResolution resolution;
    i32 idx = FindResolutionIndex(width, height, colorDepth);
    if (idx != -1 && idx < m_displayModes.GetSize()) {
        idx++;
        if (idx < m_displayModes.GetSize()) {
            for (; idx < m_displayModes.GetSize(); idx++) {
                DDSURFACEDESC* mode = static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[idx]);
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

RVA(0x00143590, 0x7e)
DisplayResolution
CDDrawDeviceManager::FindPreviousResolution(i32 width, i32 height, ColorDepth colorDepth) {
    DisplayResolution resolution;
    i32 idx = FindResolutionIndex(width, height, colorDepth);
    if (idx != -1 && idx < m_displayModes.GetSize()) {
        idx--;
        if (idx >= 0) {
            for (; idx >= 0; idx--) {
                DDSURFACEDESC* mode = static_cast<DDSURFACEDESC*>(m_displayModes.GetData()[idx]);
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00143610, 0x1c)
DDSURFACEDESC* CDDrawDeviceManager::ResetSurfaceDesc() {
    memset(&m_surfaceDesc, 0, sizeof(m_surfaceDesc));
    m_surfaceDesc.dwSize = sizeof(m_surfaceDesc);
    return &m_surfaceDesc;
}

RVA(0x00143630, 0x10d)
CDDSurface* CDDrawDeviceManager::WrapAttachedSurface(CDDSurface* srcSurface, i32 caps) {
    IDirectDrawSurface* attached = NULL;
    DDSCAPS want;
    want.dwCaps = caps;
    i32 hr = srcSurface->m_ddSurface->GetAttachedSurface(&want, &attached);
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00143740, 0x93)
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

RVA(0x001437e0, 0xa)
void SetSurfaceRestoreHandler(SurfaceRestoreFn handler) {
    g_restoreHandler = handler;
}

RVA(0x001437f0, 0x1b)
i32 RestoreLostSurfaces() {
    if (g_restoreHandler) {
        return g_restoreHandler();
    }
    DDrawLogLine("WARNING - Surface(s) lost but no restore handler is available\n");
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00143810, 0x2b)
i32 CDDrawDeviceManager::GetAvailableVidMem(u32 caps, DWORD* total, DWORD* free) {
    DDSCAPS ddsCaps;
    ddsCaps.dwCaps = caps;
    HRESULT hr = m_device->GetAvailableVidMem(&ddsCaps, total, free);
    return hr == 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00143840, 0x32)
i32 CDDrawDeviceManager::GetFreeVidMem() {
    DDSCAPS caps;
    DWORD total;
    DWORD freeMem;
    caps.dwCaps = DDSCAPS_TEXTURE;
    i32 hr = m_device->GetAvailableVidMem(&caps, &total, &freeMem);
    return hr == 0 ? freeMem : 0;
}

RVA(0x00143880, 0x3b)
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
            g_DirectDraw = dd;
            g_ddCreateCtx = lpGuid;
            return 0;
        }
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001438c0, 0x31)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00143900, 0x4d)
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

RVA(0x00143950, 0x56)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001439b0, 0x3f)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001439f0, 0x35)
i32 CDDrawDeviceManager::SetDisplayPaletteFromTrailingRgb(u8* buf, i32 size, i32 tag) {
    if (buf == NULL) {
        return 0;
    }
    if (static_cast<u32>(size) < 0x3e8) {
        return 0;
    }
    return SetDisplayPaletteFromRgb(buf + size - PALETTE_RGB_BYTE_COUNT, tag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00143a30, 0xe9)
i32 CDDrawDeviceManager::LoadDisplayPaletteFromFile(const char* path, i32 z) {
    CFile file;
    if (!file.Open(path, CFile::modeRead, NULL)) {
        return 0;
    }
    file.Seek(-PALETTE_RGB_BYTE_COUNT, CFile::end);
    u8 buf[PALETTE_RGB_BYTE_COUNT];
    if (file.Read(buf, PALETTE_RGB_BYTE_COUNT) != PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    return SetDisplayPaletteFromRgb(buf, z);
}

RVA(0x00143b20, 0xfc)
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

RVA(0x00143c20, 0x84)
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
        hr = static_cast<i32>(0x80004005);
        if (m_lastError == DDRAWERR_NONE) {
            m_lastError = DDRAWERR_COLOR_MASKS;
        }
    }
    return hr;
}

RVA(0x00143cb0, 0x6)
DDSurfacePoolKind CDDrawOverlaySurface::GetPoolKind() {
    return POOLKIND_OVERLAY;
}

RVA(0x00143cc0, 0x6)
DDSurfacePoolKind CFileImageSurface::GetPoolKind() {
    return POOLKIND_FILEIMAGE;
}

RVA(0x00143cd0, 0x6)
DDSurfacePoolKind CDDrawPrimarySurface::GetPoolKind() {
    return POOLKIND_MODE;
}

RVA(0x00143ce0, 0x6)
DDSurfacePoolKind CDDrawZBufferSurface::GetPoolKind() {
    return POOLKIND_ZBUFFER;
}
