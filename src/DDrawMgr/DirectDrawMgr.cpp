#include <rva.h>

#include <DDrawMgr/DirectDrawMgr.h>

#include <AddrWord.h>
#include <ComOutRef.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/PixelShift.h>
#include <Dsndmgr/SoundBankLoad.h>
#include <EmptyString.h>
#include <Image/Image.h>
#include <Io/FileStream.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

#define DDRAWMGR_FILE "C:\\Proj\\DDrawMgr\\DDRAWMGR.CPP"
#define DDRAWMGR_H_FILE "C:\\Proj\\DDrawMgr\\ddrawmgr.h"

VTBL(CFileImageSurface, 0x001efa58);
VTBL(CPoolItemA88, 0x001efa88);
VTBL(CPoolItemAB8, 0x001efab8);
VTBL(CPoolItemAE8, 0x001efae8);
DATA(0x002bed00)
CDDrawPtrCollections* g_DirectDrawMgr = 0;

DATA(0x00283eb8)
i32 g_ddLogEnabled = 0;
DATA(0x00283ebc)
i32 g_ddMsgBoxEnabled = 0;
DATA(0x00283ec0)
i32 g_ddBeepEnabled = 0;
DATA(0x00283ec4)
i32 g_ddThirdEnabled = 0;

DATA(0x00283edc)
i32 (*g_restoreHandler)() = 0;

DATA(0x00283ee8)
IDirectDraw2* g_DirectDraw = 0;
DATA(0x00283ec8)
CPtrArray g_modeArray;
DATA(0x00283ee4)
void* g_ddCreateCtx = 0;

RVA(0x001413d0, 0x27)
void SetDDrawReportModes(i32 log, i32 msgBox, i32 beep, i32 third) {
    g_ddLogEnabled = log;
    g_ddMsgBoxEnabled = msgBox;
    g_ddBeepEnabled = beep;
    g_ddThirdEnabled = third;
}

