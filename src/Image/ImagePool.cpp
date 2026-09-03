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
#include <MakeRect.h>
#include <Pix16.h>
#include <Rez/RezMgr.h>

#include <string.h>

#define RGB_TO_16(entry)                                                                           \
    static_cast<u16>(                                                                              \
        ((static_cast<u16>((entry).peRed) >> 3) << 10)                                             \
        | ((static_cast<u16>((entry).peGreen) >> 3) << 5)                                          \
        | (static_cast<u16>((entry).peBlue) >> 3)                                                  \
    )

DATA(0x002bf6e0)
HINSTANCE CDibMgr::s_hInst = NULL;

DATA(0x0021aabc)
char g_bmpHeaderTemplate[4] = "BM";

RVA(0x00174e90, 0x1c)
i32 CDibMgr::Init(HINSTANCE instance, HWND window, u32 flags) {
    m_hInst = instance;
    m_hWnd = window;
    m_dwFlags = flags;
    return 1;
}

RVA(0x00174eb0, 0x1b)
void CDibMgr::Term() {
    RemoveAllDibs();
    RemoveAllPals();
    m_hInst = NULL;
    m_hWnd = NULL;
    m_dwFlags = 0;
}

RVA(0x00174ed0, 0x5d)
void CDibMgr::RemoveDib(CDib* dib) {
    if (dib == NULL) {
        return;
    }
    CDibPal* palette = dib->GetPalette();
    if (palette != NULL && dib->IsPaletteOwner()) {
        RemovePal(palette);
        dib->SetPalette(NULL, false);
    }
    POSITION pos = dib->GetPos();
    if (pos != NULL) {
        m_collDibs.RemoveAt(pos);
    }
    delete dib;
}

RVA(0x00174f30, 0x30)
void CDibMgr::RemovePal(CDibPal* palette) {
    if (palette == NULL) {
        return;
    }
    POSITION pos = palette->GetPos();
    if (pos != NULL) {
        m_collPals.RemoveAt(pos);
    }
    delete palette;
}

RVA(0x00174f60, 0x37)
void CDibMgr::RemoveAllDibs() {
    POSITION pos = m_collDibs.GetHeadPosition();
    while (pos) {
        CDib* item = static_cast<CDib*>(m_collDibs.GetNext(pos));
        delete item;
    }
    m_collDibs.RemoveAll();
}

RVA(0x00174fa0, 0x3e)
void CDibMgr::RemoveAllPals() {
    POSITION pos = m_collPals.GetHeadPosition();
    while (pos) {
        CDibPal* item = static_cast<CDibPal*>(m_collPals.GetNext(pos));
        delete item;
    }
    m_collPals.RemoveAll();
    m_pCurPal = NULL;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00174fe0, 0xfe)
