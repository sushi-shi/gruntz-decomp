// DirectDrawMgr.cpp - the ORIGINAL C:\Proj\DDrawMgr\DDRAWMGR.CPP TU (interval
// dossier #14H): one obj spanning retail 0x1413d0-0x143ca4. The
// __FILE__ assert string is referenced from both the CDirectDrawMgr methods
// (CreateDevice/Init/SetupCaps/CreatePoolItem/GetDisplayMode) AND the former
// ddrawptrcollections unit's fns (ComputeColorMasks/ConfigureSurface); the obj's
// init frag (@0x141c70) + atexit companion ClearModeArray_141c80 sit at the
// mode-array static's source position; the private cell 0x21a9f8 is shared by
// fns of BOTH former units - one obj. The 34 in-band CDDrawPtrCollections
// methods (pools/factories/palette installs) are folded in, in retail-RVA order,
// plus the CFileImageSurface dtor pair (0x142340/0x142360 - the a58 pool item's
// kept ??_G/~ COMDAT copies, emitted from this obj).
//
// NOTE: CDDrawPtrCollections and CDirectDrawMgr are two VIEWS of this
// one DDRAWMGR manager class (+0x00/+0x04 device slots == m_device/m_dd1, the
// +0x4b4 array, the +0x93c..+0x944 tail are the same cells) - flagged for a
// canonical-class unification pass; the physical TU is already one file.
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

// The process-wide CDirectDrawMgr singleton (Init installs `this`, teardown nulls it;
// DirPal.cpp reads it). Owned by this TU; DEFINED here (.bss zero-init), reference
// extern kept in <Globals.h>. (REHOME DD-Drain-1)
extern "C" {
    DATA(0x002bed00)
    CDirectDrawMgr* g_DirectDrawMgr = 0; // 0x6bed00
}

// Reporting-mode globals (live in .data), consumed by SetDDrawReportModes/GetErrorString.
// Module-distinct names (g_dd*): each engine module has its OWN copy of these debug
// flags at a module-specific rva (DDrawMgr @0x283exx); the shared donor name
// g_<flag>Enabled conflated four modules' cells onto one symbol.
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

// The global "restore lost surfaces" handler slot (RelayHwnd installs it,
// RestoreLostSurfaces_1437f0 reads it). Owned by this TU; DEFINED here (zero-init
// .bss), DATA()-pinned; reference extern kept in <Globals.h>.
DATA(0x00283edc)
i32 (*g_restoreHandler)() = 0; // 0x683edc

// Empty mutable string in .data copied into the working buffer up front.

// DDrawLogLine (the DDrawMgr-local TRACE logger, defined below @0x141cb0) is declared
// in <DDrawMgr/DirectDrawMgr.h> (included above).

// Heap alloc/free are ::operator new @0x1b9b46 / ::operator delete @0x1b9b82
// (NAFXCW), declared by <Mfc.h> - reloc-masked library calls.

// IID_IDirectDraw2 - the real dxguid GUID constant in .rdata, passed to QueryInterface
// by REFIID. <ddraw.h> declares it (EXTERN_C const GUID); redeclared with DATA() to bind
// its retail address so the `push OFFSET` reloc-masks (same idiom as DirectInputMgr2.cpp).
DATA(0x001ef848)
extern "C" const GUID IID_IDirectDraw2; // 0x5ef848

// The process-wide DirectDraw object + the enumerated-display-mode array + the create
// context (owner window) that produced the current g_DirectDraw object (.data).
// g_DirectDraw + g_ddCreateCtx DEFINED here (directdrawmgr.obj's .bss, zero-init).
// g_modeArray (0x283ec8) is a real MFC CPtrArray shared with GruntzMgr's display-bounds
// checks; its non-trivial ctor is run explicitly by ClearModeArray_141c80, so it keeps
// its extern binding here (defining it would emit an unwanted dynamic initializer).
extern "C" {
    DATA(0x00283ee8)
    IDirectDraw2* g_DirectDraw = 0; // 0x683ee8
}
DATA(0x00283ec8)
CPtrArray g_modeArray;
DATA(0x00283ee4)
void* g_ddCreateCtx = 0; // 0x683ee4