RVA(0x00141400, 0x870)
void CDDrawPtrCollections::GetErrorString(char* file, i32 line, i32 hr) {
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
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case static_cast<i32>(0x80004001):
            strcpy(szCode, "DDERR_UNSUPPORTED");
            strcpy(szMsg, "Action not supported");
            break;
        case static_cast<i32>(0x80004005):
            strcpy(szCode, "DDERR_GENERIC");
            strcpy(szMsg, "Generic failure");
            break;
        case static_cast<i32>(0x8007000e):
            strcpy(szCode, "DDERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x80070057):
            strcpy(szCode, "DDERR_INVALIDPARAMS");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760064):
            strcpy(szCode, "DDERR_INVALIDCAPS");
            strcpy(szMsg, "One or more of the caps bits passed to the callback are incorrect");
            break;
        case static_cast<i32>(0x88760078):
            strcpy(szCode, "DDERR_INVALIDMODE");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760082):
            strcpy(szCode, "DDERR_INVALIDOBJECT");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760091):
            strcpy(szCode, "DDERR_INVALIDPIXELFORMAT");
            strcpy(szMsg, "Pixel format was invalid as specified.");
            break;
        case static_cast<i32>(0x88760096):
            strcpy(szCode, "DDERR_INVALIDRECT");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887600a0):
            strcpy(szCode, "DDERR_LOCKEDSURFACES");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887600aa):
            strcpy(szCode, "DDERR_NO3D");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887600b4):
            strcpy(szCode, "DDERR_NOALPHAHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887600d2):
            strcpy(szCode, "DDERR_NOCOLORCONVHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887600d4):
            strcpy(szCode, "DDERR_NOCOOPERATIVELEVELSET");
            strcpy(
                szMsg,
                "Create function called without DirectDraw object method SetCooperativeLevel being "
                "called"
            );
            break;
        case static_cast<i32>(0x887600e1):
            strcpy(szCode, "DDERR_NOEXCLUSIVEMODE");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887600f0):
            strcpy(szCode, "DDERR_NOGDI");
            strcpy(szMsg, "There is no GDI present");
            break;
        case static_cast<i32>(0x887600fa):
            strcpy(szCode, "DDERR_NOMIRRORHW");
            strcpy(
                szMsg,
                "Operation could not be carried out because there is no hardware present or "
                "available."
            );
            break;
        case static_cast<i32>(0x887600ff):
            strcpy(szCode, "DDERR_NOTFOUND");
            strcpy(szMsg, "Request item was not found");
            break;
        case static_cast<i32>(0x88760104):
            strcpy(szCode, "DDERR_NOOVERLAYHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760118):
            strcpy(szCode, "DDERR_NORASTEROPHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760122):
            strcpy(szCode, "DDERR_NOROTATEHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760136):
            strcpy(szCode, "DDERR_NOSTRETCHHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760140):
            strcpy(szCode, "DDERR_NOT8BITCOLOR");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8876014a):
            strcpy(szCode, "DDERR_NOTEXTUREHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8876014f):
            strcpy(szCode, "DDERR_NOVSYNCHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760154):
            strcpy(szCode, "DDERR_NOZBUFFERHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760168):
            strcpy(szCode, "DDERR_OUTOFCAPS");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8876017c):
            strcpy(szCode, "DDERR_OUTOFVIDEOMEMORY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760183):
            strcpy(szCode, "DDERR_PALETTEBUSY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887601ae):
            strcpy(szCode, "DDERR_SURFACEBUSY");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887601b8):
            strcpy(szCode, "DDERR_SURFACEISOBSCURED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887601c2):
            strcpy(szCode, "DDERR_SURFACELOST");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887601cc):
            strcpy(szCode, "DDERR_SURFACENOTATTACHED");
            strcpy(szMsg, "The requested surface is not attached");
            break;
        case static_cast<i32>(0x887601e0):
            strcpy(szCode, "DDERR_TOOBIGSIZE");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x887601ea):
            strcpy(szCode, "DDERR_TOOBIGWIDTH");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760219):
            strcpy(szCode, "DDERR_VERTICALBLANKINPROGRESS");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8876021c):
            strcpy(szCode, "DDERR_WASTILLDRAWING");
            strcpy(
                szMsg,
                "The previous Blt which is transfering information to or from this Surface is "
                "incomplete"
            );
            break;
        case static_cast<i32>(0x88760233):
            strcpy(szCode, "DDERR_NODIRECTDRAWHW");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760232):
            strcpy(szCode, "DDERR_DIRECTDRAWALREADYCREATED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760230):
            strcpy(szCode, "DDERR_XALIGN");
            strcpy(szMsg, "Rectangle provided was not horizontally aligned on a DWORD boundary");
            break;
        case static_cast<i32>(0x8876023a):
            strcpy(szCode, "DDERR_HWNDSUBCLASSED");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8876023b):
            strcpy(szCode, "DDERR_HWNDALREADYSET");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x8876023d):
            strcpy(szCode, "DDERR_NOPALETTEHW");
            strcpy(szMsg, "No hardware support for 16 or 256 color palettes");
            break;
        case static_cast<i32>(0x88760234):
            strcpy(szCode, "DDERR_PRIMARYSURFACEALREADYEXISTS");
            strcpy(szMsg, "This process already has created a primary surface");
            break;
        case static_cast<i32>(0x88760245):
            strcpy(szCode, "DDERR_EXCLUSIVEMODEALREADYSET");
            strcpy(szMsg, "No message");
            break;
        case static_cast<i32>(0x88760248):
            strcpy(szCode, "DDERR_LOCKEDSURFACES");
            strcpy(szMsg, "No message");
            break;
        case 0:
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
CDDrawPtrCollections::CDDrawPtrCollections() : m_poolA(0xa), m_poolB(0xa), m_poolItems() {
    m_device = NULL;
    m_directDraw1 = NULL;
    m_bltCaps = 0;
    m_palBpp = 0;
    m_hasPalette = 0;
    m_paletteTag = 0;
    m_lastError = 0;
}

RVA(0x00141d50, 0x6f)
CDDrawPtrCollections::~CDDrawPtrCollections() {
    Clear(1);
}

RVA(0x00141dc0, 0x224)
i32 CDDrawPtrCollections::CreateDevice(
    void* hwnd,
    void* driverGuid,
    i32 width,
    i32 height,
    i32 bpp,
    u32 coopFlags
) {
    m_hasPalette = 0;
    m_paletteTag = 0;
    IDirectDraw2* dd = g_DirectDraw;
    if (dd != NULL) {
        m_device = dd;
    } else {
        i32 chr = DirectDrawCreate(static_cast<GUID*>(driverGuid), &m_directDraw1, 0);
        if (chr != 0) {
            CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0x88, chr);
            if (m_lastError == 0) {
                m_lastError = 0x3e9;
            }
            return 0;
        }
        ComOutRef<IDirectDraw2> devOut;
        devOut.m_asTyped = &m_device;
        chr = m_directDraw1->QueryInterface(IID_IDirectDraw2, devOut.m_asVoid);
        if (chr != 0) {
            CDDrawPtrCollections::GetErrorString(0, 0, chr);
            if (m_lastError == 0) {
                m_lastError = 0x3ef;
            }
            return 0;
        }
    }

    i32 hr = m_device->SetCooperativeLevel(static_cast<HWND>(hwnd), coopFlags);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DDRAWMGR_H_FILE, 0x120, hr);
    }
    if (hr != 0) {
        if (m_lastError == 0) {
            m_lastError = 0x3ea;
        }
        return 0;
    }

    memset(&m_driverCaps, 0, sizeof(m_driverCaps));
    memset(&m_helCaps, 0, sizeof(m_helCaps));
    m_driverCaps.dwSize = sizeof(DDCAPS);
    m_helCaps.dwSize = sizeof(DDCAPS);
    hr = m_device->GetCaps(&m_driverCaps, &m_helCaps);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0xad, hr);
    }
    m_bltCaps = m_driverCaps.dwCaps & 0x8000000;
    SetupCaps();

    if (width > 0 && height > 0) {
        hr = ConfigureSurface(width, height, bpp, 0, 0);
        if (hr != 0) {
            CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0xc2, hr);
            if (m_lastError == 0) {
                m_lastError = 0x3ec;
            }
            return 0;
        }
        m_palBpp = bpp;
    }

    if (bpp == 0) {
        DDSURFACEDESC desc;
        memset(&desc, 0, sizeof(desc));
        desc.dwSize = 0x6c;
        hr = m_device->GetDisplayMode(&desc);
        if (hr == 0) {
            m_palBpp = desc.ddpfPixelFormat.dwRGBBitCount;
        }
    }

    g_DirectDrawMgr = this;
    return 1;
}

