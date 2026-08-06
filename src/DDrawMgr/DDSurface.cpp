#include <rva.h>

#include <DDrawMgr/DDSurface.h>

#include <ComOutRef.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/PixelShift.h>
#include <DDrawMgr/WallProject.h>
#include <Enums.h>
#include <Image/ByteRunEncoding.h>
#include <Image/Image.h>
#include <Image/ImageRotate.h>
#include <Image/PcxFormat.h>
#include <Image/RasterVtx.h>
#include <Io/FileStream.h>
#include <Pix16.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

#define DIRSURF_FILE "C:\\Proj\\DDrawMgr\\DIRSURF.CPP"

DATA(0x00253c88)
CPtrArray g_imageCache;
DATA(0x00253c9e)
u8 g_clut[0x30000];

DATA(0x00283ca0)
u16 g_lut16[256] = {0};
DATA(0x00283ea0)
i32 g_rUp;
DATA(0x00283ea4)
i32 g_gUp;
DATA(0x00283ea8)
i32 g_bUp;

DATA(0x00283eac)
i32 g_rDown;
DATA(0x00283eb0)
i32 g_gDown;
DATA(0x00283eb4)
i32 g_bDown;

static inline u16* Row16(u8* locked, i32 row, i32 pitch) {
    Pix16Ptr p;
    p.m_bytes = locked + row * pitch;
    return p.m_words;
}

static inline u16 Clut16(u32 byteOff) {
    Pix16Ptr p;
    p.m_bytes = g_clut + byteOff;
    return *p.m_words;
}
static inline void ClutStore16(u32 byteOff, u16 v) {
    Pix16Ptr p;
    p.m_bytes = g_clut + byteOff;
    *p.m_words = v;
}

RVA(0x0013e0a0, 0x27)
i32 CDDSurface::CreateFromDesc(CDDrawPtrCollections* h, const DDSURFACEDESC* desc) {
    if (desc != NULL) {
        memcpy(m_descWords, desc, sizeof(DDSURFACEDESC));
    }
    return BlitIntoDesc(h);
}

RVA(0x0013e0d0, 0x66)
i32 CDDSurface::BlitSurf(void* surf, i32 width, i32 height, ColorDepth bitDepth, i32 caps) {
    i32* desc = this->m_descWords;
    for (i32 i = 0x1b; i != 0; i--) {
        *desc++ = 0;
    }
    this->m_surfaceCaps = caps;
    this->m_width = width;
    this->m_height = height;
    this->m_descSize = sizeof(DDSURFACEDESC);
    this->m_descFlags = 7;
    if (bitDepth != BPP_UNSET && bitDepth != (static_cast<CDDrawPtrCollections*>(surf))->m_palBpp) {
        this->m_descFlags = 0x1007;
        this->m_pixelFormatSize = sizeof(DDPIXELFORMAT);
        this->m_srcBitDepth = bitDepth;
    }
    return this->BlitIntoDesc(surf);
}

RVA(0x0013e140, 0x1a0)
i32 CDDSurface::Refresh(IDirectDrawSurface* surf) {
    m_ddSurface = surf;
    i32 i;
    i32* d = m_descWords;
    for (i = 0x1b; i != 0; i--) {
        *d++ = 0;
    }
    m_descSize = sizeof(DDSURFACEDESC);
    i32 hr = m_ddSurface->GetSurfaceDesc(&m_apiDesc);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x7e, hr);
    }

    ColorDepth bits = m_srcBitDepth;
    m_hasColorKey = 0;
    m_bitDepth = bits;

    switch (bits) {
        case BPP_PALETTED_8:
            m_bytesPerRow = m_width;
            break;
        case BPP_RGB_16:
            m_bytesPerRow = m_width * 2;
            break;
        case BPP_RGB_24:
            m_bytesPerRow = m_width * 3;
            break;
        case BPP_RGB_32:
            m_bytesPerRow = m_width * 4;
            break;
        default:
            m_bytesPerRow = m_width;
            break;
    }

    switch (bits) {
        case BPP_PALETTED_8:
            m_bytesPerPixel = 1;
            break;
        case BPP_RGB_16:
            m_bytesPerPixel = 2;
            break;
        case BPP_RGB_24:
            m_bytesPerPixel = 3;
            break;
        case BPP_RGB_32:
            m_bytesPerPixel = 4;
            break;
        default:
            m_bytesPerPixel = 1;
            break;
    }

    m_pixelsPerRow = static_cast<u32>(m_pitch) / static_cast<u32>(m_bytesPerPixel);
    m_fullRect.left = 0;
    m_fullRect.top = 0;
    m_fullRect.right = m_width;
    m_fullRect.bottom = m_height;
    m_imageBytes = m_height * m_bytesPerRow;
    m_dontOwn = m_dontOwn | 1;
    return 1;
}

RVA(0x0013e2e0, 0x1f0)
i32 CDDSurface::BlitIntoDesc(void* a) {
    CDDrawPtrCollections* mgr = static_cast<CDDrawPtrCollections*>(a);
    if (mgr->m_device == NULL) {
        return 0;
    }

    i32 hr = mgr->m_device->CreateSurface(&m_apiDesc, &m_ddSurfaceBack, 0);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0xd5, hr);
        return 0;
    }

    ComOutRef<IDirectDrawSurface> surfOut;
    surfOut.m_asTyped = &m_ddSurface;
    hr = m_ddSurfaceBack->QueryInterface(IID_IDirectDrawSurface3, surfOut.m_asVoid);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(0, 0, hr);
        return 0;
    }

    i32* d = m_descWords;
    for (i32 i = 0x1b; i != 0; i--) {
        *d++ = 0;
    }
    m_descSize = sizeof(DDSURFACEDESC);
    hr = m_ddSurface->GetSurfaceDesc(&m_apiDesc);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0xeb, hr);
    }

    ColorDepth bits = m_srcBitDepth;
    m_hasColorKey = 0;
    m_bitDepth = bits;
    switch (bits) {
        case BPP_PALETTED_8:
            m_bytesPerRow = m_width;
            break;
        case BPP_RGB_16:
            m_bytesPerRow = m_width * 2;
            break;
        case BPP_RGB_24:
            m_bytesPerRow = m_width * 3;
            break;
        case BPP_RGB_32:
            m_bytesPerRow = m_width * 4;
            break;
        default:
            m_bytesPerRow = m_width;
            break;
    }

    switch (bits) {
        case BPP_PALETTED_8:
            m_bytesPerPixel = 1;
            break;
        case BPP_RGB_16:
            m_bytesPerPixel = 2;
            break;
        case BPP_RGB_24:
            m_bytesPerPixel = 3;
            break;
        case BPP_RGB_32:
            m_bytesPerPixel = 4;
            break;
        default:
            m_bytesPerPixel = 1;
            break;
    }

    m_pixelsPerRow = static_cast<u32>(m_pitch) / static_cast<u32>(m_bytesPerPixel);
    m_fullRect.left = 0;
    m_fullRect.top = 0;
    m_fullRect.right = m_width;
    m_fullRect.bottom = m_height;
    m_imageBytes = m_height * m_bytesPerRow;
    return 1;
}

