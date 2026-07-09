// DirectDrawMgr.cpp - the DDrawMgr module's DirectDraw manager group
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
// Every wrapper does iface->Method(args...) so the retail
// `call *off(reg)` COM dispatch falls out; only the called slots are pinned.
// Locals are placeholders, the switch case VALUES, GetErrorString (file, line,
// hr) tuples and string contents are load-bearing.
// The DIRPAL.CPP palette loaders open files through the engine's MFC-derived
// CFileIO, so this TU is an MFC TU: <Io/FileStream.h> pulls <Mfc.h> FIRST (afx
// brings <windows.h> the controlled way), which also supplies MessageBeep /
// MessageBoxA + BOOL/HWND/LPCSTR/UINT for GetErrorString (uType =
// MB_ICONEXCLAMATION, 0x30). Mfc.h precedes any DirectX header.
#include <Io/FileStream.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <ddraw.h> // real DirectDraw SDK (IDirectDraw/2, IDirectDrawSurface/Palette, DirectDrawCreate, IIDs); Mfc.h (via FileStream.h) supplies windows.h first
#include <rva.h>
#include <stdio.h>  // engine sprintf (reloc-masked)
#include <string.h> // inline strcpy / memcpy (rep movs / repne scasb), strrchr / _stricmp
#include <Globals.h>

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
extern void __cdecl DDrawLogLine(char* fmt, ...); // 0x141cb0 (printf-style TRACE)

// The Rez heap allocator/free (also re-declared near CreatePoolItem below).
extern "C" void* RezAlloc(unsigned int); // 0x1b9b46
extern "C" void RezFree(void* p);        // 0x1b9b82

// The cached frame-clock fn-ptr (retail _g_pTimeGetTime @ 0x6c4650); the palette
// fade times through `call ds:[0x6c4650]`, NOT the WINMM import.
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650

// The DDERR_NAME line numbers all reference these source-path $SG constants.
#define DIRSURF_FILE "C:\\Proj\\DDrawMgr\\DIRSURF.CPP"
#define DIRPAL_FILE "C:\\Proj\\DDrawMgr\\DIRPAL.CPP"
#define DDRAWMGR_FILE "C:\\Proj\\DDrawMgr\\DDRAWMGR.CPP"
#define DDRAWMGR_H_FILE "C:\\Proj\\DDrawMgr\\ddrawmgr.h"

// DDERR_SURFACELOST (the lost-surface retry HRESULT, MAKE_DDHRESULT(450) =
// 0x887601c2) and DDERR_NOCOLORKEY (GetColorKey's "no key set" non-error,
// MAKE_DDHRESULT(215) = 0x887600d7) come from <ddraw.h>.

// DDRAW.dll DirectDrawCreate is declared by <ddraw.h> (HRESULT WINAPI, no
// __declspec(dllimport)) -> a direct `e8 rel32` to the incremental-link thunk
// (reloc-masked), like DirectSoundCreate, not an `ff 15 [IAT]` indirect.

// IID_IDirectDraw2 / IID_IDirectDrawSurface3 - the real dxguid GUID constants in
// .rdata, passed to QueryInterface by REFIID. <ddraw.h> declares them (EXTERN_C
// const GUID); redeclared here with DATA() to bind their retail addresses so the
// `push OFFSET` reloc-masks (same idiom as DirectInputMgr2.cpp's IID redeclare).
DATA(0x001ef848)
extern "C" const GUID IID_IDirectDraw2; // 0x5ef848
DATA(0x001ef888)
extern "C" const GUID IID_IDirectDrawSurface3; // 0x5ef888

// operator new(size_t) - the engine allocator (reloc-masked rel32).
void* operator new(u32);

// The three file-extension literals the LoadFromFile dispatcher stricmp-ladders
// over (reloc-masked .rdata globals; .BMP/.PCX share the Image.cpp addresses).
// File-scope so each `push OFFSET` matches the binary's direct-address push.
static const char s_extBmp[] = ".BMP";
static const char s_extPcx[] = ".PCX";
static const char s_extPal[] = ".PAL";