// The RGB low-bit-position / 8-minus-bitcount pair tables ComputeColorMasks fills from
// the back-buffer's pixel format. DEFINED in src/DDrawMgr/DDSurface.cpp (owner TU);
// GruntzMgr.cpp's 16-bit pack reads the same six words. Reference externs only.

// The post-mask surface-format apply (BuildColorChannelTables @0x13f740, the
// DDSurface.cpp obj); free-fn decl so the bare `call rel32` reloc-masks.
void BuildColorChannelTables();

// The pool items' operator delete (invoked by the scalar-deleting dtors); the
// engine free, reloc-masked rel32.
void operator delete(void*);

// The two pool lists (m_poolA/m_poolB) are real MFC CPtrLists; the drain walks read
// the head node (GetHeadPosition) and advance element-by-element (GetNext) - the
// afxcoll.inl inlines lower to the identical `mov reg,[list+4]` head read + node
// pNext/data walk.

// The shared pool-item base ctor + dtor, defined INLINE here (before the derived
// classes' bodies) so each derived dtor inlines the shared teardown - retail has
// no out-of-line base dtor in this TU; its kept COMDAT is ~CDDSurface @0x141350
// in the DIRSURF obj. See docs/patterns/surface-pool-comdat-dtors.md.
inline CDDSurface::CDDSurface() {
    m_8 = 0;
    m_c = 0;
    m_pos = 0;
    m_dontOwn = 0;
    m_bitDepth = 0;
    m_b8 = 0;
}
inline CDDSurface::~CDDSurface() {
    FreeSurfaces();
}

// 0x1413d0 - set the four GetErrorString reporting-mode flags (log / message-box /
// beep / third) from the four args. __cdecl free helper.
RVA(0x001413d0, 0x27)
void SetDDrawReportModes(i32 log, i32 msgBox, i32 beep, i32 third) {
    g_ddLogEnabled = log;
    g_ddMsgBoxEnabled = msgBox;
    g_ddBeepEnabled = beep;
    g_ddThirdEnabled = third;
}

// CDirectDrawMgr::GetErrorString
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
        MessageBoxA((HWND)0, szLine, "DirectDrawMgr", MB_ICONEXCLAMATION);
    }
}

// Global-mode-array teardown tail-forward (0x141c80): mov ecx,&g_modeArray; jmp
// ??0CPtrArray@@QAE@XZ (0x1b4f0b). Retail re-runs the CPtrArray CTOR in place (stamps
// vtable + zeroes the {m_pData,m_nSize,m_nMaxSize,m_nGrowBy} block), NOT RemoveAll.
// The MSVC5 explicit-ctor-call extension gives the clean guard-free tail-jmp (a
// placement-new would keep the null-check) - docs/patterns/explicit-ctor-call-inplace-
// tail-jmp.md. g_modeArray is now a real MFC CPtrArray, so the ctor reloc binds to
// the HIGH-confidence library label ??0CPtrArray @0x1b4f0b.
RVA(0x00141c80, 0xa)
void ClearModeArray_141c80() {
    g_modeArray.CPtrArray::CPtrArray();
}

// The printf-style TRACE logger (0x141cb0). In the retail RELEASE build the body is
// compiled out to a bare `ret` (1 byte); callers still push the format string + args.
RVA(0x00141cb0, 0x1)
void __cdecl DDrawLogLine(char*, ...) {}

// ---------------------------------------------------------------------------
// Constructor (0x141cc0).  /GX EH frame to unwind the three containers.
// ---------------------------------------------------------------------------
RVA(0x00141cc0, 0x84)
CDDrawPtrCollections::CDDrawPtrCollections() : m_poolA(0xa), m_poolB(0xa), m_array() {
    m_surf0 = 0;
    m_surf4 = 0;
    m_534 = 0;
    m_palBpp = 0;
    m_hasPalette = 0;
    m_940 = 0;
    m_944 = 0;
}

