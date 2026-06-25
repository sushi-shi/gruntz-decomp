// CDirectDrawMgr.cpp - the DDrawMgr module's DirectDraw manager group
// (C:\Proj\DDrawMgr\). Three classes share one TU here because they all funnel
// failed HRESULTs through CDirectDrawMgr::GetErrorString:
//
//   * CDirectDrawMgr (DDRAWMGR.CPP / ddrawmgr.h) - the device manager:
//     GetErrorString plus the two DirectDrawCreate bring-up methods.
//   * CDDSurface (DIRSURF.CPP) - thin IDirectDrawSurface wrapper thunks. On a
//     DDERR_SURFACELOST they call the wrapper's own virtual (slot 7) to restore
//     and retry, then report a still-bad HRESULT.
//   * CDDPalette (DIRPAL.CPP) - palette Get/SetEntries thunks.
//
// Every wrapper does iface->vtbl->Method(iface, args...) so the retail
// `call *off(reg)` COM dispatch falls out; only the called slots are pinned.
// Locals are placeholders, the switch case VALUES, GetErrorString (file, line,
// hr) tuples and string contents are load-bearing.
#include <Gruntz/CDirectDrawMgr.h>
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy / memcpy (rep movs / repne scasb)

// MessageBeep / MessageBoxA + BOOL/HWND/LPCSTR/UINT come from the real
// <windows.h> (via Win32.h; pure-Win32 TU, no MFC). The reporter's uType is
// MB_ICONEXCLAMATION (0x30).
#include <Win32.h>

// Reporting-mode globals (live in .data).
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
extern void __cdecl DDrawLogLine(char* line); // 0x141cb0

// The DDERR_NAME line numbers all reference these source-path $SG constants.
#define DIRSURF_FILE "C:\\Proj\\DDrawMgr\\DIRSURF.CPP"
#define DIRPAL_FILE "C:\\Proj\\DDrawMgr\\DIRPAL.CPP"
#define DDRAWMGR_FILE "C:\\Proj\\DDrawMgr\\DDRAWMGR.CPP"
#define DDRAWMGR_H_FILE "C:\\Proj\\DDrawMgr\\ddrawmgr.h"

// DDERR_SURFACELOST - the lost-surface HRESULT the wrappers retry on.
#define DDERR_SURFACELOST 0x887601c2
// DDERR_NOCOLORKEY - GetColorKey's "no key set" non-error.
#define DDERR_NOCOLORKEY 0x887600d7

// DDRAW.dll DirectDrawCreate - direct `e8 rel32` to the incremental-link thunk
// (reloc-masked), like DirectSoundCreate, not an `ff 15 [IAT]` indirect.
extern "C" i32 __stdcall
DirectDrawCreate(void* lpGuid, IDirectDrawSurfaceZ** ppDD, void* pUnkOuter);

// IID_IDirectDraw2 / IID_IDirectDrawSurface3 - dxguid GUID constants in .rdata,
// passed to QueryInterface. Reloc-masked DATA() externs.
DATA(0x001ef848)
extern const u8 IID_IDirectDraw2[16]; // 0x5ef848
DATA(0x001ef888)
extern const u8 IID_IDirectDrawSurface3[16]; // 0x5ef888

// operator new(size_t) - the engine allocator (reloc-masked rel32).
void* operator new(u32);

// The process-wide DirectDraw object + the CDirectDrawMgr singleton (.data).
DATA(0x00283ee8)
extern "C" IDirectDraw2Z* g_DirectDraw; // 0x683ee8
DATA(0x002bed00)
extern "C" CDirectDrawMgr* g_DirectDrawMgr; // 0x6bed00

// ===========================================================================
// CDDSurface (DIRSURF.CPP) - the IDirectDrawSurface wrapper thunks.
// ===========================================================================

// CDDSurface::Lock (__thiscall). Lock(this->m_desc) with NULL rect / flags 1;
// on SURFACELOST restore-and-retry, else report. Returns m_34 on hard fail.
RVA(0x0013e6d0, 0x88)
i32 CDDSurface::Lock(void* rect) {
    i32 hr = m_8->vtbl->Lock(m_8, rect, m_desc, 1, 0);
    if (hr == 0) {
        return m_34;
    }
    if (hr == (i32)DDERR_SURFACELOST) {
        if (RestoreLost() == 0) {
            return 0;
        }
        hr = m_8->vtbl->Lock(m_8, 0, m_desc, 1, 0);
        if (hr == 0) {
            return m_34;
        }
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x203, hr);
        return 0;
    }
    CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x209, hr);
    return 0;
}