RVA(0x0013e4d0, 0x7e)
void CDDSurface::FreeSurfaces() {

    for (u32 i = 0; i < static_cast<u32>(m_elements.GetSize()); i++) {
        CDDSurface* e = static_cast<CDDSurface*>(m_elements[i]);
        delete e;
    }
    m_elements.SetSize(0, -1);
    if (this->m_ddSurface != NULL) {
        if ((this->m_dontOwn & 1) == 0) {
            this->m_ddSurface->Release();
        }
        this->m_ddSurface = NULL;
    }
    if (this->m_ddSurfaceBack != NULL) {
        if ((this->m_dontOwn & 1) == 0) {
            this->m_ddSurfaceBack->Release();
        }
        this->m_ddSurfaceBack = NULL;
    }
    this->m_restoreCallback = NULL;
}

RVA(0x0013e550, 0x71)
i32 CDDSurface::Resolve(
    CDDrawPtrCollections* pal,
    void* buf,
    FileImageFormat type,
    u32 size,
    u32 colorKey
) {
    if (size == 0) {
        return 0;
    }
    switch (type) {
        case FMT_PID:
            if (!DecodePid(pal, static_cast<PidHeader*>(buf), size, colorKey)) {
                return 0;
            }
            break;
        case FMT_PCX:
            if (!DecodePcx(pal, static_cast<PcxHeader*>(buf), size)) {
                return 0;
            }
            break;
        case FMT_BMP:
            if (!DecodeBmp(pal, buf, size)) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

RVA(0x0013e5d0, 0xb1)
i32 CDDSurface::MakeImageKey(CDDrawPtrCollections* pal, char* name, u32 colorKey) {
    char* ext = strrchr(name, '.');
    if (ext && _strcmpi(ext, ".BMP") == 0) {
        if (!LoadBmp(pal, name)) {
            return 0;
        }
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        if (!LoadPcx(pal, name)) {
            return 0;
        }
    } else if (ext && _strcmpi(ext, ".PID") == 0) {
        if (!LoadPid(pal, name, colorKey)) {
            return 0;
        }
    }
    return 1;
}

RVA(0x0013e690, 0x35)
i32 CDDSurface::SetPalette(CDDPalette* pal, i32 unused) {
    i32 hr = m_ddSurface->SetPalette(pal->m_palette);
    if (hr == 0) {
        return 1;
    }
    CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x1d2, hr);
    return 0;
}

RVA(0x0013e6d0, 0x88)
void* CDDSurface::Lock(void* rect) {
    i32 hr = m_ddSurface->Lock(static_cast<LPRECT>(rect), &m_apiDesc, 1, 0);
    if (hr == 0) {
        return m_lockBits;
    }
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost() == 0) {
            return 0;
        }
        hr = m_ddSurface->Lock(0, &m_apiDesc, 1, 0);
        if (hr == 0) {
            return m_lockBits;
        }
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x203, hr);
        return 0;
    }
    CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x209, hr);
    return 0;
}

RVA(0x0013e760, 0x63)
i32 CDDSurface::Fill(u32 color) {
    i32 fx[0x19];
    i32* p = fx;
    for (i32 i = 0x19; i != 0; i--) {
        *p++ = 0;
    }
    fx[0] = 0x64;
    fx[0x14] = static_cast<i32>(color);
    i32 hr = this->BltEx(0, 0, 0, 0x1000400, fx);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(
            const_cast<char*>("C:\\Proj\\DDrawMgr\\DIRSURF.CPP"),
            0x22c,
            hr
        );
    }
    return hr == 0;
}

RVA(0x0013e7d0, 0x73)
i32 CDDSurface::Restore(void* dstRect, i32 fillColor) {
    if (dstRect == NULL) {
        return 0;
    }
    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(DDBLTFX);
    fx.dwFillColor = fillColor;
    i32 hr = BltEx(dstRect, 0, 0, DDBLT_WAIT | DDBLT_COLORFILL, &fx);
    if (hr) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x26d, hr);
    }
    return hr == 0;
}

RVA(0x0013e850, 0x93)
i32 CDDSurface::Flip(CDDSurface* target) {
    IDirectDrawSurface* tsurf = 0;
    if (target != NULL) {
        tsurf = target->m_ddSurface;
    }
    i32 hr = m_ddSurface->Flip(tsurf, 1);
    if (hr == 0) {
        return 0;
    }
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost() == 0) {
            return hr;
        }
        hr = m_ddSurface->Flip(tsurf, 1);
        if (hr == 0) {
            return 0;
        }
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x2ae, hr);
        return hr;
    }
    CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x2b4, hr);
    return hr;
}

RVA(0x0013e8f0, 0xb0)
void CDDSurface::ReloadImageCache() {
    u32 i = 0;
    if (static_cast<u32>(m_elements.GetSize()) > 0) {
        do {
            CDDSurface* item = static_cast<CDDSurface*>(g_imageCache[i]);
            if (item != NULL) {
                delete item;
            }
            i++;
        } while (i < static_cast<u32>(m_elements.GetSize()));
    }
    m_elements.SetSize(0, -1);
    g_imageCache.SetSize(0, -1);
    i32 hr = m_ddSurface->EnumAttachedSurfaces(0, &EnumSurfacesCallback);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x2dd, hr);
    }
    u32 j = 0;
    if (static_cast<u32>(g_imageCache.GetSize()) > 0) {
        do {

            m_elements.Add(g_imageCache[j]);
            j++;
        } while (j < static_cast<u32>(g_imageCache.GetSize()));
    }
    g_imageCache.SetSize(0, -1);
}