RVA(0x00141ff0, 0x6c)
i32 CDDrawPtrCollections::Init(
    void* factory,
    void* hwnd,
    i32 width,
    i32 height,
    i32 bpp,
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
        CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0xf4, hr);
        return 0;
    }
    return CreateDevice(hwnd, g_ddCreateCtx, width, height, bpp, coop);
}

RVA(0x00142060, 0x9d)
void CDDrawPtrCollections::Clear(i32 mode) {
    if (mode && m_device) {
        m_device->RestoreDisplayMode();
    }
    for (i32 i = 0; i < m_poolItems.GetSize(); i++) {
        ::operator delete(m_poolItems.GetData()[i]);
    }
    m_poolItems.SetSize(0, -1);
    EmptyPoolA();
    EmptyPoolB();
    g_DirectDrawMgr = NULL;
    if (m_device) {
        m_device->Release();
        m_device = NULL;
    }
    if (m_directDraw1) {
        m_directDraw1->Release();
        m_directDraw1 = NULL;
    }
    m_bltCaps = 0;
}

RVA(0x00142100, 0x18)
void CDDrawPtrCollections::AddItemA(CDDSurface* item) {
    item->m_pos = m_poolA.AddTail(item);
}

RVA(0x00142120, 0x31)
void CDDrawPtrCollections::EmptyPoolA() {
    POSITION pos = m_poolA.GetHeadPosition();
    while (pos) {
        CDDSurface* item = static_cast<CDDSurface*>(m_poolA.GetNext(pos));
        delete item;
    }
    m_poolA.RemoveAll();
}

