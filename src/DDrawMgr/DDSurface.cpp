// DDSurface.cpp - CDDSurface, the ORIGINAL C:\Proj\DDrawMgr\DIRSURF.CPP TU
// (interval dossier #14G): one obj spanning retail 0x13e060-0x1413cb.
// The __FILE__ assert string is referenced from 16 sites across the whole span; the
// obj's single init frag (@0x13e060) + its atexit companion ClearImageCache_13e070
// lead it. The former image / fileimage / fileimageblit / fileimagerundecode /
// lutshaderect units' fns in this span were fn-granularity-interleaved slices of
// this file and are folded in, in retail-RVA order: the IDirectDrawSurface COM
// thunks, the surface geometry/blit/save ops, the pixel-format converter blitters
// (Blit<dest><src>), the in-surface RLE decoders (DecodeRun8/24 - retail compiled
// these UNOPTIMIZED: `#pragma optimize("",off)` islands, see below), the CImage
// factory/cache, and the rotate-blit forwarders. RezMgr::MakeImageKey (0x13e5d0,
// text-contained) rides along. EH sites at Build_13e9a0 + ~CDDSurface prove the
// obj is /GX.
//
// On a DDERR_SURFACELOST the COM thunks call the wrapper's own virtual RestoreLost
// (slot 7) to restore + retry, then route a still-bad HRESULT through
// CDirectDrawMgr::GetErrorString. The class's pool ctor/dtor emission anchor is
// the DDRAWMGR TU (DirectDrawMgr.cpp); ~CDDSurface's kept COMDAT copy (0x141350)
// is THIS obj's.
#include <Io/FileStream.h>
#include <Rez/RezAlloc.h> // RezAlloc/RezFree

#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/DDrawPtrCollections.h> // the palette/pool context the decoders read
#include <Image/Image.h>                  // CFileImageElement / CFileImageSrc
#include <ddraw.h> // real DirectDraw SDK (IDirectDrawSurface, DDBLTFX, DDCOLORKEY, DDERR_*/DDBD_*/DDSCAPS_*)
#include <rva.h>
#include <stdio.h>
#include <string.h>  // inline strcpy / memcpy / memset
#include <Globals.h> // g_clut (bank planes = interior slices; RGB565 shift/size ints are local: g_rUp/g_gUp/g_rDown/g_gDown/g_bDown)

#define DIRSURF_FILE "C:\\Proj\\DDrawMgr\\DIRSURF.CPP"

// The engine logger DDrawLogLine (0x141cb0) is declared in <DDrawMgr/DirectDrawMgr.h>
// (included below). Rez heap + operator new (reloc-masked):
void* operator new(u32); // engine allocator (reloc-masked rel32)

// The live screen RGB-format shift table (same globals ShadeTableCache reads): the
// per-channel "up" shifts (ea0/ea4/ea8 = R/G/B) and the device "down" widths
// (eac/eb0/eb4). Reloc-masked DIR32.
// g_lut16 (the 16-bit palette LUT, 256 u16) sits just before g_rUp (0x283ca0..0x283ea0):
// DEFINED here (owner ddsurface.obj's .bss, zero-init) - REHOME DD-D, extern-only.
DATA(0x00283ca0)
u16 g_lut16[256] = {0}; // 0x683ca0
// The live screen RGB565 pixel-format shift/size ints (0x683ea0..0x683eb4, .bss set at
// device init). DEFINED here (owner TU); the g_pf* names (g_rUp/GreenSize/RedSize/
// GreenShift/BlueSize) in Globals.cpp were a redundant SECOND labeling of these same
// datums - FOLDED onto the canonical g_rUp/g_gUp/g_rDown/g_gDown/g_bDown. Reference
// externs stay local in the using TUs. (REHOME DD-G, conflation resolved)
DATA(0x00283ea0)
i32 g_rUp; // 0x683ea0  (== ex g_rUp)
DATA(0x00283ea4)
i32 g_gUp; // 0x683ea4  (== ex g_gUp)
DATA(0x00283ea8)
i32 g_bUp; // 0x683ea8
DATA(0x00283eac)
i32 g_rDown; // 0x683eac  (== ex g_rDown)
DATA(0x00283eb0)
i32 g_gDown; // 0x683eb0  (== ex g_gDown)
DATA(0x00283eb4)
i32 g_bDown; // 0x683eb4  (== ex g_bDown)

// The engine RECT-copier fn-ptr (0x6c44bc), used by ShadeBlt to snapshot the rects.

// The global image cache the new item is filed into: a real MFC CPtrArray holding
// the 0xc0 CDDSurface pool items the 0x13e9a0 factory below builds. Stored
// in retail .data at 0x653c88; DATA-referenced (reloc-masked), so declared extern.
DATA(0x00253c88)
CPtrArray g_imageCache;
// g_imageCacheIndex (the next-free slot, an i32 right after the 8-byte CPtrArray)
// DEFINED here (owner ddsurface.obj's .bss, zero-init) - REHOME DD-D, extern-only.
DATA(0x00253c90)
i32 g_imageCacheIndex = 0; // 0x653c90
// The 3-plane 16bpp colour LUT (0x653c9e; G plane @+0, B @+0x10000, R @+0x20000 -
// 0x30000 bytes ending just before g_lut16 @0x683ca0). .bss, built at runtime. The
// per-plane read pointers are interior slices of g_clut: bank1 (G) = g_clut+0x2
// (0x653ca0), bank2 (B) = g_clut+0x10002 (0x663ca0), bank0 (R) = g_clut+0x20002
// (0x673ca0) - SUBSUMED, no separate bank symbols. DEFINED here (owner TU), reference
// extern stays in <Globals.h>. (REHOME DD-G / DD-Drain-1)
DATA(0x00253c9e)
u8 g_clut[0x30000]; // 0x653c9e

// Global image-cache in-place reconstruction (0x13e070): mov ecx,&g_imageCache; jmp
// ??0CPtrArray@@QAE@XZ (0x1b4f0b). This is the in-place re-construct of the CPtrArray
// (vptr-stamp + zero the 4 dwords), NOT RemoveAll (which frees the storage). The
// explicit-ctor-call extension gives the clean tail-jmp with no placement-new
// null-guard (docs/patterns/explicit-ctor-call-inplace-tail-jmp.md). Folded from
// Stub/BoundaryUpper.cpp.
RVA(0x0013e070, 0xa)
void ClearImageCache_13e070() {
    g_imageCache.CPtrArray::CPtrArray();
}

// ---------------------------------------------------------------------------
// CDDSurface::Init1 (0x13e0a0, vtable slot 2): if the descriptor arg (a, a 0x6c-byte
// Blk6c* passed as an int handle) is non-null, copy it into the +0x10 DDSURFACEDESC
// scratch, then dispatch the surface own slot-8 BlitIntoDesc with the collection context h;
// return its result. Folded from Stub/BoundaryUpper.cpp (ImgOwned::Apply - the view
// IS CDDSurface; the "ambiguous CDDSurface::Init1" note is resolved). Decl in DDSurface.h.
// ---------------------------------------------------------------------------
RVA(0x0013e0a0, 0x27)
i32 CDDSurface::Init1(CDDrawPtrCollections* h, i32 a) {
    if (a != 0) {
        memcpy((char*)this + 0x10, (const void*)a, 0x6c);
    }
    return BlitIntoDesc(h);
}

// ---------------------------------------------------------------------------
// CDDSurface::BlitSurf
// The DecodePcxData destination setup: zero the surface's DDSURFACEDESC, stash
// the colour-key arg (m_78), record width/height into the desc, set dwSize/
// dwFlags, and - when a4 names a non-zero source bpp that differs from the
// palette context's bpp (surf->m_palBpp, the display manager passed in) - flag a colour-key blit (dwFlags|0x1000,
// ddckCKSrcBlt dwFlags 0x20, the key colour at m_64). Then dispatch the surface's
// own slot-8 virtual with `surf`.
RVA(0x0013e0d0, 0x66)
i32 CDDSurface::BlitSurf(void* surf, i32 width, i32 height, i32 a4, i32 a5) {
    i32* desc = (i32*)(this->m_desc);
    for (i32 i = 0x1b; i != 0; i--) {
        *desc++ = 0;
    }
    *(i32*)(this->m_desc + 0x68) = a5; // m_78
    this->m_width = width;
    this->m_height = height;
    *(i32*)(this->m_desc) = 0x6c;  // dwSize
    *(i32*)(this->m_desc + 4) = 7; // dwFlags
    if (a4 != 0 && a4 != ((CDDrawPtrCollections*)surf)->m_palBpp) {
        *(i32*)(this->m_desc + 4) = 0x1007;
        *(i32*)(this->m_desc + 0x48) = 0x20; // m_58
        this->m_64 = a4;
    }
    return this->BlitIntoDesc(surf); // slot-8 virtual dispatch (+0x20)
}