RVA(0x0013e9a0, 0xcc)
HRESULT __stdcall EnumSurfacesCallback(IDirectDrawSurface* surf, DDSURFACEDESC* desc, void* ctx) {
    void* payload = 0;
    // The HRESULT must land in a named local: testing the call result inline gives
    // `test eax,eax` where retail compares against the live zero (`cmp eax,edi`).
    HRESULT hr = surf->QueryInterface(IID_IDirectDrawSurface3, &payload);
    if (hr == 0) {
        CDDSurface* item = new CDDSurface;

        if (item->Refresh(static_cast<IDirectDrawSurface*>(payload)) == 0) {
            delete item;
        } else {
            g_imageCache.SetAtGrow(g_imageCache.GetSize(), item);
        }
    }
    return 1;
}

RVA(0x0013ea70, 0x21)
void* CDDSurface::GetElementAt(i32 i) {
    if (i >= 0 && i < m_elements.GetSize()) {
        return m_elements.GetAt(i);
    }
    return 0;
}

RVA(0x0013eaa0, 0x39)
i32 CDDSurface::SetColorKey(u32 flags, void* key) {
    i32 hr = m_ddSurface->SetColorKey(flags, static_cast<LPDDCOLORKEY>(key));
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x353, hr);
        return hr;
    }
    return 0;
}

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

RVA(0x0013eb40, 0x3c)
void CDDSurface::FillPalette(u32 key) {
    u32 ck[2];
    ck[0] = key;
    ck[1] = key;
    if (static_cast<i32>(key) != -1) {
        this->m_hasColorKey = 1;
    } else {
        this->m_hasColorKey = 0;
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

// @early-stop
RVA(0x0013ebb0, 0x126)
void CDDSurface::FlipVertical() {
    if (m_height <= 1) {
        return;
    }
    u8* buf = static_cast<u8*>(Lock(0));
    if (buf == NULL) {
        return;
    }
    u8* tmp = new u8[m_width];
    if (tmp == NULL) {
        m_ddSurface->Unlock(0);
        return;
    }

    i32 width = m_width;
    i32 i = 0;
    i32 half = m_height / 2;
    if (half > 0) {
        do {

            u8* top = buf + i * m_pitch;
            i32 j = 0;
            if (width > 0) {
                do {
                    tmp[j] = *top;
                    ++top;
                    ++j;
                } while (j < width);
            }

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

    m_ddSurface->Unlock(0);
    delete[] tmp;
}

RVA(0x0013ece0, 0xc7)
i32 CDDSurface::BlitDirect(void* src, RasterRowOrder rowOrder) {
    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == NULL) {
        return 0;
    }
    u8* p = static_cast<u8*>(src);
    if (rowOrder == RASTER_ROWS_BOTTOM_UP) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            u8* sp = p;
            i32 n = this->m_bytesPerRow;
            for (i32 i = n; i > 0; i--) {
                *dst++ = *sp++;
            }
            p += n;
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u8* dst = locked + row * this->m_pitch;
            u8* sp = p;
            i32 n = this->m_bytesPerRow;
            for (i32 i = n; i > 0; i--) {
                *dst++ = *sp++;
            }
            p += n;
        }
    }
    this->m_ddSurface->Unlock(0);
    return 1;
}

RVA(0x0013edb0, 0x78)
void CDDSurface::Clear(i32 white) {

    BltFxWords fx;
    i32* p = fx.m_words;
    for (i32 i = 0x19; i != 0; i--) {
        *p++ = 0;
    }
    fx.m_fx.dwSize = sizeof(fx.m_fx);

    fx.m_fx.dwROP = white ? static_cast<i32>(0xff0062) : 0x42;
    i32 hr = this->m_ddSurface->Blt(0, 0, 0, 0x1020000, &fx.m_fx);
    if (hr != 0) {
        if (white != 0) {
            Fill(0xff);
        } else {
            Fill(0);
        }
    }
}

RVA(0x0013ee30, 0x29)
void CDDSurface::WaitFlip() {
    while (m_ddSurface->GetFlipStatus(DDGFS_ISFLIPDONE) == DDERR_WASSTILLDRAWING) {
    }
}

RVA(0x0013ee60, 0x8d)
i32 CDDSurface::Blt(CDDSurface* src) {
    LPRECT srcRect = &src->m_fullRect;
    LPRECT dstRect = &m_fullRect;
    i32 hr = m_ddSurface->Blt(dstRect, src->m_ddSurface, srcRect, 0x1000000, 0);
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost()) {
            hr = m_ddSurface->Blt(dstRect, src->m_ddSurface, srcRect, 0x1000000, 0);
        } else {
            return static_cast<i32>(DDERR_SURFACELOST);
        }
    }
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x48c, hr);
    }
    return hr;
}

RVA(0x0013eef0, 0x98)
i32 CDDSurface::BltEx(void* dstRect, CDDSurface* src, void* srcRect, u32 flags, void* fx) {
    i32 hr;
    if (src != NULL) {
        hr = m_ddSurface->Blt(
            static_cast<LPRECT>(dstRect),
            src->m_ddSurface,
            static_cast<LPRECT>(srcRect),
            flags,
            static_cast<LPDDBLTFX>(fx)
        );
    } else {
        hr = m_ddSurface->Blt(
            static_cast<LPRECT>(dstRect),
            0,
            static_cast<LPRECT>(srcRect),
            flags,
            static_cast<LPDDBLTFX>(fx)
        );
    }
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost()) {
            hr = m_ddSurface->Blt(
                static_cast<LPRECT>(dstRect),
                src->m_ddSurface,
                static_cast<LPRECT>(srcRect),
                flags,
                static_cast<LPDDBLTFX>(fx)
            );
        } else {
            return static_cast<i32>(DDERR_SURFACELOST);
        }
    }
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x4b0, hr);
    }
    return hr;
}