RVA(0x00142160, 0x24)
void CDDrawPtrCollections::RemoveItemA(CDDSurface* item) {
    m_poolA.RemoveAt(item->m_pos);
    delete item;
}

RVA(0x001421a0, 0xbe)
CDDSurface* CDDrawPtrCollections::CreateSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CDDSurface* item = new CDDSurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x00142260, 0xd2)
CDDSurface* CDDrawPtrCollections::LoadSurfaceFromPid(
    PidHeader* hdr,
    FileImageFormat type,
    u32 size,
    i32 ctrl,
    i32 trans
) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->ResolveEx(this, hdr, type, size, ctrl, trans)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA_COMPGEN(0x00142340, 0x1e, ??_GCFileImageSurface@@UAEPAXI@Z)

RVA(0x00142360, 0x53)
CFileImageSurface::~CFileImageSurface() {}

RVA(0x001423c0, 0xd2)
CDDSurface*
CDDrawPtrCollections::CreateKeyedSurface(i32 width, i32 height, i32 bitDepth, i32 caps, i32 key) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->LoadKeyed(this, width, height, bitDepth, caps, key)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x001424a0, 0xbe)
CDDSurface* CDDrawPtrCollections::CreateFileSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x00142560, 0xc8)
CDDSurface* CDDrawPtrCollections::LoadFileSurface(char* path, i32 caps, i32 colorKey) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->LoadByExt(this, path, caps, colorKey)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

// @early-stop
RVA(0x00142630, 0xfe)
i32 CDDrawPtrCollections::CreateRange(
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
    CDDSurface** p = out;
    for (i32 i = start; i < end; i++) {
        char buf[32];
        sprintf(buf, "%s%i", baseName, i);
        if (suffix != NULL) {
            if (suffix[0] != '.') {
                strcpy(buf, g_dot);
            }
            strcat(buf, suffix);
        }
        CDDSurface* item = LoadFileSurface(buf, caps, colorKey);
        if (item == NULL) {
            break;
        }
        *p++ = item;
        n++;
    }
    return n;
}

