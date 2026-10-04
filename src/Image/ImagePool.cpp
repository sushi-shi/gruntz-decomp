#include <StdAfx.h>
#include <Io/File.h>

#include <Ints.h>

#include <Image/ImagePool.h>

#include <ComOutRef.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DirPal.h>
#include <DDrawMgr/PaletteSize.h>
#include <DDrawMgr/PixelFormatMacros.h>
#include <DDrawMgr/PixelShift.h>
#include <Enums.h>
#include <Image/RasterData.h>
#include <Image/FileImageRecords.h>
#include <Image/Image.h>
#include <Image/ImagePaletteNode.h>
#include <Image/RezDecodeKind.h>
#include <Pix16.h>
#include <RectMacros.h>
#include <Rez/RezMgr.h>
#include <SafeDelete.h>

#include <string.h>

HINSTANCE CDibMgr::s_hInst = NULL;

char g_bmpHeaderTemplate[4] = "BM";

i32 CDibMgr::Init(HINSTANCE instance, HWND window, u32 flags) {
    m_hInst = instance;
    m_hWnd = window;
    m_dwFlags = flags;
    return 1;
}

void CDibMgr::Term() {
    RemoveAllDibs();
    RemoveAllPals();
    m_hInst = NULL;
    m_hWnd = NULL;
    m_dwFlags = 0;
}

void CDibMgr::RemoveDib(CDib* dib) {
    if (dib == NULL) {
        return;
    }
    CDibPal* palette = dib->GetPalette();
    if (palette != NULL && dib->IsPaletteOwner()) {
        RemovePal(palette);
        SetPalette(NULL, FALSE);
    }
    std::list<CDib*>::iterator pos = std::find(m_collDibs.begin(), m_collDibs.end(), dib);
    if (pos != m_collDibs.end()) {
        m_collDibs.erase(pos);
    }
    delete dib;
}

void CDibMgr::RemovePal(CDibPal* palette) {
    if (palette == NULL) {
        return;
    }
    std::list<CDibPal*>::iterator pos = std::find(m_collPals.begin(), m_collPals.end(), palette);
    if (pos != m_collPals.end()) {
        m_collPals.erase(pos);
    }
    delete palette;
}

void CDibMgr::RemoveAllDibs() {
    std::list<CDib*>::iterator pos = m_collDibs.begin();
    while (pos != m_collDibs.end()) {
        CDib* item = static_cast<CDib*>(*(pos++));
        delete item;
    }
    m_collDibs.clear();
}

void CDibMgr::RemoveAllPals() {
    std::list<CDibPal*>::iterator pos = m_collPals.begin();
    while (pos != m_collPals.end()) {
        CDibPal* item = static_cast<CDibPal*>(*(pos++));
        delete item;
    }
    m_collPals.clear();
    m_pCurPal = NULL;
}