// ---------------------------------------------------------------------------
// Destructor (0x141d50).  Clear(1), then tear down the two CPtrLists + CPtrArray
// (reverse construction order).  /GX EH frame.
// ---------------------------------------------------------------------------
RVA(0x00141d50, 0x6f)
CDDrawPtrCollections::~CDDrawPtrCollections() {
    Clear(1);
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
    (reinterpret_cast<LPDDCAPS>(m_caps))->dwSize = sizeof(DDCAPS);
    (reinterpret_cast<LPDDCAPS>(m_helCaps))->dwSize = sizeof(DDCAPS);
    hr = m_device->GetCaps(reinterpret_cast<LPDDCAPS>(m_caps), reinterpret_cast<LPDDCAPS>(m_helCaps));
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0xad, hr);
    }
    m_bltCaps = (reinterpret_cast<LPDDCAPS>(m_caps))->dwCaps & 0x8000000;
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

// ---------------------------------------------------------------------------
// Clear (0x142060).  mode!=0 -> Release the cached +0x00 surface; free every entry
// in the raw pointer array (+0x4b8/+0x4bc); RemoveAll the CPtrArray; drain both
// pools; null g_DirectDrawMgr; Release+null both cached surfaces; zero +0x534.
// ---------------------------------------------------------------------------
RVA(0x00142060, 0x9d)
void CDDrawPtrCollections::Clear(i32 mode) {
    if (mode && m_surf0) {
        m_surf0->RestoreDisplayMode();
    }
    for (i32 i = 0; i < m_array.GetSize(); i++) {
        ::operator delete(m_array.GetData()[i]);
    }
    m_array.SetSize(0, -1);
    EmptyPoolA();
    EmptyPoolB();
    g_DirectDrawMgr = 0;
    if (m_surf0) {
        m_surf0->Release();
        m_surf0 = 0;
    }
    if (m_surf4) {
        m_surf4->Release();
        m_surf4 = 0;
    }
    m_534 = 0;
}

// ---------------------------------------------------------------------------
// AddItemA (0x142100).  pool.AddTail(item); item->pos = position.
// ---------------------------------------------------------------------------
RVA(0x00142100, 0x18)
void CDDrawPtrCollections::AddItemA(CDDSurface* item) {
    item->m_pos = m_poolA.AddTail(item);
}

// ---------------------------------------------------------------------------
// EmptyPoolA (0x142120).  Walk the +0x47c list, virtual-delete each item, RemoveAll.
// ---------------------------------------------------------------------------
RVA(0x00142120, 0x31)
void CDDrawPtrCollections::EmptyPoolA() {
    POSITION pos = m_poolA.GetHeadPosition();
    while (pos) {
        CDDSurface* item = reinterpret_cast<CDDSurface*>(m_poolA.GetNext(pos));
        delete item;
    }
    m_poolA.RemoveAll();
}

// ---------------------------------------------------------------------------
// RemoveItemA (0x142160).  pool.RemoveAt(item->pos); virtual-delete item.
// ---------------------------------------------------------------------------
RVA(0x00142160, 0x24)
void CDDrawPtrCollections::RemoveItemA(CDDSurface* item) {
    m_poolA.RemoveAt(item->m_pos);
    delete item;
}

