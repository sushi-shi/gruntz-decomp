#include <Io/FileStream.h>
#include <EmptyString.h>         // g_emptyString
#include <DDrawMgr/PixelShift.h> // g_rUp/g_gUp/g_bUp/g_rDown/g_gDown/g_bDown

#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/DDrawPtrCollections.h> // the pool half of this obj's manager (+ CPoolItem*)
#include <Image/Image.h>                  // CFileImageSurface (the a58 pool item's dtor pair)
#include <ddraw.h> // real DirectDraw SDK (IDirectDraw/2, DirectDrawCreate, DirectDrawEnumerateA, DDCAPS, IID_IDirectDraw2)
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy / memcpy / memset (rep stos)
#include <Globals.h>

#define DDRAWMGR_FILE "C:\\Proj\\DDrawMgr\\DDRAWMGR.CPP"
#define DDRAWMGR_H_FILE "C:\\Proj\\DDrawMgr\\ddrawmgr.h"

extern "C" {
    DATA(0x002bed00)
    CDirectDrawMgr* g_DirectDrawMgr = 0; // 0x6bed00
}

extern "C" {
    DATA(0x00283ec0)
    i32 g_ddBeepEnabled = 0; // 0x683ec0
    DATA(0x00283eb8)
    i32 g_ddLogEnabled = 0; // 0x683eb8
    DATA(0x00283ebc)
    i32 g_ddMsgBoxEnabled = 0; // 0x683ebc
    DATA(0x00283ec4)
    i32 g_ddThirdEnabled = 0; // 0x683ec4
}

DATA(0x00283edc)
i32 (*g_restoreHandler)() = 0; // 0x683edc

DATA(0x001ef848)
extern "C" const GUID IID_IDirectDraw2; // 0x5ef848

extern "C" {
    DATA(0x00283ee8)
    IDirectDraw2* g_DirectDraw = 0; // 0x683ee8
}
DATA(0x00283ec8)
CPtrArray g_modeArray;
DATA(0x00283ee4)
void* g_ddCreateCtx = 0; // 0x683ee4

void BuildColorChannelTables();

void operator delete(void*);

inline CDDSurface::CDDSurface() {
    m_ddSurface = 0;
    m_ddSurfaceBack = 0;
    m_pos = 0;
    m_dontOwn = 0;
    m_bitDepth = 0;
    m_b8 = 0;
}
inline CDDSurface::~CDDSurface() {
    FreeSurfaces();
}

RVA(0x001413d0, 0x27)
void SetDDrawReportModes(i32 log, i32 msgBox, i32 beep, i32 third) {
    g_ddLogEnabled = log;
    g_ddMsgBoxEnabled = msgBox;
    g_ddBeepEnabled = beep;
    g_ddThirdEnabled = third;
}

RVA(0x00141400, 0x835)
void CDirectDrawMgr::GetErrorString(char* file, i32 line, i32 hr) {
    char szCode[64];  // local_340 - error-code name
    char szMsg[256];  // local_300 - description
    char szLine[512]; // local_200 - formatted output line

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
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
        DDrawLogLine(szLine);
    }
    if (g_ddMsgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA(static_cast<HWND>(0), szLine, "DirectDrawMgr", MB_ICONEXCLAMATION);
    }
}

RVA(0x00141c80, 0xa)
void ClearModeArray_141c80() {
    g_modeArray.CPtrArray::CPtrArray();
}

RVA(0x00141cb0, 0x1)
void __cdecl DDrawLogLine(char*, ...) {}

RVA(0x00141cc0, 0x84)
CDDrawPtrCollections::CDDrawPtrCollections() : m_poolA(0xa), m_poolB(0xa), m_poolItems() {
    m_device = 0;
    m_dd1 = 0;
    m_bltCaps = 0;
    m_palBpp = 0;
    m_hasPalette = 0;
    m_940 = 0;
    m_lastError = 0;
}

RVA(0x00141d50, 0x6f)
CDDrawPtrCollections::~CDDrawPtrCollections() {
    Clear(1);
}