RVA(0x0013ef90, 0x8b)
i32 CDDSurface::BltFast(u32 x, u32 y, CDDSurface* src, void* srcRect, u32 trans) {
    i32 hr = m_ddSurface->BltFast(x, y, src->m_ddSurface, static_cast<LPRECT>(srcRect), trans);
    if (hr == static_cast<i32>(DDERR_SURFACELOST)) {
        if (RestoreLost()) {
            hr = m_ddSurface->BltFast(x, y, src->m_ddSurface, static_cast<LPRECT>(srcRect), trans);
        } else {
            return static_cast<i32>(DDERR_SURFACELOST);
        }
    }
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x4da, hr);
    }
    return hr;
}

// @early-stop
RVA(0x0013f020, 0x43f)
i32 CDDSurface::ShadeBlt(
    struct tagRECT* dstRect,
    CDDSurface* src,
    struct tagRECT* srcRect,
    i32 shade
) {
    RECT dr, sr;
    CopyRect(&dr, dstRect);
    CopyRect(&sr, srcRect);
    if (m_bytesPerPixel != PIXEL16_BYTES_PER_PIXEL) {
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

    u16* dstBits = static_cast<u16*>(Lock(0));
    u16* srcBits = static_cast<u16*>(src->Lock(0));
    i32 dstStride = m_pitch / 2;
    u16* dstPtr = dstBits + (dr.top * dstStride + dr.left);
    i32 srcStride = src->m_pitch / 2;
    u16* srcPtr = srcBits + (sr.top * srcStride + sr.left);
    i32 dstRowAdv = dstStride - dstW;
    i32 srcRowAdv = srcStride - srcW;
    u16* temp = new u16[dstW * 2];
    i32 bank = ((shade & 0xff) >> 3) << 0xb;

    if (g_rDown != PIXEL16_RED_DOWN) {
        goto reject;
    }
    if (g_gDown == RGB555_GREEN_DOWN && g_bDown == PIXEL16_BLUE_DOWN && g_rUp == RGB555_RED_UP
        && g_gUp == PIXEL16_GREEN_UP) {

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

                        Pix16Ptr c;
                        c.m_bytes =
                            ((g_clut + 0x10002) + bank + (((tp & 0x1f) << 5) + (sp & 0x1f)) * 2);
                        u16 v = *c.m_words;
                        c.m_bytes =
                            ((g_clut + 0x20002) + bank + ((sp >> 0xa) + ((tp >> 5) & ~0x1f)) * 2);
                        v |= *c.m_words;
                        c.m_bytes =
                            ((g_clut + 0x2) + bank
                             + ((((tp >> 5) & 0x1f) << 5) + (0x1f & (sp >> 5))) * 2);
                        v |= *c.m_words;
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
    } else if (g_rDown == PIXEL16_RED_DOWN && g_gDown == RGB565_GREEN_DOWN
               && g_bDown == PIXEL16_BLUE_DOWN && g_rUp == RGB565_RED_UP
               && g_gUp == PIXEL16_GREEN_UP) {

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

                        Pix16Ptr c;
                        c.m_bytes =
                            ((g_clut + 0x10002) + bank + (((tp & 0x1f) << 5) + (sp & 0x1f)) * 2);
                        u16 v = *c.m_words;
                        c.m_bytes =
                            ((g_clut + 0x20002) + bank + ((sp >> 0xb) + ((tp >> 6) & ~0x1f)) * 2);
                        v |= *c.m_words;
                        c.m_bytes =
                            ((g_clut + 0x2) + bank
                             + ((((tp >> 6) & 0x1f) << 5) + ((sp >> 6) & 0x1f)) * 2);
                        v |= *c.m_words;
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
        goto reject;
    }
    m_ddSurface->Unlock(0);
    src->m_ddSurface->Unlock(0);
    delete[] temp;
    return 1;
reject:
    delete[] temp;
    m_ddSurface->Unlock(0);
    src->m_ddSurface->Unlock(0);
    return 0;
}

// @early-stop
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
    pct = pct * 32 / 100;
    u16* src = static_cast<u16*>(Lock(0));
    i32 rowPix = m_pitch / 2;
    u16* srcPix = src + rc.top * rowPix + rc.left;
    i32 stride = rc.left - rc.right + rowPix;
    i32 width = rc.right - rc.left;
    i32 height = rc.bottom - rc.top;
    u16* scratch = new u16[width * 2];
    i32 off = pct << 11;

    if (g_rDown == PIXEL16_RED_DOWN) {
        if (g_gDown == RGB555_GREEN_DOWN && g_bDown == PIXEL16_BLUE_DOWN && g_rUp == RGB555_RED_UP
            && g_gUp == PIXEL16_GREEN_UP) {
            for (; height > 0; height--) {
                memcpy(scratch, srcPix, width * 2);
                u16* rd = scratch;
                for (i32 x = width; x > 0; x--) {
                    u32 p = *rd++;
                    u32 blue = p & 0x1f;
                    u32 hi = p >> 5;
                    u32 green = hi & 0x1f;
                    u32 red = hi & 0xffffffe0;
                    *srcPix++ = static_cast<u16>(
                        (Clut16(0x10002 + off + (blue << 6)) | Clut16(0x2 + off + (green << 6))
                         | Clut16(0x20002 + off + red * 2))
                    );
                }
                srcPix += stride;
            }
        } else if (g_gDown == RGB565_GREEN_DOWN && g_bDown == PIXEL16_BLUE_DOWN
                   && g_rUp == RGB565_RED_UP && g_gUp == PIXEL16_GREEN_UP) {
            for (; height > 0; height--) {
                memcpy(scratch, srcPix, width * 2);
                u16* rd = scratch;
                for (i32 x = width; x > 0; x--) {
                    u32 p = *rd++;
                    u32 blue = p & 0x1f;
                    u32 hi = p >> 6;
                    u32 green = hi & 0x1f;
                    u32 red = hi & 0xffffffe0;
                    *srcPix++ = static_cast<u16>(
                        (Clut16(0x10002 + off + (blue << 6)) | Clut16(0x2 + off + (green << 6))
                         | Clut16(0x20002 + off + red * 2))
                    );
                }
                srcPix += stride;
            }
        } else {
            delete[] scratch;
            m_ddSurface->Unlock(0);
            return 0;
        }
    } else {
        delete[] scratch;
        m_ddSurface->Unlock(0);
        return 0;
    }

    m_ddSurface->Unlock(0);
    delete[] scratch;
    return 1;
}

