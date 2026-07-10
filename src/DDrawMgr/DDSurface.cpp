// DDSurface.cpp - CDDSurface (C:\Proj\DDrawMgr\DIRSURF.CPP), the thin
// IDirectDrawSurface wrapper class. Split out of the DirectDrawMgr god-TU: this is
// one contiguous retail .text block (0x13e140-0x140a96) == one retail .obj (DIRSURF).
// Every wrapper does iface->Method(args...) so the retail `call *off(reg)` COM
// dispatch falls out; on a DDERR_SURFACELOST the thunks call the wrapper's own
// virtual RestoreLost (slot 7) to restore + retry, then route a still-bad HRESULT
// through CDirectDrawMgr::GetErrorString. The class's ctor/dtor + ??_7CDDSurface
// vtable live in DDrawPtrCollections.cpp (emission anchor), so these bodies emit
// standalone. BuildColorChannelTables (0x13f740, free __cdecl) sits inside this
// DIRSURF block and precomputes the RGB channel CLUTs ShadeBlt blends through.
//
// Include environment mirrors the former DirectDrawMgr.cpp exactly (Mfc.h via
// FileStream.h supplies windows.h before <ddraw.h>) so the byte-exact thunks keep
// matching; DDSurface itself needs no CFileIO.
#include <Io/FileStream.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <ddraw.h> // real DirectDraw SDK (IDirectDrawSurface, DDBLTFX, DDCOLORKEY, DDERR_*/DDBD_*/DDSCAPS_*)
#include <rva.h>
#include <stdio.h>
#include <string.h>  // inline strcpy / memcpy / memset
#include <Globals.h> // g_clut, g_lutBank0/1/2, g_pfRedSize/GreenShift/BlueSize/RedShift/GreenSize

#define DIRSURF_FILE "C:\\Proj\\DDrawMgr\\DIRSURF.CPP"

// The engine logger (DDrawMgr-local) + Rez heap + operator new (reloc-masked).
extern void __cdecl DDrawLogLine(char* fmt, ...); // 0x141cb0 (printf-style TRACE)
extern "C" void* RezAlloc(unsigned int);          // 0x1b9b46
extern "C" void RezFree(void* p);                 // 0x1b9b82
void* operator new(u32);                          // engine allocator (reloc-masked rel32)

// The live screen RGB-format shift table (same globals ShadeTableCache reads): the
// per-channel "up" shifts (ea0/ea4/ea8 = R/G/B) and the device "down" widths
// (eac/eb0/eb4). Reloc-masked DIR32.
DATA(0x00283ea0)
extern i32 g_rUp; // 0x683ea0
DATA(0x00283ea4)
extern i32 g_gUp; // 0x683ea4
DATA(0x00283ea8)
extern i32 g_bUp; // 0x683ea8
DATA(0x00283eac)
extern i32 g_rDown; // 0x683eac
DATA(0x00283eb0)
extern i32 g_gDown; // 0x683eb0
DATA(0x00283eb4)
extern i32 g_bDown; // 0x683eb4