CDib* CDibMgr::AddDib(i32 width, i32 height, ColorDepth depth, u32 flags) {
    HDC dc = GetDC(false);
    CDib* dib = new CDib();
    if (!dib->Init(dc, width, height, depth, flags)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    m_collDibs.push_back(dib);
    ReleaseDC(dc);
    return dib;
}

CDib* CDibMgr::AddDib(u8* bytes, i32 width, i32 height, ColorDepth depth, u32 flags) {
    HDC dc = GetDC(false);
    CDib* dib = new CDib();
    if (!dib->Init(bytes, dc, width, height, depth, flags)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    m_collDibs.push_back(dib);
    ReleaseDC(dc);
    return dib;
}

CDib* CDibMgr::AddDib(u8* bytes, u32 dataSize, RezDecodeKind type, u32 flags) {
    HDC dc = GetDC(false);
    CDib* dib = new CDib();
    if (!dib->Init(bytes, dataSize, type, dc, flags)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    m_collDibs.push_back(dib);
    ReleaseDC(dc);
    return dib;
}

CDib* CDibMgr::AddDib(const char* file, u32 flags) {
    HDC dc = GetDC(false);
    CDibMgr::s_hInst = m_hInst;
    CDib* dib = new CDib();
    if (!dib->Init(file, dc, flags)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    m_collDibs.push_back(dib);
    ReleaseDC(dc);
    return dib;
}

CDib* CDibMgr::AddDib(CDib* original, CDibPal* palette) {
    HDC dc = GetDC(false);
    CDib* dib = new CDib();
    if (!dib->Init(dc, original, palette)) {
        ReleaseDC(dc);
        delete dib;
        return NULL;
    }
    m_collDibs.push_back(dib);
    ReleaseDC(dc);
    return dib;
}

CDibPal* CDibMgr::AddPal(PALETTEENTRY* entries, u32 flags) {
    CDibPal* palette = new CDibPal();
    if (!palette->Init(entries, flags)) {
        delete palette;
        return NULL;
    }
    m_collPals.push_back(palette);
    return palette;
}

CDibPal* CDibMgr::AddPal(u8* rgb, u32 flags) {
    CDibPal* palette = new CDibPal();
    if (!palette->Init(rgb, flags)) {
        delete palette;
        return NULL;
    }
    m_collPals.push_back(palette);
    return palette;
}

CDibPal* CDibMgr::AddPal(const char* file, u32 flags) {
    CDibMgr::s_hInst = m_hInst;
    CDibPal* palette = new CDibPal();
    if (!palette->Init(file, flags)) {
        delete palette;
        return NULL;
    }
    m_collPals.push_back(palette);
    return palette;
}

CDibPal* CDibMgr::AddPal(u8* data, u32 dataSize, RezDecodeKind type, u32 flags) {
    CDibPal* palette = new CDibPal();
    if (!palette->Init(data, dataSize, type, flags)) {
        delete palette;
        return NULL;
    }
    m_collPals.push_back(palette);
    return palette;
}

i32 CDibMgr::ResizeDib(CDib* dib, i32 width, i32 height, ColorDepth depth, u32 flags) {
    if (dib == NULL) {
        return 0;
    }
    HDC dc = GetDC(false);
    i32 result = dib->Resize(dc, width, height, depth, flags);
    ReleaseDC(dc);
    return result;
}

void CDibMgr::SetPalette(CDib* dib, CDibPal* palette, b32 owner) {
    CDibPal* oldPalette = dib->GetPalette();
    if (oldPalette != NULL && dib->IsPaletteOwner()) {
        RemovePal(oldPalette);
        dib->SetPalette(NULL, false);
    }
    dib->SetPalette(palette, owner);
}

i32 CDib::Init(u8* buf, u32 dataSize, RezDecodeKind kind, HDC dc, u32 ctrl) {
    switch (kind) {
        case DECODE_PCX:
            return InitPcx(buf, dataSize, dc, ctrl);
        case DECODE_BMP:
            return InitBmp(buf, dc, ctrl);
        case DECODE_RID:
            return InitRid(buf, dc, ctrl);
        case DECODE_PID:
            return InitPid(buf, dataSize, dc, ctrl);
    }
    return 0;
}

i32 CDib::Init(const char* name, HDC dc, u32 ctrl) {
    const char* ext = strrchr(name, '.');

    if (ext && stricmp(ext, ".BMP") == 0) {
        return InitBmp(name, dc, ctrl);
    } else if (ext && stricmp(ext, ".PCX") == 0) {
        return InitPcx(name, dc, ctrl);
    } else if (ext && stricmp(ext, ".RID") == 0) {
        return InitRid(name, dc, ctrl);
    } else if (ext && stricmp(ext, ".PID") == 0) {
        return InitPid(name, dc, ctrl);
    }

    return InitRes(name, dc, ctrl);
}



i32 CDib::Resize(HDC dc, i32 w, i32 h, ColorDepth bitCount, u32 flag) {
    if (m_hBmp && m_pBytes && m_pLines && m_nWidth == w && m_nHeight == h) {
        return 1;
    }
    Term();
    return Init(dc, w, h, bitCount, flag);
}

void CDib::Fill(u8 value) {
    if (m_nStride == 0) {
        i32 fill = value & PIXEL_BYTE_MASK;
        memset(m_pBytes, fill, m_nPitch * m_nHeight);
    } else {

        i32 y = 0;
        if (y < m_nHeight) {
            i32 fill = value & PIXEL_BYTE_MASK;
            do {
                memset(m_pBytes + m_pLines[y], fill, m_nWidth * (IDX(m_nDepth) / 8));
                y++;
            } while (y < m_nHeight);
        }
    }
}

i32 CDib::InitBmp(u8* buf, HDC dc, u32 ctrl) {
    BITMAPINFOHEADER* ih = static_cast<BITMAPINFOHEADER*>(static_cast<void*>(buf));
    i32 width = ih->biWidth;
    i32 height = ih->biHeight;
    ColorDepth bitcount = static_cast<ColorDepth>(ih->biBitCount);
    RecordBytes<BITMAPINFOHEADER> data;
    data.m_rec = ih;
    u8* src = data.m_bytes + sizeof(BITMAPINFOHEADER) + 4;
    if (bitcount == BPP_PALETTED_8) {
        src = data.m_bytes + ih->biSize + sizeof(RGBQUAD) * PALETTE_ENTRY_COUNT;
    }
    if (!Init(dc, width, height, bitcount, ctrl)) return 0;
    memcpy(m_pBytes, src, GetBufferSize());
    return 1;
}

i32 CDib::InitBmp(const char* name, HDC dc, u32 ctrl) {
    io::File file;
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;

    if (!file.open(name, io::ReadOnly)) {
        return 0;
    }
    if (file.read(&fh, sizeof(fh)) != sizeof(fh)) {
        return 0;
    }
    if (file.read(&ih, sizeof(ih)) != sizeof(ih)) {
        return 0;
    }

    i32 height = ih.biHeight;
    i32 width = ih.biWidth;
    ColorDepth bitcount = static_cast<ColorDepth>(LOWORD(ih.biBitCount));
    if (!Init(dc, width, height, bitcount, ctrl)) {
        return 0;
    }

    file.seek(fh.bfOffBits, io::Start);
    u8* bytes = GetBytes();
    u32 size = GetBufferSize();
    if (file.read(bytes, size) != size) {
        return 0;
    }
    return 1;
}

i32 CDib::InitPcx(const char* name, HDC dc, u32 ctrl) {
    io::File file;

    if (!file.open(name, io::ReadOnly)) {
        return 0;
    }
    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    if (len == 0) {
        return 0;
    }
    u8* buf = new u8[len];
    if (!buf) {
        return 0;
    }
    if (file.read(buf, len) != len) { delete[] buf; return 0; }
    i32 result = InitPcx(buf, len, dc, ctrl);
    delete[] buf;
    return result;
}

i32 CDib::InitRid(u8* buf, HDC dc, u32 ctrl) {
    RecordBytes<PidHeader> p;
    p.m_bytes = static_cast<u8*>(buf);
    p.m_bytes += 2 * sizeof(u32);
    i32 width = *p.m_dwords;
    p.m_bytes += sizeof(u32);
    i32 height = *p.m_dwords;
    p.m_bytes += sizeof(u32);
    p.m_bytes += 4 * sizeof(u32);
    // Preserve the legacy RID orientation until its wire layout has a dedicated decoder.
    const RasterRowOrder rowOrder = (width & 3) ? RASTER_ROWS_TOP_DOWN : RASTER_ROWS_BOTTOM_UP;
    i32 ok = Init(p.m_bytes, dc, width, height, BPP_PALETTED_8, ctrl, rowOrder);
    if (!(ctrl & 1)) {
        m_bTransparent = false;
    }
    return ok;
}

i32 CDib::InitRid(const char* name, HDC dc, u32 ctrl) {
    io::File file;

    if (!file.open(name, io::ReadOnly)) {
        return 0;
    }
    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    if (len == 0) {
        return 0;
    }
    u8* buf = new u8[len];
    if (!buf) {
        return 0;
    }
    if (file.read(buf, len) != len) { delete[] buf; return 0; }
    i32 result = InitRid(buf, dc, ctrl);
    delete[] buf;
    return result;
}

i32 CDib::InitPid(const char* name, HDC dc, u32 ctrl) {
    io::File file;

    if (!file.open(name, io::ReadOnly)) {
        return 0;
    }
    u32 len = file.size();
    if (!file.good() || len == 0) return 0;
    if (len == 0) {
        return 0;
    }
    u8* buf = new u8[len];
    if (!buf) {
        return 0;
    }
    if (file.read(buf, len) != len) { delete[] buf; return 0; }
    i32 result = InitPid(buf, len, dc, ctrl);
    delete[] buf;
    return result;
}

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

void CDib::SetPalette(CDibPal* palette, b32 owner) {

    m_pPal = palette;
    m_bPalOwner = owner;
}

i32 CDib::Scale(i32 newWidth, i32 newHeight, i32 newDepth, u32 flags) {
    return 0;
}

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
    info.m_bmiHeader.biSize = sizeof(info.m_bmiHeader);
    info.m_bmiHeader.biWidth = GetWidth();
    info.m_bmiHeader.biHeight = GetHeight();
    info.m_bmiHeader.biPlanes = 1;
    info.m_bmiHeader.biBitCount = 8;
    info.m_bmiHeader.biCompression = BI_RGB;
    info.m_bmiHeader.biSizeImage = 0;

    PALETTEENTRY* pal = paletteObj->GetPes();
    if (pal == NULL) {
        return 0;
    }

    for (i32 i = 0; i < 0x100; i++) {
        info.m_bmiColors[i].rgbRed = pal[i].peRed;
        info.m_bmiColors[i].rgbGreen = pal[i].peGreen;
        info.m_bmiColors[i].rgbBlue = pal[i].peBlue;
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

    io::File file;
    if (!file.open(filename, io::Replace)) {
        return 0;
    }
    file.write(&fileHdr.m_hdr, sizeof(fileHdr.m_hdr));
    file.write(&info, sizeof(info));
    for (i32 row = GetHeight() - 1; row >= 0; row--) {
        u32 index = GetIndex(row);
        file.write(&pixels[index], GetWidth());
    }
    return file.finish();
}

void CDib::FillRect(RECT* r, u32 color) {
    i32 width = r->right - r->left;
    for (i32 y = r->top; y <= r->bottom; ++y) {
        i32 off = m_pLines[y] + r->left;
        memset(m_pBytes + off, color, width);
    }
}

void CDib::FillRect(i32 dx, i32 dy, RECT* src, u32 color) {
    RECT r;
    SET_RECT_COMPONENTS(r, dx, dy, src->right + dx - src->left, src->bottom - src->top + dy);
    FillRect(&r, color);
}

i32 CDibPal::Init(PALETTEENTRY* entries, u32 flags) {
    m_dwFlags = flags;
    m_logPal.m_numEntries = 0x100;
    m_logPal.m_version = LOGICAL_PALETTE_VERSION;
    for (i32 i = 0; i < 0x100; i++) {
        m_logPal.m_entries[i] = entries[i];
        m_logPal.m_entries[i].peFlags = 0;
    }
    if (CDibPal::IsPaletteDevice() && !(flags & IDX(DMPF_NOIDENTITY))) {
        MakeIdentity();
        m_bIdentity = true;
    }
    m_hPal = CreatePalette(static_cast<LOGPALETTE*>(static_cast<void*>(&m_logPal)));
    return m_hPal != NULL;
}

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

i32 CDibPal::Init(RGBQUAD* quads, u32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        pal[i].peRed = quads[i].rgbRed;
        pal[i].peGreen = quads[i].rgbGreen;
        pal[i].peBlue = quads[i].rgbBlue;
    }
    return Init(pal, flags);
}

i32 CDibPal::Init(RGBTRIPLE* triples, u32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        pal[i].peRed = triples[i].rgbtRed;
        pal[i].peGreen = triples[i].rgbtGreen;
        pal[i].peBlue = triples[i].rgbtBlue;
    }
    return Init(pal, flags);
}

i32 CDibPal::Init(const char* path, u32 flags) {
    const char* ext = strrchr(path, '.');

    if (ext && stricmp(ext, ".BMP") == 0) {
        return InitBmp(path, flags);
    } else if (ext && stricmp(ext, ".PCX") == 0) {
        return InitPcx(path, flags);
    } else if (ext && stricmp(ext, ".PAL") == 0) {
        return InitPal(path, flags);
    }

    return InitRes(path, flags);
}

i32 CDibPal::Init(u8* data, u32 dataSize, RezDecodeKind type, u32 flags) {
    if (type == DECODE_PCX) {
        return InitPcx(data, dataSize, flags);
    }
    return 0;
}



i32 CDibPal::IsPaletteDevice() {
    HDC ic = CreateICA("DISPLAY", NULL, NULL, NULL);
    if (ic) {
        i32 caps = GetDeviceCaps(ic, RASTERCAPS) & RC_PALETTE;
        DeleteDC(ic);
        return caps;
    }
    return 0;
}

void CDibPal::MakeIdentity() {
    CDibPal::ClearSystemPalette();
    HDC dc = CreateDCA("DISPLAY", NULL, NULL, NULL);
    i32 sizePal = GetDeviceCaps(dc, SIZEPALETTE);
    i32 numReserved = GetDeviceCaps(dc, NUMRESERVED);
    i32 half = numReserved / 2;
    GetSystemPaletteEntries(dc, 0, half, m_logPal.m_entries);
    GetSystemPaletteEntries(
        dc,
        sizePal - half,
        half,
        &m_logPal.m_entries[m_logPal.m_numEntries - half]
    );
    for (i32 i = half; i < sizePal - half; i++) {
        m_logPal.m_entries[i].peFlags = PC_RESERVED;
    }
    DeleteDC(dc);
}

void CDibPal::ClearSystemPalette() {

    LogPal256 lp;
    HDC hdc = GetDC(NULL);
    lp.m_palVersion = LOGICAL_PALETTE_VERSION;
    lp.m_palNumEntries = 256;
    for (i32 i = 0; i < PALETTE_ENTRY_COUNT; i++) {
        lp.m_palPalEntry[i].peRed = 0;
        lp.m_palPalEntry[i].peGreen = 0;
        lp.m_palPalEntry[i].peBlue = 0;
        lp.m_palPalEntry[i].peFlags = PC_NOCOLLAPSE;
    }
    HPALETTE hpal = CreatePalette(&lp.m_lp);
    if (hpal) {
        HPALETTE old = SelectPalette(hdc, hpal, false);
        RealizePalette(hdc);
        DeleteObject(SelectPalette(hdc, old, false));
    }
    ReleaseDC(NULL, hdc);
}

i32 CDibPal::InitPal(const char* path, u32 flags) {
    io::File file;
    u8 rgb[PALETTE_RGB_BYTE_COUNT];

    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }
    if (file.size() != PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    if (file.read(rgb, PALETTE_RGB_BYTE_COUNT) != PALETTE_RGB_BYTE_COUNT) return 0;
    return Init(rgb, flags);
}

i32 CDibPal::InitPcx(const char* path, u32 flags) {
    io::File file;
    u8 rgb[PALETTE_RGB_BYTE_COUNT];

    PALETTEENTRY rgbq[PALETTE_ENTRY_COUNT];

    if (!file.open(path, io::ReadOnly)) {
        return 0;
    }
    file.seek(-PALETTE_RGB_BYTE_COUNT, io::End);
    if (file.read(rgb, PALETTE_RGB_BYTE_COUNT) != PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }

    u8* src = rgb;
    COPY_RGB_PALETTE(rgbq, src, i, PALETTE_ENTRY_COUNT)
    return Init(rgbq, flags);
}

i32 CDibPal::InitPcx(u8* data, u32 dataSize, u32 flags) {
    PALETTEENTRY pal[PALETTE_ENTRY_COUNT];
    if (dataSize < PALETTE_RGB_BYTE_COUNT) {
        return 0;
    }
    u8* s = data + dataSize - PALETTE_RGB_BYTE_COUNT;
    COPY_RGB_PALETTE(pal, s, i, PALETTE_ENTRY_COUNT)
    return Init(pal, flags);
}