// CDDSurface::SetPalette (__thiscall). m_8->SetPalette(pal->m_4).
RVA(0x0013e690, 0x35)
i32 CDDSurface::SetPalette(CDDPalette* pal, i32 unused) {
    i32 hr = m_8->vtbl->SetPalette(m_8, pal->m_4);
    if (hr == 0) {
        return 1;
    }
    CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x1d2, hr);
    return 0;
}

// CDDSurface::Flip (__thiscall). Flip(target->m_8, 1); SURFACELOST retry.
RVA(0x0013e850, 0x93)
i32 CDDSurface::Flip(CDDSurface* target) {
    IDirectDrawSurfaceZ* tsurf = 0;
    if (target != 0) {
        tsurf = target->m_8;
    }
    i32 hr = m_8->vtbl->Flip(m_8, tsurf, 1);
    if (hr == 0) {
        return 0;
    }
    if (hr == (i32)DDERR_SURFACELOST) {
        if (RestoreLost() == 0) {
            return hr;
        }
        hr = m_8->vtbl->Flip(m_8, tsurf, 1);
        if (hr == 0) {
            return 0;
        }
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x2ae, hr);
        return hr;
    }
    CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x2b4, hr);
    return hr;
}

// CDDSurface::SetColorKey (__thiscall). m_8->SetColorKey(flags, key).
RVA(0x0013eaa0, 0x39)
i32 CDDSurface::SetColorKey(u32 flags, void* key) {
    i32 hr = m_8->vtbl->SetColorKey(m_8, flags, key);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x353, hr);
        return hr;
    }
    return 0;
}

// CDDSurface::Blt (__thiscall, ret 4 => 1 arg). Blts src's RECT (src->m_80) into
// this surface's RECT (this->m_80) with flags 0x1000000; SURFACELOST retry.
RVA(0x0013ee60, 0x8d)
i32 CDDSurface::Blt(CDDSurface* src) {
    void* srcRect = src->m_80;
    void* dstRect = m_80;
    i32 hr = m_8->vtbl->Blt(m_8, dstRect, src->m_8, srcRect, 0x1000000, 0);
    if (hr == (i32)DDERR_SURFACELOST) {
        if (RestoreLost()) {
            hr = m_8->vtbl->Blt(m_8, dstRect, src->m_8, srcRect, 0x1000000, 0);
        } else {
            return (i32)DDERR_SURFACELOST;
        }
    }
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x48c, hr);
    }
    return hr;
}

// CDDSurface::BltEx (__thiscall, ret 0x14 => 5 args). Generic Blt with the source
// surface conditional (src may be NULL); SURFACELOST retry.
RVA(0x0013eef0, 0x98)
i32 CDDSurface::BltEx(void* dstRect, CDDSurface* src, void* srcRect, u32 flags, void* fx) {
    i32 hr;
    if (src != 0) {
        hr = m_8->vtbl->Blt(m_8, dstRect, src->m_8, srcRect, flags, fx);
    } else {
        hr = m_8->vtbl->Blt(m_8, dstRect, 0, srcRect, flags, fx);
    }
    if (hr == (i32)DDERR_SURFACELOST) {
        if (RestoreLost()) {
            hr = m_8->vtbl->Blt(m_8, dstRect, src->m_8, srcRect, flags, fx);
        } else {
            return (i32)DDERR_SURFACELOST;
        }
    }
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x4b0, hr);
    }
    return hr;
}

// CDDSurface::BltFast (__thiscall, ret 0x14 => 5 args). SURFACELOST retry.
RVA(0x0013ef90, 0x8b)
i32 CDDSurface::BltFast(u32 x, u32 y, CDDSurface* src, void* srcRect, u32 trans) {
    i32 hr = m_8->vtbl->BltFast(m_8, x, y, src->m_8, srcRect, trans);
    if (hr == (i32)DDERR_SURFACELOST) {
        if (RestoreLost()) {
            hr = m_8->vtbl->BltFast(m_8, x, y, src->m_8, srcRect, trans);
        } else {
            return (i32)DDERR_SURFACELOST;
        }
    }
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x4da, hr);
    }
    return hr;
}