// The process-wide DirectDraw object + the CDirectDrawMgr singleton (.data).
DATA(0x00283ee8)
extern "C" IDirectDraw2* g_DirectDraw; // 0x683ee8

// The global enumerated-display-mode array (CObArray of 0x6c-byte mode records).
DATA(0x00283ec8)
extern CDdObArray g_modeArray; // 0x683ec8

// The context (owner window) that produced the current g_DirectDraw object.
DATA(0x00283ee4)
extern void* g_ddCreateCtx; // 0x683ee4

// ===========================================================================
// CDDSurface (DIRSURF.CPP) - the IDirectDrawSurface wrapper thunks.
// ===========================================================================

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

// The engine RECT-copier fn-ptr (0x6c44bc) + the Rez heap (0x1b9b46 / 0x1b9b82).
DATA(0x002c44bc)
extern void(__stdcall* g_pCopyRect)(struct tagRECT*, const struct tagRECT*);
extern "C" void* RezAlloc(unsigned int size); // 0x1b9b46
extern "C" void RezFree(void* p);             // 0x1b9b82

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

// ===========================================================================
// CDirectDrawMgr (DDRAWMGR.CPP) - the device manager.
// ===========================================================================

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

// ===========================================================================
// CDDPalette (DIRPAL.CPP) - the palette wrapper thunks.
// ===========================================================================

// CDDPalette::Create (__thiscall, ret 0xc => 3 args). Caches a copy of the
// PALETTEENTRY array (m_cacheA), allocates a second cache (m_cacheB), then creates the
// DirectDraw palette via IDirectDraw::CreatePalette into m_palette.
RVA(0x00147390, 0x78)
i32 CDDPalette::Create(IDirectDraw2* dd, void* entries, u32 flags) {
    m_cacheA = (u8*)operator new(0x400);
    // Plateau note: byte-for-byte except the copy loop's SIB base/index roles
    // (retail encodes [entries+i]/[m_cacheA+i] with i as the index; MSVC here makes i
    // the base) - a 1-byte-per-insn encoding choice, semantically identical.
    for (i32 i = 0; i < 0x400; i += 4) {
        *(i32*)(m_cacheA + i) = *(i32*)((char*)entries + i);
    }
    m_cacheB = (u8*)operator new(0x400);
    i32 hr = dd->CreatePalette(flags, (LPPALETTEENTRY)entries, &m_palette, 0);
    if (hr == 0) {
        return 1;
    }
    CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x4b, hr);
    return 0;
}

// CDDPalette::LoadDefault (0x1479e0) is the no-extension / unresolved-extension
// fallback loader - a separate DIRPAL.CPP method defined in another base obj. It
// is declared on the class (header) but NOT defined here, so the dispatcher's
// tail call to it reloc-masks (resolved by the engine_label_stubs unit).

// CDDPalette::LoadFromFile (__thiscall, ret 0xc => 3 args). Pick the palette
// loader by file extension: take ext = strrchr(filename,'.') then a stricmp
// ladder on .BMP/.PCX/.PAL, forwarding (dd, filename, flags) verbatim to the
// matching loader; no/unresolved extension -> LoadDefault. Same idiom as
// CImage::LoadFromRez. Each branch re-tests `ext != 0` (the target's per-case
// `test esi; je default`).
RVA(0x00147410, 0xbc)
i32 CDDPalette::LoadFromFile(IDirectDraw2* dd, char* filename, u32 flags) {
    char* ext = strrchr(filename, '.');
    if (ext && _stricmp(ext, s_extBmp) == 0) {
        return LoadBmp(dd, filename, flags);
    } else if (ext && _stricmp(ext, s_extPcx) == 0) {
        return LoadPcx(dd, filename, flags);
    } else if (ext && _stricmp(ext, s_extPal) == 0) {
        return LoadPal(dd, filename, flags);
    }
    return LoadDefault(dd, filename, flags);
}