RVA(0x00142730, 0xc8)
CDDSurface* CDDrawPtrCollections::CreateBlit7Surface(i32 a, i32 b, i32 c) {
    CPoolItemA88* item = new CPoolItemA88;
    if (!item->Blit7(this, a, b, c)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA_COMPGEN(0x00142800, 0x1e, ??_GCPoolItemA88@@UAEPAXI@Z)
RVA(0x00142820, 0x53)
CPoolItemA88::~CPoolItemA88() {}

RVA(0x00142880, 0xbe)
CDDSurface* CDDrawPtrCollections::CreateBlit7SurfaceFromDesc(const DDSURFACEDESC* desc) {
    CPoolItemA88* item = new CPoolItemA88;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x00142940, 0xd4)
CDDSurface* CDDrawPtrCollections::CreatePaletteSurface(i32 a, i32 b, i32 c) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (!item->Setup(this, a, b, c)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    m_palBpp = item->m_bitDepth;
    return item;
}

RVA_COMPGEN(0x00142a20, 0x1e, ??_GCPoolItemAB8@@UAEPAXI@Z)
RVA(0x00142a40, 0x53)
CPoolItemAB8::~CPoolItemAB8() {}

RVA(0x00142aa0, 0xca)
CDDSurface* CDDrawPtrCollections::CreatePaletteSurfaceFromDesc(const DDSURFACEDESC* desc) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    m_palBpp = item->m_bitDepth;
    return item;
}

RVA(0x00142b70, 0xce)
CDDSurface* CDDrawPtrCollections::Create24BitPaletteSurface(i32 a) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (!item->Setup(this, 0x18, 0x21, a)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    m_palBpp = item->m_bitDepth;
    return item;
}

RVA(0x00142c40, 0xd7)
CDDSurface* CDDrawPtrCollections::CreateBlit47Surface(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    CPoolItemAE8* item = new CPoolItemAE8;
    if (!item->Blit47(this, a, b, c, d, e, f)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA_COMPGEN(0x00142d20, 0x1e, ??_GCPoolItemAE8@@UAEPAXI@Z)
RVA(0x00142d40, 0x53)
CPoolItemAE8::~CPoolItemAE8() {}

RVA(0x00142da0, 0xbe)
CDDSurface* CDDrawPtrCollections::CreateBlit47SurfaceFromDesc(const DDSURFACEDESC* desc) {
    CPoolItemAE8* item = new CPoolItemAE8;
    if (!item->CreateFromDesc(this, desc)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x00142e60, 0x27)
CDDSurface*
CDDrawPtrCollections::MakeAndAddB(i32 width, i32 height, i32 bitDepth, i32 caps, i32 key) {
    return CreateKeyedSurface(width, height, bitDepth, caps | 0x840, key);
}

RVA(0x00142eb0, 0x17)
void CDDrawPtrCollections::AddItemB(CDDPalette* item) {
    item->m_pos = m_poolB.AddTail(item);
}

RVA(0x00142ed0, 0x3d)
void CDDrawPtrCollections::EmptyPoolB() {
    POSITION pos = m_poolB.GetHeadPosition();
    while (pos) {
        CDDPalette* item = static_cast<CDDPalette*>(m_poolB.GetNext(pos));
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
    }
    m_poolB.RemoveAll();
}

RVA(0x00142f10, 0x2b)
void CDDrawPtrCollections::RemoveItemB(CDDPalette* item) {
    m_poolB.RemoveAt(item->m_pos);
    if (item) {
        item->Destroy();
        ::operator delete(item);
    }
}

RVA(0x00142f40, 0x7c)
CDDPalette* CDDrawPtrCollections::LoadPaletteFromFile(char* path, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->LoadFromFile(m_device, path, flags)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return 0;
    }
    AddItemB(item);
    return item;
}

RVA(0x00142fc0, 0x7c)
CDDPalette* CDDrawPtrCollections::CreateRgbPalette(void* rgb, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateRGB(m_device, rgb, flags)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return 0;
    }
    AddItemB(item);
    return item;
}

RVA(0x00143040, 0x7c)
CDDPalette* CDDrawPtrCollections::CreatePaletteFromEntries(i32 a, i32 b) {
    CDDPalette* item = new CDDPalette;

    AddrWord<PALETTEENTRY> entries;
    entries.m_word = a;
    if (!item->Create(m_device, entries.m_addr, b)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return 0;
    }
    AddItemB(item);
    return item;
}

RVA(0x001430c0, 0x81)
CDDPalette* CDDrawPtrCollections::CreatePaletteFromTrailingData(void* a, u32 b, i32 c) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateFromTrailing(m_device, a, b, c)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return 0;
    }
    AddItemB(item);
    return item;
}

RVA(0x00143150, 0xe9)
CDDPalette* CDDrawPtrCollections::LoadTrailingRgbPalette(const char* path, i32 z) {
    CFile file;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    char buf[0x300];
    if (file.Read(buf, 0x300) != 0x300) {
        return 0;
    }
    return CreateRgbPalette(buf, z);
}

// @early-stop
RVA(0x00143240, 0x143)
void CDDrawPtrCollections::SetupCaps() {
    for (i32 i = 0; i < m_poolItems.GetSize(); i++) {
        ::operator delete(m_poolItems.GetData()[i]);
    }
    m_poolItems.SetSize(0, -1);
    g_modeArray.SetSize(0, -1);
    DdModeEnumFn modeCb;
    modeCb.m_body = DdEnumModesCallback;
    i32 hr = m_device->EnumDisplayModes(0, 0, 0, modeCb.m_sdk);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0x507, hr);
    }

    CPtrArray* items = &m_poolItems;
    for (i32 j = 0; j < g_modeArray.GetSize(); j++) {
        items->SetAtGrow(items->GetSize(), g_modeArray.GetData()[j]);
    }
    g_modeArray.SetSize(0, -1);
    i32 n = m_poolItems.GetSize();
    for (i32 a = 0; a < n - 1; a++) {
        for (i32 b = a + 1; b < n; b++) {

            void* pa = m_poolItems.GetData()[a];
            void* pb = m_poolItems.GetData()[b];
            if (Compare(pa, pb)) {
                m_poolItems.GetData()[a] = pb;
                m_poolItems.GetData()[b] = pa;
            }
        }
    }
}