// CDDSurface::GetColorKey (__thiscall). GetColorKey(8, &local); NOCOLORKEY is a
// non-error returning -1; on success returns the key, on error reports + -1.
RVA(0x0013fa60, 0x40)
i32 CDDSurface::GetColorKey() {
    u32 key[2];
    i32 hr = m_8->vtbl->GetColorKey(m_8, 8, key);
    if (hr != (i32)DDERR_NOCOLORKEY) {
        if (hr == 0) {
            return key[0];
        }
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x695, hr);
    }
    return -1;
}

// CDDSurface::Refresh (__thiscall, ret 4 => 1 arg). GetSurfaceDesc into the
// scratch desc, then derive the row/pixel geometry by a bit-depth switch.
RVA(0x0013e140, 0x133)
i32 CDDSurface::Refresh(IDirectDrawSurfaceZ* surf) {
    m_8 = surf;
    i32 i;
    i32* d = (i32*)m_desc;
    for (i = 0x1b; i != 0; i--) {
        *d++ = 0;
    }
    *(i32*)m_desc = 0x6c; // dwSize
    i32 hr = m_8->vtbl->GetSurfaceDesc(m_8, m_desc);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x7e, hr);
    }

    i32 bits = m_64;
    m_bc = 0;
    m_a8 = bits;
    // NOTE: retail emits two MSVC jump tables here (selector = bits-8, range 25),
    // with the tables placed INLINE in .text after the body. Writing the switch
    // with a `case 8` makes MSVC emit the same jump-table structure, but the
    // delinker carves only the 0x133 function body (the inline tables land in a
    // separate base-obj symbol), so objdiff scores that 0%. The branch-chain form
    // below is what objdiff can align (code matches through the switch heads);
    // it is the jump-table plateau, same family as GetErrorString's 96.24%.
    switch (bits) {
        case 16:
            m_ac = *(i32*)(m_desc + 0xc) * 2; // m_1c = dwWidth
            break;
        case 24:
            m_ac = *(i32*)(m_desc + 0xc) * 3;
            break;
        case 32:
            m_ac = *(i32*)(m_desc + 0xc) * 4;
            break;
        default:
            m_ac = *(i32*)(m_desc + 0xc);
            break;
    }

    i32 divisor = 1;
    switch (bits) {
        case 16:
            divisor = 2;
            break;
        case 24:
            divisor = 3;
            break;
        case 32:
            divisor = 4;
            break;
        default:
            divisor = 1;
            break;
    }
    m_b0 = divisor;

    m_88 = *(i32*)(m_desc + 0xc);                     // m_1c (width) cached after switch
    m_b4 = (u32) * (i32*)(m_desc + 0x10) / (u32)m_b0; // m_20 = lPitch
    m_80[0] = 0;
    m_80[1] = 0;
    i32 height = *(i32*)(m_desc + 8); // m_18 = dwHeight
    m_8c = height;
    m_90 = m_ac * height;
    m_7c = m_7c | 1;
    return 1;
}

// ===========================================================================
// CDirectDrawMgr (DDRAWMGR.CPP) - the device manager.
// ===========================================================================

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
    IDirectDraw2Z* dd = g_DirectDraw;
    if (dd != 0) {
        m_0 = dd;
    } else {
        i32 chr = DirectDrawCreate(hwnd, (IDirectDrawSurfaceZ**)&m_4, 0);
        if (chr != 0) {
            CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0x88, chr);
            if (m_944 == 0) {
                m_944 = 0x3e9;
            }
            return 0;
        }
        chr = m_4->vtbl->QueryInterface(m_4, IID_IDirectDraw2, (void**)this);
        if (chr != 0) {
            CDirectDrawMgr::GetErrorString(0, 0, chr);
            if (m_944 == 0) {
                m_944 = 0x3ef;
            }
            return 0;
        }
    }

    i32 hr = m_0->vtbl->SetCooperativeLevel(m_0, hwnd, coopFlags);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_H_FILE, 0x120, hr);
    }
    if (hr != 0) {
        if (m_944 == 0) {
            m_944 = 0x3ea;
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
    m_caps[0] = 0x17c;
    m_helCaps[0] = 0x17c;
    hr = m_0->vtbl->GetCaps(m_0, m_caps, m_helCaps);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0xad, hr);
    }
    m_534 = m_caps[1] & 0x8000000;
    SetupCaps();

    if (width > 0 && height > 0) {
        hr = SetVideoMode(width, height, bpp, 0, 0);
        if (hr != 0) {
            CDirectDrawMgr::GetErrorString(DDRAWMGR_FILE, 0xc2, hr);
            if (m_944 == 0) {
                m_944 = 0x3ec;
            }
            return 0;
        }
        m_538 = bpp;
    }

    if (bpp == 0) {
        i32 desc[0x1b];
        i32 j;
        i32* d = desc;
        for (j = 0x1b; j != 0; j--) {
            *d++ = 0;
        }
        desc[0] = 0x6c;
        hr = m_0->vtbl->GetDisplayMode(m_0, desc);
        if (hr == 0) {
            m_538 = desc[0x15];
        }
    }

    g_DirectDrawMgr = this;
    return 1;
}