// CDDPalette::CreateRGB (__thiscall, ret 0xc => 3 args). Takes a packed 256x3
// RGB-triplet array, expands it on the stack into PALETTEENTRY[256] (peFlags=0),
// then forwards to Create. The 0x400-byte stack buffer drives `sub esp,0x400`.
RVA(0x001474d0, 0x60)
i32 CDDPalette::CreateRGB(IDirectDraw2* dd, void* rgb, u32 flags) {
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

// CDDPalette::Destroy (__thiscall, no args). Nulls m_0/m_palette/m_8, frees the three
// owned buffers (m_cacheA/m_cacheB/m_18) via operator delete, clears m_34. m_palette is
// only nulled (not Released) here.
RVA(0x00147530, 0x54)
void CDDPalette::Destroy() {
    m_0 = 0;
    m_8 = 0;
    if (m_palette != 0) {
        m_palette = 0;
    }
    if (m_cacheA != 0) {
        operator delete(m_cacheA);
        m_cacheA = 0;
    }
    if (m_cacheB != 0) {
        operator delete(m_cacheB);
        m_cacheB = 0;
    }
    if (m_18 != 0) {
        operator delete(m_18);
        m_18 = 0;
    }
    m_34 = 0;
}

// CDDPalette::LoadBmp (__thiscall, ret 0xc => 3 args). Open the .BMP file, read
// the 14-byte BITMAPFILEHEADER then the 0x428-byte info region (BITMAPINFOHEADER
// + the 256-entry RGBQUAD table) then the 0x400-byte RGBQUAD palette, expand each
// RGBQUAD (B,G,R) to a PALETTEENTRY (R,G,B,0), and Create. Any short read fails
// (returns 0). The stack CFileIO forces a /GX EH frame. The CFileIO ctor/Open/
// Read/dtor are reloc-masked engine calls.
// @early-stop
// big EH frame (0x17e B): logic/CFG/all four CFileIO calls/the RGBQUAD->PALETTEENTRY
// expand loop/the Create call all reproduced. Residual is the EH stack-slot layout
// of the 0x848-byte scratch frame (retail's exact [esp+N] slot choices for the read
// buffers + the EH-state stores) + the reloc-masked CFileIO/EH symbol names.
// Deferred to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x00147590, 0x17e)
i32 CDDPalette::LoadBmp(IDirectDraw2* dd, char* filename, u32 flags) {
    u8 hdr14[0xe];   // BITMAPFILEHEADER
    u8 pe[0x400];    // expanded PALETTEENTRY[256]
    u8 info[0x428];  // BITMAPINFOHEADER + the in-file palette region
    u8 quads[0x400]; // 256-entry RGBQUAD palette
    CFileIO file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    if (file.Read(hdr14, 0xe) != 0xe) {
        return 0;
    }
    if (file.Read(info, 0x428) != 0x428) {
        return 0;
    }
    if (file.Read(quads, 0x400) != 0x400) {
        return 0;
    }
    for (i32 i = 0; i < 0x400; i += 4) {
        pe[i + 0] = quads[i + 2]; // R <- RGBQUAD.rgbRed
        pe[i + 1] = quads[i + 1]; // G <- rgbGreen
        pe[i + 2] = quads[i + 0]; // B <- rgbBlue
        pe[i + 3] = 0;            // peFlags
    }
    return Create(dd, pe, flags);
}

// CDDPalette::LoadPcx (__thiscall, ret 0xc => 3 args). Open the .PCX file, Seek
// to -0x300 from EOF (the trailing 768-byte VGA palette), read the 0x300 RGB
// triplets, expand each to a PALETTEENTRY (R,G,B,0), and Create. /GX EH frame for
// the stack CFileIO.
// @early-stop
// big EH frame (0x122 B): the Open/Seek/Read/Create call chain, the 0x300-triplet
// expand loop and the Create forward all reproduced. Residual is the EH stack-slot
// layout of the 0x710-byte scratch frame + the reloc-masked CFileIO/EH symbol
// names. Deferred to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x00147710, 0x122)
i32 CDDPalette::LoadPcx(IDirectDraw2* dd, char* filename, u32 flags) {
    u8 pe[0x400];  // expanded PALETTEENTRY[256]
    u8 rgb[0x300]; // 256 packed RGB triplets (trailing VGA palette)
    CFileIO file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    file.Seek(-0x300, 2); // SEEK_END
    if (file.Read(rgb, 0x300) != 0x300) {
        return 0;
    }
    u8* src = rgb;
    for (i32 i = 0; i < 0x400; i += 4) {
        pe[i + 0] = src[0];
        pe[i + 1] = src[1];
        pe[i + 2] = src[2];
        pe[i + 3] = 0;
        src += 3;
    }
    return Create(dd, pe, flags);
}

// CDDPalette::CreateFromTrailing (__thiscall, ret 0x10 => 4 args). When `size` is
// at least 0x300 the palette is the trailing 768-byte VGA block (data+size-0x300):
// expand its 256 RGB triplets to a stack PALETTEENTRY[256] (peFlags=0) and Create;
// short data returns 0. No EH frame (no destructible local). The 0x400-byte stack
// buffer drives `sub esp,0x400`.
// @early-stop
// loop-scheduling wall (89.09%): byte-identical to retail EXCEPT the inner expand
// loop's `add edx,4` placement (retail bumps the dst pointer after the first store,
// our cl bumps it at the loop tail) - a code-scheduling choice, not source-steerable.
// Same family as Create's SIB-encoding plateau. docs/patterns/zero-register-pinning.md.
RVA(0x00147840, 0x7e)
i32 CDDPalette::CreateFromTrailing(IDirectDraw2* dd, void* data, u32 size, u32 flags) {
    if (size < 0x300) {
        return 0;
    }
    u8 entries[0x400];
    u8* src = (u8*)data + size - 0x300;
    // Per-byte src increment (`*src++`) + a running dst pointer reproduce retail's
    // `inc eax`x3 / `add edx,4` loop shape (vs the `src+=3` bulk-add form).
    u8* dst = entries;
    for (i32 i = 0; i < 0x100; i++) {
        dst[0] = *src++;
        dst[1] = *src++;
        dst[2] = *src++;
        dst[3] = 0;
        dst += 4;
    }
    return Create(dd, entries, flags);
}

// CDDPalette::LoadPal (__thiscall, ret 0xc => 3 args). Open the .PAL file, read
// the 0x300-byte RGB-triplet block, expand each to a PALETTEENTRY (R,G,B,0), and
// Create. /GX EH frame for the stack CFileIO.
// @early-stop
// big EH frame (0x112 B): the Open/Read/Create chain, the 0x300-triplet expand
// loop and the Create forward all reproduced. Residual is the EH stack-slot layout
// of the 0x710-byte scratch frame + the reloc-masked CFileIO/EH symbol names.
// Deferred to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x001478c0, 0x112)
i32 CDDPalette::LoadPal(IDirectDraw2* dd, char* filename, u32 flags) {
    u8 pe[0x400];  // expanded PALETTEENTRY[256]
    u8 rgb[0x300]; // 256 packed RGB triplets
    CFileIO file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    if (file.Read(rgb, 0x300) != 0x300) {
        return 0;
    }
    u8* src = rgb;
    for (i32 i = 0; i < 0x400; i += 4) {
        pe[i + 0] = src[0];
        pe[i + 1] = src[1];
        pe[i + 2] = src[2];
        pe[i + 3] = 0;
        src += 3;
    }
    return Create(dd, pe, flags);
}

// CDDPalette::SetAndNotify (__thiscall, ret 0x10 => 4 args; a4 unused). Cache the
// `count` supplied PALETTEENTRYs into m_cacheA starting at `start`, wait for the next
// vertical blank through the global DirectDrawMgr's device (slot 22, @+0x58), then
// push the range straight into the DirectDraw palette via SetEntries(0, start,
// count, data). The notify only fires when the singleton is up.
// @early-stop
// copy-loop SIB / arg-regalloc wall (61.87%): the VBlank-notify + SetEntries call
// chain match, but MSVC5's strength-reduced cache copy loop (`m_cacheA[start+i]=data[i]`)
// picks a different induction-var/SIB form + arg-register assignment than retail.
// Same family as Create's copy-loop plateau. docs/patterns/zero-register-pinning.md.
RVA(0x00147aa0, 0x6a)
i32 CDDPalette::SetAndNotify(i32 start, i32 count, i32* data, i32 a4) {
    i32* cache = (i32*)m_cacheA;
    for (i32 i = 0; i < count; i++) {
        cache[start + i] = data[i];
    }
    if (g_DirectDrawMgr != 0) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    return m_palette->SetEntries(0, start, count, (LPPALETTEENTRY)data);
}

// CDDPalette::SetEntriesQuad (0x147b10, __thiscall, ret 0x10 => 4 args). Allocate
// a count*4 working buffer, expand `count` RGBQUAD entries (R/B swapped, as in
// LoadBmp) into PALETTEENTRY, SetAndNotify the range, free, return the HRESULT.
RVA(0x00147b10, 0x8b)
i32 CDDPalette::SetEntriesQuad(i32 start, i32 count, u8* quads, i32 a4) {
    u8* buf = (u8*)RezAlloc(count * 4);
    if (buf == 0) {
        return 0x80070057;
    }
    for (i32 i = 0; i < count; i++) {
        buf[i * 4 + 0] = quads[i * 4 + 2];
        buf[i * 4 + 1] = quads[i * 4 + 1];
        buf[i * 4 + 2] = quads[i * 4 + 0];
        buf[i * 4 + 3] = 0;
    }
    i32 hr = SetAndNotify(start, count, (i32*)buf, a4);
    RezFree(buf);
    return hr;
}

// CDDPalette::SetEntriesRGB (0x147ba0, __thiscall, ret 0x10 => 4 args). As above
// but from a packed 3-byte RGB source (straight copy, as in CreateRGB).
// @early-stop
// store-scheduling tail (~93%; permuter confirms no source-steerable gain): body/
// alloc/SetAndNotify/free are byte-exact. Retail schedules the last entry's peBlue
// store before the peFlags=0 store (my cl emits them reversed) + the trailing src++.
// Twin SetEntriesQuad (stride-4 read) IS exact; the stride-3 read reshuffles the tail.
RVA(0x00147ba0, 0x82)
i32 CDDPalette::SetEntriesRGB(i32 start, i32 count, u8* rgb, i32 a4) {
    u8* buf = (u8*)RezAlloc(count * 4);
    if (buf == 0) {
        return 0x80070057;
    }
    u8* src = rgb;
    u8* d = buf;
    for (i32 i = 0; i < count; i++) {
        d[0] = *src++;
        d[1] = *src++;
        d[2] = *src++;
        d[3] = 0;
        d += 4;
    }
    i32 hr = SetAndNotify(start, count, (i32*)buf, a4);
    RezFree(buf);
    return hr;
}

// CDDPalette::FadeRange (0x147d50, __thiscall, ret 0x18 => 6 args). Snapshot the
// current palette (GetEntries into m_cacheA), then over durationMs interpolate
// each entry in [start,start+count) linearly from its snapshot value toward the
// solid color (r,g,b), pushing SetEntries once per changed millisecond. Finally
// SetRange to the exact target. RezAlloc snapshot copy freed at the end. The
// frame clock is the cached g_pTimeGetTime fn-ptr.
// @early-stop
// timing-loop scheduling tail (~97%; permuter no gain): the snapshot copy, the
// per-channel (target-cur)*t/duration lerp, the recompute-only-on-tick guard, the
// SetEntries/SetRange calls and the final RezFree are all byte-faithful. Residual is
// MSVC5's exact [esp+N] slot choices + register schedule across the rotated timing
// loop (the elapsed/prev/t0 live-range packing). Not source-steerable.
RVA(0x00147d50, 0x1d2)
void CDDPalette::FadeRange(i32 start, i32 count, i32 r, i32 g, i32 b, i32 durationMs) {
    i32 hr = m_palette->GetEntries(0, 0, 0x100, (LPPALETTEENTRY)m_cacheA);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x2c0, hr);
    }
    u8* snapshot = (u8*)RezAlloc(0x400);
    for (i32 i = 0; i < 0x400; i += 4) {
        *(i32*)(snapshot + i) = *(i32*)(m_cacheA + i);
    }
    i32 t0 = g_pTimeGetTime();
    i32 prev = 9;
    for (i32 t = 10; t < durationMs; t = g_pTimeGetTime() - t0) {
        if (t != prev) {
            for (i32 j = start; j < start + count; j++) {
                m_cacheA[j * 4 + 0] =
                    (u8)(((r & 0xff) - snapshot[j * 4 + 0]) * t / durationMs + snapshot[j * 4 + 0]);
                m_cacheA[j * 4 + 1] =
                    (u8)(((g & 0xff) - snapshot[j * 4 + 1]) * t / durationMs + snapshot[j * 4 + 1]);
                m_cacheA[j * 4 + 2] =
                    (u8)(((b & 0xff) - snapshot[j * 4 + 2]) * t / durationMs + snapshot[j * 4 + 2]);
            }
            m_palette->SetEntries(0, start, count, (LPPALETTEENTRY)(m_cacheA + start * 4));
        }
        prev = t;
    }
    SetRange(start, count, r, g, b, 0);
    RezFree(snapshot);
}

