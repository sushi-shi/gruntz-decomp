#include <rva.h>

#include <DDrawMgr/DDrawDeviceManager.h>

#include <Mfc.h>

#include <DDrawMgr/PixelShift.h>
#include <Image/Image.h>
#include <Ints.h>

#include <ddraw.h>
#include <string.h>

RVA(0x00148840, 0x47)
i32 CFileImageSurface::LoadKeyed(
    CDDrawDeviceManager* manager,
    i32 width,
    i32 height,
    ColorDepth bitDepth,
    i32 caps,
    i32 colorKey
) {

    if (CDDSurface::BlitSurf(manager, width, height, bitDepth, caps | 0x40) == BPP_UNSET) {
        return 0;
    }
    if (colorKey != -1) {
        FillPalette(colorKey);
    }
    return 1;
}

RVA(0x00148890, 0xad)
i32 CFileImageSurface::ResolveEx(
    CDDrawDeviceManager* manager,
    void* data,
    FileImageFormat format,
    u32 dataSize,
    i32 surfaceCaps,
    i32 colorKey
) {
    if (dataSize == 0) {
        return 0;
    }
    i32 adjustedCaps = surfaceCaps | 0x40;
    switch (format) {
        case FMT_PID:
            if (!DecodePcxData(
                    manager,
                    static_cast<PidHeader*>(data),
                    dataSize,
                    adjustedCaps,
                    colorKey
                )) {
                return 0;
            }
            break;
        case FMT_PCX:
            if (!Decode(
                    manager,
                    static_cast<PcxHeader*>(data),
                    static_cast<i32>(dataSize),
                    adjustedCaps
                )) {
                return 0;
            }
            break;
        case FMT_BMP:
            if (!DecodeRun(
                    manager,
                    static_cast<BmpFileImage*>(data),
                    static_cast<i32>(dataSize),
                    adjustedCaps
                )) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    if (colorKey != -1 && format != FMT_PID) {
        FillPalette(colorKey);
    }
    return 1;
}

RVA(0x00148940, 0x102)
i32 CFileImageSurface::LoadByExt(
    CDDrawDeviceManager* manager,
    char* path,
    i32 surfaceCaps,
    i32 colorKey
) {
    surfaceCaps |= 0x40;
    i32 applyColorKey = 1;
    char* ext = strrchr(path, '.');
    if (ext != NULL && _strcmpi(ext, ".BMP") == 0) {
        if (LoadFile2(manager, path, surfaceCaps) == 0) {
            return 0;
        }
    } else if (ext != NULL && _strcmpi(ext, ".PCX") == 0) {
        if (LoadFile(manager, path, surfaceCaps) == 0) {
            return 0;
        }
    } else if (ext != NULL && _strcmpi(ext, ".PID") == 0) {
        if (DecodePcxEx(manager, path, surfaceCaps, colorKey) == 0) {
            return 0;
        }
        applyColorKey = 0;
    } else if (this->Load(manager, path, surfaceCaps) == 0) {
        return 0;
    }
    if (colorKey != -1 && applyColorKey != 0) {
        FillPalette(colorKey);
    }
    return 1;
}

RVA(0x00148a50, 0x6b)
i32 CDDrawOverlaySurface::CreateOverlay(
    CDDrawDeviceManager* manager,
    i32 width,
    i32 height,
    i32 caps
) {
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.ddsCaps.dwCaps = caps | DDSCAPS_OVERLAY;
    desc.dwWidth = width;
    desc.dwHeight = height;
    desc.ddckCKSrcBlt.dwColorSpaceLowValue = 1;
    desc.ddckCKSrcBlt.dwColorSpaceHighValue = 1;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    return CDDSurface::CreateFromDesc(manager, &desc) != 0;
}

RVA(0x00148ac0, 0x2b)
i32 CDDrawOverlaySurface::UpdateOverlay(
    RECT* srcRect,
    CDDSurface* dest,
    RECT* destRect,
    u32 flags,
    DDOVERLAYFX* fx
) {
    return m_ddSurface->UpdateOverlay(srcRect, dest->m_ddSurface, destRect, flags, fx);
}

RVA(0x00148af0, 0x58)
i32 CDDrawPrimarySurface::CreatePrimary(
    CDDrawDeviceManager* manager,
    i32 caps,
    i32 descFlags,
    i32 backBufferCount
) {
    memset(m_descWords, 0, sizeof(DDSURFACEDESC));
    m_descSize = sizeof(DDSURFACEDESC);
    m_surfaceCaps = caps | 0x200;
    m_descFlags = descFlags;
    m_backBufferCount = backBufferCount;
    if (!CDDSurface::CreateFromDesc(manager, NULL)) {
        return 0;
    }
    InstallColorFormat();
    return 1;
}

RVA(0x00148b50, 0x2c)
i32 CDDrawPrimarySurface::CreateFromDesc(CDDrawDeviceManager* manager, const DDSURFACEDESC* desc) {
    if (CDDSurface::CreateFromDesc(manager, desc) == 0) {
        return 0;
    }
    InstallColorFormat();
    return 1;
}

RVA(0x00148b80, 0xb5)
i32 CDDrawPrimarySurface::InstallColorFormat() {
    u32 mask = m_rMask;
    i32 bitCount = 0;
    i32 firstBit;
    firstBit = -1;
    for (i32 redBit = 0; redBit < 0x20; redBit++) {
        if ((mask & 1) == 1) {
            if (firstBit == -1) {
                firstBit = redBit;
            }
            bitCount++;
        }
        mask >>= 1;
    }
    g_rDown = 8 - bitCount;
    g_rUp = firstBit;

    bitCount = 0;
    mask = m_gMask;
    firstBit = -1;
    for (i32 greenBit = 0; greenBit < 0x20; greenBit++) {
        if ((1 & mask) == 1) {
            if (firstBit == -1) {
                firstBit = greenBit;
            }
            bitCount++;
        }
        mask >>= 1;
    }
    g_gDown = 8 - bitCount;
    g_gUp = firstBit;

    bitCount = 0;
    mask = m_bMask;
    firstBit = -1;
    for (i32 blueBit = 0; blueBit < 0x20; blueBit++) {
        if ((mask & 1) == 1) {
            if (firstBit == -1) {
                firstBit = blueBit;
            }
            bitCount++;
        }
        mask >>= 1;
    }
    g_bDown = 8 - bitCount;
    g_bUp = firstBit;

    BuildColorChannelTables();
    return 1;
}

RVA(0x00148c40, 0x75)
i32 CDDrawZBufferSurface::CreateZBuffer(
    CDDrawDeviceManager* manager,
    i32 width,
    i32 height,
    i32 caps,
    i32 extraCaps,
    i32 unused,
    i32 zBufferBitDepth
) {
    static_cast<void>(unused);
    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    desc.ddsCaps.dwCaps = extraCaps | caps | DDSCAPS_ZBUFFER;
    desc.dwWidth = width;
    desc.dwHeight = height;
    desc.dwZBufferBitDepth = zBufferBitDepth;
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_ZBUFFERBITDEPTH;
    return CDDSurface::CreateFromDesc(manager, &desc) != 0;
}

RVA(0x00148cc0, 0x18)
i32 CDDrawZBufferSurface::CreateFromDesc(CDDrawDeviceManager* manager, const DDSURFACEDESC* desc) {
    return CDDSurface::CreateFromDesc(manager, desc) != 0;
}