// @early-stop
RVA(0x0013f740, 0x1c8)
void BuildColorChannelTables() {
    if (g_rDown == PIXEL16_RED_DOWN && g_gDown == RGB555_GREEN_DOWN && g_bDown == PIXEL16_BLUE_DOWN
        && g_rUp == RGB555_RED_UP && g_gUp == PIXEL16_GREEN_UP) {
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
                    ClutStore16(0x20000 + base, static_cast<u16>((sum << 0xa)));
                    ClutStore16(base, static_cast<u16>((sum << 5)));
                    ClutStore16(0x10000 + base, static_cast<u16>((sum << bShift)));
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
                    ClutStore16(0x20000 + base, static_cast<u16>((sum << g_rUp)));
                    ClutStore16(base, static_cast<u16>(((sum << g_gUp) << 1)));
                    ClutStore16(0x10000 + base, static_cast<u16>((sum << g_bUp)));
                    varD += stepA;
                } while (--k != 0);
                varB += a;
            } while (--countB != 0);
            a++;
        } while (--stepA > 0);
    }
}

// `type` is Resolve's FileImageFormat: only FMT_BMP is implemented (the one
// caller, SaveScreenshot, writes "Gruntz%04i.BMP"), and SaveDispatch then picks
// the container by bit depth - .BMP at 8bpp, RLE16 at 16, .TGA at 24.
RVA(0x0013f910, 0x4a)
i32 CDDSurface::SaveFile(char* buf, FileImageFormat type, void* pal, i32 flag) {
    if (this->IsValid() == 0) {
        return 0;
    }
    if (buf == NULL) {
        return 0;
    }
    if (*buf == 0) {
        return 0;
    }
    switch (type) {
        case FMT_BMP:
            return SaveDispatch(buf, pal, flag);
        default:
            return 0;
    }
}

RVA(0x0013f960, 0x22)
i32 CDDSurface::RestoreLost() {
    if (m_restoreCallback != NULL) {
        if (m_restoreCallback(this) != 0) {
            return 1;
        }
    }
    RestoreLostSurfaces();
    return 0;
}

RVA(0x0013f990, 0xc4)
void CDDSurface::Tile(CDDSurface* src, i32 useColorKey) {
    i32 dwTrans = 0x10 + (useColorKey != 0);
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
            m_ddSurface->BltFast(x, y, src->m_ddSurface, pRect, dwTrans);
        }
    }
}

RVA(0x0013fa60, 0x40)
i32 CDDSurface::GetColorKey() {
    DDCOLORKEY key;
    i32 hr = m_ddSurface->GetColorKey(8, &key);
    if (hr != static_cast<i32>(DDERR_NOCOLORKEY)) {
        if (hr == 0) {
            return key.dwColorSpaceLowValue;
        }
        CDDrawPtrCollections::GetErrorString(DIRSURF_FILE, 0x695, hr);
    }
    return -1;
}

RVA(0x0013faa0, 0x108)
i32 CDDSurface::Blit(void* src, ColorDepth bitcount, void* palette, RasterRowOrder rowOrder) {
    ColorDepth dest = this->m_bitDepth;
    if (static_cast<ColorDepth>(dest == BPP_UNSET) == bitcount) {
        return BlitDirect(src, rowOrder);
    }
    switch (dest) {
        case BPP_PALETTED_8:
            switch (bitcount) {
                case BPP_RGB_16:
                    return Blit816(src, palette, rowOrder);
                case BPP_RGB_24:
                    return Blit824(src, palette, rowOrder);
            }
            return 0;
        case BPP_RGB_16:
            switch (bitcount) {
                case BPP_PALETTED_8:
                    return Blit168(src, palette, rowOrder);
                case BPP_RGB_24:
                    return Blit1624(src, rowOrder);
            }
            return 0;
        case BPP_RGB_24:
            switch (bitcount) {
                case BPP_PALETTED_8:
                    return Blit248(src, palette, rowOrder);
                case BPP_RGB_16:
                    return Blit2416(src, rowOrder);
            }
            return 0;
    }
    return 0;
}

// @early-stop
RVA(0x0013fbb0, 0x126)
i32 CDDSurface::Blit168(void* srcv, void* palv, RasterRowOrder rowOrder) {
    u8* pal = static_cast<u8*>(palv);
    if (pal == NULL) {
        return 0;
    }

    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        u8 r = static_cast<u8>((static_cast<u8>(pal[0]) >> g_rDown));
        pal += 4;
        u8 g = static_cast<u8>((static_cast<u8>(pal[-3]) >> g_gDown));
        u8 b = static_cast<u8>((static_cast<u8>(pal[-2]) >> g_bDown));
        g_lut16[i] = static_cast<u16>(
            ((static_cast<u32>(r) << g_rUp) | (static_cast<u32>(g) << g_gUp) | static_cast<u32>(b))
        );
    }
    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == NULL) {
        return 0;
    }
    u8* src = static_cast<u8*>(srcv);
    if (rowOrder == RASTER_ROWS_BOTTOM_UP) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = Row16(locked, row, m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = g_lut16[idx];
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = Row16(locked, row, m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = g_lut16[idx];
            }
        }
    }
    this->m_ddSurface->Unlock(0);
    return 1;
}

