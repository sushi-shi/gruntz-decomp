#include <rva.h>

#include <Image/ImagePool.h>

#include <Mfc.h>

#include <ComOutRef.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirPal.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/PixelShift.h>
#include <Enums.h>
#include <Image/ByteRunEncoding.h>
#include <Image/FileImageRecords.h>
#include <Image/Image.h>
#include <Image/ImagePaletteNode.h>
#include <Image/RezDecodeKind.h>
#include <Pix16.h>
#include <Rez/RezMgr.h>
#include <Wap32/TileGeometry.h>

#include <string.h>

DATA(0x002bf6e0)
HINSTANCE g_hResModule = NULL;

DATA(0x0021aabc)
char g_bmpHeaderTemplate[4] = "BM";

RVA(0x00174e90, 0x1c)
i32 CImagePool::Configure(HINSTANCE resourceModule, HWND sourceWindow, i32 reserved) {
    m_resourceModuleHandle = resourceModule;
    m_sourceHwnd = sourceWindow;
    m_reserved08 = reserved;
    return 1;
}

RVA(0x00174eb0, 0x1b)
void CImagePool::Clear() {
    ClearSurfaces();
    ClearPalettes();
    m_resourceModuleHandle = NULL;
    m_sourceHwnd = NULL;
    m_reserved08 = 0;
}

RVA(0x00174ed0, 0x5d)
void CImagePool::RemoveSurface(CRezImage* image) {
    if (!image) {
        return;
    }
    if (image->m_paletteNode && image->m_paletteScalar) {
        RemovePalette(image->m_paletteNode);
        SetImagePalette(NULL, NULL, 0);
    }
    if (image->m_listPosition) {
        m_surfaces.RemoveAt(image->m_listPosition);
    }
    image->Free();
    delete image;
}

RVA(0x00174f30, 0x30)
void CImagePool::RemovePalette(CImagePaletteNode* palette) {
    if (!palette) {
        return;
    }
    if (palette->m_listPosition) {
        m_palettes.RemoveAt(palette->m_listPosition);
    }
    palette->Destroy();
    delete palette;
}

RVA(0x00174f60, 0x37)
void CImagePool::ClearSurfaces() {
    POSITION pos = m_surfaces.GetHeadPosition();
    while (pos) {
        CRezImage* item = static_cast<CRezImage*>(m_surfaces.GetNext(pos));
        if (item) {
            item->Free();
            delete item;
        }
    }
    m_surfaces.RemoveAll();
}

RVA(0x00174fa0, 0x3e)
void CImagePool::ClearPalettes() {
    POSITION pos = m_palettes.GetHeadPosition();
    while (pos) {
        CImagePaletteNode* item = static_cast<CImagePaletteNode*>(m_palettes.GetNext(pos));
        if (item) {
            item->Destroy();
            delete item;
        }
    }
    m_palettes.RemoveAll();
    m_reserved48 = 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00174fe0, 0xfe)
CRezImage* CImagePool::CreateSurface(i32 width, i32 height, ColorDepth bitDepth, i32 flags) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DecodeBmpHeader(hdc, width, height, bitDepth, flags) == BPP_UNSET) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, false);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, false);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001750e0, 0x103)
CRezImage* CImagePool::CreateSurfaceFromPixels(
    u8* pixels,
    i32 width,
    i32 height,
    ColorDepth bitDepth,
    i32 flags
) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DecodeBlit(pixels, hdc, width, height, bitDepth, flags) == BPP_UNSET) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, false);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, false);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

RVA(0x001751f0, 0xf9)
CRezImage* CImagePool::LoadSurfaceFromData(u8* data, RezDecodeKind format, i32 flags) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DispatchDecode(data, format, hdc, flags) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, false);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, false);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001752f0, 0xfc)
CRezImage* CImagePool::LoadSurfaceFromResource(char* resourceName, i32 flags) {
    HDC hdc = GetDC(m_sourceHwnd);
    g_hResModule = m_resourceModuleHandle;
    CRezImage* node = new CRezImage();
    if (node->LoadFromRez(resourceName, hdc, flags) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, false);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, false);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001753f0, 0xf4)