// ===========================================================================
// CDDPalette (DIRPAL.CPP) - the palette wrapper thunks.
// ===========================================================================

// CDDPalette::Create (__thiscall, ret 0xc => 3 args). Caches a copy of the
// PALETTEENTRY array (m_c), allocates a second cache (m_10), then creates the
// DirectDraw palette via IDirectDraw::CreatePalette into m_4.
RVA(0x00147390, 0x78)
i32 CDDPalette::Create(IDirectDraw2Z* dd, void* entries, u32 flags) {
    m_c = (u8*)operator new(0x400);
    // Plateau note: byte-for-byte except the copy loop's SIB base/index roles
    // (retail encodes [entries+i]/[m_c+i] with i as the index; MSVC here makes i
    // the base) - a 1-byte-per-insn encoding choice, semantically identical.
    for (i32 i = 0; i < 0x400; i += 4) {
        *(i32*)(m_c + i) = *(i32*)((char*)entries + i);
    }
    m_10 = (u8*)operator new(0x400);
    i32 hr = dd->vtbl->CreatePalette(dd, flags, entries, &m_4, 0);
    if (hr == 0) {
        return 1;
    }
    CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x4b, hr);
    return 0;
}

// CDDPalette::CreateRGB (__thiscall, ret 0xc => 3 args). Takes a packed 256x3
// RGB-triplet array, expands it on the stack into PALETTEENTRY[256] (peFlags=0),
// then forwards to Create. The 0x400-byte stack buffer drives `sub esp,0x400`.
RVA(0x001474d0, 0x60)
i32 CDDPalette::CreateRGB(IDirectDraw2Z* dd, void* rgb, u32 flags) {
    u8 entries[0x400];
    u8* src = (u8*)rgb;
    for (i32 i = 0; i < 0x100; i++) {
        entries[i * 4 + 0] = src[0];
        entries[i * 4 + 1] = src[1];
        entries[i * 4 + 2] = src[2];
        entries[i * 4 + 3] = 0;
        src += 3;
    }
    return Create(dd, entries, flags);
}

// CDDPalette::Destroy (__thiscall, no args). Nulls m_0/m_4/m_8, frees the three
// owned buffers (m_c/m_10/m_18) via operator delete, clears m_34. m_4 is only
// nulled (not Released) here.
RVA(0x00147530, 0x54)
void CDDPalette::Destroy() {
    m_0 = 0;
    m_8 = 0;
    if (m_4 != 0) {
        m_4 = 0;
    }
    if (m_c != 0) {
        operator delete(m_c);
        m_c = 0;
    }
    if (m_10 != 0) {
        operator delete(m_10);
        m_10 = 0;
    }
    if (m_18 != 0) {
        operator delete(m_18);
        m_18 = 0;
    }
    m_34 = 0;
}