// @early-stop
RVA(0x0013fce0, 0x17f)
i32 CDDSurface::Blit1624(void* srcv, RasterRowOrder rowOrder) {
    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == NULL) {
        return 0;
    }
    u8* src = static_cast<u8*>(srcv);
    if (rowOrder == RASTER_ROWS_BOTTOM_UP) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = Row16(locked, row, m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 b = src[0];
                u8 g = src[1];
                u8 r = src[2];
                src += 3;
                *dst++ = static_cast<u16>(
                    ((static_cast<u8>((static_cast<u8>(g) >> g_gDown)) << g_gUp)
                     | (static_cast<u8>((static_cast<u8>(r) >> g_rDown)) << g_rUp)
                     | static_cast<u8>((static_cast<u8>(b) >> g_bDown)))
                );
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = Row16(locked, row, m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u8 b = src[0];
                u8 g = src[1];
                u8 r = src[2];
                src += 3;
                *dst++ = static_cast<u16>(
                    ((static_cast<u8>((static_cast<u8>(g) >> g_gDown)) << g_gUp)
                     | (static_cast<u8>((static_cast<u8>(r) >> g_rDown)) << g_rUp)
                     | static_cast<u8>((static_cast<u8>(b) >> g_bDown)))
                );
            }
        }
    }
    this->m_ddSurface->Unlock(0);
    return 1;
}

// @early-stop
RVA(0x0013fe60, 0x11e)
i32 CDDSurface::Blit248(void* srcv, void* palv, RasterRowOrder rowOrder) {
    PALETTEENTRY* pal = static_cast<PALETTEENTRY*>(palv);
    if (pal == NULL) {
        return 0;
    }
    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == NULL) {
        return 0;
    }
    u8* src = static_cast<u8*>(srcv);
    if (rowOrder == RASTER_ROWS_BOTTOM_UP) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = pal[idx].peBlue;
                *dst++ = pal[idx].peGreen;
                *dst++ = pal[idx].peRed;
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u8 idx = *src++;
                *dst++ = pal[idx].peBlue;
                *dst++ = pal[idx].peGreen;
                *dst++ = pal[idx].peRed;
            }
        }
    }
    this->m_ddSurface->Unlock(0);
    return 1;
}

RVA(0x0013ff80, 0x184)
i32 CDDSurface::Blit2416(void* srcv, RasterRowOrder rowOrder) {
    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == NULL) {
        return 0;
    }
    u16* src = static_cast<u16*>(srcv);
    if (rowOrder == RASTER_ROWS_BOTTOM_UP) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u16* dst = Row16(locked, row, m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                dst[0] = static_cast<u8>((static_cast<u8>((px >> g_rUp)) << g_rDown));
                dst[1] = static_cast<u8>((static_cast<u8>((px >> g_gUp)) << g_gDown));
                dst[2] = static_cast<u8>((static_cast<u8>(px) << g_bDown));
                dst += 3;
            }
        }
    } else {
        for (i32 row = 0; row < this->m_height; row++) {
            u16* dst = Row16(locked, row, m_pitch);
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                dst[0] = static_cast<u8>((static_cast<u8>((px >> g_rUp)) << g_rDown));
                dst[1] = static_cast<u8>((static_cast<u8>((px >> g_gUp)) << g_gDown));
                dst[2] = static_cast<u8>((static_cast<u8>(px) << g_bDown));
                dst += 3;
            }
        }
    }
    this->m_ddSurface->Unlock(0);
    return 1;
}

// @early-stop
RVA(0x00140110, 0x30b)
i32 CDDSurface::Blit824(void* srcv, void* palv, RasterRowOrder rowOrder) {
    PALETTEENTRY* pal = static_cast<PALETTEENTRY*>(palv);
    if (pal == NULL) {
        return 0;
    }
    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == NULL) {
        return 0;
    }
    u8* src = static_cast<u8*>(srcv);
    if (rowOrder == RASTER_ROWS_BOTTOM_UP) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u8 s0 = *src++;
                u8 s1 = *src++;
                u8 s2 = *src++;
                i32 best = 0;
                i32 d0 = s2 - pal[0].peRed;
                i32 d1 = s1 - pal[0].peGreen;
                i32 d2 = s0 - pal[0].peBlue;
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < PALETTE_ENTRY_COUNT; k++) {
                    i32 e0 = s2 - pal[k].peRed;
                    i32 e1 = s1 - pal[k].peGreen;
                    i32 e2 = s0 - pal[k].peBlue;
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
                u8 s0 = *src++;
                u8 s1 = *src++;
                u8 s2 = *src++;
                i32 best = 0;
                i32 d0 = s2 - pal[0].peRed;
                i32 d1 = s1 - pal[0].peGreen;
                i32 d2 = s0 - pal[0].peBlue;
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < PALETTE_ENTRY_COUNT; k++) {
                    i32 e0 = s2 - pal[k].peRed;
                    i32 e1 = s1 - pal[k].peGreen;
                    i32 e2 = s0 - pal[k].peBlue;
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
    this->m_ddSurface->Unlock(0);
    return 1;
}