RVA(0x00143390, 0x35)
i32 __stdcall DdEnumModesCallback(void* mode, i32 unused) {
    void* rec = operator new(0x6c);
    memcpy(rec, mode, 0x6c);
    g_modeArray.SetAtGrow(g_modeArray.GetSize(), rec);
    return 1;
}

RVA(0x001433d0, 0x4f)
i32 __stdcall CDDrawPtrCollections::Compare(void* pa, void* pb) {
    DDSURFACEDESC* a = static_cast<DDSURFACEDESC*>(pa);
    DDSURFACEDESC* b = static_cast<DDSURFACEDESC*>(pb);
    if (a->dwWidth > b->dwWidth) {
        return 1;
    }
    if (a->dwWidth < b->dwWidth) {
        return 0;
    }
    if (a->dwHeight > b->dwHeight) {
        return 1;
    }
    if (a->dwHeight < b->dwHeight) {
        return 0;
    }
    return a->ddpfPixelFormat.dwRGBBitCount > b->ddpfPixelFormat.dwRGBBitCount;
}

RVA(0x00143420, 0x4b)
CDdModePair CDDrawPtrCollections::FindMatch(u32 k0, u32 k1, i32 k2) {
    i32 idx = FindLast(k0, k1, k2);
    if (idx == -1) {
        CDdModePair none;
        none.a = -1;
        none.b = -1;
        return none;
    }
    DDSURFACEDESC* e = static_cast<DDSURFACEDESC*>(m_poolItems.GetData()[idx]);
    CDdModePair r;
    r.a = e->dwWidth;
    r.b = e->dwHeight;
    return r;
}

RVA(0x00143470, 0x47)
i32 CDDrawPtrCollections::FindLast(u32 k0, u32 k1, i32 k2) {
    i32 r = -1;
    for (i32 i = m_poolItems.GetSize() - 1; i >= 0; i--) {
        DDSURFACEDESC* e = static_cast<DDSURFACEDESC*>(m_poolItems.GetData()[i]);
        if (e->dwWidth >= k0 && e->dwHeight >= k1 && e->ddpfPixelFormat.dwRGBBitCount == k2) {
            r = i;
        }
    }
    return r;
}

RVA(0x001434c0, 0x45)
i32 CDDrawPtrCollections::FindIndex(i32 k0, i32 k1, i32 k2) {
    for (i32 i = 0; i < m_poolItems.GetSize(); i++) {
        DDSURFACEDESC* e = static_cast<DDSURFACEDESC*>(m_poolItems.GetData()[i]);
        if (e->dwWidth == static_cast<u32>(k0) && e->dwHeight == static_cast<u32>(k1)
            && e->ddpfPixelFormat.dwRGBBitCount == k2) {
            return i;
        }
    }
    return -1;
}

RVA(0x00143510, 0x71)
CDdModePair CDDrawPtrCollections::FindFwd(i32 k0, i32 k1, i32 k2) {
    CDdModePair r;
    i32 idx = FindIndex(k0, k1, k2);
    if (idx != -1 && idx < m_poolItems.GetSize()) {
        idx++;
        if (idx < m_poolItems.GetSize()) {
            for (; idx < m_poolItems.GetSize(); idx++) {
                DDSURFACEDESC* e = static_cast<DDSURFACEDESC*>(m_poolItems.GetData()[idx]);
                if (e->ddpfPixelFormat.dwRGBBitCount == k2) {
                    r.a = e->dwWidth;
                    r.b = e->dwHeight;
                    return r;
                }
            }
        }
    }
    r.a = -1;
    r.b = -1;
    return r;
}