// CDDPalette::BlendRange (0x1482c0, __thiscall, ret 0x18 => 6 args). Blend each
// entry in [start,start+count) pct% (0..100) toward the solid color (r,g,b) in a
// single pass and push it straight to the DirectDraw palette via SetEntries.
// @early-stop
// regalloc / arg-slot wall (~94%): logic/CFG/the (target-cur)*pct/100 magic-divide
// blend/the SetEntries+assert tail are byte-faithful. Retail masks r,g,b in place into
// their arg stack slots (reused) after the loop guard + keeps the cache byte in bl;
// this C spelling hoists the masks into fresh temps (sub esp,0xc) + spills the byte.
// The in-place-mask / do-while restructurings scored strictly lower. Not steerable.
RVA(0x001482c0, 0x2e9)
void CDDPalette::BlendRange(i32 pct, i32 start, i32 count, i32 r, i32 g, i32 b) {
    for (i32 i = start; i < start + count; i++) {
        u8 cr = m_cacheA[i * 4 + 0];
        m_cacheA[i * 4 + 0] = (u8)(((r & 0xff) - cr) * pct / 100 + cr);
        u8 cg = m_cacheA[i * 4 + 1];
        m_cacheA[i * 4 + 1] = (u8)(((g & 0xff) - cg) * pct / 100 + cg);
        u8 cb = m_cacheA[i * 4 + 2];
        m_cacheA[i * 4 + 2] = (u8)(((b & 0xff) - cb) * pct / 100 + cb);
    }
    i32 hr = m_palette->SetEntries(0, start, count, (LPPALETTEENTRY)(m_cacheA + start * 4));
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x406, hr);
    }
}

