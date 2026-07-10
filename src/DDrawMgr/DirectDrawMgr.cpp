// DirectDrawMgr.cpp - CDirectDrawMgr (C:\Proj\DDrawMgr\DDRAWMGR.CPP), the top-level
// DirectDraw device manager. Split out of the former DirectDrawMgr god-TU (which
// aggregated three sibling DDrawMgr classes); this is now one contiguous retail .text
// block (0x1413d0-0x1438f1) == one retail .obj (DDRAWMGR). Its siblings moved to
// DDSurface.cpp (CDDSurface / DIRSURF.CPP) and DDPalette.cpp (CDDPalette / DIRPAL.CPP);
// CDDPageMgr::Init/CheckMode16 rejoined DDPageMgr.cpp (their own .obj).
//
// CDirectDrawMgr owns GetErrorString (the shared diagnostic HRESULT formatter every
// DDrawMgr class funnels failures through), the DirectDrawCreate device bring-up
// (CreateDevice/Init), the display-mode pool (SetupCaps + the Compare/Find* searches)
// and CreatePoolItem. Include environment mirrors the former god-TU (Mfc.h via
// FileStream.h supplies windows.h before <ddraw.h>; CreatePoolItem needs CPtrArray).
#include <Io/FileStream.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <ddraw.h> // real DirectDraw SDK (IDirectDraw/2, DirectDrawCreate, DirectDrawEnumerateA, DDCAPS, IID_IDirectDraw2)
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy / memcpy
#include <Globals.h>

#define DDRAWMGR_FILE "C:\\Proj\\DDrawMgr\\DDRAWMGR.CPP"
#define DDRAWMGR_H_FILE "C:\\Proj\\DDrawMgr\\ddrawmgr.h"

// Reporting-mode globals (live in .data), consumed by SetDDrawReportModes/GetErrorString.
DATA(0x00283ec0)
extern "C" i32 g_beepEnabled; // 0x683ec0
DATA(0x00283eb8)
extern "C" i32 g_logEnabled; // 0x683eb8
DATA(0x00283ebc)
extern "C" i32 g_msgBoxEnabled; // 0x683ebc
DATA(0x00283ec4)
extern "C" i32 g_thirdEnabled; // 0x683ec4

// Empty mutable string in .data copied into the working buffer up front.
DATA(0x002293f4)
extern "C" char g_emptyString[]; // 0x6293f4

// The engine logger that consumes the formatted line (DDrawMgr-local helper).
extern void __cdecl DDrawLogLine(char* fmt, ...); // 0x141cb0 (printf-style TRACE)

// The Rez heap allocator/free + operator new (reloc-masked engine leaves).
extern "C" void* RezAlloc(unsigned int); // 0x1b9b46
extern "C" void RezFree(void* p);        // 0x1b9b82
void* operator new(u32);                 // engine allocator (reloc-masked rel32)

// IID_IDirectDraw2 - the real dxguid GUID constant in .rdata, passed to QueryInterface
// by REFIID. <ddraw.h> declares it (EXTERN_C const GUID); redeclared with DATA() to bind
// its retail address so the `push OFFSET` reloc-masks (same idiom as DirectInputMgr2.cpp).
DATA(0x001ef848)
extern "C" const GUID IID_IDirectDraw2; // 0x5ef848

// The process-wide DirectDraw object + the enumerated-display-mode array + the create
// context (owner window) that produced the current g_DirectDraw object (.data).
DATA(0x00283ee8)
extern "C" IDirectDraw2* g_DirectDraw; // 0x683ee8
DATA(0x00283ec8)
extern CDdObArray g_modeArray; // 0x683ec8 (CObArray of 0x6c-byte mode records)
DATA(0x00283ee4)
extern void* g_ddCreateCtx; // 0x683ee4