CDib* CDibMgr::AddDib(i32 width, i32 height, ColorDepth depth, u32 flags) {
    HDC dc = GetDC(false);
    CDib* dib = new CDib();
    if (!dib->Init(dc, width, height, depth, flags)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    POSITION pos = m_collDibs.AddTail(dib);
    dib->SetPos(pos);
    ReleaseDC(dc);
    return dib;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001750e0, 0x103)
CDib* CDibMgr::AddDib(u8* bytes, i32 width, i32 height, ColorDepth depth, u32 flags) {
    HDC dc = GetDC(false);
    CDib* dib = new CDib();
    if (!dib->Init(bytes, dc, width, height, depth, flags)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    POSITION pos = m_collDibs.AddTail(dib);
    dib->SetPos(pos);
    ReleaseDC(dc);
    return dib;
}

RVA(0x001751f0, 0xf9)
CDib* CDibMgr::AddDib(u8* bytes, RezDecodeKind type, u32 flags) {
    HDC dc = GetDC(false);
    CDib* dib = new CDib();
    if (!dib->Init(bytes, type, dc, flags)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    POSITION pos = m_collDibs.AddTail(dib);
    dib->SetPos(pos);
    ReleaseDC(dc);
    return dib;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001752f0, 0xfc)
CDib* CDibMgr::AddDib(const char* file, u32 flags) {
    HDC dc = GetDC(false);
    CDibMgr::s_hInst = m_hInst;
    CDib* dib = new CDib();
    if (!dib->Init(file, dc, flags)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    POSITION pos = m_collDibs.AddTail(dib);
    dib->SetPos(pos);
    ReleaseDC(dc);
    return dib;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001753f0, 0xf4)
CDib* CDibMgr::AddDib(CDib* original, CDibPal* palette) {
    HDC dc = GetDC(false);
    CDib* dib = new CDib();
    if (!dib->Init(dc, original, palette)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    POSITION pos = m_collDibs.AddTail(dib);
    dib->SetPos(pos);
    ReleaseDC(dc);
    return dib;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001754f0, 0x7b)
CDibPal* CDibMgr::AddPal(PALETTEENTRY* entries, u32 flags) {
    CDibPal* palette = new CDibPal();
    if (!palette->Init(entries, flags)) {
        delete palette;
        return NULL;
    }
    POSITION pos = m_collPals.AddTail(palette);
    palette->SetPos(pos);
    return palette;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00175570, 0x7b)
CDibPal* CDibMgr::AddPal(u8* rgb, u32 flags) {
    CDibPal* palette = new CDibPal();
    if (!palette->Init(rgb, flags)) {
        delete palette;
        return NULL;
    }
    POSITION pos = m_collPals.AddTail(palette);
    palette->SetPos(pos);
    return palette;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001755f0, 0x82)
CDibPal* CDibMgr::AddPal(const char* file, u32 flags) {
    CDibMgr::s_hInst = m_hInst;
    CDibPal* palette = new CDibPal();
    if (!palette->Init(file, flags)) {
        delete palette;
        return NULL;
    }
    POSITION pos = m_collPals.AddTail(palette);
    palette->SetPos(pos);
    return palette;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00175680, 0x85)
CDibPal* CDibMgr::AddPal(u8* data, u32 dataSize, RezDecodeKind type, u32 flags) {
    CDibPal* palette = new CDibPal();
    if (!palette->Init(data, dataSize, type, flags)) {
        delete palette;
        return NULL;
    }
    POSITION pos = m_collPals.AddTail(palette);
    palette->SetPos(pos);
    return palette;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00175710, 0x69)
i32 CDibMgr::ResizeDib(CDib* dib, i32 width, i32 height, ColorDepth depth, u32 flags) {
    if (dib == NULL) {
        return 0;
    }
    HDC dc = GetDC(false);
    i32 result = dib->Resize(dc, width, height, depth, flags);
    ReleaseDC(dc);
    return result;
}

RVA(0x00175780, 0x3f)
void CDibMgr::SetPalette(CDib* dib, CDibPal* palette, b32 owner) {
    CDibPal* oldPalette = dib->GetPalette();
    if (oldPalette != NULL && dib->IsPaletteOwner()) {
        RemovePal(oldPalette);
        dib->SetPalette(NULL, false);
    }
    dib->SetPalette(palette, owner);
}

RVA(0x001757c0, 0x16f)
i32 CDib::Init(HDC dc, i32 width, i32 height, ColorDepth bitcount, u32 ctrl) {
    m_dwFlags = 0;
    m_nWidth = width;
    m_nHeight = (height < 0) ? -height : height;
    m_nDepth = bitcount;
    if (bitcount == BPP_PALETTED_8) {
        m_nPitch = ((width + 3) / 4) * 4;
    } else {
        m_nPitch = width;
    }
    m_nStride = m_nPitch - width;
    m_bPalOwner = 0;
    m_pPal = NULL;
    m_bTransparent = true;
    memset(&m_bmi.hdr, 0, sizeof(BITMAPINFOHEADER));
    m_bmi.hdr.biWidth = m_nWidth;
    m_bmi.hdr.biBitCount = static_cast<WORD>(IDX(m_nDepth));
    m_bmi.hdr.biSize = sizeof(BITMAPINFOHEADER);
    m_bmi.hdr.biHeight = height;
    m_bmi.hdr.biPlanes = 1;
    m_bmi.hdr.biCompression = BI_RGB;
    m_bmi.hdr.biSizeImage = 0;
    m_bmi.hdr.biClrUsed = 0;
    m_bmi.hdr.biClrImportant = 0;

    u16* pal = static_cast<u16*>(static_cast<void*>(m_bmi.colors));
    if (m_nDepth == BPP_PALETTED_8) {
        for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
            *pal++ = static_cast<u16>(i);
        }
        m_hBmp = CreateDIBSection(
            dc,
            static_cast<BITMAPINFO*>(static_cast<void*>(&m_bmi)),
            DIB_PAL_COLORS,
            PtrOut(&m_pBytes),
            NULL,
            0
        );
    } else {
        m_hBmp = CreateDIBSection(
            dc,
            static_cast<BITMAPINFO*>(static_cast<void*>(&m_bmi)),
            DIB_RGB_COLORS,
            PtrOut(&m_pBytes),
            NULL,
            0
        );
    }
    if (!m_hBmp) {
        return 0;
    }
    m_pLines = new u32[m_nHeight];
    for (i32 i = 0; i < m_nHeight; i++) {
        m_pLines[i] = (m_nHeight - i - 1) * (IDX(m_nDepth) / 8) * m_nPitch;
    }
    return 1;
}

// @early-stop
RVA(0x00175930, 0xc6)
i32 CDib::Init(u8* src, HDC dc, i32 width, i32 height, ColorDepth bitcount, u32 ctrl) {
    if (!Init(dc, width, height, bitcount, ctrl)) {
        return 0;
    }
    if (IsStrideless()) {
        memcpy(m_pBytes, src, (GetBufferSize() * IDX(bitcount)) / 8);
    } else {
        for (i32 row = 0; row < GetHeight(); row++) {
            memcpy(&m_pBytes[GetIndex(row)], src, GetWidth());
            src += GetWidth();
        }
    }
    return 1;
}

RVA(0x00175a00, 0x90)
i32 CDib::Init(u8* buf, RezDecodeKind kind, HDC dc, u32 ctrl) {
    switch (kind) {
        case DECODE_PCX:
            return InitPcx(buf, dc, ctrl);
        case DECODE_BMP:
            return InitBmp(buf, dc, ctrl);
        case DECODE_RID:
            return InitRid(buf, dc, ctrl);
        case DECODE_PID:
            return InitPid(buf, dc, ctrl);
    }
    return 0;
}

RVA(0x00175a90, 0xee)
i32 CDib::Init(const char* name, HDC dc, u32 ctrl) {
    const char* ext = strrchr(name, '.');

    if (ext && _strcmpi(ext, ".BMP") == 0) {
        return InitBmp(name, dc, ctrl);
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        return InitPcx(name, dc, ctrl);
    } else if (ext && _strcmpi(ext, ".RID") == 0) {
        return InitRid(name, dc, ctrl);
    } else if (ext && _strcmpi(ext, ".PID") == 0) {
        return InitPid(name, dc, ctrl);
    }

    return InitRes(name, dc, ctrl);
}

RVA(0x00175b80, 0x105)
i32 CDib::Init(HDC dc, CDib* src, CDibPal* pal) {
    u8* srcBuf;
    u16* destBuf;
    i32 x;
    i32 y;
    PALETTEENTRY* entries;
    PALETTEENTRY entry;

    if (pal == NULL) {
        return 0;
    }
    entries = pal->GetPes();
    if (entries == NULL) {
        return 0;
    }
    if (!Init(dc, src->GetWidth(), src->GetHeight(), BPP_RGB_16, 0)) {
        return 0;
    }

    for (y = 0; y < GetHeight(); y++) {
        srcBuf = &src->GetBytes()[y * src->GetPitch()];
        destBuf = &GetBuf16()[y * GetPitch()];

        for (x = 0; x < GetWidth(); x++) {
            entry = entries[*srcBuf];
            *destBuf = RGB_TO_16(entry);
            srcBuf++;
            destBuf++;
        }
    }
    return 1;
}

RVA(0x00175c90, 0x45)
void CDib::Term() {
    if (m_hBmp) {
        DeleteObject(m_hBmp);
        m_hBmp = NULL;
    }
    if (m_pLines) {
        delete[] m_pLines;
        m_pLines = NULL;
    }
    m_pBytes = NULL;
    m_pPal = NULL;
}

RVA(0x00175ce0, 0x6b)
i32 CDib::Resize(HDC dc, i32 w, i32 h, ColorDepth bitCount, u32 flag) {
    if (m_hBmp && m_pBytes && m_pLines && m_nWidth == w && m_nHeight == h) {
        return 1;
    }
    Term();
    return Init(dc, w, h, bitCount, flag);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00175d50, 0xad)
void CDib::Fill(u8 value) {
    if (m_nStride == 0) {
        i32 fill = value & PIXEL_BYTE_MASK;
        memset(m_pBytes, fill, m_nPitch * m_nHeight);
    } else {

        i32 y = 0;
        if (y < m_nHeight) {
            i32 fill = value & PIXEL_BYTE_MASK;
            do {
                memset(m_pBytes + m_pLines[y], fill, m_nWidth);
                y++;
            } while (y < m_nHeight);
        }
    }
}

// @early-stop
RVA(0x00175e00, 0x3d)
i32 CDib::InitBmp(u8* buf, HDC dc, u32 ctrl) {
    BITMAPINFOHEADER* ih = static_cast<BITMAPINFOHEADER*>(static_cast<void*>(buf));
    CSize imageSize(ih->biWidth, ih->biHeight);
    ColorDepth bitcount = static_cast<ColorDepth>(ih->biBitCount);
    RecordBytes<BITMAPINFOHEADER> data;
    data.m_rec = ih;
    u8* src = data.m_bytes + sizeof(BITMAPINFOHEADER) + 4;
    if (bitcount == BPP_PALETTED_8) {
        src = data.m_bytes + ih->biSize + sizeof(RGBQUAD) * PALETTE_ENTRY_COUNT;
    }
    i32 r = Init(src, dc, imageSize.cx, imageSize.cy, bitcount, ctrl);
    return r;
}

RVA(0x00175e40, 0x1b3)
i32 CDib::InitBmp(const char* name, HDC dc, u32 ctrl) {
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

    CSize imageSize(ih.biWidth, ih.biHeight);
    ColorDepth bitcount = static_cast<ColorDepth>(ih.biBitCount & 0xffff);
    if (!Init(dc, imageSize.cx, imageSize.cy, bitcount, ctrl)) {
        return 0;
    }

    file.Seek(fh.bfOffBits, 0);
    u32 size = (IDX(bitcount) / 8) * m_nPitch * imageSize.cy;
    if (file.Read(m_pBytes, size) != size) {
        return 0;
    }
    return 1;
}

// @early-stop
RVA(0x00176000, 0x18f)
i32 CDib::InitPcx(u8* buf, HDC dc, u32 ctrl) {
    u8* pStart = buf;
    PcxHeader* hdr = static_cast<PcxHeader*>(static_cast<void*>(pStart));
    CSize imageSize(hdr->m_xMax - hdr->m_xMin + 1, hdr->m_yMax - hdr->m_yMin + 1);
    if (hdr->m_bitsPerPixel != PCX_BITS_PER_PLANE_8) {
        return 0;
    }
    if (!Init(
            dc,
            imageSize.cx,
            imageSize.cy,
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

    scan = new u8[(imageSize.cx * IDX(hdr->m_bitsPerPixel) * IDX(hdr->m_planes)) / 8];

    for (y = 0; y < imageSize.cy; y++) {
        dst = m_pBytes + m_pLines[y];
        remaining = imageSize.cx * IDX(hdr->m_planes);

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
            for (i = imageSize.cx; i != 0; i--) {
                *dst++ = scan[i - 1];
            }
        } else if (hdr->m_planes == PCX_PLANES_RGB) {
            for (i = imageSize.cx; i != 0; i--) {
                *dst++ = scan[i - 1];
                *dst++ = scan[imageSize.cx + i - 1];
                *dst++ = scan[2 * imageSize.cx + i - 1];
            }
        }
    }

    delete[] scan;
    return 1;
}

RVA(0x00176190, 0x126)
i32 CDib::InitPcx(const char* name, HDC dc, u32 ctrl) {
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
    i32 result = InitPcx(buf, dc, ctrl);
    delete[] buf;
    return result;
}

RVA(0x001762c0, 0x42)
i32 CDib::InitRid(u8* buf, HDC dc, u32 ctrl) {
    RecordBytes<PidHeader> p;
    p.m_bytes = static_cast<u8*>(buf);
    p.m_bytes += 2 * sizeof(u32);
    SIZE
    imageSize;
    imageSize.cx = *p.m_dwords;
    p.m_bytes += sizeof(u32);
    imageSize.cy = *p.m_dwords;
    p.m_bytes += sizeof(u32);
    p.m_bytes += 4 * sizeof(u32);
    i32 ok = Init(p.m_bytes, dc, imageSize.cx, imageSize.cy, BPP_PALETTED_8, ctrl);
    if (!(ctrl & IDX(DIB_INIT_KEEP_TRANSPARENCY))) {
        m_bTransparent = false;
    }
    return ok;
}

RVA(0x00176310, 0x126)
i32 CDib::InitRid(const char* name, HDC dc, u32 ctrl) {
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
    i32 result = InitRid(buf, dc, ctrl);
    delete[] buf;
    return result;
}

RVA(0x00176440, 0x25d)
i32 CDib::InitPid(u8* buf, HDC dc, u32 ctrl) {
    PidHeader* header = static_cast<PidHeader*>(static_cast<void*>(buf));
    u32* dword = &header->formatTag;
    u32 formatTag = *dword++;
    PidFlags flags = static_cast<PidFlags>(*dword++);
    SIZE
    imageSize;
    imageSize.cx = *dword++;
    imageSize.cy = *dword++;
    POINT offset;
    offset.x = *dword++;
    offset.y = *dword++;
    u32 fill = *dword++;
    u32 reserved = *dword++;

    if (!Init(dc, imageSize.cx, imageSize.cy, BPP_PALETTED_8, ctrl)) {
        return 0;
    }
    if (!(ctrl & IDX(DIB_INIT_KEEP_TRANSPARENCY))) {
        m_bTransparent = false;
    }

    u8* packed = static_cast<u8*>(static_cast<void*>(dword));

    i32 transparentIndex;
    if (HAS(flags, PID_FILL_IS_WORD)) {
        transparentIndex = fill & PIXEL16_VALUE_MASK;
    } else {
        transparentIndex = 0;
    }

    if (HAS(flags, PID_GRAMMAR_SKIPRUN)) {
        m_bTransparent = true;
        CPoint output(0, 0);
        u32 offset = 0;
        u8* dst = m_pBytes + m_pLines[output.y];

        while (output.y < m_nHeight) {
            if (packed[offset] & PID_SKIP_RUN_MARKER) {
                memset(dst + output.x, transparentIndex, packed[offset] - PID_SKIP_RUN_MARKER);
                output.x += packed[offset] - PID_SKIP_RUN_MARKER;
                offset++;
            } else {
                memcpy(dst + output.x, packed + offset + 1, packed[offset]);
                output.x += packed[offset];
                offset += packed[offset] + 1;
            }

            if (output.x >= m_nWidth) {
                output.y++;
                output.x = 0;
                if (output.y < m_nHeight) {
                    dst = m_pBytes + m_pLines[output.y];
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

        for (y = 0; y < static_cast<u32>(imageSize.cy); y++) {
            dst = m_pBytes + m_pLines[y];
            n = imageSize.cx;

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
i32 CDib::InitPid(const char* name, HDC dc, u32 ctrl) {
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
    i32 result = InitPid(buf, dc, ctrl);
    delete[] buf;
    return result;
}

RVA(0x001767d0, 0x64)
i32 CDib::InitRes(const char* name, HDC dc, u32 ctrl) {
    HINSTANCE hModule = CDibMgr::GetGlobalInstanceHandle();
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
    u8* data = static_cast<u8*>(LockResource(hGlobal));
    if (!data) {
        return 0;
    }
    return InitBmp(data, dc, ctrl);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176840, 0x11f)
void CDib::Invert() {
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
            scratch[j] = m_pBytes[k++];
        }

        source = (height - 1 - i) * width;
        destination = i * width;
        for (j = 0; j < width; j++) {
            m_pBytes[destination++] = m_pBytes[source++];
        }

        destination = (height - 1 - i) * width;
        for (j = 0; j < width; j++) {
            m_pBytes[destination++] = scratch[j];
        }
    }

    delete[] scratch;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176960, 0x168)
i32 CDib::Blt(CDib* src, i32 x, i32 y) {
    i32 h = src->m_nHeight;
    i32 w = src->m_nWidth;
    i32 dstW = m_nWidth;
    i32 dstH = m_nHeight;
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

    if (src->m_bTransparent) {
        for (i32 row = 0; row < h; row++) {
            u8* d = m_pBytes + m_pLines[y + row] + x;
            u8* s = src->m_pBytes + src->m_pLines[row];
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
            u8* s = src->m_pBytes + src->m_pLines[row];
            u8* d = m_pBytes + m_pLines[y + row] + x;
            memcpy(d, s, w);
        }
    }
    return h;
}

RVA(0x00176ad0, 0x17)
void CDib::SetPalette(CDibPal* palette, b32 owner) {

    m_pPal = palette;
    m_bPalOwner = owner;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176af0, 0x5)
i32 CDib::Scale(i32 newWidth, i32 newHeight, i32 newDepth, u32 flags) {
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176b00, 0x2c)
i32 CDib::Save(const char* filename, CDibPal* paletteObj) {
    switch (m_nDepth) {
        case BPP_PALETTED_8:
            return Save8(filename, paletteObj);
        case BPP_RGB_16:
            return 0;
        case BPP_RGB_24:
            return 0;
    }
    return 0;
}

RVA(0x00176b30, 0x1e5)
i32 CDib::Save8(const char* filename, CDibPal* paletteObj) {
    ASSERT(IsValid());
    ASSERT(filename);

    if (paletteObj == NULL) {
        paletteObj = m_pPal;
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

    PALETTEENTRY* pal = paletteObj->GetPes();
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
void CDib::FillRect(RECT* r, u32 color) {
    i32 width = r->right - r->left;
    for (i32 y = r->top; y <= r->bottom; ++y) {
        i32 off = m_pLines[y] + r->left;
        memset(m_pBytes + off, color, width);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176da0, 0x4b)
void CDib::FillRect(i32 dx, i32 dy, RECT* src, u32 color) {
    RECT r;
    r = MakeRect(dx, dy, src->right + dx - src->left, src->bottom - src->top + dy);
    FillRect(&r, color);
}

RVA(0x00176df0, 0x71)
i32 CDibPal::Init(PALETTEENTRY* entries, u32 flags) {
    m_dwFlags = flags;
    m_logPal.numEntries = 0x100;
    m_logPal.version = LOGICAL_PALETTE_VERSION;
    for (i32 i = 0; i < 0x100; i++) {
        m_logPal.entries[i] = entries[i];
        m_logPal.entries[i].peFlags = 0;
    }
    if (CDibPal::IsPaletteDevice() && !(flags & IDX(DMPF_NOIDENTITY))) {
        MakeIdentity();
        m_bIdentity = true;
    }
    m_hPal = CreatePalette(static_cast<LOGPALETTE*>(static_cast<void*>(&m_logPal)));
    return m_hPal != NULL;
}

RVA(0x00176e70, 0x4e)
i32 CDibPal::Init(u8* rgb, u32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    u8* s = rgb;

    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        pal[i].peRed = *s++;
        pal[i].peGreen = *s++;
        pal[i].peBlue = *s++;
    }
    return Init(pal, flags);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176ec0, 0x64)
i32 CDibPal::Init(RGBQUAD* quads, u32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        pal[i].peRed = quads[i].rgbRed;
        pal[i].peGreen = quads[i].rgbGreen;
        pal[i].peBlue = quads[i].rgbBlue;
    }
    return Init(pal, flags);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00176f30, 0x51)
i32 CDibPal::Init(RGBTRIPLE* triples, u32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        pal[i].peRed = triples[i].rgbtRed;
        pal[i].peGreen = triples[i].rgbtGreen;
        pal[i].peBlue = triples[i].rgbtBlue;
    }
    return Init(pal, flags);
}

RVA(0x00176f90, 0xa4)
i32 CDibPal::Init(const char* path, u32 flags) {
    const char* ext = strrchr(path, '.');

    if (ext && _strcmpi(ext, ".BMP") == 0) {
        return InitBmp(path, flags);
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        return InitPcx(path, flags);
    } else if (ext && _strcmpi(ext, ".PAL") == 0) {
        return InitPal(path, flags);
    }

    return InitRes(path, flags);
}

RVA(0x00177040, 0x23)
i32 CDibPal::Init(u8* data, u32 dataSize, RezDecodeKind type, u32 flags) {
    if (type == DECODE_PCX) {
        return InitPcx(data, dataSize, flags);
    }
    return 0;
}

RVA(0x00177070, 0x22)
void CDibPal::Term() {
    if (m_hPal) {
        DeleteObject(m_hPal);
        m_hPal = NULL;
    }
    m_dwFlags = 0;
}

RVA(0x001770a0, 0x3a)
i32 CDibPal::IsPaletteDevice() {
    HDC ic = CreateICA("DISPLAY", NULL, NULL, NULL);
    if (ic) {
        i32 caps = GetDeviceCaps(ic, RASTERCAPS) & RC_PALETTE;
        DeleteDC(ic);
        return caps;
    }
    return 0;
}

RVA(0x001770e0, 0x7c)
void CDibPal::MakeIdentity() {
    CDibPal::ClearSystemPalette();
    HDC dc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    i32 sizePal = GetDeviceCaps(dc, SIZEPALETTE);
    i32 numReserved = GetDeviceCaps(dc, NUMRESERVED);
    i32 half = numReserved / 2;
    GetSystemPaletteEntries(dc, 0, half, m_logPal.entries);
    GetSystemPaletteEntries(
        dc,
        sizePal - half,
        half,
        &m_logPal.entries[m_logPal.numEntries - half]
    );
    for (i32 i = half; i < sizePal - half; i++) {
        m_logPal.entries[i].peFlags = PC_RESERVED;
    }
    DeleteDC(dc);
}

RVA(0x00177160, 0x81)
void CDibPal::ClearSystemPalette() {

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
i32 CDibPal::InitPal(const char* path, u32 flags) {
    CFile file;
    u8 rgb[PALETTE_RGB_BYTE_COUNT];

    if (!file.Open(path, 0, NULL)) {
        return 0;
    }
    if (file.GetLength() != PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    file.Read(rgb, PALETTE_RGB_BYTE_COUNT);
    return Init(rgb, flags);
}

RVA(0x001772e0, 0x117)
i32 CDibPal::InitPcx(const char* path, u32 flags) {
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
    return Init(rgbq, flags);
}

RVA(0x00177400, 0x76)
i32 CDibPal::InitPcx(u8* data, u32 dataSize, u32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    if (dataSize < PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    u8* s = data + dataSize - PALETTE_RGB_BYTE_COUNT;
    COPY_RGB_PALETTE(pal, s, i, PALETTE_ENTRY_COUNT)
    return Init(pal, flags);
}