// CDDPalette::GetEntries (__thiscall, ret 0 => no args). Lazily allocates the
// readback cache (m_cacheB), then reads all 256 entries; reports a bad HRESULT.
RVA(0x00147c30, 0x4d)
i32 CDDPalette::GetEntries() {
    // Lazily allocates the readback cache, then reads all 256 entries; reports a
    // bad HRESULT. Plateau note: retail's body falls off the end (no return -
    // the int symbol returns whatever sits in eax), so it omits the trailing
    // `xor eax,eax` this `return 0` emits. MSVC 5.0 C++ forbids fall-off
    // (C2561), so the one-instruction `xor` gap can't be reproduced from clean C.
    if (m_cacheB == 0) {
        m_cacheB = (u8*)operator new(0x400);
        if (m_cacheB == 0) {
            return 0;
        }
    }
    i32 hr = m_palette->GetEntries(0, 0, 0x100, (LPPALETTEENTRY)m_cacheB);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x265, hr);
    }
    return 0;
}

// CDDPalette::Apply (__thiscall, ret 4 but no real arg). When the readback cache
// (m_cacheB) is populated, copy it into the working cache (m_cacheA, 0x400 bytes), wait for
// the next vertical blank through the global DirectDrawMgr's device (slot 22,
// @+0x58), then push all 256 entries into the DirectDraw palette via SetEntries(0,
// 0, 0x100, m_cacheB).
// @early-stop
// regalloc coin-flip (97.58%): every code byte matches retail EXCEPT the register
// the m_palette load for SetEntries lands in (retail reuses esi, ours uses eax). Same
// values/stores/order; not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x00147c80, 0x4d)
void CDDPalette::Apply(i32 a1) {
    u8* readback = m_cacheB;
    if (readback == 0) {
        return;
    }
    // Byte-offset copy loop (i+=4, cmp 0x400) matches retail's index-walk form.
    for (i32 i = 0; i < 0x400; i += 4) {
        *(i32*)(m_cacheA + i) = *(i32*)(readback + i);
    }
    if (g_DirectDrawMgr != 0) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    m_palette->SetEntries(0, 0, 0x100, (LPPALETTEENTRY)readback);
}