// 0x1413d0 - set the four GetErrorString reporting-mode flags (log / message-box /
// beep / third) from the four args. __cdecl free helper.
RVA(0x001413d0, 0x27)
void SetDDrawReportModes(i32 log, i32 msgBox, i32 beep, i32 third) {
    g_logEnabled = log;
    g_msgBoxEnabled = msgBox;
    g_beepEnabled = beep;
    g_thirdEnabled = third;
}

// CDirectDrawMgr::GetErrorString
RVA(0x00141400, 0x835)
void CDirectDrawMgr::GetErrorString(char* file, i32 line, i32 hr) {
    char szCode[64];  // local_340 - error-code name
    char szMsg[256];  // local_300 - description
    char szLine[512]; // local_200 - formatted output line

    if (g_beepEnabled) {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    if (!g_logEnabled && !g_msgBoxEnabled && !g_thirdEnabled) {
        return;
    }

    i32 code = hr & 0xffff;

    strcpy(szMsg, "Unknown Error Message");
    sprintf(szCode, "Unknown Error Code");
    strcpy(szLine, g_emptyString);

    switch (hr) {
        case (i32)0x80004001:
            strcpy(szCode, "DDERR_UNSUPPORTED");
            strcpy(szMsg, "Action not supported");
            break;
        case (i32)0x80004005:
            strcpy(szCode, "DDERR_GENERIC");
            strcpy(szMsg, "Generic failure");
            break;
        case (i32)0x8007000e:
            strcpy(szCode, "DDERR_OUTOFMEMORY");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x80070057:
            strcpy(szCode, "DDERR_INVALIDPARAMS");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760064:
            strcpy(szCode, "DDERR_INVALIDCAPS");
            strcpy(szMsg, "One or more of the caps bits passed to the callback are incorrect");
            break;
        case (i32)0x88760078:
            strcpy(szCode, "DDERR_INVALIDMODE");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760082:
            strcpy(szCode, "DDERR_INVALIDOBJECT");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760091:
            strcpy(szCode, "DDERR_INVALIDPIXELFORMAT");
            strcpy(szMsg, "Pixel format was invalid as specified.");
            break;
        case (i32)0x88760096:
            strcpy(szCode, "DDERR_INVALIDRECT");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887600a0:
            strcpy(szCode, "DDERR_LOCKEDSURFACES");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887600aa:
            strcpy(szCode, "DDERR_NO3D");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887600b4:
            strcpy(szCode, "DDERR_NOALPHAHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887600d2:
            strcpy(szCode, "DDERR_NOCOLORCONVHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887600d4:
            strcpy(szCode, "DDERR_NOCOOPERATIVELEVELSET");
            strcpy(
                szMsg,
                "Create function called without DirectDraw object method SetCooperativeLevel being "
                "called"
            );
            break;
        case (i32)0x887600e1:
            strcpy(szCode, "DDERR_NOEXCLUSIVEMODE");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887600f0:
            strcpy(szCode, "DDERR_NOGDI");
            strcpy(szMsg, "There is no GDI present");
            break;
        case (i32)0x887600fa:
            strcpy(szCode, "DDERR_NOMIRRORHW");
            strcpy(
                szMsg,
                "Operation could not be carried out because there is no hardware present or "
                "available."
            );
            break;
        case (i32)0x887600ff:
            strcpy(szCode, "DDERR_NOTFOUND");
            strcpy(szMsg, "Request item was not found");
            break;
        case (i32)0x88760104:
            strcpy(szCode, "DDERR_NOOVERLAYHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760118:
            strcpy(szCode, "DDERR_NORASTEROPHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760122:
            strcpy(szCode, "DDERR_NOROTATEHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760136:
            strcpy(szCode, "DDERR_NOSTRETCHHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760140:
            strcpy(szCode, "DDERR_NOT8BITCOLOR");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8876014a:
            strcpy(szCode, "DDERR_NOTEXTUREHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8876014f:
            strcpy(szCode, "DDERR_NOVSYNCHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760154:
            strcpy(szCode, "DDERR_NOZBUFFERHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760168:
            strcpy(szCode, "DDERR_OUTOFCAPS");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8876017c:
            strcpy(szCode, "DDERR_OUTOFVIDEOMEMORY");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760183:
            strcpy(szCode, "DDERR_PALETTEBUSY");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887601ae:
            strcpy(szCode, "DDERR_SURFACEBUSY");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887601b8:
            strcpy(szCode, "DDERR_SURFACEISOBSCURED");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887601c2:
            strcpy(szCode, "DDERR_SURFACELOST");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887601cc:
            strcpy(szCode, "DDERR_SURFACENOTATTACHED");
            strcpy(szMsg, "The requested surface is not attached");
            break;
        case (i32)0x887601e0:
            strcpy(szCode, "DDERR_TOOBIGSIZE");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x887601ea:
            strcpy(szCode, "DDERR_TOOBIGWIDTH");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760219:
            strcpy(szCode, "DDERR_VERTICALBLANKINPROGRESS");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8876021c:
            strcpy(szCode, "DDERR_WASTILLDRAWING");
            strcpy(
                szMsg,
                "The previous Blt which is transfering information to or from this Surface is "
                "incomplete"
            );
            break;
        case (i32)0x88760233:
            strcpy(szCode, "DDERR_NODIRECTDRAWHW");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760232:
            strcpy(szCode, "DDERR_DIRECTDRAWALREADYCREATED");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760230:
            strcpy(szCode, "DDERR_XALIGN");
            strcpy(szMsg, "Rectangle provided was not horizontally aligned on a DWORD boundary");
            break;
        case (i32)0x8876023a:
            strcpy(szCode, "DDERR_HWNDSUBCLASSED");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8876023b:
            strcpy(szCode, "DDERR_HWNDALREADYSET");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x8876023d:
            strcpy(szCode, "DDERR_NOPALETTEHW");
            strcpy(szMsg, "No hardware support for 16 or 256 color palettes");
            break;
        case (i32)0x88760234:
            strcpy(szCode, "DDERR_PRIMARYSURFACEALREADYEXISTS");
            strcpy(szMsg, "This process already has created a primary surface");
            break;
        case (i32)0x88760245:
            strcpy(szCode, "DDERR_EXCLUSIVEMODEALREADYSET");
            strcpy(szMsg, "No message");
            break;
        case (i32)0x88760248:
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

    if (g_logEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i) - %s\n", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i: %s (%i) - %s\n", file, line, szCode, code, szMsg);
        }
        DDrawLogLine(szLine);
    }
    if (g_msgBoxEnabled) {
        if (file == 0 || line <= 0) {
            sprintf(szLine, "%s (%i)\n\n%s", szCode, code, szMsg);
        } else {
            sprintf(szLine, "%s, line %i\n\n%s (%i)\n\n%s", file, line, szCode, code, szMsg);
        }
        MessageBoxA((HWND)0, szLine, "DirectDrawMgr", MB_ICONEXCLAMATION);
    }
}

// Global-mode-array teardown tail-forward (0x141c80): mov ecx,&g_modeArray; jmp
// RemoveAll. Folded from Stub/BoundaryUpper.cpp (ClearModeArray_141c80).
RVA(0x00141c80, 0xa)
void ClearModeArray_141c80() {
    g_modeArray.RemoveAll();
}

// CDirectDrawMgr::CreateDevice (__thiscall, ret 0x18 => 6 args; arg1 unused).
// Brings up the DirectDraw device and caches it as the singleton.
RVA(0x00141dc0, 0x224)
i32 CDirectDrawMgr::CreateDevice(
    void* a1,
    void* hwnd,
    i32 width,
    i32 height,
    i32 bpp,
    u32 coopFlags
) {
    m_93c = 0;
    m_940 = 0;
    IDirectDraw2* dd = g_DirectDraw;
    if (dd != 0) {
        m_device = dd;
    } else {
        i32 chr = DirectDrawCreate((GUID*)hwnd, &m_dd1, 0);
        if (chr != 0) {
            CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x88, chr);
            if (m_lastError == 0) {
                m_lastError = 0x3e9;
            }
            return 0;
        }
        chr = m_dd1->QueryInterface(IID_IDirectDraw2, (void**)&m_device);
        if (chr != 0) {
            CDirectDrawMgr::GetErrorString(0, 0, chr);
            if (m_lastError == 0) {
                m_lastError = 0x3ef;
            }
            return 0;
        }
    }

    i32 hr = m_device->SetCooperativeLevel((HWND)hwnd, coopFlags);
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
    i32* p = m_caps;
    for (i = 0x5f; i != 0; i--) {
        *p++ = 0;
    }
    i32* q = m_helCaps;
    for (i = 0x5f; i != 0; i--) {
        *q++ = 0;
    }
    // m_caps/m_helCaps are the driver + HEL DDCAPS_DX6 blocks (raw i32[0x5f] in the
    // lean header; sizeof(DDCAPS)==0x17c is exactly 0x5f*4). Access them through the
    // real SDK type so the dwSize/dwCaps fields are named, not magic indices.
    ((LPDDCAPS)m_caps)->dwSize = sizeof(DDCAPS);
    ((LPDDCAPS)m_helCaps)->dwSize = sizeof(DDCAPS);
    hr = m_device->GetCaps((LPDDCAPS)m_caps, (LPDDCAPS)m_helCaps);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0xad, hr);
    }
    m_bltCaps = ((LPDDCAPS)m_caps)->dwCaps & 0x8000000;
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
        m_bpp = bpp;
    }

    if (bpp == 0) {
        DDSURFACEDESC desc;
        i32 j;
        i32* d = (i32*)&desc;
        for (j = 0x1b; j != 0; j--) {
            *d++ = 0;
        }
        desc.dwSize = 0x6c;
        hr = m_device->GetDisplayMode(&desc);
        if (hr == 0) {
            m_bpp = desc.ddpfPixelFormat.dwRGBBitCount;
        }
    }

    g_DirectDrawMgr = this;
    return 1;
}

// The driver factory helper defined below (0x143880); forward-declared so Init's
// `push OFFSET CreateDirectDrawVia` reloc-masks against the enum callback cast.
i32 __stdcall
CreateDirectDrawVia(void* ctx, i32 a1, i32 a2, IDirectDraw2*(__cdecl* factory)(void*, i32, i32));

// CDirectDrawMgr::Init (0x141ff0) - enumerate DirectDraw drivers (the supplied
// factory becomes CreateDirectDrawVia's context, caching g_ddCreateCtx), then
// bring the device up via CreateDevice. A null factory or a failed enum reports
// and fails. __thiscall, ret 0x18 => 6 args.
RVA(0x00141ff0, 0x6c)
i32 CDirectDrawMgr::Init(void* factory, void* a1, i32 width, i32 height, i32 bpp, u32 coop) {
    if (factory == 0) {
        return 0;
    }
    g_ddCreateCtx = 0;
    i32 hr = DirectDrawEnumerateA((LPDDENUMCALLBACKA)CreateDirectDrawVia, factory);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0xf4, hr);
        return 0;
    }
    return CreateDevice(a1, g_ddCreateCtx, width, height, bpp, coop);
}

// ===========================================================================
// CDirectDrawMgr pool-item collection helpers (DDRAWMGR.CPP).  The pool item
// list is a CObArray at +0x4b4; SetupCaps tears the list down, re-enumerates the
// device's display modes (rebuilding a global mode array) and re-sorts the pool;
// CreatePoolItem builds + initialises one pool item from a descriptor source.
// ===========================================================================
// Engine heap free / operator new (reloc-masked __cdecl leaves).
extern "C" void RezFree(void* p);        // 0x1b9b82
extern "C" void* RezAlloc(unsigned int); // 0x1b9b46

// The transient global mode array EnumDisplayModes rebuilds (a CObArray @0x683ec8).
// CDdObArray + the pool comparator/publisher (Compare/AddPoolItem) live on
// CDirectDrawMgr in <DDrawMgr/DirectDrawMgr.h>.
DATA(0x00283ec8)
extern CDdObArray g_modeArray;

// The EnumDisplayModes callback (0x143390); only its address is referenced.
// (EnumDisplayModes is slot 8 / +0x20 on the IDirectDraw2 interface.)
extern "C" void DdEnumModesCallback(); // 0x143390

// @early-stop
// 81% - regalloc/induction wall: the free loop, RemoveAll/Add (SetSize/SetAtGrow)
// calls, EnumDisplayModes + GetErrorString and the selection sort are byte-faithful,
// but retail colours the sort's outer index as a byte offset alongside a separate
// counter, an induction shape the recompile doesn't reproduce.  No EH frame.
RVA(0x00143240, 0x143)
void CDirectDrawMgr::SetupCaps() {
    CDdObArray* arr = &m_poolItems;
    for (i32 i = 0; i < arr->m_nSize; i++) {
        RezFree(arr->m_pData[i]);
    }
    arr->SetSize(0, -1);
    g_modeArray.SetSize(0, -1);
    i32 hr = m_device->EnumDisplayModes(0, 0, 0, (LPDDENUMMODESCALLBACK)DdEnumModesCallback);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x507, hr);
    }
    for (i32 j = 0; j < g_modeArray.m_nSize; j++) {
        arr->SetAtGrow(arr->m_nSize, g_modeArray.m_pData[j]);
    }
    g_modeArray.SetSize(0, -1);
    i32 n = arr->m_nSize;
    for (i32 a = 0; a < n - 1; a++) {
        for (i32 b = a + 1; b < n; b++) {
            if (Compare(arr->m_pData[a], arr->m_pData[b])) {
                void* tmp = arr->m_pData[a];
                arr->m_pData[a] = arr->m_pData[b];
                arr->m_pData[b] = tmp;
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
    g_modeArray.SetAtGrow(g_modeArray.m_nSize, rec);
    return 1;
}

// ===========================================================================
// The display-mode pool comparator + searches over m_poolItems (each m_pData[i]
// is a CDdMode*). Folded from Stub/BoundaryUpper.cpp (Compare_1433d0 + ModeArr::
// Find*) - ModeArr IS CDirectDrawMgr (m_4b8/m_4bc = m_poolItems.m_pData/m_nSize).
// Compare's the selection-sort predicate (used by SetupCaps above); FindFwd/FindBack
// are reached via CGruntzMgr::CheckDisplayBoundsA/B.
// ===========================================================================

// Ordered compare: unsigned m_c then m_8, then m_54 as a 0/1 tie-break. __stdcall.
RVA(0x001433d0, 0x4f)
i32 __stdcall CDirectDrawMgr::Compare(void* pa, void* pb) {
    CDdMode* a = (CDdMode*)pa;
    CDdMode* b = (CDdMode*)pb;
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
        CDdMode* e = (CDdMode*)m_poolItems.m_pData[idx];
        out->a = e->m_c;
        out->b = e->m_8;
    }
}

// >= range search from the end (last matching index), else -1.
RVA(0x00143470, 0x47)
i32 CDirectDrawMgr::FindLast(u32 k0, u32 k1, i32 k2) {
    i32 r = -1;
    for (i32 i = m_poolItems.m_nSize - 1; i >= 0; i--) {
        CDdMode* e = (CDdMode*)m_poolItems.m_pData[i];
        if (e->m_c >= k0 && e->m_8 >= k1 && e->m_54 == k2) {
            r = i;
        }
    }
    return r;
}

// Exact 3-key match, else -1.
RVA(0x001434c0, 0x45)
i32 CDirectDrawMgr::FindIndex(i32 k0, i32 k1, i32 k2) {
    for (i32 i = 0; i < m_poolItems.m_nSize; i++) {
        CDdMode* e = (CDdMode*)m_poolItems.m_pData[i];
        if (e->m_c == (u32)k0 && e->m_8 == (u32)k1 && e->m_54 == k2) {
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
    if (idx != -1 && idx < m_poolItems.m_nSize) {
        idx++;
        if (idx < m_poolItems.m_nSize) {
            for (; idx < m_poolItems.m_nSize; idx++) {
                CDdMode* e = (CDdMode*)m_poolItems.m_pData[idx];
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
    if (idx != -1 && idx < m_poolItems.m_nSize) {
        idx--;
        if (idx >= 0) {
            for (; idx >= 0; idx--) {
                CDdMode* e = (CDdMode*)m_poolItems.m_pData[idx];
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
// The descriptor source (arg0->m_8): a COM-style interface; slot +0x30 fills the
// two out params.  Reloc-masked __stdcall.
struct CDdDescSrc {
    // COM-style interface (abstract, __stdcall); only slot 12 (+0x30) is invoked.
    // STDMETHOD form == `virtual HRESULT __stdcall`, so `m_8->Make(...)` lowers to
    // `mov eax,[m_8]; call [eax+0x30]` (was the manual vtbl-struct dispatch).
    STDMETHOD(v00)() PURE;
    STDMETHOD(v01)() PURE;
    STDMETHOD(v02)() PURE;
    STDMETHOD(v03)() PURE;
    STDMETHOD(v04)() PURE;
    STDMETHOD(v05)() PURE;
    STDMETHOD(v06)() PURE;
    STDMETHOD(v07)() PURE;
    STDMETHOD(v08)() PURE;
    STDMETHOD(v09)() PURE;
    STDMETHOD(v0a)() PURE;
    STDMETHOD(v0b)() PURE;
    STDMETHOD(Make)(void* outB, void* outA) PURE; // slot 12 (+0x30)
};
struct CDdCreateArg {
    char m_pad00[8];
    CDdDescSrc* m_8; // +0x08 descriptor source
};

// The pool item: a polymorphic object whose vtable is the RETAIL vtable at 0x5ef7f0.
// Its Init (slot 1) / scalar-deleting-dtor (slot 0) virtual bodies live in other
// DDrawMgr TUs, so cl cannot emit a matching ??_7 here; the factory operator-new's
// the raw 0xc0 block and stamps that retail vtable by ADDRESS (transitional
// workaround - a non-ctor stamp, the vtable-realization compiler-model wall, same
// idiom as g_planeRenderVtbl in WwdFile.cpp). The dispatch is expressed as real
// thiscall virtuals (no hand-rolled vtbl struct): `pi->Init(x)` lowers to the retail
// `mov edx,[pi]; mov ecx,pi; call [edx+4]`.

struct CDdPoolSub {
    // Ctor @0x1b4f0b IS the CPtrArray in-place ctor; placement-new at the call.
};
struct CDdPoolItem {
    virtual i32 Dtor(i32);   // slot 0 (+0x00) scalar-deleting dtor (delete-flag arg)
    virtual i32 Init(void*); // slot 1 (+0x04)
};

RVA(0x00143630, 0x10d)
void* CDirectDrawMgr::CreatePoolItem(void* arg0v, void* arg1) {
    CDdCreateArg* arg0 = (CDdCreateArg*)arg0v;
    void* outA = 0;
    void* outB;
    i32 hr = arg0->m_8->Make(&outB, &outA);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x6ae, hr);
        return 0;
    }
    char* item = (char*)RezAlloc(0xc0);
    if (item != 0) {
        new ((CPtrArray*)(item + 0x94)) CPtrArray(); // in-place CPtrArray ctor (0x1b4f0b)
        // Manual vptr stamp by ADDRESS (the vtable-realization wall - cl cannot emit a
        // matching ??_7CDDSurface in this DDrawMgr TU); the scalar zeroing is typed onto
        // the unified CDDSurface fields (same offsets, byte-identical stores).
        // factory ctor vptr install dropped (model as compiler-emitted vtable; % ok per drive-to-0)
        CDDSurface* s = (CDDSurface*)item;
        s->m_8 = 0;
        s->m_c = 0;
        s->m_pos = 0;
        s->m_dontOwn = 0;
        s->m_bitDepth = 0;
        s->m_b8 = 0;
    } else {
        item = 0;
    }
    CDdPoolItem* pi = (CDdPoolItem*)item;
    if (pi->Init(outA) == 0) {
        if (item != 0) {
            pi->Dtor(1);
        }
        return 0;
    }
    AddPoolItem(item);
    return item;
}

// CDirectDrawMgr::GetDisplayMode (__thiscall, ret 0xc => 3 args). Zero a scratch
// DDSURFACEDESC, query IDirectDraw2::GetDisplayMode and hand back width / height /
// bit-depth through the out-pointers; on failure zero all three, report and fail.
RVA(0x00143740, 0x93)
i32 CDirectDrawMgr::GetDisplayMode(i32* pWidth, i32* pHeight, i32* pBpp) {
    DDSURFACEDESC desc;
    i32 j;
    i32* d = (i32*)&desc;
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

// 0x1437e0 - install the global "restore lost surfaces" handler: store the supplied
// callback into g_restoreHandler (0x683edc), read back by RestoreLostSurfaces_1437f0
// below. __cdecl. (Re-homed from src/Stub/BoundaryUpper2.cpp; the sole caller is
// CDDrawSurfaceMgr::SetHwnd @0x155f50. Byte-exact.)
RVA(0x001437e0, 0xa)
void RelayHwnd(i32 (*handler)()) {
    g_restoreHandler = handler;
}

// 0x1437f0 - the "restore lost surfaces" fallback CDDSurface::RestoreLost (slot 7)
// tail-calls when a surface's own restore callback is absent/failed: if the global
// handler is installed, tail-jump it; else log a warning and return 0. __cdecl.
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
    return m_device->GetAvailableVidMem((LPDDSCAPS)&caps, (LPDWORD)total, (LPDWORD)free) == 0;
}

// CDDrawMgr::GetFreeVidMem (__thiscall, no args). Query the device for available
// texture video memory; return the free-byte count on success, 0 on failure.
RVA(0x00143840, 0x32)
i32 CDirectDrawMgr::GetFreeVidMem() {
    DDSCAPS caps;
    DWORD total;
    DWORD freeMem;
    caps.dwCaps = 0x1000;
    i32 hr = m_device->GetAvailableVidMem(&caps, &total, &freeMem);
    return hr == 0 ? freeMem : 0;
}

// 0x143880 - create the process DirectDraw object via a supplied factory callback and,
// on success, cache it (g_DirectDraw) with its owner context (g_ddCreateCtx); returns 0
// on success, 1 if the factory is null or fails. __stdcall (ret 0x10 => 4 args).
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

// CDDrawMgr::GetGDISurface (__thiscall, no args). Ask the device for the shared
// GDI (primary) surface; on a failed COM call TRACE the method name and return 0.
RVA(0x001438c0, 0x31)
IDirectDrawSurface* CDirectDrawMgr::GetGDISurface() {
    IDirectDrawSurface* surf = 0;
    i32 hr = m_device->GetGDISurface(&surf);
    if (hr != 0) {
        DDrawLogLine((char*)"CDirectDrawMgr::GetGDISurface()");
        return 0;
    }
    return surf;
}

SIZE_UNKNOWN(CDdCreateArg);
SIZE_UNKNOWN(CDdDescSrc);
SIZE_UNKNOWN(CDdEnumVtbl);
SIZE_UNKNOWN(CDdPoolItem);
SIZE_UNKNOWN(CDdPoolSub);