// CDDSurface::Refresh (__thiscall, ret 4 => 1 arg). GetSurfaceDesc into the
// scratch desc, then derive the row/pixel geometry by a bit-depth switch. The
// desc geometry is read through the named DDSURFACEDESC fields (m_descSize/
// m_height/m_width/m_pitch) of the surface's embedded scratch.
RVA(0x0013e140, 0x133)
i32 CDDSurface::Refresh(IDirectDrawSurface* surf) {
    m_8 = surf;
    i32 i;
    i32* d = (i32*)m_desc;
    for (i = 0x1b; i != 0; i--) {
        *d++ = 0;
    }
    m_descSize = 0x6c;
    i32 hr = m_8->GetSurfaceDesc((LPDDSURFACEDESC)m_desc);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x7e, hr);
    }

    i32 bits = m_64;
    m_bc = 0;
    m_bitDepth = bits;
    // NOTE: retail emits two MSVC jump tables here (selector = bits-8, range 25),
    // with the tables placed INLINE in .text after the body. Writing the switch
    // with a `case 8` makes MSVC emit the same jump-table structure, but the
    // delinker carves only the 0x133 function body (the inline tables land in a
    // separate base-obj symbol), so objdiff scores that 0%. The branch-chain form
    // below is what objdiff can align (code matches through the switch heads);
    // it is the jump-table plateau, same family as GetErrorString's 96.24%.
    switch (bits) {
        case 16:
            m_ac = m_width * 2;
            break;
        case 24:
            m_ac = m_width * 3;
            break;
        case 32:
            m_ac = m_width * 4;
            break;
        default:
            m_ac = m_width;
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

    m_88 = m_width;                  // dwWidth cached after switch
    m_b4 = static_cast<u32>(m_pitch) / static_cast<u32>(m_b0); // lPitch / divisor
    m_80[0] = 0;
    m_80[1] = 0;
    i32 height = m_height;
    m_8c = height;
    m_90 = m_ac * height;
    m_dontOwn = m_dontOwn | 1;
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::FreeSurfaces (vtable slot 4, @+0x10) - the shared surface teardown.
// Walk the +0x94 CPtrArray (m_pData@0x98, count@0x9c, unsigned) running each
// element's slot-0 scalar-deleting destructor, RemoveAll the array (SetSize(0,-1)),
// then - unless the "don't-own" flag (m_dontOwn & 1) is set - Release the two held
// IDirectDrawSurfaces (m_8/m_c) and null them, and clear m_b8.
RVA(0x0013e4d0, 0x7e)
void CDDSurface::FreeSurfaces() {
    for (u32 i = 0; i < static_cast<u32>(m_elements.GetSize()); i++) {
        CFileImageElement* e = (CFileImageElement*)m_elements[i];
        delete e;
    }
    m_elements.SetSize(0, -1);
    if (this->m_8 != 0) {
        if ((this->m_dontOwn & 1) == 0) {
            this->m_8->Release();
        }
        this->m_8 = 0;
    }
    if (this->m_c != 0) {
        if ((this->m_dontOwn & 1) == 0) {
            this->m_c->Release();
        }
        this->m_c = 0;
    }
    this->m_b8 = 0;
}

// ---------------------------------------------------------------------------
// CDDSurface::Resolve
// The file-format dispatcher (the .REZ payload path). `type` (1=BMP, 2=PCX,
// 4=PID) selects the matching decoder; the destination buffer arg (`size`) must
// be non-zero. Each decoder is handed (surf, buf, size); PID additionally takes
// the transparency-colour pass-through (surf2). Returns 1 on a successful decode,
// else 0. The near-consecutive case labels lower to MSVC's running-subtract chain
// (dec/dec/sub 2), so this is spelled as a switch (see
// docs/patterns/switch-subtract-chain-vs-ifelse.md), case bodies in retail .text
// order (4, 2, 1).
RVA(0x0013e550, 0x71)
i32 CDDSurface::Resolve(void* surf, void* buf, i32 type, u32 size, void* surf2) {
    if (size == 0) {
        return 0;
    }
    switch (type) {
        case FMT_PID:
            if (!DecodePid(surf, buf, size, surf2)) {
                return 0;
            }
            break;
        case FMT_PCX:
            if (!DecodePcx(surf, buf, size)) {
                return 0;
            }
            break;
        case FMT_BMP:
            if (!DecodeBmp(surf, buf, size)) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::MakeImageKey (0x13e5d0; text contained in THIS obj, between Resolve
// and SetPalette). Dispatch a resource load by the file extension of `name`:
// locate the last '.' (strrchr @0x120680), then case-insensitively match
// (_strcmpi @0x11fdf0) .BMP/.PCX/.PID and hand off to the matching loader on
// `this` (LoadBmp/LoadPcx take (name,path); LoadPid takes (name,path,arg3)).
// Returns 1 unless the extension matched but its loader failed (then 0); an
// unrecognised/absent extension also returns 1. (`this` == the CDDSurface loading
// the resource - the calls target CDDSurface::LoadBmp/Pcx/Pid @0x144110/0x145110/
// 0x145cd0.)
RVA(0x0013e5d0, 0xb1)
i32 CDDSurface::MakeImageKey(void* arg1, char* name, void* arg3) {
    char* ext = strrchr(name, '.');
    if (ext && _strcmpi(ext, ".BMP") == 0) {
        if (!LoadBmp((char*)arg1, name)) {
            return 0;
        }
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        if (!LoadPcx((char*)arg1, name)) {
            return 0;
        }
    } else if (ext && _strcmpi(ext, ".PID") == 0) {
        if (!LoadPid((char*)arg1, name, arg3)) {
            return 0;
        }
    }
    return 1;
}

// CDDSurface::SetPalette (__thiscall). m_8->SetPalette(pal->m_palette).
RVA(0x0013e690, 0x35)
i32 CDDSurface::SetPalette(CDDPalette* pal, i32 unused) {
    i32 hr = m_8->SetPalette(pal->m_palette);
    if (hr == 0) {
        return 1;
    }
    CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x1d2, hr);
    return 0;
}

// CDDSurface::Lock (__thiscall). Lock(this->m_desc) with NULL rect / flags 1;
// on SURFACELOST restore-and-retry, else report. Returns m_lockBits on hard fail.
RVA(0x0013e6d0, 0x88)
i32 CDDSurface::Lock(void* rect) {
    i32 hr = m_8->Lock((LPRECT)rect, (LPDDSURFACEDESC)m_desc, 1, 0);
    if (hr == 0) {
        return m_lockBits;
    }
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost() == 0) {
            return 0;
        }
        hr = m_8->Lock(0, (LPDDSURFACEDESC)m_desc, 1, 0);
        if (hr == 0) {
            return m_lockBits;
        }
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x203, hr);
        return 0;
    }
    CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x209, hr);
    return 0;
}

// ---------------------------------------------------------------------------
// CDDSurface::Fill
// Colour-fill blt: build a zeroed DDBLTFX (dwSize 0x64, dwFillColor = `color` at
// +0x50) and Blt(NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT (0x1000400), &fx)
// through the surface's BltEx thunk. A bad HRESULT routes through
// CDirectDrawMgr::GetErrorString (DIRSURF.CPP, line 0x22c). Returns hr == DD_OK.
RVA(0x0013e760, 0x63)
i32 CDDSurface::Fill(u32 color) {
    i32 fx[0x19]; // DDBLTFX (0x64 bytes)
    i32* p = fx;
    for (i32 i = 0x19; i != 0; i--) {
        *p++ = 0;
    }
    fx[0] = 0x64;          // dwSize
    fx[0x14] = static_cast<i32>(color); // dwFillColor @ +0x50
    i32 hr = this->BltEx(0, 0, 0, 0x1000400, fx);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString((char*)"C:\\Proj\\DDrawMgr\\DIRSURF.CPP", 0x22c, hr);
    }
    return hr == 0;
}

// CDDSurface::Restore (__thiscall, 0x13e7d0):
// color-fill the dstRect region with `fillColor` via a DDBLT_COLORFILL Blt (BltEx). The
// stack DDBLTFX is zeroed (dwSize=0x64, dwFillColor=fillColor); reports a non-zero
// HRESULT through GetErrorString. Guarded on a non-null rect; returns 1 on success.
RVA(0x0013e7d0, 0x73)
i32 CDDSurface::Restore(void* dstRect, i32 fillColor) {
    if (dstRect == 0) {
        return 0;
    }
    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(DDBLTFX);
    fx.dwFillColor = fillColor;
    i32 hr = BltEx(dstRect, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
    if (hr) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x26d, hr);
    }
    return hr == 0;
}

// CDDSurface::Flip (__thiscall). Flip(target->m_8, 1); SURFACELOST retry.
RVA(0x0013e850, 0x93)
i32 CDDSurface::Flip(CDDSurface* target) {
    IDirectDrawSurface* tsurf = 0;
    if (target != 0) {
        tsurf = target->m_8;
    }
    i32 hr = m_8->Flip(tsurf, 1);
    if (hr == 0) {
        return 0;
    }
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost() == 0) {
            return hr;
        }
        hr = m_8->Flip(tsurf, 1);
        if (hr == 0) {
            return 0;
        }
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x2ae, hr);
        return hr;
    }
    CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x2b4, hr);
    return hr;
}

// @early-stop
// 0x13e8f0 (176 B) = a DIRSURF image-cache reload (__thiscall): scalar-deletes each
// g_imageCache element, clears this->m_94 (CDdObArray) + g_imageCache, rebuilds via
// this->m_8->EnumAttachedSurfaces(0, &EnumSurfacesCallback) (slot 9), reports through
// CDirectDrawMgr::GetErrorString, then repopulates m_94 from g_imageCache. Homed from
// GapFunctions.cpp (matcher-5); lives in the DIRSURF block (DDSurface.cpp) by RVA.
// Homed pending the owning class (m_8 vtable + m_94 array) + the Build PMF push modelled.
RVA(0x0013e8f0, 0xb0)
i32 Gap_13e8f0(void) {
    return 0;
}

// The IID passed to the QI below is the REAL SDK GUID IID_IDirectDrawSurface3
// (its bytes at 0x5ef888 ARE {DA044E00-69B2-11D0-A1D5-00AA00B8DFBB}; DEFINED in
// DDPageMgr.cpp).
extern "C" const GUID IID_IDirectDrawSurface3; // 0x5ef888