// CDDPalette::SetRange (__thiscall, ret 0x18 => 6 args).
RVA(0x00147cd0, 0x78)
i32 CDDPalette::SetRange(i32 start, i32 count, u8 r, u8 g, u8 b, u32 flags) {
    for (i32 i = start; i < start + count; i++) {
        m_cacheA[i * 4 + 0] = r;
        m_cacheA[i * 4 + 1] = g;
        m_cacheA[i * 4 + 2] = b;
    }
    i32 hr = m_palette->SetEntries(flags, start, count, (LPPALETTEENTRY)(m_cacheA + start * 4));
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x2a3, hr);
    }
    return hr;
}

// ===========================================================================
// CDDPalette::Flush (0x148250, re-homed from BoundaryTail) - flush a pending
// blit: if nothing pending (m_34==0) return; clear the pending flag; when a fill
// color m_14 is set dispatch the solid blit SetAndNotify(m_2c,m_30,m_14,0) and
// clear m_14; otherwise dispatch the keyed blit SetRange passing m_1c plus its
// byte-shifted views (the engine re-reads the packed color at +1/+2 byte offsets
// through an 8-byte stack temp). __thiscall.
// ===========================================================================
RVA(0x00148250, 0x61)
void CDDPalette::Flush() {
    if (m_34 == 0) {
        return;
    }
    i32 v = m_14;
    m_34 = 0;
    if (v != 0) {
        SetAndNotify(m_2c, m_30, (i32*)v, 0);
        m_14 = 0;
    } else {
        char buf[8];
        *(i32*)buf = m_1c;
        SetRange(m_2c, m_30, *(i32*)buf, *(i32*)(buf + 1), *(i32*)(buf + 2), 0);
    }
}