RVA(0x00141dc0, 0x224)
i32 CDirectDrawMgr::CreateDevice(
    void* a1,
    void* hwnd,
    i32 width,
    i32 height,
    i32 bpp,
    u32 coopFlags
) {
    m_hasPalette = 0;
    m_940 = 0;
    IDirectDraw2* dd = g_DirectDraw;
    if (dd != 0) {
        m_device = dd;
    } else {
        i32 chr = DirectDrawCreate(static_cast<GUID*>(hwnd), &m_dd1, 0);
        if (chr != 0) {
            CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x88, chr);
            if (m_lastError == 0) {
                m_lastError = 0x3e9;
            }
            return 0;
        }
        chr = m_dd1->QueryInterface(IID_IDirectDraw2, reinterpret_cast<void**>(&m_device));
        if (chr != 0) {
            CDirectDrawMgr::GetErrorString(0, 0, chr);
            if (m_lastError == 0) {
                m_lastError = 0x3ef;
            }
            return 0;
        }
    }

    i32 hr = m_device->SetCooperativeLevel(static_cast<HWND>(hwnd), coopFlags);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_H_FILE, 0x120, hr);
    }
    if (hr != 0) {
        if (m_lastError == 0) {
            m_lastError = 0x3ea;
        }
        return 0;
    }

    i32 i;
    i32* p = m_driverCaps;
    for (i = 0x5f; i != 0; i--) {
        *p++ = 0;
    }
    i32* q = m_helCaps;
    for (i = 0x5f; i != 0; i--) {
        *q++ = 0;
    }
    // m_driverCaps/m_helCaps are the driver + HEL DDCAPS_DX6 blocks (raw i32[0x5f] in the
    // lean header; sizeof(DDCAPS)==0x17c is exactly 0x5f*4). Access them through the
    // real SDK type so the dwSize/dwCaps fields are named, not magic indices.
    (reinterpret_cast<LPDDCAPS>(m_driverCaps))->dwSize = sizeof(DDCAPS);
    (reinterpret_cast<LPDDCAPS>(m_helCaps))->dwSize = sizeof(DDCAPS);
    hr = m_device->GetCaps(
        reinterpret_cast<LPDDCAPS>(m_driverCaps),
        reinterpret_cast<LPDDCAPS>(m_helCaps)
    );
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0xad, hr);
    }
    m_bltCaps = (reinterpret_cast<LPDDCAPS>(m_driverCaps))->dwCaps & 0x8000000;
    SetupCaps();

    if (width > 0 && height > 0) {
        hr = SetVideoMode(width, height, bpp, 0, 0);
        if (hr != 0) {
            CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0xc2, hr);
            if (m_lastError == 0) {
                m_lastError = 0x3ec;
            }
            return 0;
        }
        m_palBpp = bpp;
    }

    if (bpp == 0) {
        DDSURFACEDESC desc;
        i32 j;
        i32* d = reinterpret_cast<i32*>(&desc);
        for (j = 0x1b; j != 0; j--) {
            *d++ = 0;
        }
        desc.dwSize = 0x6c;
        hr = m_device->GetDisplayMode(&desc);
        if (hr == 0) {
            m_palBpp = desc.ddpfPixelFormat.dwRGBBitCount;
        }
    }

    g_DirectDrawMgr = this;
    return 1;
}

i32 __stdcall
CreateDirectDrawVia(void* ctx, i32 a1, i32 a2, IDirectDraw2*(__cdecl* factory)(void*, i32, i32));

RVA(0x00141ff0, 0x6c)
i32 CDirectDrawMgr::Init(void* factory, void* a1, i32 width, i32 height, i32 bpp, u32 coop) {
    if (factory == 0) {
        return 0;
    }
    g_ddCreateCtx = 0;
    i32 hr =
        DirectDrawEnumerateA(reinterpret_cast<LPDDENUMCALLBACKA>(CreateDirectDrawVia), factory);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0xf4, hr);
        return 0;
    }
    return CreateDevice(a1, g_ddCreateCtx, width, height, bpp, coop);
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
    g_DirectDrawMgr = 0;
    if (m_device) {
        m_device->Release();
        m_device = 0;
    }
    if (m_dd1) {
        m_dd1->Release();
        m_dd1 = 0;
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
        CDDSurface* item = reinterpret_cast<CDDSurface*>(m_poolA.GetNext(pos));
        delete item;
    }
    m_poolA.RemoveAll();
}

RVA(0x00142160, 0x24)
void CDDrawPtrCollections::RemoveItemA(CDDSurface* item) {
    m_poolA.RemoveAt(item->m_pos);
    delete item;
}