// ---------------------------------------------------------------------------
// 0x13e9a0: the IDirectDrawSurface::EnumAttachedSurfaces callback (DDENUMSURFACESCALLBACK,
// __stdcall / WINAPI, 3 args -> ret 0xc; disasm-proven, `mov eax,1; ret 0xc`). Passed as
// the fn-ptr to m_8->EnumAttachedSurfaces (vtable slot 9) by the image-cache reload at
// 0x13e8f0. For each enumerated surface: QueryInterface (slot 0, `call [surf_vtable]` with
// `this`=surf PUSHED - a __stdcall COM call, NOT thiscall) it for IID_IDirectDrawSurface3;
// on S_OK (== 0) wrap the v3 interface in a fresh CDDSurface (`new CDDSurface` = the retail
// operator-new(0xc0) + inlined ctor: CPtrArray @+0x94, vptr stamp 0x5ef7f0, 6 field zeros),
// Refresh caches its geometry, and file it into the global image cache; a failed QI or
// Refresh drops it (delete = slot-0 scalar-deleting dtor under the null-guard). `desc`/`ctx`
// are unused. The throwing CPtrArray member ctor gives the /GX ctor-in-flight EH frame.
//
// The former CImageFactory (fabricated __thiscall receiver - `this` is never touched, and
// the true fn is a FREE __stdcall callback) + CRezImageSource (the "probe source", really a
// real <ddraw.h> IDirectDrawSurface COM interface whose slot 0 is IUnknown::QueryInterface)
// the QI probe builds on QI == S_OK (retail).
// @early-stop
// ~62%: complete + correct reconstruction (QI slot 0, `new CDDSurface`, slot-1 Refresh,
// SetAtGrow/delete, ret 0xc all disasm-verified). Residual is (a) `new CDDSurface` here
// emits an EXTERNAL ctor call because CDDSurface's inline ctor body lives in
// DirectDrawMgr.cpp (not DDSurface.h), while retail INLINES the ctor at this new-site;
// (b) the /GX ctor-in-flight EH-state index (the Create7f0_1/CreateA factory-EH family
// wall). Fix for (a) = move CDDSurface::CDDSurface() inline into DDSurface.h (touches a
// widely-included header - deferred to avoid the butterfly mid-cleanup).
RVA(0x0013e9a0, 0xcc)
i32 __stdcall EnumSurfacesCallback(IDirectDrawSurface* surf, DDSURFACEDESC* desc, void* ctx) {
    void* payload = 0;
    if (surf->QueryInterface(IID_IDirectDrawSurface3, &payload) == 0) {
        CDDSurface* item = new CDDSurface;
        if (item->Refresh((IDirectDrawSurface*)payload)) { // slot 1 @+0x04
            g_imageCache.SetAtGrow(g_imageCacheIndex, item);
        } else if (item) {
            delete item; // slot 0 @+0x00  scalar-deleting dtor
        }
    }
    return 1; // DDENUMRET_OK (continue enumeration)
}

// CDDSurface::GetElementAt (__thiscall): bounds-checked m_elements[i], or 0.
RVA(0x0013ea70, 0x21)
void* CDDSurface::GetElementAt(i32 i) {
    if (i >= 0 && i < m_elements.GetSize()) {
        return m_elements.GetAt(i);
    }
    return 0;
}

// CDDSurface::SetColorKey (__thiscall). m_8->SetColorKey(flags, key).
RVA(0x0013eaa0, 0x39)
i32 CDDSurface::SetColorKey(u32 flags, void* key) {
    i32 hr = m_8->SetColorKey(flags, (LPDDCOLORKEY)key);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x353, hr);
        return hr;
    }
    return 0;
}

// CDDSurface color-key convenience overloads (0x13eae0/0x13eb10/0x13eb80): build a
// DDCOLORKEY {low,high} on the stack and forward to SetColorKey.
RVA(0x0013eae0, 0x24)
i32 CDDSurface::SetColorKeyVal(u32 flags, u32 key) {
    DDCOLORKEY ck;
    ck.dwColorSpaceLowValue = key;
    ck.dwColorSpaceHighValue = key;
    return SetColorKey(flags, &ck);
}
RVA(0x0013eb10, 0x28)
i32 CDDSurface::SetColorKeyRange(u32 flags, u32 lo, u32 hi) {
    DDCOLORKEY ck;
    ck.dwColorSpaceLowValue = lo;
    ck.dwColorSpaceHighValue = hi;
    return SetColorKey(flags, &ck);
}

// ---------------------------------------------------------------------------
// CDDSurface::FillPalette
// Installs the transparency colour. arg == -1 means "no colour key": clear the
// have-key flag (m_bc) and pass {-1,-1}; otherwise set m_bc and set the surface
// source colour key to {arg, arg} (DDCKEY_SRCBLT = 8).
RVA(0x0013eb40, 0x3c)
void CDDSurface::FillPalette(u32 key) {
    u32 ck[2];
    ck[0] = key;
    ck[1] = key;
    if (static_cast<i32>(key) != -1) {
        this->m_bc = 1;
    } else {
        this->m_bc = 0;
    }
    this->SetColorKey(8, ck);
}

RVA(0x0013eb80, 0x21)
i32 CDDSurface::SetDestColorKey(u32 key) {
    DDCOLORKEY ck;
    ck.dwColorSpaceLowValue = key;
    ck.dwColorSpaceHighValue = key;
    return SetColorKey(DDCKEY_DESTBLT, &ck);
}

// ---------------------------------------------------------------------------
// FlipVertical - swap the locked surface's rows top-to-bottom through a
// one-row temp buffer. No-op for a <= 1-row image. Locks the surface (Lock), and on
// success allocates the temp row; a failed temp alloc unlocks and returns. __thiscall.
//
// @early-stop
// regalloc wall (~62%): logic + offsets + CFG + the 3 inner row-copy loops are exact.
// Residue is the callee-saved-register assignment cascade - retail pins `this` in ebx
// (every member read is [ebx+N]) and the loop var i in esi, while our cl assigns this
// to ebp and spills i to [esp+0x10]; the prologue push order (push esi before the
// first member read) and the running bottom-row pointer (add ecx,ebx) all cascade from
// that one choice. docs/patterns/zero-register-pinning.md + reread-member-view-pointer.md.
// Not source-steerable on a leaf this small; deferred to the final sweep.
RVA(0x0013ebb0, 0x126)
void CDDSurface::FlipVertical() {
    if (m_height <= 1) {
        return;
    }
    u8* buf = (u8*)Lock(0);
    if (buf == 0) {
        return;
    }
    u8* tmp = (u8*)operator new(m_width);
    if (tmp == 0) {
        m_8->Unlock(0);
        return;
    }

    i32 width = m_width;
    i32 i = 0;
    i32 half = m_height / 2;
    if (half > 0) {
        do {
            // top row -> tmp
            u8* top = buf + i * m_pitch;
            i32 j = 0;
            if (width > 0) {
                do {
                    tmp[j] = *top;
                    ++top;
                    ++j;
                } while (j < width);
            }
            // bottom row -> top row
            i32 botRow = m_height - i - 1;
            u8* topDst = buf + i * m_pitch;
            u8* botSrc = buf + botRow * m_pitch;
            if (width > 0) {
                i32 k = width;
                do {
                    *topDst = *botSrc;
                    ++topDst;
                    ++botSrc;
                    --k;
                } while (k != 0);
            }
            // tmp -> bottom row
            u8* botDst = buf + botRow * m_pitch;
            i32 m = 0;
            if (width > 0) {
                do {
                    ++botDst;
                    botDst[-1] = tmp[m];
                    ++m;
                } while (m < width);
            }
            ++i;
        } while (i < half);
    }

    m_8->Unlock(0);
    RezFree(tmp);
}

// ---------------------------------------------------------------------------
// CDDSurface::BlitDirect
// Straight copy of `src` into the locked surface. Lock() returns the locked bits
// pointer (m_34); on failure return 0. Each row of m_ac bytes is copied into the
// row at locked + row*lPitch; mode 2 walks rows bottom-up (flipped), else top-
// down. Unlock and return 1.
RVA(0x0013ece0, 0xc7)
i32 CDDSurface::BlitDirect(void* src, i32 mode) {
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* p = (u8*)src;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            u8* sp = p;
            i32 n = this->m_ac;
            for (i32 i = n; i > 0; i--) {
                *dst++ = *sp++;
            }
            p += n;
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u8* dst = locked + row * this->m_pitch;
            u8* sp = p;
            i32 n = this->m_ac;
            for (i32 i = n; i > 0; i--) {
                *dst++ = *sp++;
            }
            p += n;
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::Clear (ret 4) - blank the surface. Build a zeroed 0x64-byte DDBLTFX
// on the stack (dwSize@+0x0 = 0x64, fill flags@+0x8 = 0x42 | (white ? 0xff0020 :
// 0)), Blt(NULL, NULL, NULL, 0x1020000, &fx) through the held surface, and on a
// non-zero (failed/lost) HRESULT colour-fill it white (0xff) or black (0) via Fill.
RVA(0x0013edb0, 0x78)
void CDDSurface::Clear(i32 white) {
    DDBLTFX fx;
    i32* p = (i32*)&fx;
    for (i32 i = 0x19; i != 0; i--) {
        *p++ = 0;
    }
    fx.dwSize = 0x64;
    fx.dwROP = white ? static_cast<i32>(0xff0062) : 0x42; // WHITENESS : BLACKNESS (DDBLT_ROP)
    i32 hr = this->m_8->Blt(0, 0, 0, 0x1020000, &fx);
    if (hr != 0) {
        if (white != 0) {
            Fill(0xff);
        } else {
            Fill(0);
        }
    }
}

// ---------------------------------------------------------------------------
// 0x13ee30 - a surface flip-wait: `while(m_8->Flip(2) == DDERR_WASSTILLDRAWING);`. The
// +0x8 surface is an IDirectDrawSurface-STYLE object but NOT the ddraw.h one (retail's
// Flip pushes ONE arg here, slot 0x48), so it is modeled as a minimal own-vtable view -
// distinct from CDDSurface (whose Flip is 0x13e850). Placeholder identity, RVA-adjacent
// to CDDSurface::Clear. Re-homed from src/Stub/BoundaryUpper.cpp.
// 0x13ee30 - CDDSurface::WaitFlip: spin until the held surface's pending flip retires.
// The IDDS_ee30 (18 filler slots) + B_13ee30 views that stood here are GONE - they are
// the REAL <ddraw.h> IDirectDrawSurface and THIS class:
//   * "Flip at slot 18 (+0x48)" IS IDirectDrawSurface::GetFlipStatus - slot 18 of the
//     real DX surface vtable (QI/AddRef/Release/AddAttachedSurface/AddOverlayDirtyRect/
//     Blt/BltBatch/BltFast/DeleteAttachedSurface/EnumAttachedSurfaces/EnumOverlayZOrders/
//     Flip/GetAttachedSurface/GetBltStatus/GetCaps/GetClipper/GetColorKey/GetDC/
//     GetFlipStatus). Its argument 2 is DDGFS_ISFLIPDONE and the compared 0x8876021c is
//     DDERR_WASSTILLDRAWING - the spin is textbook "wait for the flip".
//   * B_13ee30's m_8 @+0x08 IS CDDSurface::m_8, the same held IDirectDrawSurface every
//     other method in this TU already dispatches on.
RVA(0x0013ee30, 0x29)
void CDDSurface::WaitFlip() {
    while (m_8->GetFlipStatus(DDGFS_ISFLIPDONE) == DDERR_WASSTILLDRAWING) {
    }
}

// CDDSurface::Blt (__thiscall, ret 4 => 1 arg). Blts src's RECT (src->m_80) into
// this surface's RECT (this->m_80) with flags 0x1000000; SURFACELOST retry.
RVA(0x0013ee60, 0x8d)
i32 CDDSurface::Blt(CDDSurface* src) {
    void* srcRect = src->m_80;
    void* dstRect = m_80;
    i32 hr = m_8->Blt((LPRECT)dstRect, src->m_8, (LPRECT)srcRect, 0x1000000, 0);
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost()) {
            hr = m_8->Blt((LPRECT)dstRect, src->m_8, (LPRECT)srcRect, 0x1000000, 0);
        } else {
            return static_cast<i32>(DDERR_SURFACELOST);
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
        hr = m_8->Blt((LPRECT)dstRect, src->m_8, (LPRECT)srcRect, flags, (LPDDBLTFX)fx);
    } else {
        hr = m_8->Blt((LPRECT)dstRect, 0, (LPRECT)srcRect, flags, (LPDDBLTFX)fx);
    }
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost()) {
            hr = m_8->Blt((LPRECT)dstRect, src->m_8, (LPRECT)srcRect, flags, (LPDDBLTFX)fx);
        } else {
            return static_cast<i32>(DDERR_SURFACELOST);
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
    i32 hr = m_8->BltFast(x, y, src->m_8, (LPRECT)srcRect, trans);
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost()) {
            hr = m_8->BltFast(x, y, src->m_8, (LPRECT)srcRect, trans);
        } else {
            return static_cast<i32>(DDERR_SURFACELOST);
        }
    }
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x4da, hr);
    }
    return hr;
}