// ===========================================================================
// CDDPageMgr (DDrawMgr) - the primary-surface bring-up (second DirectDrawCreate
// caller). On a failed COM call it routes through its own HandleError, not
// GetErrorString.
// ===========================================================================

// CDDPageMgr::Init (__thiscall, ret 0xc => 3 args). Creates its own DirectDraw,
// QueryInterfaces IDirectDraw2, sets the cooperative level + display mode,
// creates the primary surface (QI'd to IDirectDrawSurface3) and, for 8bpp, a
// palette; caches the geometry and shows the cursor. The desc scratch is filled
// through the named DDSURFACEDESC fields (m_descSize / ddsCaps.dwCaps m_descCaps).
RVA(0x0017c040, 0x25d)
i32 CDDPageMgr::Init(void* window, DDModeInfo* mode, u32 coopFlags) {
    if (m_initialized != 0) {
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
    if (DirectDrawCreate(0, &m_dd1, 0) != 0) {
        return 0;
    }
    if (m_dd1->QueryInterface(IID_IDirectDraw2, (void**)&m_dd2) != 0) {
        return 0;
    }
    if (m_dd2->SetCooperativeLevel((HWND)window, coopFlags) != 0) {
        HandleError();
        return 0;
    }
    if (m_dd2->SetDisplayMode(w, h, bpp, 0, 0) != 0) {
        HandleError();
        return 0;
    }

    i32 i;
    i32* d = (i32*)m_desc;
    for (i = 0x1b; i != 0; i--) {
        *d++ = 0;
    }
    m_descSize = 0x6c;
    m_24 = 1;
    m_descCaps = 0x200; // ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE
    if (m_dd2->CreateSurface((LPDDSURFACEDESC)m_desc, &m_primarySurfaceRaw, 0) != 0) {
        HandleError();
        return 0;
    }

    if (m_primarySurfaceRaw->QueryInterface(IID_IDirectDrawSurface3, (void**)&m_primarySurface)
        != 0) {
        return 0;
    }

    OnModeSet(w);

    if (mode->bpp == 8) {
        if (m_dd2->CreatePalette(4, (LPPALETTEENTRY)m_palEntries, &m_palette, 0) != 0) {
            HandleError();
            return 0;
        }
        m_primarySurface->SetPalette(m_palette);
        m_modeTag = 0;
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

    m_width = w;
    m_24 = 0;
    m_28 = 0;
    m_height = h;
    m_bpp = bpp;
    m_window = window;
    m_c = 0;
    ShowCursor(0);
    m_initialized = 1;
    FinishInit();
    return 1;
}

// ---------------------------------------------------------------------------
// CDDPageMgr::CheckMode16 (@0x17d2b0, __thiscall, no args)
// Read the current display mode's pixel format (IDirectDraw2::GetDisplayMode,
// vtbl slot 12), popcount its R/G/B channel bit-masks, and classify a 16-bit
// mode: 5/5/5 -> tag m_modeTag = 0x80000000, 5/6/5 -> 0xc0000000. Returns 1 on a
// recognised 16-bit mode, 0 otherwise (incl. a failed GetDisplayMode).
// @early-stop
// 83.9% - logic/CFG/the GetDisplayMode COM call/the three 32-iter popcount loops/
// the 5-5-5 vs 5-6-5 classification are all reproduced. The residual is a regalloc
// coin-flip: retail spills `this` to a stack slot (sub esp,0x70) and uses ebx as a
// bit counter, while we keep `this` in ebx (sub esp,0x6c) and use edi for the third
// counter; this cascades the loop register operands + the desc stack offset.
// Not source-steerable; deferred to the final sweep.
RVA(0x0017d2b0, 0xfa)
i32 CDDPageMgr::CheckMode16() {
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = 0x6c;
    if (m_dd2->GetDisplayMode(&desc) != 0) {
        return 0;
    }

    i32 r = 0;
    i32 g = 0;
    i32 b = 0;
    i32 i;
    u32 m;

    m = desc.ddpfPixelFormat.dwRBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            r++;
        }
        m >>= 1;
    }
    m = desc.ddpfPixelFormat.dwGBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            g++;
        }
        m >>= 1;
    }
    m = desc.ddpfPixelFormat.dwBBitMask;
    for (i = 0; i < 32; i++) {
        if ((m & 1) == 1) {
            b++;
        }
        m >>= 1;
    }

    if (r == 5 && g == 5 && b == 5) {
        m_modeTag = (i32)0x80000000;
        return 1;
    }
    if (r == 5 && g == 6 && b == 5) {
        m_modeTag = (i32)0xc0000000;
        return 1;
    }
    return 0;
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

// Global-mode-array teardown tail-forward (0x141c80): mov ecx,&g_modeArray; jmp
// RemoveAll. Folded from Stub/BoundaryUpper.cpp (ClearModeArray_141c80).
RVA(0x00141c80, 0xa)
void ClearModeArray_141c80() {
    g_modeArray.RemoveAll();
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

SIZE_UNKNOWN(CDdCreateArg);
SIZE_UNKNOWN(CDdDescSrc);
SIZE_UNKNOWN(CDdEnumVtbl);
SIZE_UNKNOWN(CDdPoolItem);
SIZE_UNKNOWN(CDdPoolSub);

// --- vtable catalog (reduced-view classes share their base vtable rva) ---