RVA(0x00143590, 0x7e)
CDdModePair CDDrawPtrCollections::FindBack(i32 k0, i32 k1, i32 k2) {
    CDdModePair r;
    i32 idx = FindIndex(k0, k1, k2);
    if (idx != -1 && idx < m_poolItems.GetSize()) {
        idx--;
        if (idx >= 0) {
            for (; idx >= 0; idx--) {
                DDSURFACEDESC* e = static_cast<DDSURFACEDESC*>(m_poolItems.GetData()[idx]);
                if (e->ddpfPixelFormat.dwRGBBitCount == k2) {
                    r.a = e->dwWidth;
                    r.b = e->dwHeight;
                    return r;
                }
            }
        }
    }
    r.a = -1;
    r.b = -1;
    return r;
}

RVA(0x00143630, 0x10d)
void* CDDrawPtrCollections::CreatePoolItem(void* srcSurfacev, i32 caps) {
    CDDSurface* srcSurface = static_cast<CDDSurface*>(srcSurfacev);
    IDirectDrawSurface* attached = 0;
    DDSCAPS want;
    want.dwCaps = caps;
    i32 hr = srcSurface->m_ddSurface->GetAttachedSurface(&want, &attached);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0x6ae, hr);
        return 0;
    }

    CDDSurface* item = new CDDSurface;
    if (item->Refresh(attached) == 0) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x00143740, 0x93)