// CDDSurface::ShadeBlt (__thiscall, ret 0x10 => 4 args). 16bpp shaded-blend blit:
// copy the two RECTs, validate (m_b0==2 + equal dims + in-bounds), Lock this + src,
// per row copy the old dst pixels into a temp then blend temp+src through the three
// shade-level colour LUTs (bank = ((shade&0xff)>>3)<<0xb), selected by the live
// pixel-format globals (565 vs 555). Unlock both + free the temp; 0 on any reject.
// @early-stop
// regalloc-cascade wall (~65%): complete + correct - both the 565 and 555 three-LUT
// blend loops, the 10-check validation, the Lock/Unlock (slot 0x80) and RezAlloc temp
// are all reconstructed with the right operations. The residual is an MSVC5
// register-allocation coin-flip seeded in the setup (validation keeps dstW/srcW in regs
// + recomputes dstRowAdv from the rect fields where cl reuses the validation locals),
// which cascades a register-name shift through the whole body. The permuter finds no
// operand-order fix, and caching ::CopyRect in a local (to match retail's `mov edi;
// call edi`) REGRESSED it 65->62 - proving the cascade is not source-steerable. Banked
// for the final sweep.
RVA(0x0013f020, 0x43f)
i32 CDDSurface::ShadeBlt(
    struct tagRECT* dstRect,
    CDDSurface* src,
    struct tagRECT* srcRect,
    i32 shade
) {
    RECT dr, sr;
    ::CopyRect(&dr, dstRect);
    ::CopyRect(&sr, srcRect);
    if (m_b0 != 2) {
        return 0;
    }
    i32 srcW = sr.right - sr.left;
    i32 dstW;
    dstW = dr.right - dr.left;
    if (srcW != dstW) {
        return 0;
    }
    i32 srcH = sr.bottom - sr.top;
    i32 dstH = dr.bottom - dr.top;
    if (srcH != dstH) {
        return 0;
    }
    if (dr.left < 0) {
        return 0;
    }
    if (dr.top < 0) {
        return 0;
    }
    if (dr.right > m_width) {
        return 0;
    }
    if (dr.bottom > m_height) {
        return 0;
    }
    if (sr.left < 0) {
        return 0;
    }
    if (sr.top < 0) {
        return 0;
    }
    if (sr.right > srcW) {
        return 0;
    }
    if (sr.bottom > srcH) {
        return 0;
    }

    u16* dstBits = (u16*)Lock(0);
    u16* srcBits = (u16*)src->Lock(0);
    i32 dstStride = m_pitch / 2;
    u16* dstPtr = dstBits + (dr.top * dstStride + dr.left);
    i32 srcStride = src->m_pitch / 2;
    u16* srcPtr = srcBits + (sr.top * srcStride + sr.left);
    i32 dstRowAdv = dstStride - dstW;
    i32 srcRowAdv = srcStride - srcW;
    u16* temp = (u16*)RezAlloc(dstW * 4);
    i32 bank = ((shade & 0xff) >> 3) << 0xb;

    if (g_rDown == 3 && g_gDown == 3 && g_bDown == 3 && g_rUp == 0xa && g_gUp == 5) {
        // 565
        i32 rows = dstH;
        if (rows > 0) {
            do {
                memcpy(temp, dstPtr, dstW * 2);
                i32 n = dstW;
                if (n > 0) {
                    u16* t = temp;
                    do {
                        u32 tp = *t;
                        u32 sp = *srcPtr;
                        u16 v = *(u16*)((g_clut + 0x10002) + bank
                                        + (((tp & 0x1f) << 5) + (sp & 0x1f)) * 2);
                        v |= *(u16*)((g_clut + 0x20002) + bank
                                     + ((sp >> 0xa) + ((tp >> 5) & ~0x1f)) * 2);
                        v |= *(u16*)((g_clut + 0x2) + bank
                                     + ((((tp >> 5) & 0x1f) << 5) + (0x1f & (sp >> 5))) * 2);
                        *dstPtr = v;
                        dstPtr++;
                        srcPtr++;
                        t++;
                    } while (--n != 0);
                }
                dstPtr += dstRowAdv;
                srcPtr += srcRowAdv;
            } while (--rows != 0);
        }
    } else if (g_rDown == 3 && g_gDown == 2 && g_bDown == 3 && g_rUp == 0xb && g_gUp == 5) {
        // 555
        i32 rows = dstH;
        if (rows > 0) {
            do {
                memcpy(temp, dstPtr, dstW * 2);
                i32 n = dstW;
                if (n > 0) {
                    u16* t = temp;
                    do {
                        u32 tp = *t;
                        u32 sp = *srcPtr;
                        u16 v = *(u16*)((g_clut + 0x10002) + bank
                                        + (((tp & 0x1f) << 5) + (sp & 0x1f)) * 2);
                        v |= *(u16*)((g_clut + 0x20002) + bank
                                     + ((sp >> 0xb) + ((tp >> 6) & ~0x1f)) * 2);
                        v |= *(u16*)((g_clut + 0x2) + bank
                                     + ((((tp >> 6) & 0x1f) << 5) + ((sp >> 6) & 0x1f)) * 2);
                        *dstPtr = v;
                        dstPtr++;
                        srcPtr++;
                        t++;
                    } while (--n != 0);
                }
                dstPtr += dstRowAdv;
                srcPtr += srcRowAdv;
            } while (--rows != 0);
        }
    } else {
        RezFree(temp);
        m_8->Unlock(0);
        src->m_8->Unlock(0);
        return 0;
    }
    m_8->Unlock(0);
    src->m_8->Unlock(0);
    RezFree(temp);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::ShadeRect (0x13f460). __thiscall ShadeRect(pct, clip): validate + clip the target
// rectangle, scale the fade percentage into a LUT bank offset, then walk the
// surface rectangle row-by-row (copy the row to a scratch line, split each
// RGB565/555 pixel and recombine the three channels through the three shade-LUT
// banks, write back in place), then notify the surface + free the scratch line.
// @early-stop
// regalloc wall (~67%, was a 0.9% `return 0` stub). Logic + offsets + structure
// are faithful (verified base-vs-target via llvm-objdump -dr: the clip/CopyRect,
// scale imul-by-100, geometry, operator-new, memcpy row copy, both config-gated
// variants and the notify/free tails all line up instruction-for-instruction).
// The residual is a whole-function register-coloring divergence: retail pins
// this->edi and pct/off->esi and uses direct `test`/`jl` on the clip fields,
// while cl pins this->esi and materializes a zero in edi (regalloc-zero-pin) for
// the same compares - so every [this+off] ModRM byte and the channel-split reg
// choices differ. A regalloc coin-flip, not a codegen miss.
RVA(0x0013f460, 0x2da)
i32 CDDSurface::ShadeRect(i32 pct, RECT* clip) {
    if (pct > 100) {
        return 0;
    }
    RECT rc;
    if (clip) {
        if (clip->left < 0) {
            return 0;
        }
        if (clip->right > m_width) {
            return 0;
        }
        if (clip->top < 0) {
            return 0;
        }
        if (clip->bottom > m_height) {
            return 0;
        }
        CopyRect(&rc, clip);
    } else {
        rc.left = 0;
        rc.top = 0;
        rc.right = m_width;
        rc.bottom = m_height;
    }
    i32 scale = pct * 32 / 100;
    u16* src = (u16*)Lock(0);
    i32 rowPix = m_pitch / 2;
    u16* srcPix = src + rc.top * rowPix + rc.left;
    i32 stride = rc.left - rc.right + rowPix;
    i32 width = rc.right - rc.left;
    i32 height = rc.bottom - rc.top;
    u16* scratch = (u16*)operator new(width * 4);
    i32 off = scale << 11;

    if (g_rDown == 3) {
        if (g_gDown == 3 && g_bDown == 3 && g_rUp == 0xa && g_gUp == 5) {
            for (; height > 0; height--) {
                memcpy(scratch, srcPix, width * 2);
                u16* rd = scratch;
                for (i32 x = width; x > 0; x--) {
                    u32 p = *rd++;
                    u32 blue = p & 0x1f;
                    u32 hi = p >> 5;
                    u32 green = hi & 0x1f;
                    u32 red = hi & 0xffffffe0;
                    *srcPix++ = static_cast<u16>((*(u16*)((char*)(g_clut + 0x10002) + off + (blue << 6))
                                      | *(u16*)((char*)(g_clut + 0x2) + off + (green << 6))
                                      | *(u16*)((char*)(g_clut + 0x20002) + off + red * 2)));
                }
                srcPix += stride;
            }
        } else if (g_gDown == 2 && g_bDown == 3 && g_rUp == 0xb && g_gUp == 5) {
            for (; height > 0; height--) {
                memcpy(scratch, srcPix, width * 2);
                u16* rd = scratch;
                for (i32 x = width; x > 0; x--) {
                    u32 p = *rd++;
                    u32 blue = p & 0x1f;
                    u32 hi = p >> 6;
                    u32 green = hi & 0x1f;
                    u32 red = hi & 0xffffffe0;
                    *srcPix++ = static_cast<u16>((*(u16*)((char*)(g_clut + 0x10002) + off + (blue << 6))
                                      | *(u16*)((char*)(g_clut + 0x2) + off + (green << 6))
                                      | *(u16*)((char*)(g_clut + 0x20002) + off + red * 2)));
                }
                srcPix += stride;
            }
        } else {
            operator delete(scratch);
            m_8->Unlock(0);
            return 0;
        }
    } else {
        operator delete(scratch);
        m_8->Unlock(0);
        return 0;
    }

    m_8->Unlock(0);
    operator delete(scratch);
    return 1;
}

// The three 64 KB RGB channel-spread lookup tables, one contiguous 0x30000 block:
// G @0x653c9e, B @+0x10000, R @+0x20000. Indexed by a byte offset built from the
// (a<<11) block + 2*n. Modeled as one base so the three writes keep retail's three
// independent disp32 encodings (each masked by its own DIR32 reloc).

// BuildColorChannelTables (0x13f740, __cdecl) - precompute the per-channel CLUTs that
// map a (row, hi, lo) triple onto a packed 16-bit colour. The 32x32x32 nest folds a
// row/col interpolation (rounded /32) into a channel sum that is shifted into the R/G/B
// field positions. The common 555 device (rUp==0xa, gUp==5, all downs==3) takes a fast
// path with hard-coded R<<10 / G<<5 shifts; every other format re-reads the live shifts
// per write (green gets an extra <<1).
// @early-stop
// scheduling tail (~97%): logic + the (a<<11)+2n index + the single-base disp32 table
// writes are byte-exact; retail computes the blue `sum<<bShift` one slot earlier (before
// the green store) than our cl schedules it. Hoisting it into a temp spills (regresses to
// ~90%); not source-steerable. Entropy tail. topic:wall.
RVA(0x0013f740, 0x1c8)
void BuildColorChannelTables() {
    if (g_rDown == 3 && g_gDown == 3 && g_bDown == 3 && g_rUp == 0xa && g_gUp == 5) {
        i32 bShift = g_bUp;
        i32 a = 0;
        i32 stepA = 0x20;
        do {
            i32 base = a << 0xb;
            i32 varB = 0;
            i32 countB = 0x20;
            do {
                i32 bDiv = varB / 32;
                i32 varD = 0;
                i32 k = 0x20;
                do {
                    base += 2;
                    i32 sum = varD / 32 + bDiv;
                    *(i16*)(g_clut + 0x20000 + base) = static_cast<i16>((sum << 0xa));
                    *(i16*)(g_clut + base) = static_cast<i16>((sum << 5));
                    *(i16*)(g_clut + 0x10000 + base) = static_cast<i16>((sum << bShift));
                    varD += stepA;
                } while (--k != 0);
                varB += a;
            } while (--countB != 0);
            a++;
        } while (--stepA > 0);
    } else {
        i32 a = 0;
        i32 stepA = 0x20;
        do {
            i32 base = a << 0xb;
            i32 varB = 0;
            i32 countB = 0x20;
            do {
                i32 bDiv = varB / 32;
                i32 varD = 0;
                i32 k = 0x20;
                do {
                    base += 2;
                    i32 sum = varD / 32 + bDiv;
                    *(i16*)(g_clut + 0x20000 + base) = static_cast<i16>((sum << g_rUp));
                    *(i16*)(g_clut + base) = static_cast<i16>(((sum << g_gUp) << 1));
                    *(i16*)(g_clut + 0x10000 + base) = static_cast<i16>((sum << g_bUp));
                    varD += stepA;
                } while (--k != 0);
                varB += a;
            } while (--countB != 0);
            a++;
        } while (--stepA > 0);
    }
}

// ---------------------------------------------------------------------------
// CDDSurface::SaveFile (ret 0x10) - the surface SAVE entry point. Bail (return 0)
// unless the surface is valid (slot-5 IsValid), `buf` is non-null and non-empty
// (*buf != 0), and `type` == 1. Then hand (buf, a3, a4) to the per-bit-depth
// dispatcher and return its result.
RVA(0x0013f910, 0x4a)
i32 CDDSurface::SaveFile(char* buf, i32 type, void* a3, void* a4) {
    if (this->IsValid() == 0) { // slot-5 virtual dispatch (+0x14)
        return 0;
    }
    if (buf == 0) {
        return 0;
    }
    if (*buf == 0) {
        return 0;
    }
    switch (type) {
        case 1:
            return SaveDispatch(buf, a3, a4);
        default:
            return 0;
    }
}

// CDDSurface::RestoreLost (__thiscall, slot 7, 0x13f960): if a per-surface restore
// callback is installed (m_b8) and succeeds, done (1); otherwise run the shared
// CDDrawPtrCollections restore trampoline and report failure (0).
extern i32 RestoreLostSurfaces_1437f0(); // 0x1437f0 (BoundaryUpper2.cpp)
RVA(0x0013f960, 0x22)
i32 CDDSurface::RestoreLost() {
    if (m_b8 != 0) {
        if (m_b8(this) != 0) {
            return 1;
        }
    }
    RestoreLostSurfaces_1437f0();
    return 0;
}

// CDDSurface::Tile (__thiscall, ret 8 => 2 args). Tile the source surface across
// this surface with raw IDirectDrawSurface::BltFast (DDBLTFAST_WAIT, +SRCCOLORKEY
// when useColorKey), clipping the source rect for the right/bottom edge tiles.
RVA(0x0013f990, 0xc4)
void CDDSurface::Tile(CDDSurface* src, i32 useColorKey) {
    i32 dwTrans = 0x10 + (useColorKey != 0); // DDBLTFAST_WAIT (+DDBLTFAST_SRCCOLORKEY)
    for (i32 y = 0; y < m_height; y += src->m_height) {
        for (i32 x = 0; x < m_width; x += src->m_width) {
            RECT rect;
            RECT* pRect = 0;
            if (x + src->m_width >= m_width || y + src->m_height >= m_height) {
                rect.left = 0;
                rect.top = 0;
                i32 w = m_width - x;
                if (w >= src->m_width) {
                    w = src->m_width;
                }
                rect.right = w;
                i32 h = m_height - y;
                if (h >= src->m_height) {
                    h = src->m_height;
                }
                rect.bottom = h;
                pRect = &rect;
            }
            m_8->BltFast(x, y, src->m_8, pRect, dwTrans);
        }
    }
}

// CDDSurface::GetColorKey (__thiscall). GetColorKey(8, &local); NOCOLORKEY is a
// non-error returning -1; on success returns the key, on error reports + -1.
RVA(0x0013fa60, 0x40)
i32 CDDSurface::GetColorKey() {
    DDCOLORKEY key;
    i32 hr = m_8->GetColorKey(8, &key);
    if (hr != static_cast<i32>(DDERR_NOCOLORKEY)) {
        if (hr == 0) {
            return key.dwColorSpaceLowValue;
        }
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x695, hr);
    }
    return -1;
}