// ---------------------------------------------------------------------------
// Create7f0_1 (0x1421a0).  new 0xc0 item; ctor (CByteArray @+0x94, vtbl 0x5ef7f0
// stamped FIRST, then zero fields); dispatch vtbl[0x08] with 1 arg; on success
// AddItemA, else virtual-delete. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall: real-polymorphic `new CDDSurface` now emits the /GX ctor-in-flight
// frame (the throwing CByteArray member ctor), but the global __ehfuncinfo state-index
// push differs from retail (not reproducible from one TU); body byte-exact. Deferred.
RVA(0x001421a0, 0xbe)
CDDSurface* CDDrawPtrCollections::Create7f0_1(i32 a) {
    CDDSurface* item = new CDDSurface;
    if (item->Init1(this, a)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// CreateA (0x142260).  new 0xc0 item, ctor (CFileImageSurface @+0x94 / vtbl 0x5efa58),
// dispatch vtbl[0x24]; on success register via AddItemA, else virtual-delete. /GX.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall: real-polymorphic `new CFileImageSurface` emits the /GX frame; residue is the
// global __ehfuncinfo state-index push (per-TU) + the redundant base-then-derived vptr
// stamp order. Body byte-faithful. Deferred to the final sweep.
RVA(0x00142260, 0xd2)
CDDSurface* CDDrawPtrCollections::CreateA(i32 a, i32 b, i32 c, i32 d, i32 e) {
    CFileImageSurface* item = new CFileImageSurface;
    if (item->ResolveEx(this, (void*)a, b, c, d, e)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// CFileImageSurface::ScalarDelete - the derived surface wrapper's `??_G`
// scalar-deleting destructor (vtable slot 0 @0x5efa58). Run the teardown copy, then -
// when the low bit of the hidden flags arg is set - RezFree the object; return this.
RVA(0x00142340, 0x1e)
void* CFileImageSurface::ScalarDelete(u32 flags) {
    // Qualified call -> direct (non-virtual) dispatch to the 0x142360 teardown copy,
    // matching retail's ??_G which calls the non-deleting dtor directly.
    this->CFileImageSurface::~CFileImageSurface();
    if (flags & 1) {
        ::operator delete(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// CFileImageSurface::~CFileImageSurface - the second compiled teardown copy, byte-
// identical to ~CDDSurface (0x141350). CFileImageSurface now genuinely derives
// CDDSurface and adds no destructible members of its own, so the empty derived dtor
// lowers to the /O2-inlined base teardown: MSVC5 elides the derived (0x1efa58) vptr
// stamp and emits only the base ??_7CDDSurface (0x5ef7f0) stamp, runs the base's
// FreeSurfaces, then destroys the inherited +0x94 CPtrArray under the /GX EH frame.
// The dtor's vtable-stamp reloc thus binds to ??_7CDDSurface @0x1ef7f0 (reloc-fidelity;
// was MISBOUND to this class's own 0x1efa58 when modeled standalone).
RVA(0x00142360, 0x53)
CFileImageSurface::~CFileImageSurface() {}

// ---------------------------------------------------------------------------
// CreateB (0x1423c0).  Same as CreateA but dispatches vtbl[0x2c]. /GX.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (same as CreateA, init slot 11). Body byte-faithful, /GX state-index
// residue. Deferred to the final sweep.
RVA(0x001423c0, 0xd2)
CDDSurface* CDDrawPtrCollections::CreateB(i32 a, i32 b, i32 c, i32 d, i32 e) {
    CFileImageSurface* item = new CFileImageSurface;
    if (item->LoadKeyed(this, a, b, c, d, e)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createa58_1 (0x1424a0).  new 0xc0 item; ctor (vtbl 0x5efa58); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x001424a0, 0xbe)
CDDSurface* CDDrawPtrCollections::Createa58_1(i32 a) {
    CFileImageSurface* item = new CFileImageSurface;
    if (item->Init1(this, a)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createa58_3 (0x142560).  new 0xc0 item; ctor (vtbl 0x5efa58); dispatch vtbl[0x28]
// with 3 args; AddItemA on success. /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142560, 0xc8)
CDDSurface* CDDrawPtrCollections::Createa58_3(i32 a, i32 b, i32 c) {
    CFileImageSurface* item = new CFileImageSurface;
    if (item->LoadByExt(this, reinterpret_cast<char*>(a), b, c)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// The engine CRT sprintf (0x11f890) + the shared ".." $SG constant (0x5ee8ec).
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
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142730, 0xc8)
CDDSurface* CDDrawPtrCollections::Createa88_3(i32 a, i32 b, i32 c) {
    CPoolItemA88* item = new CPoolItemA88;
    if (item->Blit7(this, a, b, c)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// The shared base teardown the derived dtors inline (defined INLINE in the class
// body above): re-stamp the base vptr (0x5ef7f0), run FreeSurfaces(), then destroy
// the owned CByteArray member (auto).  /GX (trylevel 0 -> -1 around the member dtor).
// cl folds the redundant derived vptr stamp (dead store), leaving the base 0x5ef7f0
// stamp - matching retail's per-class inlined dtors.  (Base ~CDDSurface itself is
// CFileImage::~CFileImage @0x141350 in a sibling TU; the inline definition emits no
// out-of-line body here, so it does not collide.)
// ---------------------------------------------------------------------------
// ~CPoolItemA88 (0x142820).  Derived a88 non-deleting dtor - trivial body; inlines the
// base teardown above (INLINE ~CDDSurface: implicit stamp-first, FreeSurfaces, member
// dtor - the a88 vptr stamp folds as a dead store, leaving the base 0x5ef7f0 stamp).
// __thiscall, ret 0x0.  Byte-identical to every other pool-item dtor (the vptr operand
// reloc-masks to 0x5ef7f0); the OWNING class is fixed by its ??_G (0x142800, a88 vtable
// slot 0), not the byte pattern.  Byte-exact.
// ---------------------------------------------------------------------------
RVA(0x00142820, 0x53)
CPoolItemA88::~CPoolItemA88() {}

// ---------------------------------------------------------------------------
// Createa88_1 (0x142880).  new 0xc0 item; ctor (vtbl 0x5efa88); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142880, 0xbe)
CDDSurface* CDDrawPtrCollections::Createa88_1(i32 a) {
    CPoolItemA88* item = new CPoolItemA88;
    if (item->Init1(this, a)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_3 (0x142940).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch vtbl[0x24]
// with 3 args; AddItemA + cache item->m_bitDepth into host->fieldUnknown538 on success.
// /GX. ret 0xc.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142940, 0xd4)
CDDSurface* CDDrawPtrCollections::Createab8_3(i32 a, i32 b, i32 c) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (item->Setup(this, a, b, c)) {
        AddItemA(item);
        m_palBpp = item->m_bitDepth;
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// ~CPoolItemAB8 (0x142a40).  Derived ab8 non-deleting dtor - trivial body; inlines the
// shared base teardown (INLINE ~CDDSurface: stamp 0x5ef7f0 stamp-first + FreeSurfaces +
// member dtor; the ab8 vptr stamp folds as a dead store).  __thiscall, ret 0x0.  Byte-
// identical to the other pool-item dtors; owner fixed by its ??_G (0x142a20, ab8 vtable
// slot 0).  Was formerly mislabeled ~CDDSurface in DDSurfaceDtor.cpp.  Byte-exact.
// ---------------------------------------------------------------------------
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
    if (item->Init1(this, a)) {
        AddItemA(item);
        m_palBpp = item->m_bitDepth;
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createab8_24_3 (0x142b70).  new 0xc0 item; ctor (vtbl 0x5efab8); dispatch
// vtbl[0x24] as a 3-arg init with the two literal tags (0x18, 0x21) + the incoming
// arg; AddItemA + cache item->m_bitDepth into host->fieldUnknown538 on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue). Slot 9
// (0x148af0 == CPoolItemAB8::Setup) takes exactly 4 args (info + 3 ints); this
// site passes {0x18, 0x21, a}, Createab8_3 passes {a, b, c} - one consistent signature.
RVA(0x00142b70, 0xce)
CDDSurface* CDDrawPtrCollections::Createab8_24_3(i32 a) {
    CPoolItemAB8* item = new CPoolItemAB8;
    if (item->Setup(this, 0x18, 0x21, a)) {
        AddItemA(item);
        m_palBpp = item->m_bitDepth;
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// Createae8_6 (0x142c40).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x24]
// as a 6-arg init with all six incoming args; AddItemA on success. /GX. ret 0x18.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142c40, 0xd7)
CDDSurface* CDDrawPtrCollections::Createae8_6(i32 a, i32 b, i32 c, i32 d, i32 e, i32 f) {
    CPoolItemAE8* item = new CPoolItemAE8;
    if (item->Blit47(this, a, b, c, d, e, f)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// ~CPoolItemAE8 (0x142d40).  Derived ae8 non-deleting dtor - trivial body; inlines the
// shared base teardown (INLINE ~CDDSurface: stamp 0x5ef7f0 stamp-first + FreeSurfaces +
// member dtor).  /GX, ret 0x0.  Byte-identical codegen to ~CPoolItemA (0x142820); a
// distinct subclass.  Byte-exact.
// ---------------------------------------------------------------------------
RVA(0x00142d40, 0x53)
CPoolItemAE8::~CPoolItemAE8() {}

// ---------------------------------------------------------------------------
// Createae8_1 (0x142da0).  new 0xc0 item; ctor (vtbl 0x5efae8); dispatch vtbl[0x08]
// with 1 arg; AddItemA on success. /GX. ret 0x4.
// ---------------------------------------------------------------------------
// @early-stop
// EH-state wall (real-polymorphic; body byte-faithful, /GX state-index residue).
RVA(0x00142da0, 0xbe)
CDDSurface* CDDrawPtrCollections::Createae8_1(i32 a) {
    CPoolItemAE8* item = new CPoolItemAE8;
    if (item->Init1(this, a)) {
        AddItemA(item);
        return item;
    }
    delete item;
    return 0;
}

// ---------------------------------------------------------------------------
// MakeAndAddB (0x142e60).  Tail-thunk into CreateB with arg2 |= 0x840.
// ---------------------------------------------------------------------------
RVA(0x00142e60, 0x27)
CDDSurface* CDDrawPtrCollections::MakeAndAddB(i32 a, i32 b, i32 c, i32 d, i32 e) {
    return CreateB(a, b, c, d | 0x840, e);
}

// ---------------------------------------------------------------------------
// AddItemB (0x142eb0).  pool.AddTail(item); item->pos = position.
// ---------------------------------------------------------------------------
RVA(0x00142eb0, 0x17)
void CDDrawPtrCollections::AddItemB(CDDPalette* item) {
    item->m_pos = m_poolB.AddTail(item);
}

// ---------------------------------------------------------------------------
// EmptyPoolB (0x142ed0).  Walk the +0x498 list, tear down + RezFree each item,
// RemoveAll.
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// RemoveItemB (0x142f10).  pool.RemoveAt(item->pos); tear down + RezFree item.
// ---------------------------------------------------------------------------
RVA(0x00142f10, 0x2b)
void CDDrawPtrCollections::RemoveItemB(CDDPalette* item) {
    m_poolB.RemoveAt(item->m_pos);
    if (item) {
        item->Destroy();
        ::operator delete(item);
    }
}

// ---------------------------------------------------------------------------
// MakeB2 (0x142f40).  Sibling of MakeB: RezAlloc a 0x38-byte CDDPalette, zero its
// fields, init it via the alternate Init2 (0x147410) with (m_surf0, a, b); on success
// add to pool B and return it, else tear down + RezFree and return 0.  NO EH frame
// (no destructible local), so this matches cleanly.
// ---------------------------------------------------------------------------
RVA(0x00142f40, 0x7c)
CDDPalette* CDDrawPtrCollections::MakeB2(i32 a, i32 b) {
    CDDPalette* item = new CDDPalette;
    if (!item->LoadFromFile(m_surf0, reinterpret_cast<char*>(a), b)) {
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
// MakeB (0x142fc0).  RezAlloc a 0x38-byte CDDPalette, zero its fields, init it via
// the external Item498_Init (0x1474d0) with (vtbl-of-this, a, b); on success add to
// pool B and return it, else tear down + RezFree and return 0.
// ---------------------------------------------------------------------------
RVA(0x00142fc0, 0x7c)
CDDPalette* CDDrawPtrCollections::MakeB(void* rgb, i32 flags) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateRGB(m_surf0, rgb, flags)) {
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
// Create (0x143040).  Sibling of MakeB2: RezAlloc a 0x38-byte CDDPalette, init it
// via CDDPalette::Create (0x147390) with (m_surf0, a, b); on success add to pool B
// and return it, else tear down + RezFree and return 0.
// ---------------------------------------------------------------------------
RVA(0x00143040, 0x7c)
CDDPalette* CDDrawPtrCollections::Create(i32 a, i32 b) {
    CDDPalette* item = new CDDPalette;
    if (!item->Create(m_surf0, (void*)a, b)) {
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
// MakeB3 (0x1430c0).  Third sibling of MakeB: RezAlloc a 0x38-byte CDDPalette,
// zero its fields, init it via the 3-param Init3 (0x147840) with (m_surf0, a, b,
// c); on success add to pool B and return it, else tear down + RezFree and return
// 0.  No EH frame (no destructible local) -> matches cleanly.
// ---------------------------------------------------------------------------
RVA(0x001430c0, 0x81)
CDDPalette* CDDrawPtrCollections::MakeB3(i32 a, i32 b, i32 c) {
    CDDPalette* item = new CDDPalette;
    if (!item->CreateFromTrailing(m_surf0, (void*)a, b, c)) {
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
    z = 0;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    char buf[0x300];
    if (file.Read(buf, 0x300) != 0x300) {
        return 0;
    }
    return MakeB(buf, z);
}

// ===========================================================================
// CDirectDrawMgr pool-item collection helpers (DDRAWMGR.CPP).  The pool item
// list is a CObArray at +0x4b4; SetupCaps tears the list down, re-enumerates the
// device's display modes (rebuilding a global mode array) and re-sorts the pool;
// CreatePoolItem builds + initialises one pool item from a descriptor source.
// ===========================================================================

// The transient global mode array EnumDisplayModes rebuilds (a real MFC CPtrArray
// @0x683ec8; DEFINED above, near the top of this TU). The m_poolItems array (real MFC
// CPtrArray) + the pool comparator/publisher (Compare/AddPoolItem) live on
// CDirectDrawMgr in <DDrawMgr/DirectDrawMgr.h>.

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
    CPtrArray* arr = &m_poolItems;
    for (i32 i = 0; i < arr->GetSize(); i++) {
        ::operator delete(arr->GetData()[i]);
    }
    arr->SetSize(0, -1);
    g_modeArray.SetSize(0, -1);
    i32 hr = m_device->EnumDisplayModes(0, 0, 0, (LPDDENUMMODESCALLBACK)DdEnumModesCallback);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x507, hr);
    }
    for (i32 j = 0; j < g_modeArray.GetSize(); j++) {
        arr->SetAtGrow(arr->GetSize(), g_modeArray.GetData()[j]);
    }
    g_modeArray.SetSize(0, -1);
    i32 n = arr->GetSize();
    for (i32 a = 0; a < n - 1; a++) {
        for (i32 b = a + 1; b < n; b++) {
            if (Compare(arr->GetData()[a], arr->GetData()[b])) {
                void* tmp = arr->GetData()[a];
                arr->GetData()[a] = arr->GetData()[b];
                arr->GetData()[b] = tmp;
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

// ===========================================================================
// The display-mode pool comparator + searches over m_poolItems (each m_pData[i]
// is a CDdMode*). Folded from Stub/BoundaryUpper.cpp (Compare_1433d0 + ModeArr::
// Find*) - ModeArr IS CDirectDrawMgr (m_4b8/m_4bc = m_poolItems' m_pData/m_nSize).
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
        CDdMode* e = reinterpret_cast<CDdMode*>(m_poolItems.GetData()[idx]);
        out->a = e->m_c;
        out->b = e->m_8;
    }
}

// >= range search from the end (last matching index), else -1.
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

// Exact 3-key match, else -1.
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
    CDdCreateArg* arg0 = (CDdCreateArg*)arg0v;
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
    if (item->Refresh((IDirectDrawSurface*)outA) == 0) {
        delete item;
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
    i32* src = (i32*)pal->m_cacheA;
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
// ~78% mirror-register wall (same family as 0x143900/0x1439b0): retail keeps src in eax
// and pre-increments dst in edx (-1/-4/-3/-2 displacements); MSVC mirrors the src/dst
// registers here. Not source-steerable (permuter marginal). docs/patterns/zero-register-pinning.md.
RVA(0x00143950, 0x56)
CDDPalette* CDDrawPtrCollections::Make950(void* buf, i32 z) {
    if (buf == 0) {
        return 0;
    }
    const u8* src = (const u8*)buf;
    u8* dst = reinterpret_cast<u8*>(m_palette);
    for (i32 i = 0; i < 256; i++) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = 0;
        dst += 4;
        src += 3;
    }
    m_hasPalette = 1;
    m_940 = z;
    return (CDDPalette*)1; // retail returns the success flag as the CDDPalette* result
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

// ---------------------------------------------------------------------------
// Make950Trailing (0x1439f0).  Install the trailing 0x300-byte packed-RGB palette
// from an in-memory file image: bail (return NULL) on a null buffer or a size < 0x3e8
// (too small to hold the palette); otherwise forward (buf + size - 0x300, tag) to
// Make950 (the sibling packed-RGB installer).  __thiscall (ecx=this passed straight
// through to Make950), ret 0xc.
// (RVA-adjacent to the Make950/palette family.)
// ---------------------------------------------------------------------------
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
// @early-stop
// ~98%: same wall as LoadPaletteMakeB (EH funcinfo state index + MakeB-tag const-fold);
// additionally the Make950 callee (0x143950) is still an unreconstructed engine_boundary
// stub, so its rel32 reloc pairs by code bytes but not by symbol name. Deferred.
RVA(0x00143a30, 0xe9)
CDDPalette* CDDrawPtrCollections::LoadPaletteMake950(const char* path, i32 z) {
    CFileIO file;
    z = 0;
    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    char buf[0x300];
    if (file.Read(buf, 0x300) != 0x300) {
        return 0;
    }
    return Make950(buf, z);
}

// ---------------------------------------------------------------------------
// ComputeColorMasks (0x143b20).  Query the device's display-mode pixel format
// (IDirectDraw2::GetDisplayMode), and for each of the R/G/B bit masks
// record the low set-bit position (the shift) and 8-minus-popcount (the scale)
// into the six g_683* globals, then apply (BuildColorChannelTables @0x13f740).
// On a failed query, report via GetErrorString and return 0.  No EH frame.
// ---------------------------------------------------------------------------
RVA(0x00143b20, 0xfc)
i32 CDDrawPtrCollections::ComputeColorMasks() {
    DDSURFACEDESC desc;
    memset(&desc, 0, 0x6c);
    desc.dwSize = 0x6c;
    i32 hr = m_surf0->GetDisplayMode(&desc);
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

// ---------------------------------------------------------------------------
// ConfigureSurface (0x143c20).  Reconfigure the display mode through the device
// (IDirectDraw2::SetDisplayMode - five args forwarded verbatim).  On a non-zero HRESULT, report through
// GetErrorString, latch m_944 = 0x3ec if unset, and return the HRESULT.
// Otherwise recompute the color masks; if that fails, latch m_944 = 0x3ed if
// unset and return E_FAIL (0x80004005).  __thiscall, ret 0x14 (5 stack args). No EH.
// ---------------------------------------------------------------------------
RVA(0x00143c20, 0x84)
i32 CDDrawPtrCollections::ConfigureSurface(i32 a0, i32 a1, i32 a2, i32 a3, i32 a4) {
    i32 hr = m_surf0->SetDisplayMode(a0, a1, a2, a3, a4);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x8a2, hr);
        if (m_944 == 0) {
            m_944 = 0x3ec;
        }
        return hr;
    }
    if (ComputeColorMasks() == 0) {
        hr = static_cast<i32>(0x80004005);
        if (m_944 == 0) {
            m_944 = 0x3ed;
        }
    }
    return hr;
}

SIZE_UNKNOWN(CDdCreateArg);
SIZE_UNKNOWN(CDdDescSrc);
// Size PROVEN from the allocation site (push 0x948; call ??2 -> the ctor), and our
// reconstruction computes exactly that. Pinned so no future note can claim it unknown.
SIZE(CDDrawPtrCollections, 0x948);

// --- vtable catalog ---