CRezImage* CImagePool::ConvertSurface(CRezImage* source, CImagePaletteNode* palette) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->Convert8To16(hdc, source, palette) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, false);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, false);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001754f0, 0x7b)
CImagePaletteNode* CImagePool::CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flags) {
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->CreateFromEntries(entries, flags) == 0) {
        if (node) {
            node->Destroy();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_palettes.AddTail(node);
    node->m_listPosition = pos;
    return node;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00175570, 0x7b)
CImagePaletteNode* CImagePool::CreatePaletteFromRgb(u8* rgb, i32 flags) {
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->CreateFromRgb(rgb, flags) == 0) {
        if (node) {
            node->Destroy();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_palettes.AddTail(node);
    node->m_listPosition = pos;
    return node;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001755f0, 0x82)
CImagePaletteNode* CImagePool::LoadPaletteFromFile(char* path, i32 flags) {
    g_hResModule = m_resourceModuleHandle;
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->LoadFromFile(path, flags) == 0) {
        if (node) {
            node->Destroy();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_palettes.AddTail(node);
    node->m_listPosition = pos;
    return node;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00175680, 0x85)
CImagePaletteNode*
CImagePool::LoadPaletteFromData(u8* data, u32 dataSize, RezDecodeKind format, i32 flags) {
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->LoadFromData(data, dataSize, format, flags) == 0) {
        if (node) {
            node->Destroy();
            delete node;
        }
        return NULL;
    }
    POSITION pos = m_palettes.AddTail(node);
    node->m_listPosition = pos;
    return node;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00175710, 0x69)
i32 CImagePool::ResizeSurface(
    CRezImage* image,
    i32 width,
    i32 height,
    ColorDepth bitDepth,
    i32 flags
) {
    if (image == NULL) {
        return 0;
    }
    HDC dc = GetDC(m_sourceHwnd);
    i32 result = image->EnsureSize(dc, width, height, bitDepth, flags);
    if (m_selectedPalette) {
        SelectPalette(dc, m_selectedPalette, false);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, dc);
    return result;
}

RVA(0x00175780, 0x3f)
void CImagePool::SetImagePalette(CRezImage* image, CImagePaletteNode* palette, i32 scalar) {
    if (image->m_paletteNode && image->m_paletteScalar) {
        RemovePalette(image->m_paletteNode);
        image->SetPalette(NULL, 0);
    }
    image->SetPalette(palette, scalar);
}

RVA(0x001757c0, 0x16f)
i32 CRezImage::DecodeBmpHeader(HDC dc, i32 width, i32 height, ColorDepth bitcount, i32 ctrl) {
    m_reserved434 = 0;
    m_width = width;
    m_height = (height < 0) ? -height : height;
    m_bitCount = bitcount;
    if (bitcount == BPP_PALETTED_8) {
        m_stride = ((width + 3) / 4) * 4;
    } else {
        m_stride = width;
    }
    m_rowPad = m_stride - width;
    m_paletteScalar = 0;
    m_paletteNode = NULL;
    m_transparent = true;
    memset(&m_bih, 0, sizeof(BITMAPINFOHEADER));
    m_bih.biWidth = m_width;
    m_bih.biBitCount = static_cast<WORD>(IDX(m_bitCount));
    m_bih.biSize = sizeof(BITMAPINFOHEADER);
    m_bih.biHeight = height;
    m_bih.biPlanes = 1;
    m_bih.biCompression = 0;
    m_bih.biSizeImage = 0;
    m_bih.biClrUsed = 0;
    m_bih.biClrImportant = 0;

    u16* pal = m_pal;
    if (m_bitCount == BPP_PALETTED_8) {
        for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
            *pal++ = static_cast<u16>(i);
        }
        m_dibSection = CreateDIBSection(dc, &m_bmi, DIB_PAL_COLORS, PtrOut(&m_pixels), NULL, 0);
    } else {
        m_dibSection = CreateDIBSection(dc, &m_bmi, DIB_RGB_COLORS, PtrOut(&m_pixels), NULL, 0);
    }
    if (!m_dibSection) {
        return 0;
    }
    m_rowOffsets = new i32[m_height];
    for (i32 i = 0; i < m_height; i++) {
        m_rowOffsets[i] = (m_height - i - 1) * (IDX(m_bitCount) / 8) * m_stride;
    }
    return 1;
}

// @early-stop
RVA(0x00175930, 0xc6)
i32 CRezImage::DecodeBlit(u8* src, HDC dc, i32 width, i32 height, ColorDepth bitcount, i32 ctrl) {
    if (!DecodeBmpHeader(dc, width, height, bitcount, ctrl)) {
        return 0;
    }
    if (IsStrideless()) {
        memcpy(m_pixels, src, (GetBufferSize() * IDX(bitcount)) / 8);
    } else {
        for (i32 row = 0; row < GetHeight(); row++) {
            memcpy(&m_pixels[GetIndex(row)], src, GetWidth());
            src += GetWidth();
        }
    }
    return 1;
}

RVA(0x00175a00, 0x90)
i32 CRezImage::DispatchDecode(u8* buf, RezDecodeKind kind, HDC dc, i32 ctrl) {
    switch (kind) {
        case DECODE_PCX: {
            RecordBytes<PcxHeader> data;
            data.m_bytes = buf;
            return DecodePcxData(data.m_rec, dc, ctrl);
        }
        case DECODE_BMP: {
            RecordBytes<BITMAPINFOHEADER> data;
            data.m_bytes = buf;
            return DecodeBmpData(data.m_rec, dc, ctrl);
        }
        case DECODE_RID: {
            RecordBytes<PidHeader> data;
            data.m_bytes = buf;
            return DecodeRidData(data.m_rec, dc, ctrl);
        }
        case DECODE_PID: {
            RecordBytes<PidHeader> data;
            data.m_bytes = buf;
            return DecodePidData(data.m_rec, dc, ctrl);
        }
    }
    return 0;
}

RVA(0x00175a90, 0xee)
i32 CRezImage::LoadFromRez(char* name, HDC dc, i32 ctrl) {
    char* ext = strrchr(name, '.');

    if (ext && _strcmpi(ext, ".BMP") == 0) {
        return LoadBmp(name, dc, ctrl);
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        return LoadPcx(name, dc, ctrl);
    } else if (ext && _strcmpi(ext, ".RID") == 0) {
        return LoadRid(name, dc, ctrl);
    } else if (ext && _strcmpi(ext, ".PID") == 0) {
        return LoadPid(name, dc, ctrl);
    }

    return LoadDefault(name, dc, ctrl);
}

RVA(0x00175b80, 0x105)
i32 CRezImage::Convert8To16(HDC dc, CRezImage* src, CImagePaletteNode* pal) {
    if (pal == NULL) {
        return 0;
    }
    PALETTEENTRY* palette = pal->m_logicalPalette.palPalEntry;
    if (palette == NULL) {
        return 0;
    }
    if (!DecodeBmpHeader(dc, src->m_width, src->m_height, BPP_RGB_16, 0)) {
        return 0;
    }
    for (i32 y = 0; y < m_height; y++) {
        u8* sp = src->m_pixels + y * src->m_stride;

        Pix16Ptr row;
        row.m_bytes = m_pixels;
        u16* dp = row.m_words + y * m_stride;
        for (i32 x = 0; x < m_width; x++) {
            PALETTEENTRY c = palette[*sp];
            u16 r = c.peRed;
            u16 g = c.peGreen;
            u8 b = c.peBlue;
            r &= ~7;
            g &= ~7;
            r <<= TILE_SHIFT_PX;
            r |= g;
            b >>= 3;
            r <<= 2;
            r |= b;
            *dp++ = r;
            sp++;
        }
    }
    return 1;
}

RVA(0x00175c90, 0x45)
void CRezImage::Free() {
    if (m_dibSection) {
        DeleteObject(m_dibSection);
        m_dibSection = NULL;
    }
    if (m_rowOffsets) {
        delete[] m_rowOffsets;
        m_rowOffsets = NULL;
    }
    m_pixels = NULL;
    m_paletteNode = NULL;
}

RVA(0x00175ce0, 0x6b)
i32 CRezImage::EnsureSize(HDC dc, i32 w, i32 h, ColorDepth bitCount, i32 flag) {
    if (m_dibSection && m_pixels && m_rowOffsets && m_width == w && m_height == h) {
        return 1;
    }
    Free();
    return DecodeBmpHeader(dc, w, h, bitCount, flag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00175d50, 0xad)
void CRezImage::Fill(i32 value) {
    if (m_rowPad == 0) {
        i32 fill = value & PIXEL_BYTE_MASK;
        memset(m_pixels, fill, m_stride * m_height);
    } else {

        i32 y = 0;
        if (y < m_height) {
            i32 fill = value & PIXEL_BYTE_MASK;
            do {
                memset(m_pixels + m_rowOffsets[y], fill, m_width);
                y++;
            } while (y < m_height);
        }
    }
}

// @early-stop
RVA(0x00175e00, 0x3d)
i32 CRezImage::DecodeBmpData(void* buf, HDC dc, i32 ctrl) {
    BITMAPINFOHEADER* ih = static_cast<BITMAPINFOHEADER*>(buf);
    i32 width = ih->biWidth;
    i32 height = ih->biHeight;
    ColorDepth bitcount = static_cast<ColorDepth>(ih->biBitCount);
    RecordBytes<BITMAPINFOHEADER> data;
    data.m_rec = ih;
    u8* src = data.m_bytes + sizeof(BITMAPINFOHEADER) + 4;
    if (bitcount == BPP_PALETTED_8) {
        src = data.m_bytes + ih->biSize + sizeof(RGBQUAD) * PALETTE_ENTRY_COUNT;
    }
    i32 r = DecodeBlit(src, dc, width, height, bitcount, ctrl);
    return r;
}

RVA(0x00175e40, 0x1b3)
i32 CRezImage::LoadBmp(char* name, HDC dc, i32 ctrl) {
    CFile file;
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;

    if (!file.Open(name, 0, NULL)) {
        return 0;
    }
    if (file.Read(&fh, sizeof(fh)) == 0) {
        return 0;
    }
    if (file.Read(&ih, sizeof(ih)) == 0) {
        return 0;
    }

    i32 height = ih.biHeight;
    i32 width = ih.biWidth;
    ColorDepth bitcount = static_cast<ColorDepth>(ih.biBitCount & 0xffff);
    if (!DecodeBmpHeader(dc, width, height, bitcount, ctrl)) {
        return 0;
    }

    file.Seek(fh.bfOffBits, 0);
    u32 size = (IDX(bitcount) / 8) * m_stride * height;
    if (file.Read(m_pixels, size) != size) {
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x00176000, 0x18f)
i32 CRezImage::DecodePcxData(void* buf, HDC dc, i32 ctrl) {
    u8* pStart = static_cast<u8*>(buf);
    PcxHeader* hdr = static_cast<PcxHeader*>(static_cast<void*>(pStart));
    i32 width = hdr->m_xMax - hdr->m_xMin + 1;
    i32 height = hdr->m_yMax - hdr->m_yMin + 1;
    if (hdr->m_bitsPerPixel != PCX_BITS_PER_PLANE_8) {
        return 0;
    }
    if (!DecodeBmpHeader(
            dc,
            width,
            height,
            static_cast<ColorDepth>(IDX(hdr->m_planes) * IDX(hdr->m_bitsPerPixel)),
            ctrl
        )) {
        return 0;
    }

    u32 offset = sizeof(PcxHeader);
    u8* packed = &pStart[offset];

    i32 i;
    i32 j;
    i32 remaining;
    i32 y;
    u8 value;
    u8* src = packed;
    u8* dst;
    u8* scan;

    scan = new u8[(width * IDX(hdr->m_bitsPerPixel) * IDX(hdr->m_planes)) / 8];

    for (y = 0; y < height; y++) {
        dst = m_pixels + m_rowOffsets[y];
        remaining = width * IDX(hdr->m_planes);

        while (remaining > 0) {
            value = *src++;

            if ((value & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                i = value & BYTE_RUN_LENGTH_MASK;
                value = *src++;

                for (j = 0; j < i; j++) {
                    scan[--remaining] = value;
                }
            } else {
                scan[--remaining] = value;
            }
        }

        if (hdr->m_planes == PCX_PLANES_PALETTED) {
            for (i = width; i != 0; i--) {
                *dst++ = scan[i - 1];
            }
        } else if (hdr->m_planes == PCX_PLANES_RGB) {
            for (i = width; i != 0; i--) {
                *dst++ = scan[i - 1];
                *dst++ = scan[width + i - 1];
                *dst++ = scan[2 * width + i - 1];
            }
        }
    }

    delete[] scan;
    return 1;
}

RVA(0x00176190, 0x126)
i32 CRezImage::LoadPcx(char* name, HDC dc, i32 ctrl) {
    CFile file;

    if (!file.Open(name, 0, NULL)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    u8* buf = new u8[len];
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    RecordBytes<PcxHeader> data;
    data.m_bytes = buf;
    i32 result = DecodePcxData(data.m_rec, dc, ctrl);
    delete[] buf;
    return result;
}

RVA(0x001762c0, 0x42)
i32 CRezImage::DecodeRidData(void* buf, HDC dc, i32 ctrl) {
    RecordBytes<PidHeader> p;
    p.m_bytes = static_cast<u8*>(buf);
    p.m_bytes += 2 * sizeof(u32);
    i32 width = *p.m_dwords;
    p.m_bytes += sizeof(u32);
    i32 height = *p.m_dwords;
    p.m_bytes += sizeof(u32);
    p.m_bytes += 4 * sizeof(u32);
    i32 ok = DecodeBlit(p.m_bytes, dc, width, height, BPP_PALETTED_8, ctrl);
    if (!(ctrl & 1)) {
        m_transparent = false;
    }
    return ok;
}

RVA(0x00176310, 0x126)
i32 CRezImage::LoadRid(char* name, HDC dc, i32 ctrl) {
    CFile file;

    if (!file.Open(name, 0, NULL)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    u8* buf = new u8[len];
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    RecordBytes<PidHeader> data;
    data.m_bytes = buf;
    i32 result = DecodeRidData(data.m_rec, dc, ctrl);
    delete[] buf;
    return result;
}

RVA(0x00176440, 0x25d)
i32 CRezImage::DecodePidData(void* buf, HDC dc, i32 ctrl) {
    PidHeader* header = static_cast<PidHeader*>(buf);
    u32* dword = &header->formatTag;
    u32 formatTag = *dword++;
    PidFlags flags = static_cast<PidFlags>(*dword++);
    u32 width = *dword++;
    u32 height = *dword++;
    u32 offsetX = *dword++;
    u32 offsetY = *dword++;
    u32 fill = *dword++;
    u32 reserved = *dword++;

    if (!DecodeBmpHeader(dc, width, height, BPP_PALETTED_8, ctrl)) {
        return 0;
    }
    if (!(ctrl & 1)) {
        m_transparent = false;
    }

    u8* packed = static_cast<u8*>(static_cast<void*>(dword));

    i32 transparentIndex;
    if (HAS(flags, PID_FILL_IS_WORD)) {
        transparentIndex = fill & PIXEL16_VALUE_MASK;
    } else {
        transparentIndex = 0;
    }

    if (HAS(flags, PID_GRAMMAR_SKIPRUN)) {
        m_transparent = true;
        i32 x = 0;
        i32 y = 0;
        u32 offset = 0;
        u8* dst = m_pixels + m_rowOffsets[y];

        while (y < m_height) {
            if (packed[offset] & 0x80) {
                memset(dst + x, transparentIndex, packed[offset] - 0x80);
                x += packed[offset] - 0x80;
                offset++;
            } else {
                memcpy(dst + x, packed + offset + 1, packed[offset]);
                x += packed[offset];
                offset += packed[offset] + 1;
            }

            if (x >= m_width) {
                y++;
                x = 0;
                if (y < m_height) {
                    dst = m_pixels + m_rowOffsets[y];
                }
            }
        }
    } else {
        i32 i;
        i32 j;
        i32 n;
        u32 y;
        u8 value;
        u8* src = packed;
        u8* dst;

        for (y = 0; y < height; y++) {
            dst = m_pixels + m_rowOffsets[y];
            n = width;

            while (n > 0) {
                value = *src++;

                if ((value & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                    i = value & BYTE_RUN_LENGTH_MASK;
                    value = *src++;

                    for (j = 0; j < i; j++) {
                        *dst++ = value;
                    }

                    n -= i;
                } else {
                    *dst++ = value;
                    n--;
                }
            }
        }
    }
    return 1;
}

RVA(0x001766a0, 0x126)
i32 CRezImage::LoadPid(char* name, HDC dc, i32 ctrl) {
    CFile file;

    if (!file.Open(name, 0, NULL)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    u8* buf = new u8[len];
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    RecordBytes<PidHeader> data;
    data.m_bytes = buf;
    i32 result = DecodePidData(data.m_rec, dc, ctrl);
    delete[] buf;
    return result;
}

RVA(0x001767d0, 0x64)
i32 CRezImage::LoadDefault(char* name, HDC dc, i32 ctrl) {
    HINSTANCE hModule = g_hResModule;
    if (!hModule) {
        return 0;
    }
    HRSRC hRsrc = FindResourceA(hModule, name, RT_BITMAP);
    if (!hRsrc) {
        return 0;
    }
    HGLOBAL hGlobal = LoadResource(hModule, hRsrc);
    if (!hGlobal) {
        return 0;
    }
    BITMAPINFOHEADER* data = static_cast<BITMAPINFOHEADER*>(LockResource(hGlobal));
    if (!data) {
        return 0;
    }
    return DecodeBmpData(data, dc, ctrl);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176840, 0x11f)
void CRezImage::FlipVertical() {
    if (GetHeight() <= 1) {
        return;
    }

    u8* scratch = new u8[GetWidth()];
    ASSERT(scratch);
    if (scratch == NULL) {
        return;
    }

    u32 k;
    u32 source;
    u32 destination;

    i32 j;
    i32 width = GetWidth();
    i32 height = GetHeight();

    for (i32 i = 0; i < height / 2; i++) {
        k = i * width;
        for (j = 0; j < width; j++) {
            scratch[j] = m_pixels[k++];
        }

        source = (height - 1 - i) * width;
        destination = i * width;
        for (j = 0; j < width; j++) {
            m_pixels[destination++] = m_pixels[source++];
        }

        destination = (height - 1 - i) * width;
        for (j = 0; j < width; j++) {
            m_pixels[destination++] = scratch[j];
        }
    }

    delete[] scratch;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176960, 0x168)
i32 CRezImage::PasteFrom(CRezImage* src, i32 x, i32 y) {
    i32 h = src->m_height;
    i32 w = src->m_width;
    i32 dstW = m_width;
    i32 dstH = m_height;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (w + x - 1 >= dstW) {
        w = dstW - x;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (h + y - 1 >= dstH) {
        h = dstH - y;
    }

    if (src->m_transparent) {
        for (i32 row = 0; row < h; row++) {
            u8* d = m_pixels + m_rowOffsets[y + row] + x;
            u8* s = src->m_pixels + src->m_rowOffsets[row];
            for (i32 i = w; i > 0; i--) {
                u8 px = *s;
                if (px != 0) {
                    *d = px;
                }
                s++;
                d++;
            }
        }
    } else {
        for (i32 row = 0; row < h; row++) {
            u8* s = src->m_pixels + src->m_rowOffsets[row];
            u8* d = m_pixels + m_rowOffsets[y + row] + x;
            memcpy(d, s, w);
        }
    }
    return h;
}

RVA(0x00176ad0, 0x17)
void CRezImage::SetPalette(CImagePaletteNode* paletteNode, i32 scalar) {

    m_paletteNode = paletteNode;
    m_paletteScalar = scalar;
}

// @identity-TODO: owner, four-argument ABI, and failure result are proven; the operation is not.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176af0, 0x5)
i32 CRezImage::SaveByType(
    const char* filename,
    FileImageFormat type,
    CImagePaletteNode* paletteObj,
    i32 flags
) {
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176b00, 0x2c)
i32 CRezImage::Save(const char* filename, CImagePaletteNode* paletteObj) {
    switch (m_bitCount) {
        case BPP_PALETTED_8:
            return SaveBmp(filename, paletteObj);
        case BPP_RGB_16:
            return 0;
        case BPP_RGB_24:
            return 0;
    }
    return 0;
}

RVA(0x00176b30, 0x1e5)
i32 CRezImage::SaveBmp(const char* filename, CImagePaletteNode* paletteObj) {
    ASSERT(IsValid());
    ASSERT(filename);

    if (paletteObj == NULL) {
        paletteObj = m_paletteNode;
    }
    if (paletteObj == NULL) {
        return 0;
    }

    BmpFileHeaderStamp fileHdr;
    Bmp256Info info;
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = GetWidth();
    info.bmiHeader.biHeight = GetHeight();
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 8;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = 0;

    PALETTEENTRY* pal = paletteObj->Entries();
    if (pal == NULL) {
        return 0;
    }

    for (i32 i = 0; i < 0x100; i++) {
        info.bmiColors[i].rgbRed = pal[i].peRed;
        info.bmiColors[i].rgbGreen = pal[i].peGreen;
        info.bmiColors[i].rgbBlue = pal[i].peBlue;
    }

    memset(&fileHdr, 0, sizeof(fileHdr));
    strcpy(fileHdr.m_bytes, g_bmpHeaderTemplate);
    fileHdr.m_hdr.bfSize =
        sizeof(BITMAPFILEHEADER) + sizeof(Bmp256Info) + (GetWidth() * GetHeight());
    fileHdr.m_hdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(Bmp256Info);

    u8* pixels = GetBytes();
    if (pixels == NULL) {
        return 0;
    }

    CFile file;
    if (!file.Open(filename, CFile::modeCreate | CFile::modeWrite)) {
        return 0;
    }
    file.Write(&fileHdr.m_hdr, sizeof(fileHdr.m_hdr));
    file.Write(&info, sizeof(info));
    for (i32 row = GetHeight() - 1; row >= 0; row--) {
        u32 index = GetIndex(row);
        file.Write(&pixels[index], GetWidth());
    }
    return 1;
}

RVA(0x00176d20, 0x71)
void CRezImage::FillRect(CRezFillRect* r, i32 color) {
    i32 width = r->right - r->left;
    for (i32 y = r->top; y <= r->bottom; ++y) {
        i32 off = m_rowOffsets[y] + r->left;
        memset(m_pixels + off, color, width);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176da0, 0x4b)
void CRezImage::FillRectAt(i32 dx, i32 dy, CRezFillRect* src, i32 color) {
    CRezFillRect r;
    r.left = dx;
    r.top = dy;
    r.right = src->right + dx - src->left;
    r.bottom = src->bottom - src->top + dy;
    FillRect(&r, color);
}

RVA(0x00176df0, 0x71)
i32 CImagePaletteNode::CreateFromEntries(PALETTEENTRY* entries, i32 flags) {
    m_flags = flags;
    m_logicalPalette.palNumEntries = 0x100;
    m_logicalPalette.palVersion = LOGICAL_PALETTE_VERSION;
    for (i32 i = 0; i < 0x100; i++) {
        m_logicalPalette.palPalEntry[i] = entries[i];
        m_logicalPalette.palPalEntry[i].peFlags = 0;
    }
    if (DisplayUsesPalette() && !(flags & 1)) {
        ReserveSystemColors();
        m_reservedSystemColors = true;
    }
    m_palette = CreatePalette(&m_logicalPalette);
    return m_palette != NULL;
}

RVA(0x00176e70, 0x4e)
i32 CImagePaletteNode::CreateFromRgb(u8* rgb, i32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    u8* s = rgb;

    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        pal[i].peRed = *s++;
        pal[i].peGreen = *s++;
        pal[i].peBlue = *s++;
    }
    return CreateFromEntries(pal, flags);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176ec0, 0x64)
i32 CImagePaletteNode::CreateFromBgrx(u8* bgrx, i32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        u8* s = bgrx + i * 4;
        pal[i].peRed = s[2];
        pal[i].peGreen = s[1];
        pal[i].peBlue = s[0];
    }
    return CreateFromEntries(pal, flags);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176f30, 0x51)
i32 CImagePaletteNode::CreateFromBgr(u8* bgr, i32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        u8* s = bgr + i * 3;
        pal[i].peRed = s[2];
        pal[i].peGreen = s[1];
        pal[i].peBlue = s[0];
    }
    return CreateFromEntries(pal, flags);
}

RVA(0x00176f90, 0xa4)
i32 CImagePaletteNode::LoadFromFile(char* path, i32 flags) {
    char* ext = strrchr(path, '.');

    if (ext && _strcmpi(ext, ".BMP") == 0) {
        return LoadBmpFile(path, flags);
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        return LoadPcxFile(path, flags);
    } else if (ext && _strcmpi(ext, ".PAL") == 0) {
        return LoadPalFile(path, flags);
    }

    return LoadFromResource(path, flags);
}

RVA(0x00177040, 0x23)
i32 CImagePaletteNode::LoadFromData(u8* data, u32 dataSize, RezDecodeKind format, i32 flags) {
    if (format == DECODE_PCX) {
        return CreateFromTrailingRgb(data, dataSize, flags);
    }
    return 0;
}

RVA(0x00177070, 0x22)
void CImagePaletteNode::Destroy() {
    if (m_palette) {
        DeleteObject(m_palette);
        m_palette = NULL;
    }
    m_flags = 0;
}

RVA(0x001770a0, 0x3a)
i32 DisplayUsesPalette() {
    HDC ic = CreateICA("DISPLAY", NULL, NULL, NULL);
    if (ic) {
        i32 caps = GetDeviceCaps(ic, RASTERCAPS) & RC_PALETTE;
        DeleteDC(ic);
        return caps;
    }
    return 0;
}

RVA(0x001770e0, 0x7c)
void CImagePaletteNode::ReserveSystemColors() {
    ResetSystemPalette();
    HDC dc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    i32 sizePal = GetDeviceCaps(dc, SIZEPALETTE);
    i32 numReserved = GetDeviceCaps(dc, NUMRESERVED);
    i32 half = numReserved / 2;
    GetSystemPaletteEntries(dc, 0, half, m_logicalPalette.palPalEntry);
    GetSystemPaletteEntries(
        dc,
        sizePal - half,
        half,
        &m_logicalPalette.palPalEntry[m_logicalPalette.palNumEntries - half]
    );
    for (i32 i = half; i < sizePal - half; i++) {
        m_logicalPalette.palPalEntry[i].peFlags = 1;
    }
    DeleteDC(dc);
}

RVA(0x00177160, 0x81)
void ResetSystemPalette() {

    LogPal256 lp;
    HDC hdc = GetDC(NULL);
    lp.palVersion = LOGICAL_PALETTE_VERSION;
    lp.palNumEntries = 256;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        lp.palPalEntry[i].peRed = 0;
        lp.palPalEntry[i].peGreen = 0;
        lp.palPalEntry[i].peBlue = 0;
        lp.palPalEntry[i].peFlags = 4;
    }
    HPALETTE hpal = CreatePalette(&lp.m_lp);
    if (hpal) {
        HPALETTE old = SelectPalette(hdc, hpal, false);
        RealizePalette(hdc);
        DeleteObject(SelectPalette(hdc, old, false));
    }
    ReleaseDC(NULL, hdc);
}

RVA(0x001771f0, 0xe2)
i32 CImagePaletteNode::LoadPalFile(char* path, i32 flags) {
    CFile file;
    u8 rgb[PALETTE_RGB_BYTE_COUNT];

    if (!file.Open(path, 0, NULL)) {
        return 0;
    }
    if (file.GetLength() != PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    file.Read(rgb, PALETTE_RGB_BYTE_COUNT);
    return CreateFromRgb(rgb, flags);
}

RVA(0x001772e0, 0x117)
i32 CImagePaletteNode::LoadPcxFile(char* path, i32 flags) {
    CFile file;
    u8 rgb[PALETTE_RGB_BYTE_COUNT];

    PALETTEENTRY rgbq[PALETTE_ENTRY_COUNT];

    if (!file.Open(path, 0, NULL)) {
        return 0;
    }
    file.Seek(-PALETTE_RGB_BYTE_COUNT, 2);
    if (file.Read(rgb, PALETTE_RGB_BYTE_COUNT) == 0) {
        return 0;
    }

    u8* src = rgb;
    COPY_RGB_PALETTE(rgbq, src, i, PALETTE_ENTRY_COUNT)
    return CreateFromEntries(rgbq, flags);
}

RVA(0x00177400, 0x76)
i32 CImagePaletteNode::CreateFromTrailingRgb(u8* data, u32 dataSize, i32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    if (dataSize < PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    u8* s = data + dataSize - PALETTE_RGB_BYTE_COUNT;
    COPY_RGB_PALETTE(pal, s, i, PALETTE_ENTRY_COUNT)
    return CreateFromEntries(pal, flags);
}