// ---------------------------------------------------------------------------
// CDDSurface::Blit
// Palette-remap copy dispatcher. Selects a specialization by (dest bpp = m_bitDepth,
// src bpp = bitcount). When m_bitDepth==0 / bitcount agree on the "no remap" fast path
// it delegates to BlitDirect; otherwise a nested switch on dest bpp (8/16/24)
// then src bpp picks the matching Blit<dest><src> specialization. Unhandled
// combinations return 0.
RVA(0x0013faa0, 0x108)
i32 CDDSurface::Blit(void* src, i32 bitcount, void* palette, i32 mode) {
    i32 dest = this->m_bitDepth;
    if ((dest == 0) == bitcount) {
        return BlitDirect(src, mode);
    }
    switch (dest) {
        case 8:
            switch (bitcount) {
                case 0x10:
                    return Blit816(src, palette, mode);
                case 0x18:
                    return Blit824(src, palette, mode);
            }
            return 0;
        case 0x10:
            switch (bitcount) {
                case 8:
                    return Blit168(src, palette, mode);
                case 0x18:
                    return Blit1624(src, mode);
            }
            return 0;
        case 0x18:
            switch (bitcount) {
                case 8:
                    return Blit248(src, palette, mode);
                case 0x10:
                    return Blit2416(src, mode);
            }
            return 0;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CDDSurface::Blit168  (8bpp src -> 16bpp dest, palette remap)
// Build a 256-entry 16bpp LUT from the source palette (the RGB shift table packs
// each {R,G,B} entry into a screen-native 16bpp word), then walk the surface row
// by row writing LUT[index] per source pixel.
// @early-stop
// Regalloc wall (~66%): the LUT-build loop's two loop-carried pointers (palette,
// LUT) take both callee-saved slots, pushing `this` out of esi into edi - which
// cascades into the blit loop (retail keeps this=esi/src=edi with no spill; ours
// shifts this=edi/src=edx and spills the source index to the stack). Logic exact;
// the inner LUT-lookup idiom is correct, only the register file is permuted.
RVA(0x0013fbb0, 0x126)
i32 CDDSurface::Blit168(void* srcv, void* palv, i32 mode) {
    u8* pal = (u8*)palv;
    if (pal == 0) {
        return 0;
    }
    u16* lut = g_lut16;
    do {
        u8 r = static_cast<u8>((static_cast<u8>(pal[0]) >> g_rDown));
        pal += 4;
        u8 g = static_cast<u8>((static_cast<u8>(pal[-3]) >> g_gDown));
        u8 b = static_cast<u8>((static_cast<u8>(pal[-2]) >> g_bDown));
        *lut++ = static_cast<u16>(((static_cast<u32>(r) << g_rUp) | (static_cast<u32>(g) << g_gUp) | static_cast<u32>(b)));
    } while (lut < g_lut16 + 256);
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* src = (u8*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = g_lut16[idx];
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = g_lut16[idx];
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::Blit1624  (24bpp src -> 16bpp dest)
// Pack each B,G,R source triple straight into a screen-native 16bpp word.
// @early-stop
// Entropy wall (~71%): the per-pixel 3-byte read + shift-pack needs a stack temp
// under register pressure; retail's spill-slot scheduling and the 8/16-bit shift
// narrowing (movb vs movzx) of the channel packs diverge from our equivalent
// codegen. Logic exact; documented MSVC5 /O2 register-allocation plateau.
RVA(0x0013fce0, 0x17f)
i32 CDDSurface::Blit1624(void* srcv, i32 mode) {
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* src = (u8*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 b = src[0];
                u8 g = src[1];
                u8 r = src[2];
                src += 3;
                *dst++ = static_cast<u16>(((static_cast<u32>((static_cast<u8>((static_cast<u8>(g) >> g_gDown)))) << g_gUp)
                               | (static_cast<u32>((static_cast<u8>((static_cast<u8>(r) >> g_rDown)))) << g_rUp)
                               | static_cast<u32>((static_cast<u8>((static_cast<u8>(b) >> g_bDown))))));
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 b = src[0];
                u8 g = src[1];
                u8 r = src[2];
                src += 3;
                *dst++ = static_cast<u16>(((static_cast<u32>((static_cast<u8>((static_cast<u8>(g) >> g_gDown)))) << g_gUp)
                               | (static_cast<u32>((static_cast<u8>((static_cast<u8>(r) >> g_rDown)))) << g_rUp)
                               | static_cast<u32>((static_cast<u8>((static_cast<u8>(b) >> g_bDown))))));
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::Blit2416  (16bpp src -> 24bpp dest, 6-byte/pixel word writes)
// Unpack each 16bpp word into an R,G,B triple, each stored as a zero-extended
// 16bpp word (the retail dest stride is 6 bytes per source pixel).
// @early-stop
// Entropy wall (~82%): the 16->8-bit shift narrowing on each channel unpack
// (shr bx then shl bl, with the shift-count load width varying word/dword) and
// the one stack temp are MSVC5 /O2 register-allocation coin-flips. Logic exact.

// ---------------------------------------------------------------------------
// CDDSurface::Blit248  (8bpp src -> 24bpp dest, palette remap)
// Lock the surface, walk it row-by-row (mode 2 = bottom-up flipped, else top-
// down) writing each source palette index's RGBQUAD bytes (2,1,0) as 3 dest
// bytes, then Unlock. Returns 0 if the palette is null or the lock fails.
// @early-stop
// 94.3% - both inner conversion loops byte-exact; residual is an edi<->ebp
// induction-variable allocation swap (src pinned in edi vs retail's ebp, which
// propagates a different ModRM byte through every src reference in both loops)
// + retail's `cmp $2,[esp+mode]` memory compare vs our reg-loaded `mov ecx,
// [mode];cmp ecx,2`. Both stem from `src` being a single live variable across
// the two branches (loaded before the mode test); a per-branch `src` flips the
// load late but un-spills `locked` and breaks the `push ecx` frame (drops to
// 91%). Regalloc-ordering wall (docs/patterns/zero-register-pinning.md).
RVA(0x0013fe60, 0x11e)
i32 CDDSurface::Blit248(void* srcv, void* palv, i32 mode) {
    u8* pal = (u8*)palv;
    if (pal == 0) {
        return 0;
    }
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* src = (u8*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = pal[idx * 4 + 2];
                *dst++ = pal[idx * 4 + 1];
                *dst++ = pal[idx * 4];
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = pal[idx * 4 + 2];
                *dst++ = pal[idx * 4 + 1];
                *dst++ = pal[idx * 4];
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// CDDSurface / CFileImageSurface / CRezSurfaceItem are all real-polymorphic
// now: cl emits their ??_7 and stamps the vptr (compiler-implicit, stamp-first)
// in the ctor/dtor. The shared surface vtable (0x5ef7f0) reloc-masks; no manual
// base-surface vtable extern/stamp remains (all-vtables mandate).

RVA(0x0013ff80, 0x184)
i32 CDDSurface::Blit2416(void* srcv, i32 mode) {
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u16* src = (u16*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                dst[0] = static_cast<u16>(static_cast<u8>((static_cast<u8>(static_cast<u16>((px >> g_rUp))) << g_rDown)));
                dst[1] = static_cast<u16>(static_cast<u8>((static_cast<u8>(static_cast<u16>((px >> g_gUp))) << g_gDown)));
                dst[2] = static_cast<u16>(static_cast<u8>((static_cast<u8>(px) << g_bDown)));
                dst += 3;
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = (u16*)(locked + row * this->m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                dst[0] = static_cast<u16>(static_cast<u8>((static_cast<u8>(static_cast<u16>((px >> g_rUp))) << g_rDown)));
                dst[1] = static_cast<u16>(static_cast<u8>((static_cast<u8>(static_cast<u16>((px >> g_gUp))) << g_gDown)));
                dst[2] = static_cast<u16>(static_cast<u8>((static_cast<u8>(px) << g_bDown)));
                dst += 3;
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::Blit824  (24bpp src -> 8bpp dest, nearest-palette quantize)
// For each B,G,R source triple, find the palette index whose entry minimizes the
// sum of squared channel differences (entry 0 seeds the best; entries 1..255 are
// scanned, breaking early on an exact match), and write that index.
// @early-stop
// Entropy wall (large /O2 body, ~0x30b): the SSD inner search spills the source
// channels and the best/bestdist accumulators across ~7 stack temps; retail's
// exact spill-slot scheduling is an MSVC5 register-allocation coin-flip. Logic
// (channel pairing s0<->pal[+2], s1<->pal[+1], s2<->pal[+0], min-SSD, exact-match
// break) is faithful; only the regalloc/scheduling of the spills diverges.
RVA(0x00140110, 0x30b)
i32 CDDSurface::Blit824(void* srcv, void* palv, i32 mode) {
    u8* pal = (u8*)palv;
    if (pal == 0) {
        return 0;
    }
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* src = (u8*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                i32 s0 = src[0];
                i32 s1 = src[1];
                i32 s2 = src[2];
                src += 3;
                i32 best = 0;
                i32 d0 = s2 - pal[0];
                i32 d1 = s1 - pal[1];
                i32 d2 = s0 - pal[2];
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < 256; k++) {
                    i32 e0 = s2 - pal[k * 4];
                    i32 e1 = s1 - pal[k * 4 + 1];
                    i32 e2 = s0 - pal[k * 4 + 2];
                    i32 d = e0 * e0 + e1 * e1 + e2 * e2;
                    if (d < bestd) {
                        best = k;
                        bestd = d;
                        if (d == 0) {
                            break;
                        }
                    }
                }
                *dst = static_cast<u8>(best);
                dst++;
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                i32 s0 = src[0];
                i32 s1 = src[1];
                i32 s2 = src[2];
                src += 3;
                i32 best = 0;
                i32 d0 = s2 - pal[0];
                i32 d1 = s1 - pal[1];
                i32 d2 = s0 - pal[2];
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < 256; k++) {
                    i32 e0 = s2 - pal[k * 4];
                    i32 e1 = s1 - pal[k * 4 + 1];
                    i32 e2 = s0 - pal[k * 4 + 2];
                    i32 d = e0 * e0 + e1 * e1 + e2 * e2;
                    if (d < bestd) {
                        best = k;
                        bestd = d;
                        if (d == 0) {
                            break;
                        }
                    }
                }
                *dst = static_cast<u8>(best);
                dst++;
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::Blit816  (16bpp src -> 8bpp dest, nearest-palette quantize)
// Unpack each 16bpp source word into an R,G,B triple (via the screen shift table),
// then find the palette index minimizing the sum of squared channel differences
// (entry 0 seeds the best; 1..255 scanned, exact-match break) and write it.
// @early-stop
// Entropy wall (large /O2 body, ~0x34f): the 16bpp unpack + SSD search spills the
// three channels and the best/bestdist accumulators across ~8 stack temps; retail's
// exact spill-slot scheduling and the 8/16-bit unpack narrowing are MSVC5 /O2
// register-allocation coin-flips. Logic (RGB unpack, red<->pal[0]/green<->pal[1]/
// blue<->pal[2] min-SSD, exact-match break) is faithful.
RVA(0x00140420, 0x34f)
i32 CDDSurface::Blit816(void* srcv, void* palv, i32 mode) {
    u8* pal = (u8*)palv;
    if (pal == 0) {
        return 0;
    }
    u8* locked = (u8*)Lock(0);
    if (locked == 0) {
        return 0;
    }
    u16* src = (u16*)srcv;
    if (mode == 2) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                i32 red = static_cast<u8>((static_cast<u8>(static_cast<u16>((px >> g_rUp))) << g_rDown));
                i32 green = static_cast<u8>((static_cast<u8>(static_cast<u16>((px >> g_gUp))) << g_gDown));
                i32 blue = static_cast<u8>((static_cast<u8>(px) << g_bDown));
                i32 best = 0;
                i32 d1 = green - pal[1];
                i32 d2 = blue - pal[2];
                i32 d0 = red - pal[0];
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < 256; k++) {
                    i32 e0 = red - pal[k * 4];
                    i32 e1 = green - pal[k * 4 + 1];
                    i32 e2 = blue - pal[k * 4 + 2];
                    i32 d = e0 * e0 + e1 * e1 + e2 * e2;
                    if (d < bestd) {
                        best = k;
                        bestd = d;
                        if (d == 0) {
                            break;
                        }
                    }
                }
                *dst = static_cast<u8>(best);
                dst++;
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                i32 red = static_cast<u8>((static_cast<u8>(static_cast<u16>((px >> g_rUp))) << g_rDown));
                i32 green = static_cast<u8>((static_cast<u8>(static_cast<u16>((px >> g_gUp))) << g_gDown));
                i32 blue = static_cast<u8>((static_cast<u8>(px) << g_bDown));
                i32 best = 0;
                i32 d1 = green - pal[1];
                i32 d2 = blue - pal[2];
                i32 d0 = red - pal[0];
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < 256; k++) {
                    i32 e0 = red - pal[k * 4];
                    i32 e1 = green - pal[k * 4 + 1];
                    i32 e2 = blue - pal[k * 4 + 2];
                    i32 d = e0 * e0 + e1 * e1 + e2 * e2;
                    if (d < bestd) {
                        best = k;
                        bestd = d;
                        if (d == 0) {
                            break;
                        }
                    }
                }
                *dst = static_cast<u8>(best);
                dst++;
            }
        }
    }
    this->m_8->Unlock(0);
    return 1;
}

// CDDSurface::DumpSurfaceInfo (__thiscall, ret 4 => 1 arg). Re-fetch the surface
// desc into the scratch, then TRACE it: `detailed==0` prints one line; otherwise
// dump the geometry, 16-bit bitmasks, colour key, Z-buffer depth and every set
// DDSCAPS flag. The DDBD_* -> bit-count and DDBD_* -> name mappings are MSVC
// binary-search branch trees (sparse cases, no jump table).
// @early-stop
// codegen block-layout wall (96.68%): all logic is correct and ALL THREE DDBD
// branch-trees + every TRACE call match byte-for-byte; the sole residual is the
// Z-buffer-name switch tail - retail duplicates `lea edx,[esp+0x10]` (&buf) into each
// switch case, cl hoists the loop-invariant address once before the shared strcpy.
// Not source-steerable (buf scope / decl order don't move it); the rest of the diff
// is disasm-spelling only (rep movsd/rep movs, test bl,0x80). Entropy tail.
RVA(0x00140770, 0x326)
void CDDSurface::DumpSurfaceInfo(i32 detailed) {
    i32 i;
    i32* p = (i32*)m_desc;
    for (i = 0x1b; i != 0; i--) {
        *p++ = 0;
    }
    m_descSize = 0x6c;
    LPDDSURFACEDESC desc = (LPDDSURFACEDESC)m_desc;
    m_8->GetSurfaceDesc(desc);
    if (desc == 0) {
        return;
    }

    if (detailed == 0) {
        i32 depth = 0;
        switch (desc->ddpfPixelFormat.dwRGBBitCount) {
            case DDBD_32:
                depth = 32;
                break;
            case DDBD_16:
                depth = 16;
                break;
            case DDBD_8:
                depth = 8;
                break;
            case DDBD_4:
                depth = 4;
                break;
            case DDBD_2:
                depth = 2;
                break;
            case DDBD_1:
                depth = 1;
                break;
        }
        DDrawLogLine(
            "Surface: width = %i, height = %i, depth = %i, pitch = %i\n",
            m_width,
            m_height,
            depth,
            m_pitch
        );
        return;
    }

    u32 caps = desc->ddsCaps.dwCaps;
    i32 colorKey = GetColorKey();
    i32 depth = 0;
    switch (desc->ddpfPixelFormat.dwRGBBitCount) {
        case DDBD_32:
            depth = 32;
            break;
        case DDBD_16:
            depth = 16;
            break;
        case DDBD_8:
            depth = 8;
            break;
        case DDBD_4:
            depth = 4;
            break;
        case DDBD_2:
            depth = 2;
            break;
        case DDBD_1:
            depth = 1;
            break;
    }
    DDrawLogLine("Surface Information for surface pointer %p:\n", this);
    DDrawLogLine(
        "width = %i, height = %i, depth = %i, pitch = %i\n",
        m_width,
        m_height,
        depth,
        m_pitch
    );
    if (depth == 16) {
        DDrawLogLine(
            "16-bit color bitmasks are: R = %04X, G = %04X, B = %04X\n",
            desc->ddpfPixelFormat.dwRBitMask,
            desc->ddpfPixelFormat.dwGBitMask,
            desc->ddpfPixelFormat.dwBBitMask
        );
    }
    if (colorKey != -1) {
        DDrawLogLine("Source color key = %lu", colorKey);
    }
    u32 zbuf = caps & DDSCAPS_ZBUFFER;
    if (zbuf != 0) {
        char* name;
        switch (desc->dwZBufferBitDepth) {
            case DDBD_32:
                name = "DDBD_32";
                break;
            case DDBD_16:
                name = "DDBD_16";
                break;
            case DDBD_8:
                name = "DDBD_8";
                break;
            case DDBD_4:
                name = "DDBD_4";
                break;
            case DDBD_2:
                name = "DDBD_2";
                break;
            case DDBD_1:
                name = "DDBD_1";
                break;
            default:
                name = "Unknown";
                break;
        }
        char buf[32];
        strcpy(buf, name);
        DDrawLogLine("Z Buffer bit depth = %s\n", buf);
    }
    if (caps & DDSCAPS_ALPHA) {
        DDrawLogLine("DDSCAPS_ALPHA is set\n");
    }
    if (caps & DDSCAPS_BACKBUFFER) {
        DDrawLogLine("DDSCAPS_BACKBUFFER is set\n");
    }
    if (caps & DDSCAPS_COMPLEX) {
        DDrawLogLine("DDSCAPS_COMPLEX is set\n");
    }
    if (caps & DDSCAPS_FLIP) {
        DDrawLogLine("DDSCAPS_FLIP is set\n");
    }
    if (caps & DDSCAPS_FRONTBUFFER) {
        DDrawLogLine("DDSCAPS_FRONTBUFFER is set\n");
    }
    if (caps & DDSCAPS_OFFSCREENPLAIN) {
        DDrawLogLine("DDSCAPS_OFFSCREENPLAIN\tis set\n");
    }
    if (caps & DDSCAPS_OVERLAY) {
        DDrawLogLine("DDSCAPS_OVERLAY is set\n");
    }
    if (caps & DDSCAPS_PALETTE) {
        DDrawLogLine("DDSCAPS_PALETTE is set\n");
    }
    if (caps & DDSCAPS_PRIMARYSURFACE) {
        DDrawLogLine("DDSCAPS_PRIMARYSURFACE is set\n");
    }
    if (caps & DDSCAPS_SYSTEMMEMORY) {
        DDrawLogLine("DDSCAPS_SYSTEMMEMORY is set\n");
    }
    if (caps & DDSCAPS_VIDEOMEMORY) {
        DDrawLogLine("DDSCAPS_VIDEOMEMORY is set\n");
    }
    if (zbuf != 0) {
        DDrawLogLine("DDSCAPS_ZBUFFER is set\n");
    }
}

// ---------------------------------------------------------------------------
// The two in-surface RLE row-decoders retail compiled UNOPTIMIZED: a full ebp
// frame with every local spilled to the stack, no register allocation or strength
// reduction - inside this otherwise-/O2 obj. One obj = one flag set, so the old
// per-unit /Od profile (fileimagerundecode) was structurally impossible; the
// only period mechanism is a `#pragma optimize("", off)` island in the source
// (VC5 supports it), modeled exactly so here. Locals are declared in retail's
// /Od stack-slot order so the [ebp-N] displacements match.
#pragma optimize("", off)

// ---------------------------------------------------------------------------
// CDDSurface::DecodeRun8 (ret 4) - RLE-decode an 8bpp run-stream (arg0)
// into the locked surface, row by row. Each token: the high two bits set (& 0xc0
// == 0xc0) means a run of (token & 0x3f) copies of the following byte; otherwise
// the token itself is one literal pixel. A run that overflows the current scanline
// carries the remainder to the next row.
// @early-stop
// /Od local-slot-ordering wall (docs/patterns/od-local-slot-ordering.md): the
// instruction stream is byte-identical to retail; only the [ebp-N] local-slot
// displacements differ (retail lays locals out sequentially in declaration order,
// our same-order recompile permutes them) - ~99.5% fuzzy / ~85% byte.
RVA(0x00140aa0, 0x1a3)
i32 CDDSurface::DecodeRun8(void* src) {
    u8* sp;
    i32 carry;
    u8 pixel;
    i32 width;
    i32 locked;
    i32 row;
    i32 run;
    u8* dst;
    i32 k;
    i32 height;
    i32 cols;
    if (src == 0) {
        return 0;
    }
    width = this->GetWidth();
    height = this->GetHeight();
    carry = 0;
    sp = (u8*)src;
    locked = this->Lock(0);
    if (locked == 0) {
        return 0;
    }
    for (row = 0; row < height; row++) {
        dst = (u8*)(locked + this->Scale(row));
        cols = width;
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst++;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst++;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst++;
                cols--;
            }
        }
    }
    this->UnlockThunk();
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::DecodeRun24 (ret 4) - the 24bpp surface RLE decoder. Like
// DecodeRun8 but planar: each row is decoded as three independent stride-3 channel
// scanlines (R at the +2 byte, G at +1, B at +0 of each BGR triple), with the run
// carry and source cursor continuous across channel and row boundaries. The row
// base is the pitch-scale helper (Scale(row)); width/height come from the geometry
// getters (re-read per use, not cached - retail's /Od shape).
// @early-stop
// /Od local-slot-ordering wall (docs/patterns/od-local-slot-ordering.md): byte-
// identical instruction stream, only the [ebp-N] local displacements differ.
RVA(0x00140c50, 0x3e2)
i32 CDDSurface::DecodeRun24(void* src) {
    u8* sp;
    i32 carry;
    u8 pixel;
    i32 locked;
    i32 row;
    i32 run;
    u8* dst;
    i32 k;
    i32 cols;
    if (src == 0) {
        return 0;
    }
    locked = this->Lock(0);
    if (locked == 0) {
        return 0;
    }
    carry = 0;
    sp = (u8*)src;
    dst = 0;
    for (row = 0; row < this->GetHeight(); row++) {
        dst = (u8*)(locked + this->Scale(row) + 2);
        cols = this->GetWidth();
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
        dst = (u8*)(locked + this->Scale(row) + 1);
        cols = this->GetWidth();
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
        dst = (u8*)(locked + this->Scale(row));
        cols = this->GetWidth();
        if (carry > 0) {
            for (k = 0; k < carry; k++) {
                *dst = pixel;
                dst += 3;
            }
            cols -= carry;
            carry = 0;
        }
        while (cols > 0) {
            pixel = *sp;
            sp++;
            if ((pixel & 0xc0) == 0xc0) {
                run = pixel & 0x3f;
                pixel = *sp;
                sp++;
                if (run > cols) {
                    carry = run - cols;
                    run = cols;
                }
                for (k = 0; k < run; k++) {
                    *dst = pixel;
                    dst += 3;
                }
                cols -= run;
            } else {
                *dst = pixel;
                dst += 3;
                cols--;
            }
        }
    }
    this->UnlockThunk();
    return 1;
}

#pragma optimize("", on)

// ---------------------------------------------------------------------------
// The rotated-blit transform-setup worker (ImageRotate.cpp, 0x145f60), __cdecl,
// 9 args. TRUE signature recovered from the retail def body (fld arg6 -> deg->rad
// -> fsin/fcos = rotation, fmul arg7 = scale, arg8/arg9 read as ints) AND the three
// thunks' push sequences: (int,int,int*,void*,void*, float rot, float scale, int
// mode, int colorkey) => ?ImageRotateBlit@@YAXHHPAHPAX1MMHH@Z. The three thunks feed
// these slots with per-thunk-typed forwarded params (below) so the call binds and
// the pushes stay byte-identical.
extern void ImageRotateBlit(
    i32 a1,
    i32 a2,
    i32* pivot,
    void* dst,
    void* in,
    float rot,   // arg6 (deg->rad rotation)
    float scale, // arg7
    i32 mode,    // arg8
    i32 colorkey // arg9
);

// RotateBlit / ScaleBlit / RotateScaleBlit (0x141040 / 0x141200 / 0x141240) - thin
// arg-reorder thunks forwarding to ImageRotateBlit with `this` as the destination.
// Each returns 1. Orphan copies (no caller); __thiscall. Each param is typed by the
// ImageRotateBlit slot it flows into (float rot/scale, int mode/colorkey) so no
// int<->float conversion is inserted and the dword pushes match retail exactly.
RVA(0x00141040, 0x36)
i32 CDDSurface::RotateBlit(
    i32 rect,
    i32 pivot,
    i32 a1,
    i32 a2,
    float scale,
    i32 mode,
    i32 colorkey
) {
    // Rotation fixed at 0.0f (no rotate); the 5th param carries the scale.
    ImageRotateBlit(a1, a2, (i32*)pivot, (void*)this, (void*)rect, 0.0f, scale, mode, colorkey);
    return 1;
}

// @early-stop
// 0x141080 (372 B) = a CDDSurface rotated-blit transform builder: fild/fxch-heavy x87
// assembly of a corner-transform record on a 0x8c-byte local, then call 0x146550
// (RotateRasterize). Homed from GapFunctions.cpp (matcher-5) by RVA neighbourhood.
// Homed pending reconstruction (x87 fld/fxch scheduling wall; sibling of ImageRotateBlit).
RVA(0x00141080, 0x174)
i32 Gap_141080(void) {
    return 0;
}

RVA(0x00141200, 0x39)
i32 CDDSurface::ScaleBlit(
    i32 rect,
    i32 pivot,
    i32 a1,
    i32 a2,
    float angle,
    i32 mode,
    i32 colorkey
) {
    // Scale fixed at 1.0f (no scale); the 5th param carries the rotation.
    ImageRotateBlit(a1, a2, (i32*)pivot, (void*)this, (void*)rect, angle, 1.0f, mode, colorkey);
    return 1;
}

RVA(0x00141240, 0x39)
i32 CDDSurface::RotateScaleBlit(
    i32 rect,
    i32 pivot,
    i32 a1,
    i32 a2,
    float angle,
    float scale,
    i32 mode,
    i32 colorkey
) {
    ImageRotateBlit(a1, a2, (i32*)pivot, (void*)this, (void*)rect, angle, scale, mode, colorkey);
    return 1;
}

// ---------------------------------------------------------------------------
// DecodeThunk - a glue forwarder that rebuilds a 16-byte rect/clip record
// from its trailing args on the stack and tail-calls the image worker (0x1471d0) with
// the six leading scalar args + that record passed by value, then cleans 0x2c of stack
// (ret 0x28). The worker `this` arrives in ecx (re-pushed, not reloaded).
//
// @early-stop
// stack-forward wall (~48%): the 16-byte record build on the stack, the scalar
// re-pushes and the `ret 0x28` are faithful; residue is the exact scratch-register
// choice for the record copy + that retail re-pushes `this` (ecx) as the trailing arg
// (the worker gets `this` both in ecx and pushed) which has no clean /O2 source
// spelling. Deferred to the final sweep.
RVA(0x00141280, 0x4a)
void CDDSurface::
    DecodeThunk(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 r0, i32 r1, i32 r2, i32 r3) {
    ClipRect16 clip;
    clip.a = r0;
    clip.b = r1;
    clip.c = r2;
    clip.d = r3;
    this->Run(a1, a2, a3, a4, a5, a6, clip);
}

// ---------------------------------------------------------------------------
// 0x1412d0 (slot 5): IsValid - a held DirectDraw surface present and a positive
// cached width/height. __thiscall, no args.
RVA(0x001412d0, 0x24)
i32 CDDSurface::IsValid() {
    if (m_8 != 0 && m_88 > 0 && m_8c > 0) {
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 0x141310 / 0x141320: the geometry accessors the run-decoders call out-of-line
// (DecodeRun8/DecodeRun24 in this unit; also LightFxRender/WarpTextureBlit). Four
// bytes each - `mov eax,[ecx+0x1c] / ret` and `mov eax,[ecx+0x18] / ret` - i.e. the
// embedded DDSURFACEDESC's dwWidth (+0x1c) and dwHeight (+0x18). They were DECLARED
// in DDSurface.h and never defined: every caller emitted a reloc to a symbol nothing
// in the tree defines (a guaranteed unresolved external; objdiff masks it).
// Out-of-line here on purpose: retail calls them (cl /O2 = /Ob1, so a non-inline
// definition in the same TU is NOT inlined into DecodeRun8/24 - matching retail).
RVA(0x00141310, 4)
i32 CDDSurface::GetWidth() {
    return m_width;
}

RVA(0x00141320, 4)
i32 CDDSurface::GetHeight() {
    return m_height;
}

// ---------------------------------------------------------------------------
// CDDSurface::~CDDSurface
// The virtual destructor: MSVC stamps the vptr (compiler-implicit, stamp-first),
// runs the shared surface teardown (FreeSurfaces: release the held DirectDraw
// surfaces + walk the +0x98 object array), then destroys the owned CPtrArray at
// +0x94. The CPtrArray member-dtor is guarded -> the /GX EH frame. The implicit
// stamp reloc-masks against the shared 0x5ef7f0 surface vtable.
RVA(0x00141350, 0x53)
CDDSurface::~CDDSurface() {
    FreeSurfaces();
}

// CDDSurface::UnlockThunk (0x1413b0): unlock the held DirectDraw surface
// (IDirectDrawSurface::Unlock(0), vtable slot 0x80). Folded from Stub/BoundaryUpper.cpp
// (Owner1413::Thunk - the m_8 sub-object IS the real m_8 IDirectDrawSurface); decl in
// DDSurface.h. The DecodeRun8 (FileImageRunDecode.cpp) run-decoder calls it on `this`.
RVA(0x001413b0, 0xf)
void CDDSurface::UnlockThunk() {
    m_8->Unlock(0);
}

// CDDSurface::Scale (0x1413c0): the pitch-scaled row offset (m_pitch * n). Folded
// from Stub/BoundaryUpper.cpp (B_1413c0::Scale - this IS CDDSurface, from DecodeRun8);
// decl in DDSurface.h.
RVA(0x001413c0, 0xb)
i32 CDDSurface::Scale(i32 n) {
    return m_pitch * n;
}

// ===========================================================================
// Class-metadata annotations (EOF-hosted).
// ===========================================================================
SIZE_UNKNOWN(CFileImageElement);
SIZE_UNKNOWN(CFileImageSrc);
SIZE(ClipRect16, 0x10); // 16-byte by-value rect/clip record

// --- vtable catalog ---
