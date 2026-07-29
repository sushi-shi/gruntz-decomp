#include <Mfc.h>                 // real MFC types (NAFXCW, reloc-masked) - afx-first
#include <DDrawMgr/PixelShift.h> // g_rUp/g_gUp/g_bUp/g_rDown/g_gDown/g_bDown
#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections + the CPoolItem* family
#include <Image/Image.h>                  // the single-source CDDSurface extras' records
#include <ddraw.h>                        // IDirectDrawSurface (UpdateOverlay passthrough)
#include <string.h>                       // memset (inlined to rep stos at /O2 /Oi)

RVA(0x00148840, 0x47)
i32 CFileImageSurface::LoadKeyed(
    void* surf,
    i32 width,
    i32 height,
    i32 bitDepth,
    i32 caps,
    i32 key
) {
    // Direct (non-virtual) dispatch to the slot-3 body: qualified call suppresses the
    // vtable indirection (retail direct-calls 0x13e0d0 here).
    if (CDDSurface::BlitSurf(surf, width, height, bitDepth, caps | 0x40) == 0) {
        return 0;
    }
    if (key != -1) {
        FillPalette(key);
    }
    return 1;
}

RVA(0x00148890, 0xad)
i32 CFileImageSurface::ResolveEx(void* surf, void* buf, i32 type, u32 size, i32 ctrl, i32 trans) {
    if (size == 0) {
        return 0;
    }
    i32 c = ctrl | 0x40;
    switch (type) {
        case FMT_PID:
            if (!DecodePcxData(
                    static_cast<CDDrawPtrCollections*>(surf),
                    static_cast<PidHeader*>(buf),
                    size,
                    c,
                    trans
                )) {
                return 0;
            }
            break;
        case FMT_PCX:
            if (!Decode(
                    static_cast<CDDrawPtrCollections*>(surf),
                    static_cast<PcxHeader*>(buf),
                    static_cast<i32>(size),
                    c
                )) {
                return 0;
            }
            break;
        case FMT_BMP:
            if (!DecodeRun(
                    static_cast<CDDrawPtrCollections*>(surf),
                    buf,
                    static_cast<i32>(size),
                    c
                )) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    if (trans != -1 && type != FMT_PID) {
        FillPalette(trans);
    }
    return 1;
}

// CDDSurface::LoadByExt - load an image by inspecting its file
// extension. Forces the IMAGEZ flag (|0x40), finds the extension, and dispatches
// to LoadFile2 (.BMP) / LoadFile (.PCX) / DecodePcxEx (.PID) or the default loader
// (Load @0x144270, the same __thiscall `this`). On a successful load (except the
// .PID path) it fills the palette from a4 when a4 != -1.
// @source: string-xref (.BMP/.PCX/.PID extension table)
//
RVA(0x00148940, 0x102)
i32 CFileImageSurface::LoadByExt(CDDrawPtrCollections* info, char* path, i32 flags, i32 key) {
    flags |= 0x40;
    i32 doFill = 1;
    char* ext = strrchr(path, '.');
    if (ext != 0 && _strcmpi(ext, ".BMP") == 0) {
        if (LoadFile2(info, path, flags) == 0) {
            return 0;
        }
    } else if (ext != 0 && _strcmpi(ext, ".PCX") == 0) {
        if (LoadFile(info, path, flags) == 0) {
            return 0;
        }
    } else if (ext != 0 && _strcmpi(ext, ".PID") == 0) {
        if (DecodePcxEx(info, path, flags, key) == 0) {
            return 0;
        }
        doFill = 0;
    } else if (this->Load(info, path, flags) == 0) {
        return 0;
    }
    if (key != -1 && doFill != 0) {
        FillPalette(key);
    }
    return 1;
}

// CPoolItemA88::Blit7 (0x148a50, slot 9): build a 0x6c-byte DDSURFACEDESC on the
// stack (mode 7, ddsCaps = a4|0x80, pitch fields), then run the base surface init
// (CDDSurface::Init1 @0x13e0a0, the descriptor-driven Apply) and return success.
// __thiscall, 4 args. (Re-homed from src/Stub/BoundaryUpper2.cpp; ImgOwnedY view
// dissolved onto the real CPoolItemA88.)
// @early-stop
// descriptor-fill scheduling wall (~85%): the Apply path is the 100% Setup path, but into
// a stack-local descriptor; retail hoists the a4 load (or al,0x80) ahead of a2 while MSVC
// loads a2 first, swapping the eax/ecx assignment + a couple store slots. Logic complete.
RVA(0x00148a50, 0x6b)
i32 CPoolItemA88::Blit7(CDDrawPtrCollections* info, i32 a2, i32 a3, i32 a4) {
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwWidth = a2;
    desc.ddsCaps.dwCaps = a4 | DDSCAPS_OVERLAY;
    desc.dwHeight = a3;
    desc.ddckCKSrcBlt.dwColorSpaceLowValue = 1;
    desc.ddckCKSrcBlt.dwColorSpaceHighValue = 1;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    return CDDSurface::Init1(info, &desc) != 0;
}

RVA(0x00148ac0, 0x2b)
i32 CPoolItemA88::UpdateOverlay(
    void* srcRect,
    CDDSurface* dest,
    void* destRect,
    u32 flags,
    void* fx
) {
    return m_ddSurface->UpdateOverlay(
        static_cast<LPRECT>(srcRect),
        dest->m_ddSurface,
        static_cast<LPRECT>(destRect),
        flags,
        static_cast<LPDDOVERLAYFX>(fx)
    );
}

RVA(0x00148af0, 0x58)
i32 CPoolItemAB8::Setup(CDDrawPtrCollections* info, i32 a2, i32 a3, i32 a4) {
    memset(m_descWords, 0, sizeof(DDSURFACEDESC));
    m_descSize = sizeof(DDSURFACEDESC);
    m_surfaceCaps = a2 | 0x200;
    m_descFlags = a3;
    m_backBufferCount = a4;
    if (!CDDSurface::Init1(info, 0)) {
        return 0;
    }
    InstallColorFormat();
    return 1;
}

RVA(0x00148b50, 0x2c)
i32 CPoolItemAB8::Init1(CDDrawPtrCollections* h, const DDSURFACEDESC* desc) {
    if (CDDSurface::Init1(h, desc) == 0) {
        return 0;
    }
    InstallColorFormat();
    return 1;
}

// ---------------------------------------------------------------------------
// CPoolItemAB8::InstallColorFormat (0x148b80, slot 10, __thiscall, no args).
// The sibling of ComputeColorMasks (0x143b20) below: derives the live screen
// RGB-format shift/loss globals, but reads the channel bitmasks straight from
// this surface's already-cached DDPIXELFORMAT fields (m_rMask/m_gMask/m_bMask)
// rather than a GetSurfaceDesc. The "up" shift is the channel's lowest set-bit
// index; the "down" loss is 8 - popcount. Then re-applies (Boundary_13f740).
// ---------------------------------------------------------------------------
// All three channel blocks use the SAME two spellings, and both are load-bearing
// (the old 99.70% "entropy tail" was the R block using the other one of each):
//   - the per-channel reseed is `count = 0; m = <mask>; shift = -1;` in THAT order,
//     which is what interleaves retail's `xor edi,edi` / mask load / `or edx,-1`
//     across the previous channel's Down store;
//   - the result pair is written DOWN-then-UP in the source, which is what makes cl
//     materialize `mov eax,8` ahead of the Up store and keep the 8-count in eax
//     (writing Up first recycles the shift register for it instead).
// Now EXACT.
RVA(0x00148b80, 0xb5)
i32 CPoolItemAB8::InstallColorFormat() {
    u32 m = m_rMask;
    i32 count = 0;
    i32 shift;
    shift = -1;
    for (i32 b = 0; b < 0x20; b++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b;
            }
            count++;
        }
        m >>= 1;
    }
    g_rDown = 8 - count;
    g_rUp = shift;

    count = 0;
    m = m_gMask;
    shift = -1;
    for (i32 b2 = 0; b2 < 0x20; b2++) {
        if ((1 & m) == 1) {
            if (shift == -1) {
                shift = b2;
            }
            count++;
        }
        m >>= 1;
    }
    g_gDown = 8 - count;
    g_gUp = shift;

    count = 0;
    m = m_bMask;
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
    g_bDown = 8 - count;
    g_bUp = shift;

    BuildColorChannelTables();
    return 1;
}