// CDDPalette::GetEntries (__thiscall, ret 0 => no args). Lazily allocates the
// readback cache (m_10), then reads all 256 entries; reports a bad HRESULT.
RVA(0x00147c30, 0x4d)
i32 CDDPalette::GetEntries() {
    // Lazily allocates the readback cache, then reads all 256 entries; reports a
    // bad HRESULT. Plateau note: retail's body falls off the end (no return -
    // the int symbol returns whatever sits in eax), so it omits the trailing
    // `xor eax,eax` this `return 0` emits. MSVC 5.0 C++ forbids fall-off
    // (C2561), so the one-instruction `xor` gap can't be reproduced from clean C.
    if (m_10 == 0) {
        m_10 = (u8*)operator new(0x400);
        if (m_10 == 0) {
            return 0;
        }
    }
    i32 hr = m_4->vtbl->GetEntries(m_4, 0, 0, 0x100, m_10);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x265, hr);
    }
    return 0;
}

// CDDPalette::SetRange (__thiscall, ret 0x18 => 6 args).
RVA(0x00147cd0, 0x78)
i32 CDDPalette::SetRange(i32 start, i32 count, u8 r, u8 g, u8 b, u32 flags) {
    for (i32 i = start; i < start + count; i++) {
        m_c[i * 4 + 0] = r;
        m_c[i * 4 + 1] = g;
        m_c[i * 4 + 2] = b;
    }
    i32 hr = m_4->vtbl->SetEntries(m_4, flags, start, count, m_c + start * 4);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x2a3, hr);
    }
    return hr;
}

// ===========================================================================
// CDDPageMgr (DDrawMgr) - the primary-surface bring-up (second DirectDrawCreate
// caller). On a failed COM call it routes through its own HandleError, not
// GetErrorString.
// ===========================================================================

// CDDPageMgr::Init (__thiscall, ret 0xc => 3 args). Creates its own DirectDraw,
// QueryInterfaces IDirectDraw2, sets the cooperative level + display mode,
// creates the primary surface (QI'd to IDirectDrawSurface3) and, for 8bpp, a
// palette; caches the geometry and shows the cursor.
RVA(0x0017c040, 0x25d)
i32 CDDPageMgr::Init(void* window, DDModeInfo* mode, u32 coopFlags) {
    if (m_4 != 0) {
        return 0;
    }
    if (window == 0) {
        return 0;
    }

    i32 w, h, bpp;
    if (mode != 0) {
        w = mode->width;
        h = mode->height;
        bpp = mode->bpp;
    } else {
        w = 0x280;
        h = 0x1e0;
        bpp = 8;
    }

    m_c = 0;
    if (DirectDrawCreate(0, (IDirectDrawSurfaceZ**)&m_18, 0) != 0) {
        return 0;
    }
    if (m_18->vtbl->QueryInterface(m_18, IID_IDirectDraw2, (void**)&m_14) != 0) {
        return 0;
    }
    if (m_14->vtbl->SetCooperativeLevel(m_14, window, coopFlags) != 0) {
        HandleError();
        return 0;
    }
    if (m_14->vtbl->SetDisplayMode(m_14, w, h, bpp, 0, 0) != 0) {
        HandleError();
        return 0;
    }

    i32 i;
    i32* d = (i32*)m_desc;
    for (i = 0x1b; i != 0; i--) {
        *d++ = 0;
    }
    *(i32*)m_desc = 0x6c;
    m_24 = 1;                       // [esi+0x34]=1 -> desc field? actually m_34; placeholder
    *(i32*)(m_desc + 0x68) = 0x200; // [esi+0x98]
    if (m_14->vtbl->CreateSurface(m_14, m_desc, &m_20, 0) != 0) {
        HandleError();
        return 0;
    }

    if (m_20->vtbl->QueryInterface(m_20, IID_IDirectDrawSurface3, (void**)&m_1c) != 0) {
        return 0;
    }

    OnModeSet(w);

    if (mode->bpp == 8) {
        if (m_14->vtbl->CreatePalette(m_14, 4, m_108, &m_2c, 0) != 0) {
            HandleError();
            return 0;
        }
        m_1c->vtbl->SetPalette(m_1c, m_2c);
        m_510 = 0;
    }

    if (mode->bpp == 0x18) {
        HandleError();
        return 0;
    }
    if (mode->bpp == 0x10) {
        if (CheckMode16() == 0) {
            HandleError();
            return 0;
        }
    }

    m_518 = w;
    m_24 = 0;
    m_28 = 0;
    m_51c = h;
    m_520 = bpp;
    m_0 = (IDirectDraw2Z*)window;
    m_c = 0;
    ShowCursor(0);
    m_4 = 1;
    FinishInit();
    return 1;
}