i32 CDDrawPtrCollections::GetDisplayMode(i32* pWidth, i32* pHeight, i32* pBpp) {
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = 0x6c;
    i32 hr = m_device->GetDisplayMode(&desc);
    if (hr != 0) {
        *pWidth = 0;
        *pHeight = 0;
        *pBpp = 0;
        CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0x6e5, hr);
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

RVA(0x00143810, 0x2b)
i32 CDDrawPtrCollections::GetAvailableVidMem(u32 caps, DWORD* total, DWORD* free) {
    DDSCAPS ddsCaps;
    ddsCaps.dwCaps = caps;
    HRESULT hr = m_device->GetAvailableVidMem(&ddsCaps, total, free);
    return hr == 0;
}

RVA(0x00143840, 0x32)
i32 CDDrawPtrCollections::GetFreeVidMem() {
    DDSCAPS caps;
    DWORD total;
    DWORD freeMem;
    caps.dwCaps = 0x1000;
    i32 hr = m_device->GetAvailableVidMem(&caps, &total, &freeMem);
    return hr == 0 ? freeMem : 0;
}

RVA(0x00143880, 0x3b)
i32 __stdcall

CreateDirectDrawVia(
    void* ctx,
    i32 driverDesc,
    i32 driverName,
    IDirectDraw2*(__cdecl* factory)(void*, i32, i32)
) {
    if (factory != NULL) {
        IDirectDraw2* dd = factory(ctx, driverDesc, driverName);
        if (dd != NULL) {
            g_DirectDraw = dd;
            g_ddCreateCtx = ctx;
            return 0;
        }
    }
    return 1;
}

RVA(0x001438c0, 0x31)
IDirectDrawSurface* CDDrawPtrCollections::GetGDISurface() {
    IDirectDrawSurface* surf = 0;
    i32 hr = m_device->GetGDISurface(&surf);
    if (hr != 0) {
        DDrawLogLine(const_cast<char*>("CDDrawPtrCollections::GetGDISurface()"));
        return 0;
    }
    return surf;
}

RVA(0x00143900, 0x4d)
i32 CDDrawPtrCollections::SetDisplayPaletteFrom(CDDPalette* pal, i32 tag) {

    if (pal == NULL) {
        return 0;
    }
    PALETTEENTRY* src = pal->m_cacheA;
    if (src == NULL) {
        return 0;
    }
    PALETTEENTRY* dst = m_palette;
    for (i32 i = 0; i < 256; i++) {
        *dst++ = *src++;
    }
    m_hasPalette = 1;
    m_paletteTag = tag;
    return 1;
}

RVA(0x00143950, 0x56)
CDDPalette* CDDrawPtrCollections::SetDisplayPaletteFromRgb(void* buf, i32 z) {
    if (buf == NULL) {
        return 0;
    }
    const u8* src = static_cast<const u8*>(buf);
    for (i32 i = 0; i < 256; i++) {
        m_palette[i].peRed = *src++;
        m_palette[i].peGreen = *src++;
        m_palette[i].peBlue = *src++;
        m_palette[i].peFlags = 0;
    }
    m_hasPalette = 1;
    m_paletteTag = z;

    AddrWord<CDDPalette> ok;
    ok.m_word = 1;
    return ok.m_addr;
}

RVA(0x001439b0, 0x3f)
i32 CDDrawPtrCollections::SetDisplayPaletteDirect(PALETTEENTRY* entries, i32 tag) {
    if (entries == NULL) {
        return 0;
    }
    PALETTEENTRY* src = entries;
    for (i32 i = 0; i < 256; i++) {
        m_palette[i] = *src++;
    }
    m_hasPalette = 1;
    m_paletteTag = tag;
    return 1;
}

RVA(0x001439f0, 0x35)
CDDPalette* CDDrawPtrCollections::SetDisplayPaletteFromTrailingRgb(u8* buf, i32 size, i32 tag) {
    if (buf == NULL) {
        return 0;
    }
    if (static_cast<u32>(size) < 0x3e8) {
        return 0;
    }
    return SetDisplayPaletteFromRgb(buf + size - 0x300, tag);
}

RVA(0x00143a30, 0xe9)
CDDPalette* CDDrawPtrCollections::LoadDisplayPaletteFromFile(const char* path, i32 z) {
    CFile file;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    char buf[0x300];
    if (file.Read(buf, 0x300) != 0x300) {
        return 0;
    }
    return SetDisplayPaletteFromRgb(buf, z);
}

RVA(0x00143b20, 0xfc)
i32 CDDrawPtrCollections::ComputeColorMasks() {
    DDSURFACEDESC desc;
    memset(&desc, 0, 0x6c);
    desc.dwSize = 0x6c;
    i32 hr = m_device->GetDisplayMode(&desc);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0x82c, hr);
        return 0;
    }

    u32 m = desc.ddpfPixelFormat.dwRBitMask;
    i32 count = 0;
    i32 shift = -1;
    for (i32 b = 0; b < 0x20; b++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b;
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
    for (i32 b2 = 0; b2 < 0x20; b2++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b2;
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
    for (i32 b3 = 0; b3 < 0x20; b3++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b3;
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
i32 CDDrawPtrCollections::ConfigureSurface(
    i32 width,
    i32 height,
    i32 bpp,
    i32 refreshRate,
    i32 flags
) {
    i32 hr = m_device->SetDisplayMode(width, height, bpp, refreshRate, flags);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DDRAWMGR_FILE, 0x8a2, hr);
        if (m_lastError == 0) {
            m_lastError = 0x3ec;
        }
        return hr;
    }
    if (ComputeColorMasks() == 0) {
        hr = static_cast<i32>(0x80004005);
        if (m_lastError == 0) {
            m_lastError = 0x3ed;
        }
    }
    return hr;
}

RVA(0x00143cb0, 0x6)
DDSurfacePoolKind CPoolItemA88::GetPoolKind() {
    return POOLKIND_BLIT7;
}

RVA(0x00143cc0, 0x6)
DDSurfacePoolKind CFileImageSurface::GetPoolKind() {
    return POOLKIND_FILEIMAGE;
}

RVA(0x00143cd0, 0x6)
DDSurfacePoolKind CPoolItemAB8::GetPoolKind() {
    return POOLKIND_MODE;
}

RVA(0x00143ce0, 0x6)
DDSurfacePoolKind CPoolItemAE8::GetPoolKind() {
    return POOLKIND_BLIT47;
}
