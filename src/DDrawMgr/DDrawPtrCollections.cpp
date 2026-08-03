#include <rva.h>

#include <DDrawMgr/DDrawPtrCollections.h>

#include <Mfc.h>

#include <DDrawMgr/PixelShift.h>
#include <Image/Image.h>
#include <Ints.h>

#include <ddraw.h>
#include <string.h>

RVA(0x00148840, 0x47)
i32 CFileImageSurface::LoadKeyed(
    void* surf,
    i32 width,
    i32 height,
    i32 bitDepth,
    i32 caps,
    i32 key
) {

    if (CDDSurface::BlitSurf(surf, width, height, bitDepth, caps | 0x40) == 0) {
        return 0;
    }
    if (key != -1) {
        FillPalette(key);
    }
    return 1;
}

RVA(0x00148890, 0xad)
i32 CFileImageSurface::ResolveEx(
    void* surf,
    void* buf,
    FileImageFormat type,
    u32 size,
    i32 ctrl,
    i32 trans
) {
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

// @early-stop
RVA(0x00148a50, 0x6b)
i32 CPoolItemA88::Blit7(CDDrawPtrCollections* info, i32 width, i32 height, i32 caps) {
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwWidth = width;
    desc.ddsCaps.dwCaps = caps | DDSCAPS_OVERLAY;
    desc.dwHeight = height;
    desc.ddckCKSrcBlt.dwColorSpaceLowValue = 1;
    desc.ddckCKSrcBlt.dwColorSpaceHighValue = 1;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    return CDDSurface::CreateFromDesc(info, &desc) != 0;
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
i32 CPoolItemAB8::Setup(CDDrawPtrCollections* info, i32 caps, i32 flags, i32 backBufferCount) {
    memset(m_descWords, 0, sizeof(DDSURFACEDESC));
    m_descSize = sizeof(DDSURFACEDESC);
    m_surfaceCaps = caps | 0x200;
    m_descFlags = flags;
    m_backBufferCount = backBufferCount;
    if (!CDDSurface::CreateFromDesc(info, 0)) {
        return 0;
    }
    InstallColorFormat();
    return 1;
}

RVA(0x00148b50, 0x2c)
i32 CPoolItemAB8::CreateFromDesc(CDDrawPtrCollections* h, const DDSURFACEDESC* desc) {
    if (CDDSurface::CreateFromDesc(h, desc) == 0) {
        return 0;
    }
    InstallColorFormat();
    return 1;
}

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

// @early-stop
RVA(0x00148c40, 0x75)
i32 CPoolItemAE8::Blit47(
    CDDrawPtrCollections* info,
    i32 width,
    i32 height,
    i32 caps,
    i32 capsExtra,
    i32 unused6,
    i32 zBufferBitDepth
) {
    static_cast<void>(unused6);
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.dwWidth = width;
    desc.ddsCaps.dwCaps = capsExtra | caps | DDSCAPS_ZBUFFER;
    desc.dwZBufferBitDepth = zBufferBitDepth;
    desc.dwHeight = height;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
    return CDDSurface::CreateFromDesc(info, &desc) != 0;
}

RVA(0x00148cc0, 0x18)
i32 CPoolItemAE8::CreateFromDesc(CDDrawPtrCollections* h, const DDSURFACEDESC* desc) {
    return CDDSurface::CreateFromDesc(h, desc) != 0;
}
