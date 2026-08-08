#include <rva.h>

#include <Image/ImagePool.h>

#include <Mfc.h>

#include <ComOutRef.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirPal.h>
#include <DDrawMgr/PaletteSize.h>
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
HINSTANCE g_hResModule = 0;

DATA(0x0021aabc)
char g_bmpHeaderTemplate[4] = "BM";

RVA(0x00174e90, 0x1c)
i32 CImagePool::SetHandles(HINSTANCE resModule, HWND src, i32 c) {
    m_resourceModuleHandle = resModule;
    m_sourceHwnd = src;
    m_reserved08 = c;
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
void CImagePool::Free(CRezImage* node) {
    if (!node) {
        return;
    }
    if (node->m_paletteNode && node->m_paletteScalar) {
        RemovePalette(node->m_paletteNode);
        B(0, 0, 0);
    }
    if (node->m_listPosition) {
        m_surfaces.RemoveAt(node->m_listPosition);
    }
    node->Free();
    ::operator delete(node);
}

RVA(0x00174f30, 0x30)
void CImagePool::RemovePalette(CImagePaletteNode* node) {
    if (!node) {
        return;
    }
    if (node->m_listPosition) {
        m_palettes.RemoveAt(node->m_listPosition);
    }
    node->Run();
    ::operator delete(node);
}

RVA(0x00174f60, 0x37)
void CImagePool::ClearSurfaces() {
    POSITION pos = m_surfaces.GetHeadPosition();
    while (pos) {
        CRezImage* item = static_cast<CRezImage*>(m_surfaces.GetNext(pos));
        if (item) {
            item->Free();
            ::operator delete(item);
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
            item->Run();
            ::operator delete(item);
        }
    }
    m_palettes.RemoveAll();
    m_reserved48 = 0;
}

RVA(0x00174fe0, 0xfe)
CRezImage* CImagePool::AddSurfaceBmp(i32 width, i32 height, ColorDepth bitCount, i32 flag) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DecodeBmpHeader(hdc, width, height, bitCount, flag) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

RVA(0x001750e0, 0x103)
CRezImage*
CImagePool::AddSurfaceBlit(void* src, i32 width, i32 height, ColorDepth bitCount, i32 flag) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DecodeBlit(src, hdc, width, height, bitCount, flag) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

RVA(0x001751f0, 0xf9)
CRezImage* CImagePool::AddSurfaceOp(void* buf, RezDecodeKind kind, i32 ctrl) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DispatchDecode(buf, kind, hdc, ctrl) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

RVA(0x001752f0, 0xfc)
CRezImage* CImagePool::AddSurfaceRez(char* name, i32 ctrl) {
    HDC hdc = GetDC(m_sourceHwnd);
    g_hResModule = m_resourceModuleHandle;
    CRezImage* node = new CRezImage();
    if (node->LoadFromRez(name, hdc, ctrl) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

RVA(0x001753f0, 0xf4)
CRezImage* CImagePool::AddSurfaceConvert(CRezImage* src, void* pal) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->Convert8To16(hdc, src, pal) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = NULL;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_surfaces.AddTail(node);
    node->m_listPosition = pos;
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

RVA(0x001754f0, 0x7b)
CImagePaletteNode* CImagePool::AddPaletteEntries(PALETTEENTRY* entries, i32 flags) {
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->Build(entries, flags) == 0) {
        if (node) {
            node->Run();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_palettes.AddTail(node);
    node->m_listPosition = pos;
    return node;
}

RVA(0x00175570, 0x7b)
CImagePaletteNode* CImagePool::AddPaletteRGB(void* rgb, i32 flags) {
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->ProcessPal(rgb, flags) == 0) {
        if (node) {
            node->Run();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_palettes.AddTail(node);
    node->m_listPosition = pos;
    return node;
}

RVA(0x001755f0, 0x82)
CImagePaletteNode* CImagePool::AddImageFile(char* path, i32 arg) {
    g_hResModule = m_resourceModuleHandle;
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->LoadByExtension(path, arg) == 0) {
        if (node) {
            node->Run();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_palettes.AddTail(node);
    node->m_listPosition = pos;
    return node;
}

RVA(0x00175680, 0x85)
CImagePaletteNode* CImagePool::AddImageDispatch(void* buf, u32 size, RezDecodeKind type, i32 ctrl) {
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->ParseDispatch(buf, size, type, ctrl) == 0) {
        if (node) {
            node->Run();
            ::operator delete(node);
        }
        return 0;
    }
    POSITION pos = m_palettes.AddTail(node);
    node->m_listPosition = pos;
    return node;
}

RVA(0x00175710, 0x69)
i32 CImagePool::EnsureSurface(CRezImage* img, i32 w, i32 h, ColorDepth bitCount, i32 flag) {
    if (img == NULL) {
        return 0;
    }
    HDC dc = GetDC(m_sourceHwnd);
    i32 result = img->EnsureSize(dc, w, h, bitCount, flag);
    if (m_selectedPalette) {
        SelectPalette(dc, m_selectedPalette, FALSE);
        m_selectedPalette = NULL;
    }
    ReleaseDC(m_sourceHwnd, dc);
    return result;
}

RVA(0x00175780, 0x3f)
void CImagePool::B(CRezImage* node, void* paletteNode, i32 b) {
    if (node->m_paletteNode && node->m_paletteScalar) {
        RemovePalette(node->m_paletteNode);
        node->SetPalette(0, 0);
    }
    node->SetPalette(paletteNode, b);
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
        m_dibSection = CreateDIBSection(dc, &m_bmi, DIB_PAL_COLORS, PtrOut(&m_pixels), 0, 0);
    } else {
        m_dibSection = CreateDIBSection(dc, &m_bmi, DIB_RGB_COLORS, PtrOut(&m_pixels), 0, 0);
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
i32 CRezImage::DecodeBlit(void* src, HDC dc, i32 width, i32 height, ColorDepth bitcount, i32 ctrl) {
    if (!DecodeBmpHeader(dc, width, height, bitcount, ctrl)) {
        return 0;
    }
    if (m_rowPad == 0) {
        memcpy(m_pixels, src, static_cast<u32>((m_stride * m_height * IDX(bitcount))) >> 3);
        return 1;
    }
    char* s = static_cast<char*>(src);
    for (i32 row = 0; row < m_height; row++) {
        memcpy(m_pixels + m_rowOffsets[row], s, m_width);
        s += m_width;
    }
    return 1;
}

// @early-stop
// Scoring artifact, not a source defect: the switch's case bodies compile into a
// SECOND symbol next to the jump table, so objdiff pairs only the dispatch prologue
// against retail's whole function (delinker jump-table dup-symbol undercount).
RVA(0x00175a00, 0x90)
i32 CRezImage::DispatchDecode(void* buf, RezDecodeKind kind, HDC dc, i32 ctrl) {
    switch (kind) {
        case DECODE_BMP:
            return DecodeBmpData(buf, dc, ctrl);
        case DECODE_PCX:
            return DecodePcxData(buf, dc, ctrl);
        case DECODE_RID:
            return DecodeRidData(buf, dc, ctrl);
        case DECODE_PID:
            return DecodePidData(buf, dc, ctrl);
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
i32 CRezImage::Convert8To16(HDC dc, CRezImage* src, void* pal) {
    if (pal == NULL) {
        return 0;
    }
    PALETTEENTRY* palette = (static_cast<ScanlinePalette*>(pal))->m_colors;
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

RVA(0x00175d50, 0xad)
void CRezImage::Fill(i32 value) {
    if (m_rowPad == 0) {
        i32 fill = value & 0xff;
        memset(m_pixels, fill, m_stride * m_height);
    } else {

        i32 y = 0;
        if (y < m_height) {
            i32 fill = value & 0xff;
            do {
                memset(m_pixels + m_rowOffsets[y], fill, m_width);
                y++;
            } while (y < m_height);
        }
    }
}

RVA(0x00175e00, 0x3d)
i32 CRezImage::DecodeBmpData(void* buf, HDC dc, i32 ctrl) {
    BITMAPINFOHEADER* ih = static_cast<BITMAPINFOHEADER*>(buf);
    i32 width = ih->biWidth;
    i32 height = ih->biHeight;
    ColorDepth bitcount = static_cast<ColorDepth>(ih->biBitCount);
    void* src = static_cast<u8*>(buf) + sizeof(BITMAPINFOHEADER) + 4;
    if (bitcount == BPP_PALETTED_8) {
        src = static_cast<u8*>(buf) + ih->biSize + sizeof(RGBQUAD) * PALETTE_ENTRY_COUNT;
    }
    i32 r = DecodeBlit(src, dc, width, height, bitcount, ctrl);
    return r;
}

RVA(0x00175e40, 0x1b3)
i32 CRezImage::LoadBmp(char* name, HDC dc, i32 ctrl) {
    CFile file;
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;

    if (!file.Open(name, 0, 0)) {
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

RVA(0x00176000, 0x18f)
i32 CRezImage::DecodePcxData(void* buf, HDC dc, i32 ctrl) {
    PcxHeader* hdr = static_cast<PcxHeader*>(buf);
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

    u8* src = hdr->m_pixels;
    i32 scanBytes = width * IDX(hdr->m_planes) * IDX(hdr->m_bitsPerPixel) / 8;
    u8* scan = new u8[scanBytes];

    for (i32 y = 0; y < height; y++) {
        u8* dst = m_pixels + m_rowOffsets[y];
        i32 n = width * IDX(hdr->m_planes);
        while (n > 0) {
            u8 c = *src++;
            if ((c & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                i32 count = c & BYTE_RUN_LENGTH_MASK;
                u8 v = *src++;
                if (count > 0) {
                    do {
                        --n;
                        --count;
                        scan[n] = v;
                    } while (count != 0);
                }
            } else {
                scan[--n] = c;
            }
        }

        if (hdr->m_planes == PCX_PLANES_PALETTED) {
            for (i32 x = width; x != 0; x--) {
                *dst++ = scan[x - 1];
            }
        } else if (hdr->m_planes == PCX_PLANES_RGB) {
            u8* g = scan + width * 2;
            u8* b = g + width;
            for (i32 x = width; x != 0; x--) {
                *dst++ = scan[x - 1];
                *dst++ = g[-1];
                *dst++ = b[-1];
                --g;
                --b;
            }
        }
    }

    delete[] scan;
    return 1;
}

RVA(0x00176190, 0x126)
i32 CRezImage::LoadPcx(char* name, HDC dc, i32 ctrl) {
    CFile file;

    if (!file.Open(name, 0, 0)) {
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
    i32 result = DecodePcxData(buf, dc, ctrl);
    delete[] buf;
    return result;
}

RVA(0x001762c0, 0x42)
i32 CRezImage::DecodeRidData(void* buf, HDC dc, i32 ctrl) {

    i32* p = &static_cast<PidHeader*>(buf)->width;
    i32 width = *p++;
    i32 height = *p;
    p += 5;

    i32 ok = DecodeBlit(p, dc, width, height, BPP_PALETTED_8, ctrl);
    if (!(ctrl & 1)) {
        m_transparent = false;
    }
    return ok;
}

RVA(0x00176310, 0x126)
i32 CRezImage::LoadRid(char* name, HDC dc, i32 ctrl) {
    CFile file;

    if (!file.Open(name, 0, 0)) {
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
    i32 result = DecodeRidData(buf, dc, ctrl);
    delete[] buf;
    return result;
}

// @early-stop
RVA(0x00176440, 0x25d)
i32 CRezImage::DecodePidData(void* buf, HDC dc, i32 ctrl) {
    Pix16Ptr src;
    src.m_bytes = static_cast<u8*>(buf);
    src.m_dwords++;

    PidFlags flags = static_cast<PidFlags>(*src.m_dwords++);
    i32 width = *src.m_dwords++;
    i32 height = *src.m_dwords++;
    src.m_dwords += 2;
    i32 fill = *src.m_dwords++;
    src.m_dwords++;

    if (!DecodeBmpHeader(dc, width, height, BPP_PALETTED_8, ctrl)) {
        return 0;
    }
    if (!(ctrl & 1)) {
        m_transparent = false;
    }

    if (HAS(flags, PID_FILL_IS_WORD)) {
        fill &= 0xffff;
    } else {
        fill = 0;
    }

    if (HAS(flags, PID_GRAMMAR_SKIPRUN)) {
        m_transparent = true;
        u8* dstRow = m_pixels + m_rowOffsets[0];
        i32 x = 0;
        i32 y = 0;
        i32 i = 0;
        while (y < m_height) {
            i32 c = src.m_bytes[i];
            if (c & 0x80) {
                i32 count = c - 0x80;
                memset(dstRow + x, static_cast<u8>(fill), count);
                x += (src.m_bytes[i] & 0xff) - 0x80;
                i++;
            } else {
                i32 count = c;
                memcpy(dstRow + x, &src.m_bytes[i + 1], count);
                x += src.m_bytes[i];
                i += src.m_bytes[i] + 1;
            }
            if (x >= m_width) {
                y++;
                x = 0;
                if (y >= m_height) {
                    break;
                }
                dstRow = m_pixels + m_rowOffsets[y];
            }
        }
    } else {
        for (i32 y = 0; static_cast<u32>(y) < static_cast<u32>(height); y++) {
            u8* dst = m_pixels + m_rowOffsets[y];
            i32 n = width;
            while (n > 0) {
                u8 c = *src.m_bytes++;
                if ((c & BYTE_RUN_CONTROL_MASK) == BYTE_RUN_MARKER) {
                    i32 count = c & BYTE_RUN_LENGTH_MASK;
                    u8 v = *src.m_bytes++;
                    if (count > 0) {
                        memset(dst, v, count);
                        dst += count;
                    }
                    n -= count;
                } else {
                    *dst++ = c;
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

    if (!file.Open(name, 0, 0)) {
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
    i32 result = DecodePidData(buf, dc, ctrl);
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
    void* data = LockResource(hGlobal);
    if (!data) {
        return 0;
    }
    return DecodeBmpData(data, dc, ctrl);
}

// @early-stop
RVA(0x00176840, 0x11f)
void CRezImage::FlipVertical() {
    if (m_height <= 1) {
        return;
    }
    u8* scratch = new u8[m_width];
    if (scratch == NULL) {
        return;
    }
    i32 wid = m_width;
    i32 pairs = m_height / 2;
    i32 x;
    for (i32 i = 0; i < pairs; i++) {
        for (x = 0; x < wid; x++) {
            scratch[x] = m_pixels[i * wid + x];
        }

        i32 botOff = (m_height - i - 1) * wid;
        for (x = 0; x < wid; x++) {
            m_pixels[i * wid + x] = m_pixels[botOff + x];
        }
        for (x = 0; x < wid; x++) {
            m_pixels[botOff + x] = scratch[x];
        }
    }
    delete[] scratch;
}

// @early-stop
RVA(0x00176960, 0x168)
i32 CRezImage::PasteFrom(CRezImage* src, i32 x, i32 y) {
    i32 h = src->m_height;
    i32 w = src->m_width;
    if (x < 0) {
        w += x;
        x = 0;
    }
    if (w + x - 1 >= m_width) {
        w = m_width - x;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (h + y - 1 >= m_height) {
        h = m_height - y;
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
        return h;
    }

    for (i32 row = 0; row < h; row++) {
        u8* s = src->m_pixels + src->m_rowOffsets[row];
        u8* d = m_pixels + m_rowOffsets[y + row] + x;
        memcpy(d, s, w);
    }
    return h;
}

RVA(0x00176ad0, 0x17)
void CRezImage::SetPalette(void* paletteNode, i32 scalar) {

    m_paletteNode = static_cast<CImagePaletteNode*>(paletteNode);
    m_paletteScalar = scalar;
}

RVA(0x00176b00, 0x2c)
i32 CRezImage::Save(const char* filename, void* paletteObj) {
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
i32 CRezImage::SaveBmp(const char* filename, void* paletteObj) {
    void* obj = paletteObj;
    if (obj == NULL) {
        obj = m_paletteNode;
        if (obj == NULL) {
            return 0;
        }
    }

    BmpFileHeaderStamp fileHdr;
    Bmp256Info info;
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = 0x28;
    info.bmiHeader.biWidth = m_width;
    info.bmiHeader.biHeight = m_height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 8;
    info.bmiHeader.biCompression = 0;
    info.bmiHeader.biSizeImage = 0;

    PALETTEENTRY* pal = static_cast<CImagePaletteNode*>(obj)->m_pal.palPalEntry;
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
    fileHdr.m_hdr.bfSize = m_width * m_height + 0x436;
    fileHdr.m_hdr.bfOffBits = 0x436;
    if (m_pixels == NULL) {
        return 0;
    }

    CFile file;
    if (file.Open(filename, 0x1001, 0) == 0) {
        return 0;
    }
    file.Write(&fileHdr.m_hdr, sizeof(fileHdr.m_hdr));
    file.Write(&info, sizeof(info));
    for (i32 row = m_height - 1; row >= 0; row--) {
        file.Write(m_pixels + m_rowOffsets[row], m_width);
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
i32 CImagePaletteNode::Build(PALETTEENTRY* src, i32 flags) {
    m_flags = flags;
    m_pal.palNumEntries = 0x100;
    m_pal.palVersion = LOGICAL_PALETTE_VERSION;
    for (i32 i = 0; i < 0x100; i++) {
        m_pal.palPalEntry[i] = src[i];
        m_pal.palPalEntry[i].peFlags = 0;
    }
    if (DisplayUsesPalette() && !(flags & 1)) {
        Tune();
        m_systemTuned = true;
    }
    m_palette = CreatePalette(&m_pal);
    return m_palette != NULL;
}

RVA(0x00176e70, 0x4e)
i32 CImagePaletteNode::ProcessPal(void* rgb, i32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    u8* s = static_cast<u8*>(rgb);

    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        pal[i].peRed = *s++;
        pal[i].peGreen = *s++;
        pal[i].peBlue = *s++;
    }
    return Build(pal, flags);
}

RVA(0x00176ec0, 0x64)
i32 CImagePaletteNode::ProcessPalQuad(void* bgr, i32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        u8* s = static_cast<u8*>(bgr) + i * 4;
        pal[i].peRed = s[2];
        pal[i].peGreen = s[1];
        pal[i].peBlue = s[0];
    }
    return Build(pal, flags);
}

RVA(0x00176f30, 0x51)
i32 CImagePaletteNode::ProcessPalBGR(void* bgr, i32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        u8* s = static_cast<u8*>(bgr) + i * 3;
        pal[i].peRed = s[2];
        pal[i].peGreen = s[1];
        pal[i].peBlue = s[0];
    }
    return Build(pal, flags);
}

RVA(0x00176f90, 0xa4)
i32 CImagePaletteNode::LoadByExtension(char* path, i32 arg) {
    char* ext = strrchr(path, '.');

    if (ext && _strcmpi(ext, ".BMP") == 0) {
        return LoadBmpFile(path, arg);
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        return LoadPcxFile(path, arg);
    } else if (ext && _strcmpi(ext, ".PAL") == 0) {
        return LoadPalFile(path, arg);
    }

    return Apply(path, arg);
}

RVA(0x00177040, 0x23)
i32 CImagePaletteNode::ParseDispatch(void* buf, u32 size, RezDecodeKind type, i32 ctrl) {
    if (type == DECODE_PCX) {
        return ParsePaletteTail(buf, size, ctrl);
    }
    return 0;
}

RVA(0x00177070, 0x22)
void CImagePaletteNode::Run() {
    if (m_palette) {
        DeleteObject(m_palette);
        m_palette = NULL;
    }
    m_flags = 0;
}

RVA(0x001770a0, 0x3a)
i32 DisplayUsesPalette() {
    HDC ic = CreateICA("DISPLAY", 0, 0, 0);
    if (ic) {
        i32 caps = GetDeviceCaps(ic, RASTERCAPS) & RC_PALETTE;
        DeleteDC(ic);
        return caps;
    }
    return 0;
}

RVA(0x001770e0, 0x7c)
void CImagePaletteNode::Tune() {
    ResetSystemPalette();
    HDC dc = CreateDCA("DISPLAY", 0, 0, 0);
    i32 sizePal = GetDeviceCaps(dc, SIZEPALETTE);
    i32 numReserved = GetDeviceCaps(dc, NUMRESERVED);
    i32 half = numReserved / 2;
    GetSystemPaletteEntries(dc, 0, half, m_pal.palPalEntry);
    GetSystemPaletteEntries(
        dc,
        sizePal - half,
        half,
        &m_pal.palPalEntry[m_pal.palNumEntries - half]
    );
    for (i32 i = half; i < sizePal - half; i++) {
        m_pal.palPalEntry[i].peFlags = 1;
    }
    DeleteDC(dc);
}

RVA(0x00177160, 0x81)
void ResetSystemPalette() {

    LogPal256 lp;
    HDC hdc = GetDC(0);
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
        HPALETTE old = SelectPalette(hdc, hpal, FALSE);
        RealizePalette(hdc);
        DeleteObject(SelectPalette(hdc, old, FALSE));
    }
    ReleaseDC(0, hdc);
}

RVA(0x001771f0, 0xe2)
i32 CImagePaletteNode::LoadPalFile(char* path, i32 arg) {
    CFile file;
    char rgb[PALETTE_RGB_BYTE_COUNT];

    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    if (file.GetLength() != PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    file.Read(rgb, PALETTE_RGB_BYTE_COUNT);
    return ProcessPal(rgb, arg);
}

// @early-stop
RVA(0x001772e0, 0x117)
i32 CImagePaletteNode::LoadPcxFile(char* path, i32 arg) {
    CFile file;
    u8 rgb[PALETTE_RGB_BYTE_COUNT];

    PALETTEENTRY rgbq[PALETTE_ENTRY_COUNT];

    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-PALETTE_RGB_BYTE_COUNT, 2);
    if (file.Read(rgb, PALETTE_RGB_BYTE_COUNT) == 0) {
        return 0;
    }

    u8* src = rgb;
    PALETTEENTRY* dst = rgbq;
    for (i32 i = 0x100; i != 0; i--) {
        dst->peRed = *src++;
        dst->peGreen = *src++;
        dst->peBlue = *src++;
        dst->peFlags = 0;
        dst++;
    }
    return Build(rgbq, arg);
}

// @early-stop
RVA(0x00177400, 0x76)
i32 CImagePaletteNode::ParsePaletteTail(void* buf, u32 size, i32 ctrl) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    if (size < PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    u8* s = static_cast<u8*>(buf) + size - PALETTE_RGB_BYTE_COUNT;
    PALETTEENTRY* d = pal;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        d->peRed = *s++;
        d->peGreen = *s++;
        d->peBlue = *s++;
        d->peFlags = 0;
        d++;
    }
    return Build(pal, ctrl);
}