// ---------------------------------------------------------------------------
// CPoolItemAE8::Blit47 (0x148c40, slot 9): build a 0x6c-byte DDSURFACEDESC on the
// stack (mode 0x47, ddsCaps = a5|a4|0x20000, [+0x18]=a7), then run the base surface init
// (CDDSurface::Init1 @0x13e0a0) and return success. __thiscall, 7 args (a6 unused).
// (Re-homed from src/Stub/BoundaryUpper2.cpp; ImgOwnedY view dissolved onto the real
// CPoolItemAE8.)
// @early-stop
// descriptor-fill scheduling wall (~85%): mirror of CPoolItemA88::Blit7 (7-arg / mode 0x47).
// Same stack-local-descriptor load/store scheduling divergence. Logic complete.
RVA(0x00148c40, 0x75)
i32 CPoolItemAE8::Blit47(
    CDDrawPtrCollections* info,
    i32 a2,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7
) {
    static_cast<void>(a6);
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwWidth = a2;
    desc.ddsCaps.dwCaps = a5 | a4 | DDSCAPS_ZBUFFER;
    desc.dwZBufferBitDepth = a7;
    desc.dwHeight = a3;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
    return CDDSurface::Init1(info, &desc) != 0;
}

RVA(0x00148cc0, 0x18)
i32 CPoolItemAE8::Init1(CDDrawPtrCollections* h, const DDSURFACEDESC* desc) {
    return CDDSurface::Init1(h, desc) != 0;
}