// @early-stop
RVA(0x00140420, 0x34f)
i32 CDDSurface::Blit816(void* srcv, void* palv, RasterRowOrder rowOrder) {
    PALETTEENTRY* pal = static_cast<PALETTEENTRY*>(palv);
    if (pal == NULL) {
        return 0;
    }
    u8* locked = static_cast<u8*>(Lock(0));
    if (locked == NULL) {
        return 0;
    }
    u16* src = static_cast<u16*>(srcv);
    if (rowOrder == RASTER_ROWS_BOTTOM_UP) {
        for (i32 row = this->m_height - 1; row >= 0; row--) {
            u8* dst = locked + row * this->m_pitch;
            for (i32 col = 0; col < this->m_width; col++) {
                u16 px = *src++;
                i32 red = static_cast<u8>((static_cast<u8>((px >> g_rUp)) << g_rDown));
                i32 green = static_cast<u8>((static_cast<u8>((px >> g_gUp)) << g_gDown));
                i32 blue = static_cast<u8>((static_cast<u8>(px) << g_bDown));
                i32 best = 0;
                i32 d1 = green - pal[0].peGreen;
                i32 d2 = blue - pal[0].peBlue;
                i32 d0 = red - pal[0].peRed;
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < PALETTE_ENTRY_COUNT; k++) {
                    i32 e0 = red - pal[k].peRed;
                    i32 e1 = green - pal[k].peGreen;
                    i32 e2 = blue - pal[k].peBlue;
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
                i32 red = static_cast<u8>((static_cast<u8>((px >> g_rUp)) << g_rDown));
                i32 green = static_cast<u8>((static_cast<u8>((px >> g_gUp)) << g_gDown));
                i32 blue = static_cast<u8>((static_cast<u8>(px) << g_bDown));
                i32 best = 0;
                i32 d1 = green - pal[0].peGreen;
                i32 d2 = blue - pal[0].peBlue;
                i32 d0 = red - pal[0].peRed;
                i32 bestd = d1 * d1 + d2 * d2 + d0 * d0;
                for (i32 k = 1; k < PALETTE_ENTRY_COUNT; k++) {
                    i32 e0 = red - pal[k].peRed;
                    i32 e1 = green - pal[k].peGreen;
                    i32 e2 = blue - pal[k].peBlue;
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
    this->m_ddSurface->Unlock(0);
    return 1;
}

RVA(0x00140770, 0x326)
void CDDSurface::DumpSurfaceInfo(i32 detailed) {
    i32 i;
    i32* p = m_descWords;
    for (i = 0x1b; i != 0; i--) {
        *p++ = 0;
    }
    m_descSize = sizeof(DDSURFACEDESC);
    LPDDSURFACEDESC desc = &m_apiDesc;
    m_ddSurface->GetSurfaceDesc(desc);
    if (desc == NULL) {
        return;
    }

    if (detailed == 0) {
        ColorDepth depth = BPP_UNSET;
        switch (desc->ddpfPixelFormat.dwRGBBitCount) {
            case DDBD_32:
                depth = BPP_RGB_32;
                break;
            case DDBD_16:
                depth = BPP_RGB_16;
                break;
            case DDBD_8:
                depth = BPP_PALETTED_8;
                break;
            case DDBD_4:
                depth = BPP_PALETTED_4;
                break;
            case DDBD_2:
                depth = BPP_PALETTED_2;
                break;
            case DDBD_1:
                depth = BPP_MONO_1;
                break;
        }
        DDrawLogLine(
            "Surface: width = %i, height = %i, depth = %i, pitch = %i\n",
            m_width,
            m_height,
            IDX(depth),
            m_pitch
        );
        return;
    }

    u32 caps = desc->ddsCaps.dwCaps;
    i32 colorKey = GetColorKey();
    ColorDepth depth = BPP_UNSET;
    switch (desc->ddpfPixelFormat.dwRGBBitCount) {
        case DDBD_32:
            depth = BPP_RGB_32;
            break;
        case DDBD_16:
            depth = BPP_RGB_16;
            break;
        case DDBD_8:
            depth = BPP_PALETTED_8;
            break;
        case DDBD_4:
            depth = BPP_PALETTED_4;
            break;
        case DDBD_2:
            depth = BPP_PALETTED_2;
            break;
        case DDBD_1:
            depth = BPP_MONO_1;
            break;
    }
    DDrawLogLine("Surface Information for surface pointer %p:\n", this);
    DDrawLogLine(
        "width = %i, height = %i, depth = %i, pitch = %i\n",
        m_width,
        m_height,
        IDX(depth),
        m_pitch
    );
    if (depth == BPP_RGB_16) {
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

        char buf[32];
        switch (desc->dwZBufferBitDepth) {
            case DDBD_32:
                strcpy(buf, "DDBD_32");
                break;
            case DDBD_16:
                strcpy(buf, "DDBD_16");
                break;
            case DDBD_8:
                strcpy(buf, "DDBD_8");
                break;
            case DDBD_4:
                strcpy(buf, "DDBD_4");
                break;
            case DDBD_2:
                strcpy(buf, "DDBD_2");
                break;
            case DDBD_1:
                strcpy(buf, "DDBD_1");
                break;
            default:
                strcpy(buf, "Unknown");
                break;
        }
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

#pragma optimize("", off)

RVA(0x00140aa0, 0x1a3)
i32 CDDSurface::DecodeRun8(void* src) {
    u8* sp;
    i32 hold;
    u8* pbits;
    i32 w;
    u8 tok;
    i32 y;
    i32 runx;
    u8* dstp;
    i32 height;
    i32 kj;
    i32 nleft;
    if (src == NULL) {
        return 0;
    }
    w = this->GetWidth();
    height = this->GetHeight();
    hold = 0;
    sp = static_cast<u8*>(src);
    pbits = static_cast<u8*>(this->Lock(0));
    if (pbits == NULL) {
        return 0;
    }
    for (y = 0; y < height; y++) {
        dstp = (pbits + this->Scale(y));
        nleft = w;
        if (hold > 0) {
            for (kj = 0; kj < hold; kj++) {
                *dstp = tok;
                dstp++;
            }
            nleft -= hold;
            hold = 0;
        }
        while (nleft > 0) {
            tok = *sp;
            sp++;
            if ((tok & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                runx = tok & BYTE_RUN_LENGTH_MASK;
                tok = *sp;
                sp++;
                if (runx > nleft) {
                    hold = runx - nleft;
                    runx = nleft;
                }
                for (kj = 0; kj < runx; kj++) {
                    *dstp = tok;
                    dstp++;
                }
                nleft -= runx;
            } else {
                *dstp = tok;
                dstp++;
                nleft--;
            }
        }
    }
    this->UnlockThunk();
    return 1;
}

RVA(0x00140c50, 0x3e2)
i32 CDDSurface::DecodeRun24(void* src) {
    u8* inp;
    i32 rest;
    u8* dst;
    i32 cnt;
    i32 nrow;
    u8* ln;
    u8 pm;
    i32 k;
    i32 cols;
    if (src == NULL) {
        return 0;
    }
    ln = static_cast<u8*>(this->Lock(0));
    if (ln == NULL) {
        return 0;
    }
    rest = 0;
    inp = static_cast<u8*>(src);
    dst = NULL;
    for (nrow = 0; nrow < this->GetHeight(); nrow++) {
        dst = (ln + this->Scale(nrow) + 2);
        cols = this->GetWidth();
        if (rest > 0) {
            for (k = 0; k < rest; k++) {
                *dst = pm;
                dst += 3;
            }
            cols -= rest;
            rest = 0;
        }
        while (cols > 0) {
            pm = *inp;
            inp++;
            if ((pm & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                cnt = pm & BYTE_RUN_LENGTH_MASK;
                pm = *inp;
                inp++;
                if (cnt > cols) {
                    rest = cnt - cols;
                    cnt = cols;
                }
                for (k = 0; k < cnt; k++) {
                    *dst = pm;
                    dst += 3;
                }
                cols -= cnt;
            } else {
                *dst = pm;
                dst += 3;
                cols--;
            }
        }
        dst = (ln + this->Scale(nrow) + 1);
        cols = this->GetWidth();
        if (rest > 0) {
            for (k = 0; k < rest; k++) {
                *dst = pm;
                dst += 3;
            }
            cols -= rest;
            rest = 0;
        }
        while (cols > 0) {
            pm = *inp;
            inp++;
            if ((pm & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                cnt = pm & BYTE_RUN_LENGTH_MASK;
                pm = *inp;
                inp++;
                if (cnt > cols) {
                    rest = cnt - cols;
                    cnt = cols;
                }
                for (k = 0; k < cnt; k++) {
                    *dst = pm;
                    dst += 3;
                }
                cols -= cnt;
            } else {
                *dst = pm;
                dst += 3;
                cols--;
            }
        }
        dst = (ln + this->Scale(nrow));
        cols = this->GetWidth();
        if (rest > 0) {
            for (k = 0; k < rest; k++) {
                *dst = pm;
                dst += 3;
            }
            cols -= rest;
            rest = 0;
        }
        while (cols > 0) {
            pm = *inp;
            inp++;
            if ((pm & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                cnt = pm & BYTE_RUN_LENGTH_MASK;
                pm = *inp;
                inp++;
                if (cnt > cols) {
                    rest = cnt - cols;
                    cnt = cols;
                }
                for (k = 0; k < cnt; k++) {
                    *dst = pm;
                    dst += 3;
                }
                cols -= cnt;
            } else {
                *dst = pm;
                dst += 3;
                cols--;
            }
        }
    }
    this->UnlockThunk();
    return 1;
}

#pragma optimize("", on)

RVA(0x00141040, 0x36)
i32 CDDSurface::RotateBlit(
    CDDSurface* src,
    i32* pivot,
    i32 destX,
    i32 destY,
    float scale,
    i32 mode,
    i32 colorkey
) {

    ImageRotateBlit(destX, destY, pivot, this, src, 0.0f, scale, mode, colorkey);
    return 1;
}

RVA(0x00141080, 0x174)
i32 CDDSurface::StretchBlit(CDDSurface* src, RECT* srcRect, RECT* dstRect, i32 mode, i32 colorkey) {
    RECT sr;
    ClipVtx v[4];

    i32 srcW = src->m_width;
    i32 srcH = src->m_height;
    if (srcRect != NULL) {
        sr = *srcRect;
    } else {
        sr.left = 0;
        sr.right = srcW - 1;
        sr.top = 0;
        sr.bottom = srcH - 1;
    }
    v[0].x = static_cast<float>(dstRect->left);
    v[0].y = static_cast<float>(dstRect->top);
    v[0].u = static_cast<float>(sr.left);
    v[0].v = static_cast<float>(sr.top);
    v[1].x = static_cast<float>(dstRect->right);
    v[1].y = static_cast<float>(dstRect->top);
    v[1].u = static_cast<float>(sr.right);
    v[1].v = static_cast<float>(sr.top);
    v[2].x = static_cast<float>(dstRect->right);
    v[2].y = static_cast<float>(dstRect->bottom);
    v[2].u = static_cast<float>(sr.right);
    v[2].v = static_cast<float>(sr.bottom);
    v[3].x = static_cast<float>(dstRect->left);
    v[3].y = static_cast<float>(dstRect->bottom);
    v[3].u = static_cast<float>(sr.left);
    v[3].v = static_cast<float>(sr.bottom);
    RotateRasterize(v, 4, this, src, mode, colorkey, -1, -1, -1, -1);
    return 1;
}

RVA(0x00141200, 0x39)
i32 CDDSurface::ScaleBlit(
    CDDSurface* src,
    i32* pivot,
    i32 destX,
    i32 destY,
    float angle,
    i32 mode,
    i32 colorkey
) {

    ImageRotateBlit(destX, destY, pivot, this, src, angle, 1.0f, mode, colorkey);
    return 1;
}

RVA(0x00141240, 0x39)
i32 CDDSurface::RotateScaleBlit(
    CDDSurface* src,
    i32* pivot,
    i32 destX,
    i32 destY,
    float angle,
    float scale,
    i32 mode,
    i32 colorkey
) {
    ImageRotateBlit(destX, destY, pivot, this, src, angle, scale, mode, colorkey);
    return 1;
}

// @early-stop
RVA(0x00141280, 0x4a)
void CDDSurface::DecodeThunk(i32 x0, i32 y0, i32 x1, i32 y1, i32 halfWidth, i16 color, RECT clip) {
    ClipRect16 rec;
    rec.a = clip.left;
    rec.b = clip.top;
    rec.c = clip.right;
    rec.d = clip.bottom;
    ProjectWallQuad(this, x0, y0, x1, y1, halfWidth, color, rec.a, rec.b, rec.c, rec.d);
}

RVA(0x001412d0, 0x24)
i32 CDDSurface::IsValid() {
    if (m_ddSurface != NULL && m_fullRect.right > 0 && m_fullRect.bottom > 0) {
        return 1;
    }
    return 0;
}

RVA(0x00141300, 0x3)
DDSurfacePoolKind CDDSurface::GetPoolKind() {
    return POOLKIND_PLAIN;
}

RVA(0x00141310, 0x4)
i32 CDDSurface::GetWidth() {
    return m_width;
}

RVA(0x00141320, 0x4)
i32 CDDSurface::GetHeight() {
    return m_height;
}

RVA_COMPGEN(0x00141330, 0x1e, ??_GCDDSurface@@UAEPAXI@Z)
RVA_COMPGEN(0x00141350, 0x53, ??1CDDSurface@@UAE@XZ)

RVA(0x001413b0, 0xf)
void CDDSurface::UnlockThunk() {
    m_ddSurface->Unlock(0);
}

RVA(0x001413c0, 0xb)
i32 CDDSurface::Scale(i32 n) {
    return m_pitch * n;
}