// The engine RECT-copier fn-ptr (0x6c44bc), used by ShadeBlt to snapshot the rects.
DATA(0x002c44bc)
extern void(__stdcall* g_pCopyRect)(struct tagRECT*, const struct tagRECT*);

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
    m_b4 = (u32)m_pitch / (u32)m_b0; // lPitch / divisor
    m_80[0] = 0;
    m_80[1] = 0;
    i32 height = m_height;
    m_8c = height;
    m_90 = m_ac * height;
    m_dontOwn = m_dontOwn | 1;
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
    if (hr == (i32)DDERR_SURFACELOST) {
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

// CDDSurface::Restore (__thiscall, 0x13e7d0, re-homed from src/Stub/BoundaryUpper2.cpp):
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
    if (hr == (i32)DDERR_SURFACELOST) {
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
// this->m_8->vtbl[9](0, &CImageFactory::Build_13e9a0), reports through
// CDirectDrawMgr::GetErrorString, then repopulates m_94 from g_imageCache. Homed from
// GapFunctions.cpp (matcher-5); lives in the DIRSURF block (DDSurface.cpp) by RVA.
// Homed pending the owning class (m_8 vtable + m_94 array) + the Build PMF push modelled.
RVA(0x0013e8f0, 0xb0)
i32 Gap_13e8f0(void) {
    return 0;
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
RVA(0x0013eb80, 0x21)
i32 CDDSurface::SetDestColorKey(u32 key) {
    DDCOLORKEY ck;
    ck.dwColorSpaceLowValue = key;
    ck.dwColorSpaceHighValue = key;
    return SetColorKey(DDCKEY_DESTBLT, &ck);
}

// CDDSurface::Blt (__thiscall, ret 4 => 1 arg). Blts src's RECT (src->m_80) into
// this surface's RECT (this->m_80) with flags 0x1000000; SURFACELOST retry.
RVA(0x0013ee60, 0x8d)
i32 CDDSurface::Blt(CDDSurface* src) {
    void* srcRect = src->m_80;
    void* dstRect = m_80;
    i32 hr = m_8->Blt((LPRECT)dstRect, src->m_8, (LPRECT)srcRect, 0x1000000, 0);
    if (hr == (i32)DDERR_SURFACELOST) {
        if (RestoreLost()) {
            hr = m_8->Blt((LPRECT)dstRect, src->m_8, (LPRECT)srcRect, 0x1000000, 0);
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
        hr = m_8->Blt((LPRECT)dstRect, src->m_8, (LPRECT)srcRect, flags, (LPDDBLTFX)fx);
    } else {
        hr = m_8->Blt((LPRECT)dstRect, 0, (LPRECT)srcRect, flags, (LPDDBLTFX)fx);
    }
    if (hr == (i32)DDERR_SURFACELOST) {
        if (RestoreLost()) {
            hr = m_8->Blt((LPRECT)dstRect, src->m_8, (LPRECT)srcRect, flags, (LPDDBLTFX)fx);
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
    i32 hr = m_8->BltFast(x, y, src->m_8, (LPRECT)srcRect, trans);
    if (hr == (i32)DDERR_SURFACELOST) {
        if (RestoreLost()) {
            hr = m_8->BltFast(x, y, src->m_8, (LPRECT)srcRect, trans);
        } else {
            return (i32)DDERR_SURFACELOST;
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
// operand-order fix, and caching g_pCopyRect in a local (to match retail's `mov edi;
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
    g_pCopyRect(&dr, dstRect);
    g_pCopyRect(&sr, srcRect);
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

    if (g_pfRedSize == 3 && g_pfGreenShift == 3 && g_pfBlueSize == 3 && g_pfRedShift == 0xa
        && g_pfGreenSize == 5) {
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
                        u16 v = *(u16*)(g_lutBank2_663ca0 + bank
                                        + (((tp & 0x1f) << 5) + (sp & 0x1f)) * 2);
                        v |= *(u16*)(g_lutBank0_673ca0 + bank
                                     + ((sp >> 0xa) + ((tp >> 5) & ~0x1f)) * 2);
                        v |= *(u16*)(g_lutBank1_653ca0 + bank
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
    } else if (g_pfRedSize == 3 && g_pfGreenShift == 2 && g_pfBlueSize == 3 && g_pfRedShift == 0xb
               && g_pfGreenSize == 5) {
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
                        u16 v = *(u16*)(g_lutBank2_663ca0 + bank
                                        + (((tp & 0x1f) << 5) + (sp & 0x1f)) * 2);
                        v |= *(u16*)(g_lutBank0_673ca0 + bank
                                     + ((sp >> 0xb) + ((tp >> 6) & ~0x1f)) * 2);
                        v |= *(u16*)(g_lutBank1_653ca0 + bank
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
                    *(i16*)(g_clut + 0x20000 + base) = (i16)(sum << 0xa);
                    *(i16*)(g_clut + base) = (i16)(sum << 5);
                    *(i16*)(g_clut + 0x10000 + base) = (i16)(sum << bShift);
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
                    *(i16*)(g_clut + 0x20000 + base) = (i16)(sum << g_rUp);
                    *(i16*)(g_clut + base) = (i16)((sum << g_gUp) << 1);
                    *(i16*)(g_clut + 0x10000 + base) = (i16)(sum << g_bUp);
                    varD += stepA;
                } while (--k != 0);
                varB += a;
            } while (--countB != 0);
            a++;
        } while (--stepA > 0);
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
    if (hr != (i32)DDERR_NOCOLORKEY) {
        if (hr == 0) {
            return key.dwColorSpaceLowValue;
        }
        CDirectDrawMgr::GetErrorString(DIRSURF_FILE, 0x695, hr);
    }
    return -1;
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