// ---------------------------------------------------------------------------
// Create7f0_1 (0x1421a0).  new 0xc0 item; ctor (CByteArray @+0x94, vtbl 0x5ef7f0
// stamped FIRST, then zero fields); dispatch vtbl[0x08] with 1 arg; on success
// AddItemA, else virtual-delete. /GX. ret 0x4. The failure (delete) path is the
// fall-through (`if (!InitX)`), matching retail's `jne success` branch polarity.
RVA(0x001421a0, 0xbe)
CDDSurface* CDDrawPtrCollections::Create7f0_1(i32 a) {
    CDDSurface* item = new CDDSurface;
    if (!item->Init1(this, a)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

// ---------------------------------------------------------------------------
// CreateA (0x142260).  new 0xc0 item, ctor (CFileImageSurface @+0x94 / vtbl 0x5efa58),
// dispatch vtbl[0x24]; on success register via AddItemA, else virtual-delete. /GX.
// Failure path is the fall-through (retail's `jne success` polarity).
RVA(0x00142260, 0xd2)
CDDSurface* CDDrawPtrCollections::CreateA(i32 a, i32 b, i32 c, i32 d, i32 e) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->ResolveEx(this, reinterpret_cast<void*>(a), b, c, d, e)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

// 0x142340 is the compiler-generated scalar-deleting destructor (auto-emitted COMDAT).
// @rva-symbol: ??_GCFileImageSurface@@UAEPAXI@Z 0x00142340 0x1e

RVA(0x00142360, 0x53)
CFileImageSurface::~CFileImageSurface() {}

// ---------------------------------------------------------------------------
// CreateB (0x1423c0).  Same as CreateA but dispatches vtbl[0x2c]. /GX.
RVA(0x001423c0, 0xd2)
CDDSurface* CDDrawPtrCollections::CreateB(i32 a, i32 b, i32 c, i32 d, i32 e) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->LoadKeyed(this, a, b, c, d, e)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

// ---------------------------------------------------------------------------
// Createa58_1 (0x1424a0).  new 0xc0 item; ctor (vtbl 0x5efa58); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
RVA(0x001424a0, 0xbe)
CDDSurface* CDDrawPtrCollections::Createa58_1(i32 a) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->Init1(this, a)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

// ---------------------------------------------------------------------------
// Createa58_3 (0x142560).  new 0xc0 item; ctor (vtbl 0x5efa58); dispatch vtbl[0x28]
// with 3 args; AddItemA on success. /GX. ret 0xc.
RVA(0x00142560, 0xc8)
CDDSurface* CDDrawPtrCollections::Createa58_3(i32 a, i32 b, i32 c) {
    CFileImageSurface* item = new CFileImageSurface;
    if (!item->LoadByExt(this, reinterpret_cast<char*>(a), b, c)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

extern "C" int sprintf(char* buf, const char* fmt, ...); // 0x11f890 (_sprintf)
extern char g_dotDot[];                                  // 0x5ee8ec  ".."

// ---------------------------------------------------------------------------
// CreateRange (0x142630). Build a numbered sequence of a58 pool items named
// "<base><index>" over [start, start+count); an optional suffix overrides the name
// (".." + suffix, or the numbered name + suffix when the suffix starts with '.').
// Createa58_3 each and gather the non-null results into `out`; return the count.
// __thiscall, ret 0x1c => 7 args.
// @early-stop
// regalloc/spill wall (~80%): logic is complete + correct (the numbered-name build,
// the ".."/suffix override, the Createa58_3 loop + non-null collect). The inline
// strlen/strcpy/strcat clobber the caller-saved regs, forcing heavy spilling; retail
// spills n/end/output-ptr and keeps the suffix in ebp (frame 0x28), while cl assigns
// the callee-saved regs differently (frame 0x20). The permuter finds no operand fix;
// same MSVC5 coin-flip its sibling factories carry. Banked for the final sweep.
RVA(0x00142630, 0xfe)
i32 CDDrawPtrCollections::CreateRange(
    CDDSurface** out,
    i32 start,
    i32 count,
    char* baseName,
    char* suffix,
    i32 a6,
    i32 a7
) {
    i32 n = 0;
    i32 end = start + count;
    CDDSurface** p = out;
    for (i32 i = start; i < end; i++) {
        char buf[32];
        sprintf(buf, "%s%i", baseName, i);
        if (suffix != 0) {
            if (suffix[0] != '.') {
                strcpy(buf, g_dotDot);
            }
            strcat(buf, suffix);
        }
        CDDSurface* item = Createa58_3(reinterpret_cast<i32>(buf), a6, a7);
        if (item == 0) {
            break;
        }
        *p++ = item;
        n++;
    }
    return n;
}

// ---------------------------------------------------------------------------
// Createa88_3 (0x142730).  new 0xc0 item; ctor (vtbl 0x5efa88); dispatch vtbl[0x24]
// with 3 args; AddItemA on success. /GX. ret 0xc.
RVA(0x00142730, 0xc8)
CDDSurface* CDDrawPtrCollections::Createa88_3(i32 a, i32 b, i32 c) {
    CPoolItemA88* item = new CPoolItemA88;
    if (!item->Blit7(this, a, b, c)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x00142820, 0x53)
CPoolItemA88::~CPoolItemA88() {}

// ---------------------------------------------------------------------------
// Createa88_1 (0x142880).  new 0xc0 item; ctor (vtbl 0x5efa88); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
RVA(0x00142880, 0xbe)
CDDSurface* CDDrawPtrCollections::Createa88_1(i32 a) {
    CPoolItemA88* item = new CPoolItemA88;
    if (!item->Init1(this, a)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

// ---------------------------------------------------------------------------
// Createab8_3 (0x142940).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch vtbl[0x24]
// with 3 args; AddItemA + cache item->m_bitDepth into host->fieldUnknown538 on success.
// /GX. ret 0xc.
RVA(0x00142940, 0xd4)
CDDSurface* CDDrawPtrCollections::Createab8_3(i32 a, i32 b, i32 c) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (!item->Setup(this, a, b, c)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    m_palBpp = item->m_bitDepth;
    return item;
}

RVA(0x00142a40, 0x53)
CPoolItemAB8::~CPoolItemAB8() {}

// ---------------------------------------------------------------------------
// Createab8_1 (0x142aa0).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch vtbl[0x08]
// with 1 arg; AddItemA + cache item->m_bitDepth into host->fieldUnknown538 on success.
// /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142aa0, 0xca)
CDDSurface* CDDrawPtrCollections::Createab8_1(i32 a) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (!item->Init1(this, a)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    m_palBpp = item->m_bitDepth;
    return item;
}

// ---------------------------------------------------------------------------
// Createab8_24_3 (0x142b70).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch
// vtbl[0x24] as a 3-arg init with the two literal tags (0x18, 0x21) + the incoming
// arg; AddItemA + cache item->m_bitDepth into host->fieldUnknown538 on success. /GX. ret 0x4.
// Slot 9 (0x148af0 == CPoolItemAB8::Setup) takes exactly 4 args (info + 3 ints); this
// site passes {0x18, 0x21, a}, Createab8_3 passes {a, b, c} - one consistent signature.
RVA(0x00142b70, 0xce)
CDDSurface* CDDrawPtrCollections::Createab8_24_3(i32 a) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (!item->Setup(this, 0x18, 0x21, a)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    m_palBpp = item->m_bitDepth;
    return item;
}

// ---------------------------------------------------------------------------
// Createae8_6 (0x142c40).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x24]
// as a 6-arg init with all six incoming args; AddItemA on success. /GX. ret 0x18.
RVA(0x00142c40, 0xd7)
CDDSurface* CDDrawPtrCollections::Createae8_6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    CPoolItemAE8* item = new CPoolItemAE8;
    if (!item->Blit47(this, a, b, c, d, e, f)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x00142d40, 0x53)
CPoolItemAE8::~CPoolItemAE8() {}

// ---------------------------------------------------------------------------
// Createae8_1 (0x142da0).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
RVA(0x00142da0, 0xbe)
CDDSurface* CDDrawPtrCollections::Createae8_1(i32 a) {
    CPoolItemAE8* item = new CPoolItemAE8;
    if (!item->Init1(this, a)) {
        delete item;
        return 0;
    }
    AddItemA(item);
    return item;
}

RVA(0x00142e60, 0x27)
CDDSurface* CDDrawPtrCollections::MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e) {
    return CreateB(a, b, c, d | 0x840, e);
}

RVA(0x00142eb0, 0x17)
void CDDrawPtrCollections::AddItemB(CDDPalette* item) {
    item->m_pos = m_poolB.AddTail(item);
}

RVA(0x00142ed0, 0x3d)
void CDDrawPtrCollections::EmptyPoolB() {
    POSITION pos = m_poolB.GetHeadPosition();
    while (pos) {
        CDDPalette* item = reinterpret_cast<CDDPalette*>(m_poolB.GetNext(pos));
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
CDDPalette* CDDrawPtrCollections::MakeB2(i32 a, i32 b) {
    CDDPalette* item = new CDDPalette;
    if (!item->LoadFromFile(m_device, reinterpret_cast<char*>(a), b)) {
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
CDDPalette* CDDrawPtrCollections::MakeB(void* rgb, i32 flags) {
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
CDDPalette* CDDrawPtrCollections::Create(i32 a, i32 b) {
    CDDPalette* item = new CDDPalette;
    if (!item->Create(m_device, reinterpret_cast<void*>(a), b)) {
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
CDDPalette* CDDrawPtrCollections::MakeB3(i32 a, i32 b, i32 c) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateFromTrailing(m_device, reinterpret_cast<void*>(a), b, c)) {
        if (item) {
            item->Destroy();
            ::operator delete(item);
        }
        return 0;
    }
    AddItemB(item);
    return item;
}

// ---------------------------------------------------------------------------
// LoadPaletteMakeB (0x143150).  Open `path` via CFileIO, seek 0x300 from the end and
// read the trailing 0x300-byte palette into a stack buffer, then register a pool-B item
// built from it (MakeB(buf, 0)).  Any failure unwinds the CFileIO + returns 0.  The
// second arg slot is reused as the (always-0) MakeB tag.  /GX EH frame.  ret 0x8.
// ---------------------------------------------------------------------------
// @early-stop
// ~98%: logic + offsets + CFG + the CFileIO open/seek/read shapes are byte-faithful.
// Residue is (a) the /GX funcinfo state index push (retail `push 0xb` vs the per-TU
// compiler-generated funcinfo @+0 - the global __ehfuncinfo numbering, not reproducible
// from one TU; docs/patterns/eh-state-numbering-base.md) and (b) MSVC folds the
// reloaded-from-param-slot MakeB tag to an immediate 0 where retail reloads it. Deferred.
RVA(0x00143150, 0xe9)
CDDPalette* CDDrawPtrCollections::LoadPaletteMakeB(const char* path, i32 z) {
    CFileIO file;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    char buf[0x300];
    if (file.Read(buf, 0x300) != 0x300) {
        return 0;
    }
    return MakeB(buf, z); // retail passes the original z tag (not const-folded 0)
}

extern "C" void DdEnumModesCallback(); // 0x143390

// @early-stop
// ~87% - selection-sort induction/spill wall (was 81%: accessing m_poolItems directly
// instead of a hoisted `arr` alias fixed the this-relative-vs-arr-relative addressing
// across the free loop + SetSize/SetAtGrow calls). Residual: retail spills n, n-1 and
// the outer index to a 0x10-byte frame and colours the sort's outer index as a byte
// offset alongside a separate counter; cl keeps them in registers (0x8 frame). Pure
// register-pressure induction shape, not source-steerable. No EH frame.
RVA(0x00143240, 0x143)
void CDirectDrawMgr::SetupCaps() {
    for (i32 i = 0; i < m_poolItems.GetSize(); i++) {
        ::operator delete(m_poolItems.GetData()[i]);
    }
    m_poolItems.SetSize(0, -1);
    g_modeArray.SetSize(0, -1);
    i32 hr = m_device->EnumDisplayModes(
        0,
        0,
        0,
        reinterpret_cast<LPDDENUMMODESCALLBACK>(DdEnumModesCallback)
    );
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x507, hr);
    }
    for (i32 j = 0; j < g_modeArray.GetSize(); j++) {
        m_poolItems.SetAtGrow(m_poolItems.GetSize(), g_modeArray.GetData()[j]);
    }
    g_modeArray.SetSize(0, -1);
    i32 n = m_poolItems.GetSize();
    for (i32 a = 0; a < n - 1; a++) {
        for (i32 b = a + 1; b < n; b++) {
            if (Compare(m_poolItems.GetData()[a], m_poolItems.GetData()[b])) {
                void* tmp = m_poolItems.GetData()[a];
                m_poolItems.GetData()[a] = m_poolItems.GetData()[b];
                m_poolItems.GetData()[b] = tmp;
            }
        }
    }
}

// 0x143390 - copy a 0x6c-byte enumerated display-mode record and append it to the
// global mode array. __stdcall (arg1 unused). Returns 1.
// @early-stop
// regalloc/scheduling wall (permuter-confirmed no-improvement, operand-order search
// exhausted): logic byte-faithful (operator new(0x6c) + the 0x1b-dword rep-movs copy +
// g_modeArray.SetAtGrow(m_nSize, rec)). Residual is (a) MSVC scheduling the operator-new
// `add esp,4` cleanup after `mov esi,[mode]` vs retail after `mov edi,eax`, and (b) the
// m_nSize arg materialized in eax vs retail's ecx - both pure regalloc, not source-steerable.
RVA(0x00143390, 0x35)
i32 __stdcall AddDisplayMode(void* mode, i32 a1) {
    void* rec = operator new(0x6c);
    memcpy(rec, mode, 0x6c);
    g_modeArray.SetAtGrow(g_modeArray.GetSize(), rec);
    return 1;
}

RVA(0x001433d0, 0x4f)
i32 __stdcall CDirectDrawMgr::Compare(void* pa, void* pb) {
    CDdMode* a = static_cast<CDdMode*>(pa);
    CDdMode* b = static_cast<CDdMode*>(pb);
    if (a->m_c > b->m_c) {
        return 1;
    }
    if (a->m_c < b->m_c) {
        return 0;
    }
    if (a->m_8 > b->m_8) {
        return 1;
    }
    if (a->m_8 < b->m_8) {
        return 0;
    }
    return a->m_54 > b->m_54;
}

// FindMatch (0x143420) - the last >= match's {m_c,m_8} dims via FindLast, or
// {-1,-1} when none. __thiscall, ret 0x10 => 4 args.
//
// @early-stop
// ~83.7% regalloc wall (sibling of FindFwd/FindBack, same archetype): body + guards
// + FindLast call byte-exact. Residue: retail materializes the {-1,-1} store via the
// leftover pushed-arg registers (or ecx,eax / or edx,eax) and loads the out-ptr last
// in the found path, while cl uses immediate stores + an early out-ptr load. Permuter
// ran (no change); not source-steerable, same as its two siblings.
RVA(0x00143420, 0x4b)
void CDirectDrawMgr::FindMatch(CDdModePair* out, u32 k0, u32 k1, i32 k2) {
    i32 idx = FindLast(k0, k1, k2);
    if (idx == -1) {
        out->a = -1;
        out->b = -1;
    } else {
        CDdMode* e = reinterpret_cast<CDdMode*>(m_poolItems.GetData()[idx]);
        out->a = e->m_c;
        out->b = e->m_8;
    }
}

RVA(0x00143470, 0x47)
i32 CDirectDrawMgr::FindLast(u32 k0, u32 k1, i32 k2) {
    i32 r = -1;
    for (i32 i = m_poolItems.GetSize() - 1; i >= 0; i--) {
        CDdMode* e = reinterpret_cast<CDdMode*>(m_poolItems.GetData()[i]);
        if (e->m_c >= k0 && e->m_8 >= k1 && e->m_54 == k2) {
            r = i;
        }
    }
    return r;
}

RVA(0x001434c0, 0x45)
i32 CDirectDrawMgr::FindIndex(i32 k0, i32 k1, i32 k2) {
    for (i32 i = 0; i < m_poolItems.GetSize(); i++) {
        CDdMode* e = reinterpret_cast<CDdMode*>(m_poolItems.GetData()[i]);
        if (e->m_c == static_cast<u32>(k0) && e->m_8 == static_cast<u32>(k1) && e->m_54 == k2) {
            return i;
        }
    }
    return -1;
}

// @early-stop
// ~82.5% regalloc wall: body + guards + FindIndex call byte-exact; in the scan loop
// retail pins the strength-reduced iterator pointer in edx and the loaded entry in
// ecx, while cl swaps them (entry in edx), cascading into the found-path field reads.
// No source spelling (index/hoisted-base/explicit pointer-walk) flips the pair.
RVA(0x00143510, 0x71)
void CDirectDrawMgr::FindFwd(CDdModePair* out, i32 k0, i32 k1, i32 k2) {
    i32 idx = FindIndex(k0, k1, k2);
    if (idx != -1 && idx < m_poolItems.GetSize()) {
        idx++;
        if (idx < m_poolItems.GetSize()) {
            for (; idx < m_poolItems.GetSize(); idx++) {
                CDdMode* e = reinterpret_cast<CDdMode*>(m_poolItems.GetData()[idx]);
                if (e->m_54 == k2) {
                    out->a = e->m_c;
                    out->b = e->m_8;
                    return;
                }
            }
        }
    }
    out->a = -1;
    out->b = -1;
}

// @early-stop
// ~72.8% regalloc wall: same iterator/entry register swap as FindFwd (mirror,
// descending scan). Logic complete.
RVA(0x00143590, 0x7e)
void CDirectDrawMgr::FindBack(CDdModePair* out, i32 k0, i32 k1, i32 k2) {
    i32 idx = FindIndex(k0, k1, k2);
    if (idx != -1 && idx < m_poolItems.GetSize()) {
        idx--;
        if (idx >= 0) {
            for (; idx >= 0; idx--) {
                CDdMode* e = reinterpret_cast<CDdMode*>(m_poolItems.GetData()[idx]);
                if (e->m_54 == k2) {
                    out->a = e->m_c;
                    out->b = e->m_8;
                    return;
                }
            }
        }
    }
    out->a = -1;
    out->b = -1;
}

// ---------------------------------------------------------------------------
// CDirectDrawMgr::CreatePoolItem (0x143630) - EH-framed factory.  Pulls a
// descriptor out of arg0's source object (slot +0x30), reports a failure through
// GetErrorString, else operator-new's the 0xc0-byte pool item, constructs its
// +0x94 sub-object + stamps the vtable, runs the item's Init (slot +0x04) and
// publishes it (AddPoolItem); a failed Init scalar-deletes (slot +0x00) the item.
// @early-stop
// operator-new ctor-in-flight EH frame: the descriptor call, construction, field
// stores + Init/Dtor dispatch are byte-faithful, but retail carries the throwing
// new's /GX cleanup frame (push -1/fs:0 + trylevel) the manual operator-new body
// omits.  docs/patterns/rezalloc-placement-new-no-eh-frame.md family.
// ---------------------------------------------------------------------------
// @identity-TODO (full xref chase run, all techniques dead-end at a void*/RTTI-less
// nested descriptor):
//   1. sema xref 0x143630 -> callers CDDrawSurfacePair::Create @0x163c90 / SetGeom @0x164250.
//   2. mangled sig of CreatePoolItem is ?...@@QAEPAXPAX0@Z -> arg0 is `void*` in retail (no
//      class name recoverable from the signature).
//   3. disasm 0x163c90 at the call: arg0 = the DEEPLY-NESTED descriptor
//      [[[CDDrawSurfacePair+0xc]+0x4]+0x10]+0x2c (5 indirections); the factory invokes only
//      its +0x08 COM interface's slot 12 (+0x30) Make(outB,outA)
//      (`mov eax,[arg0+8]; mov edx,[eax]; call [edx+0x30]`).
//   4. no RTTI COL on the descriptor or its interface (no vptr-by-address stamp).
// Kept as honest by-offset + abstract-COM-interface models (no fabricated identity) until
// the CDDrawSurfacePair->+0xc surface-descriptor subsystem is RTTI-pinned. The factory-arg
// views CDdDescSrc / CDdCreateArg live in <DDrawMgr/DdCreateArg.h>.
#include <DDrawMgr/DdCreateArg.h>

// The pool item is a real CDDSurface (vtable 0x5ef7f0): `new CDDSurface` + slot-1
// Refresh / `delete` (see CreatePoolItem below).
// @early-stop
// ~89%: `new CDDSurface` now inlines the ctor correctly (CPtrArray @+0x94, vptr stamp
// 0x5ef7f0, 6 field-zeros).
// Residual is the /GX ctor-in-flight EH-state index of the throwing CPtrArray member ctor
// (the Create7f0_1/CreateA factory-EH family wall; code bytes match, EH-frame state differs).
RVA(0x00143630, 0x10d)
void* CDirectDrawMgr::CreatePoolItem(void* arg0v, void* arg1) {
    CDdCreateArg* arg0 = static_cast<CDdCreateArg*>(arg0v);
    void* outA = 0;
    void* outB;
    i32 hr = arg0->m_8->Make(&outB, &outA);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x6ae, hr);
        return 0;
    }
    // The item IS a CDDSurface (base pool item): `new CDDSurface` emits exactly the
    // retail operator-new(0xc0) + inlined ctor - the CPtrArray member ctor at +0x94,
    // the vptr stamp (mov [esi],0x5ef7f0), then the 6 scalar-field zeros in ctor order
    // (m_8/m_c/m_pos/m_dontOwn/m_bitDepth/m_b8). The throwing CPtrArray member ctor is
    // what gives the factory its /GX ctor-in-flight EH frame. Slot 1 (Refresh) is the
    // "init"; a failed init `delete`s the item (slot-0 scalar-deleting dtor under the
    // compiler's null-guard).
    CDDSurface* item = new CDDSurface;
    if (item->Refresh(static_cast<IDirectDrawSurface*>(outA)) == 0) {
        delete item;
        return 0;
    }
    AddPoolItem(item);
    return item;
}

RVA(0x00143740, 0x93)
i32 CDirectDrawMgr::GetDisplayMode(i32* pWidth, i32* pHeight, i32* pBpp) {
    DDSURFACEDESC desc;
    i32 j;
    i32* d = reinterpret_cast<i32*>(&desc);
    for (j = 0x1b; j != 0; j--) {
        *d++ = 0;
    }
    desc.dwSize = 0x6c;
    i32 hr = m_device->GetDisplayMode(&desc);
    if (hr != 0) {
        *pWidth = 0;
        *pHeight = 0;
        *pBpp = 0;
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x6e5, hr);
        return 0;
    }
    *pWidth = desc.dwWidth;
    *pHeight = desc.dwHeight;
    *pBpp = desc.ddpfPixelFormat.dwRGBBitCount;
    return 1;
}

RVA(0x001437e0, 0xa)
void RelayHwnd(i32 (*handler)()) {
    g_restoreHandler = handler;
}

RVA(0x001437f0, 0x1b)
i32 RestoreLostSurfaces_1437f0() {
    if (g_restoreHandler) {
        return g_restoreHandler();
    }
    DDrawLogLine("WARNING - Surface(s) lost but no restore handler is available\n");
    return 0;
}

// CDirectDrawMgr::GetAvailableVidMem (0x143810) - forward to the held device's
// IDirectDraw2::GetAvailableVidMem (slot 0x5c); `caps` arrives by value and is
// passed by address as LPDDSCAPS. Returns HRESULT == 0. __thiscall, ret 0xc.
//
// @early-stop
// struct-by-value-param wall (~50%): logic/slot/offsets/CFG exact. Retail's `caps`
// param is a DDSCAPS *struct* passed by value whose address escapes into the COM
// call, so MSVC5 re-stores it into its own arg slot (mov [esp+8],eax; lea &caps)
// with no frame setup, and that register pressure forces the xor/test/sete bool
// form instead of neg/sbb/inc. Reproducing it needs `DDSCAPS caps` by value, but
// this header is deliberately ddraw.h-free (widely included) - exposing DDSCAPS
// by value would pollute every includer for a 43-byte forwarder. Deferred.
RVA(0x00143810, 0x2b)
i32 CDirectDrawMgr::GetAvailableVidMem(u32 caps, u32* total, u32* free) {
    return m_device->GetAvailableVidMem(
               reinterpret_cast<LPDDSCAPS>(&caps),
               reinterpret_cast<LPDWORD>(total),
               reinterpret_cast<LPDWORD>(free)
           )
           == 0;
}

RVA(0x00143840, 0x32)
i32 CDirectDrawMgr::GetFreeVidMem() {
    DDSCAPS caps;
    DWORD total;
    DWORD freeMem;
    caps.dwCaps = 0x1000;
    i32 hr = m_device->GetAvailableVidMem(&caps, &total, &freeMem);
    return hr == 0 ? freeMem : 0;
}

RVA(0x00143880, 0x3b)
i32 __stdcall
CreateDirectDrawVia(void* ctx, i32 a1, i32 a2, IDirectDraw2*(__cdecl* factory)(void*, i32, i32)) {
    if (factory != 0) {
        IDirectDraw2* dd = factory(ctx, a1, a2);
        if (dd != 0) {
            g_DirectDraw = dd;
            g_ddCreateCtx = ctx;
            return 0;
        }
    }
    return 1;
}

RVA(0x001438c0, 0x31)
IDirectDrawSurface* CDirectDrawMgr::GetGDISurface() {
    IDirectDrawSurface* surf = 0;
    i32 hr = m_device->GetGDISurface(&surf);
    if (hr != 0) {
        DDrawLogLine(const_cast<char*>("CDirectDrawMgr::GetGDISurface()"));
        return 0;
    }
    return surf;
}

// ---------------------------------------------------------------------------
// 0x143900 - install the display palette from a CDDPalette wrapper's PALETTEENTRY
// cache (+0x0c): straight 256-dword copy into m_palette, then flag present + tag.
// __thiscall, 2 args (ret 0x8). No return value used.
// @early-stop
// ~65% - logic/offsets/CFG byte-faithful; the residual is the mirror-register wall
// (same as the sibling Make950 @0x143950): retail keeps the source cursor in eax and
// the dst in edx and pushes esi/edi in the prologue (bail paths pop), where MSVC on
// this identical source mirrors the src/dst registers and defers the pushes past both
// null checks. Not source-steerable (permuter 150-iter marginal). Regalloc-coloring
// residue; see the 0x143950 note. docs/patterns/zero-register-pinning.md family.
RVA(0x00143900, 0x4d)
void CDDrawPtrCollections::SetDisplayPaletteFrom_143900(CDDPalette* pal, i32 tag) {
    if (pal == 0) {
        return;
    }
    i32* src = reinterpret_cast<i32*>(pal->m_cacheA);
    if (src == 0) {
        return;
    }
    i32* dst = m_palette;
    for (i32 i = 0; i < 256; i++) {
        *dst++ = *src++;
    }
    m_940 = tag;
    m_hasPalette = 1;
}

// ---------------------------------------------------------------------------
// Make950 (0x143950): install a 256-entry
// palette from a caller-supplied packed RGB-triplet buffer (buf) - expand each 3-byte
// RGB into the 4-byte display palette entry (4th byte zeroed), then flag present +
// latch the tag (z). Returns success (1). __thiscall, 2 args (ret 0x8). The palette-
// install sibling of SetDisplayPaletteFrom/Direct; LoadPaletteMake950 tail-returns it.
// @early-stop
// ~92% (was 78%: the RGB reads are `*src++` (mov bl,[eax]; inc eax), not fixed
// src[0..2]+src+=3 - now byte-exact). Residual: retail biases the dst cursor +1
// (lea edx,[edi+0x53d], stores at edx-1/-4/-3/-2) and schedules the b2 store before
// the alpha store; cl starts edx at +0 and reorders the last two stores. Pure MSVC
// addressing/scheduling coin-flip. docs/patterns/zero-register-pinning.md.
RVA(0x00143950, 0x56)
CDDPalette* CDDrawPtrCollections::Make950(void* buf, i32 z) {
    if (buf == 0) {
        return 0;
    }
    const u8* src = static_cast<const u8*>(buf);
    u8* dst = reinterpret_cast<u8*>(m_palette);
    for (i32 i = 0; i < 256; i++) {
        dst[0] = *src++; // post-inc read (mov bl,[eax]; inc eax), not fixed src[0..2]+src+=3
        dst[1] = *src++;
        dst[2] = *src++;
        dst[3] = 0;
        dst += 4;
    }
    m_hasPalette = 1;
    m_940 = z;
    return reinterpret_cast<CDDPalette*>(
        1
    ); // retail returns the success flag as the CDDPalette* result
}

// ---------------------------------------------------------------------------
// 0x1439b0 - install the display palette directly from a caller RGBQ array:
// straight 256-dword copy into m_palette, then flag present + latch tag.
// __thiscall, 2 args (ret 0x8). No return value used.
// @early-stop
// ~83% - logic/offsets/CFG byte-faithful; residual is the same mirror-register wall
// as 0x143900/0x143950: retail keeps src in eax + dst in edx, materializes the m_hasPalette
// 1 into a reused reg; MSVC on this source mirrors src/dst and stores the immediate.
// Not source-steerable (permuter 150-iter marginal). docs/patterns/zero-register-pinning.md.
RVA(0x001439b0, 0x3d)
void CDDrawPtrCollections::SetDisplayPaletteDirect_1439b0(i32* rgbq, i32 tag) {
    if (rgbq == 0) {
        return;
    }
    i32* dst = m_palette;
    for (i32 i = 0; i < 256; i++) {
        *dst++ = *rgbq++;
    }
    m_940 = tag;
    m_hasPalette = 1;
}

RVA(0x001439f0, 0x35)
CDDPalette* CDDrawPtrCollections::Make950Trailing(u8* buf, i32 size, i32 tag) {
    if (buf == 0) {
        return 0;
    }
    if (static_cast<u32>(size) < 0x3e8) {
        return 0;
    }
    return Make950(buf + size - 0x300, tag);
}

// ---------------------------------------------------------------------------
// LoadPaletteMake950 (0x143a30).  Identical shape to LoadPaletteMakeB but the trailing
// palette is handed to the sibling builder Make950 (0x143950) instead of MakeB.  /GX. ret 0x8.
// ---------------------------------------------------------------------------
RVA(0x00143a30, 0xe9)
CDDPalette* CDDrawPtrCollections::LoadPaletteMake950(const char* path, i32 z) {
    CFileIO file;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    char buf[0x300];
    if (file.Read(buf, 0x300) != 0x300) {
        return 0;
    }
    return Make950(buf, z); // retail passes the original z tag (not const-folded 0)
}

RVA(0x00143b20, 0xfc)
i32 CDDrawPtrCollections::ComputeColorMasks() {
    DDSURFACEDESC desc;
    memset(&desc, 0, 0x6c);
    desc.dwSize = 0x6c;
    i32 hr = m_device->GetDisplayMode(&desc);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x82c, hr);
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
i32 CDDrawPtrCollections::ConfigureSurface(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    i32 hr = m_device->SetDisplayMode(a0, a1, a2, a3, a4);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x8a2, hr);
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

SIZE_UNKNOWN(CDdCreateArg);
SIZE_UNKNOWN(CDdDescSrc);
SIZE(CDDrawPtrCollections, 0x948);
