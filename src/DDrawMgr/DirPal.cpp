#include <rva.h>

#include <DDrawMgr/DirPal.h>

#include <Mfc.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <Image/FileImageRecords.h>
#include <Io/FileStream.h>

#include <ddraw.h>
#include <stdio.h>
#include <string.h>

#define DIRPAL_FILE "C:\\Proj\\DDrawMgr\\DIRPAL.CPP"

RVA(0x00147390, 0x78)
i32 CDDPalette::Create(IDirectDraw2* dd, PALETTEENTRY* entries, u32 flags) {
    m_cacheA = static_cast<PALETTEENTRY*>(::operator new(0x400));

    for (i32 i = 0; i < 0x100; i++) {
        m_cacheA[i] = entries[i];
    }
    m_cacheB = static_cast<PALETTEENTRY*>(::operator new(0x400));
    i32 hr = dd->CreatePalette(flags, entries, &m_palette, 0);
    if (hr == 0) {
        return 1;
    }
    CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x4b, hr);
    return 0;
}

RVA(0x00147410, 0xbc)
i32 CDDPalette::LoadFromFile(IDirectDraw2* dd, char* filename, u32 flags) {
    char* ext = strrchr(filename, '.');
    if (ext && _strcmpi(ext, ".BMP") == 0) {
        return LoadBmp(dd, filename, flags);
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        return LoadPcx(dd, filename, flags);
    } else if (ext && _strcmpi(ext, ".PAL") == 0) {
        return LoadPal(dd, filename, flags);
    }
    return LoadDefault(dd, filename, flags);
}

RVA(0x001474d0, 0x60)
i32 CDDPalette::CreateRGB(IDirectDraw2* dd, void* rgb, u32 flags) {
    PALETTEENTRY entries[0x100];

    u8* src = static_cast<u8*>(rgb);
    for (i32 i = 0; i < 0x100; i++) {
        entries[i].peRed = *src++;
        entries[i].peGreen = *src++;
        entries[i].peBlue = *src++;
        entries[i].peFlags = 0;
    }
    return Create(dd, entries, flags);
}

RVA(0x00147530, 0x54)
void CDDPalette::Destroy() {
    m_pos = NULL;
    m_8 = 0;
    if (m_palette != NULL) {
        m_palette = NULL;
    }
    if (m_cacheA != NULL) {
        ::operator delete(m_cacheA);
        m_cacheA = NULL;
    }
    if (m_cacheB != NULL) {
        ::operator delete(m_cacheB);
        m_cacheB = NULL;
    }
    if (m_sourcePalette != NULL) {
        ::operator delete(m_sourcePalette);
        m_sourcePalette = NULL;
    }
    m_active = 0;
}

RVA(0x00147590, 0x17e)
i32 CDDPalette::LoadBmp(IDirectDraw2* dd, char* filename, u32 flags) {
    BITMAPFILEHEADER hdr;
    PALETTEENTRY pe[0x100];
    Bmp256Info info;
    CFile file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    if (file.Read(&hdr, 0xe) != 0xe) {
        return 0;
    }
    if (file.Read(&info, 0x428) != 0x428) {
        return 0;
    }

    if (file.Read(info.bmiColors, 0x400) != 0x400) {
        return 0;
    }
    for (i32 i = 0; i < 0x100; i++) {
        pe[i].peRed = info.bmiColors[i].rgbRed;
        pe[i].peGreen = info.bmiColors[i].rgbGreen;
        pe[i].peBlue = info.bmiColors[i].rgbBlue;
        pe[i].peFlags = 0;
    }
    return Create(dd, pe, flags);
}

RVA(0x00147710, 0x122)
i32 CDDPalette::LoadPcx(IDirectDraw2* dd, char* filename, u32 flags) {
    PALETTEENTRY pe[0x100];
    u8 rgb[0x300];
    CFile file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    file.Seek(-0x300, 2);
    if (file.Read(rgb, 0x300) != 0x300) {
        return 0;
    }
    u8* src = rgb;
    for (i32 i = 0; i < 0x100; i++) {
        pe[i].peRed = *src++;
        pe[i].peGreen = *src++;
        pe[i].peBlue = *src++;
        pe[i].peFlags = 0;
    }
    return Create(dd, pe, flags);
}

RVA(0x00147840, 0x7e)
i32 CDDPalette::CreateFromTrailing(IDirectDraw2* dd, void* data, u32 size, u32 flags) {
    if (size < 0x300) {
        return 0;
    }
    PALETTEENTRY entries[0x100];
    u8* src = static_cast<u8*>(data) + size - 0x300;

    for (i32 i = 0; i < 0x100; i++) {
        entries[i].peRed = *src++;
        entries[i].peGreen = *src++;
        entries[i].peBlue = *src++;
        entries[i].peFlags = 0;
    }
    return Create(dd, entries, flags);
}

RVA(0x001478c0, 0x112)
i32 CDDPalette::LoadPal(IDirectDraw2* dd, char* filename, u32 flags) {
    PALETTEENTRY pe[0x100];
    u8 rgb[0x300];
    CFile file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    if (file.Read(rgb, 0x300) != 0x300) {
        return 0;
    }
    u8* src = rgb;
    for (i32 i = 0; i < 0x100; i++) {
        pe[i].peRed = *src++;
        pe[i].peGreen = *src++;
        pe[i].peBlue = *src++;
        pe[i].peFlags = 0;
    }
    return Create(dd, pe, flags);
}

RVA(0x001479e0, 0xbb)
i32 CDDPalette::LoadDefault(IDirectDraw2* dd, char* filename, u32 flags) {
    PALETTEENTRY pal[256];
    HRSRC hr = FindResourceA(g_resModule, filename, "PALETTE");
    if (!hr) {
        return 0;
    }
    HGLOBAL hg = LoadResource(g_resModule, hr);
    if (!hg) {
        return 0;
    }
    u8* src = static_cast<u8*>(LockResource(hg));
    if (!src) {
        return 0;
    }
    for (i32 i = 0; i < 256; i++) {
        pal[i].peRed = *src++;
        pal[i].peGreen = *src++;
        pal[i].peBlue = *src++;
        pal[i].peFlags = 0;
    }
    return Create(dd, pal, flags);
}

RVA(0x00147aa0, 0x6a)
i32 CDDPalette::SetAndNotify(u32 start, u32 count, PALETTEENTRY* data, i32 unused) {

    for (u32 i = start; i < start + count; i++) {
        m_cacheA[i] = data[i - start];
    }
    if (g_DirectDrawMgr != NULL) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    return m_palette->SetEntries(0, start, count, data);
}

RVA(0x00147b10, 0x8b)
i32 CDDPalette::SetEntriesQuad(i32 start, i32 count, RGBQUAD* quads, i32 unused) {
    PALETTEENTRY* buf = static_cast<PALETTEENTRY*>(::operator new(count * 4));
    if (buf == NULL) {
        return 0x80070057;
    }

    for (i32 i = 0; i < count; i++) {
        buf[i].peRed = quads[i].rgbRed;
        buf[i].peGreen = quads[i].rgbGreen;
        buf[i].peBlue = quads[i].rgbBlue;
        buf[i].peFlags = 0;
    }
    i32 hr = SetAndNotify(start, count, buf, unused);
    ::operator delete(buf);
    return hr;
}

RVA(0x00147ba0, 0x82)
i32 CDDPalette::SetEntriesRGB(i32 start, i32 count, u8* rgb, i32 unused) {
    PALETTEENTRY* buf = static_cast<PALETTEENTRY*>(::operator new(count * 4));
    if (buf == NULL) {
        return 0x80070057;
    }

    for (i32 i = 0; i < count; i++) {
        buf[i].peRed = *rgb++;
        buf[i].peGreen = *rgb++;
        buf[i].peBlue = *rgb++;
        buf[i].peFlags = 0;
    }
    i32 hr = SetAndNotify(start, count, buf, unused);
    ::operator delete(buf);
    return hr;
}

RVA(0x00147c30, 0x4d)
void CDDPalette::GetEntries() {
    if (m_cacheB == NULL) {
        m_cacheB = static_cast<PALETTEENTRY*>(::operator new(0x400));
        if (m_cacheB == NULL) {
            return;
        }
    }
    i32 hr = m_palette->GetEntries(0, 0, 0x100, m_cacheB);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x265, hr);
    }
}

RVA(0x00147c80, 0x4d)
void CDDPalette::Apply(i32 unused) {
    PALETTEENTRY* readback = m_cacheB;
    if (readback == NULL) {
        return;
    }

    for (u32 i = 0; i < 0x100; i++) {
        m_cacheA[i] = readback[i];
    }
    if (g_DirectDrawMgr != NULL) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    m_palette->SetEntries(0, 0, 0x100, readback);
}

RVA(0x00147cd0, 0x78)
i32 CDDPalette::SetRange(i32 start, i32 count, u8 r, u8 g, u8 b, u32 flags) {
    for (i32 i = start; i < start + count; i++) {
        m_cacheA[i].peRed = r;
        m_cacheA[i].peGreen = g;
        m_cacheA[i].peBlue = b;
    }
    i32 hr = m_palette->SetEntries(flags, start, count, m_cacheA + start);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x2a3, hr);
    }
    return hr;
}

RVA(0x00147d50, 0x1d2)
void CDDPalette::FadeRange(i32 start, i32 count, i32 r, i32 g, i32 b, i32 durationMs) {
    i32 hr = m_palette->GetEntries(0, 0, 0x100, m_cacheA);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x2c0, hr);
    }
    PALETTEENTRY* snapshot = static_cast<PALETTEENTRY*>(::operator new(0x400));
    for (i32 i = 0; i < 0x100; i++) {
        snapshot[i] = m_cacheA[i];
    }
    i32 t0 = timeGetTime();
    i32 prev = 9;

    for (i32 t = 10; static_cast<u32>(t) < static_cast<u32>(durationMs); t = timeGetTime() - t0) {
        if (t != prev) {
            for (i32 j = start; j < start + count; j++) {
                m_cacheA[j].peRed = static_cast<u8>(
                    (((r & 0xff) - snapshot[j].peRed) * t / durationMs + snapshot[j].peRed)
                );
                m_cacheA[j].peGreen = static_cast<u8>(
                    (((g & 0xff) - snapshot[j].peGreen) * t / durationMs + snapshot[j].peGreen)
                );
                m_cacheA[j].peBlue = static_cast<u8>(
                    (((b & 0xff) - snapshot[j].peBlue) * t / durationMs + snapshot[j].peBlue)
                );
            }
            m_palette->SetEntries(0, start, count, m_cacheA + start);
        }
        prev = t;
    }
    SetRange(start, count, r, g, b, 0);
    ::operator delete(snapshot);
}

RVA(0x00147f30, 0xbe)
void CDDPalette::StartFadeToColor(i32 start, i32 count, char r, char g, char b, i32 durationMs) {
    if (m_active) {
        Flush();
    }
    i32 err = m_palette->GetEntries(0, 0, 0x100, m_cacheA);
    if (err) {
        CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x311, err);
    }
    m_firstColorIndex = start;
    m_colorCount = count;
    m_durationMs = durationMs;
    m_startTimeMs = timeGetTime();
    m_lastElapsedMs = -1;
    m_targetPalette = NULL;
    m_fixedColor.peRed = r;
    m_fixedColor.peGreen = g;
    m_fixedColor.peBlue = b;
    if (!m_sourcePalette) {
        m_sourcePalette = static_cast<PALETTEENTRY*>(::operator new(0x400));
    }
    for (i32 i = 0; i < 0x100; i++) {
        m_sourcePalette[i] = m_cacheA[i];
    }
    m_active = 1;
    Tick();
}

RVA(0x00147ff0, 0xa9)
void CDDPalette::StartFadeToPalette(i32 start, i32 count, PALETTEENTRY* target, i32 durationMs) {
    if (m_active) {
        Flush();
    }
    i32 err = m_palette->GetEntries(0, 0, 0x100, m_cacheA);
    if (err) {
        CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x34b, err);
    }
    m_firstColorIndex = start;
    m_colorCount = count;
    m_durationMs = durationMs;
    m_startTimeMs = timeGetTime();
    m_targetPalette = target;
    m_lastElapsedMs = -1;
    if (!m_sourcePalette) {
        m_sourcePalette = static_cast<PALETTEENTRY*>(::operator new(0x400));
    }
    for (i32 i = 0; i < 0x100; i++) {
        m_sourcePalette[i] = m_cacheA[i];
    }
    m_active = 1;
    Tick();
}

RVA(0x001480a0, 0x1a7)
i32 CDDPalette::Tick() {
    if (m_active == 0) {
        return 0;
    }
    u32 dt = timeGetTime() - m_startTimeMs;
    if (dt >= static_cast<u32>(m_durationMs)) {
        Flush();
        return 0;
    }
    if (m_targetPalette != NULL) {
        if (dt != static_cast<u32>(m_lastElapsedMs)) {
            i32 i = m_firstColorIndex;
            if (i < m_firstColorIndex + m_colorCount) {
                do {
                    m_cacheA[i].peRed = static_cast<char>(
                                            (static_cast<i32>(
                                                 ((static_cast<u32>(m_targetPalette[i].peRed)
                                                   - static_cast<u32>(m_sourcePalette[i].peRed))
                                                  * dt)
                                             )
                                             / m_durationMs)
                                        )
                                        + m_sourcePalette[i].peRed;
                    m_cacheA[i].peGreen = static_cast<char>(
                                              (static_cast<i32>(
                                                   ((static_cast<u32>(m_targetPalette[i].peGreen)
                                                     - static_cast<u32>(m_sourcePalette[i].peGreen))
                                                    * dt)
                                               )
                                               / m_durationMs)
                                          )
                                          + m_sourcePalette[i].peGreen;
                    m_cacheA[i].peBlue = static_cast<char>(
                                             (static_cast<i32>(
                                                  ((static_cast<u32>(m_targetPalette[i].peBlue)
                                                    - static_cast<u32>(m_sourcePalette[i].peBlue))
                                                   * dt)
                                              )
                                              / m_durationMs)
                                         )
                                         + m_sourcePalette[i].peBlue;
                    i++;
                } while (i < m_firstColorIndex + m_colorCount);
            }
            m_palette->SetEntries(0, m_firstColorIndex, m_colorCount, m_cacheA + m_firstColorIndex);
        }
    } else {
        if (dt != static_cast<u32>(m_lastElapsedMs)) {
            i32 i = m_firstColorIndex;
            if (i < m_firstColorIndex + m_colorCount) {
                do {
                    m_cacheA[i].peRed = static_cast<char>(
                                            (static_cast<i32>(
                                                 ((static_cast<u32>(m_fixedColor.peRed)
                                                   - static_cast<u32>(m_sourcePalette[i].peRed))
                                                  * dt)
                                             )
                                             / m_durationMs)
                                        )
                                        + m_sourcePalette[i].peRed;
                    m_cacheA[i].peGreen = static_cast<char>(
                                              (static_cast<i32>(
                                                   ((static_cast<u32>(m_fixedColor.peGreen)
                                                     - static_cast<u32>(m_sourcePalette[i].peGreen))
                                                    * dt)
                                               )
                                               / m_durationMs)
                                          )
                                          + m_sourcePalette[i].peGreen;
                    m_cacheA[i].peBlue = static_cast<char>(
                                             (static_cast<i32>(
                                                  ((static_cast<u32>(m_fixedColor.peBlue)
                                                    - static_cast<u32>(m_sourcePalette[i].peBlue))
                                                   * dt)
                                              )
                                              / m_durationMs)
                                         )
                                         + m_sourcePalette[i].peBlue;
                    i++;
                } while (i < m_firstColorIndex + m_colorCount);
            }
            m_palette->SetEntries(0, m_firstColorIndex, m_colorCount, m_cacheA + m_firstColorIndex);
        }
    }
    m_lastElapsedMs = dt;
    return 1;
}

RVA(0x00148250, 0x61)
void CDDPalette::Flush() {
    if (m_active == 0) {
        return;
    }
    PALETTEENTRY* v = m_targetPalette;
    m_active = 0;
    if (v != NULL) {
        SetAndNotify(m_firstColorIndex, m_colorCount, v, 0);
        m_targetPalette = NULL;
    } else {

        PALETTEENTRY pe = m_fixedColor;
        SetRange(m_firstColorIndex, m_colorCount, pe.peRed, pe.peGreen, pe.peBlue, 0);
    }
}

RVA(0x001482c0, 0x11f)
void CDDPalette::BlendRange(i32 pct, i32 start, i32 count, u8 r, u8 g, u8 b) {
    i32 end = start + count;
    if (start < end) {
        i32 i = start;
        do {
            m_cacheA[i].peRed =
                static_cast<u8>(((r - m_cacheA[i].peRed) * pct / 100 + m_cacheA[i].peRed));
            m_cacheA[i].peGreen =
                static_cast<u8>(((g - m_cacheA[i].peGreen) * pct / 100 + m_cacheA[i].peGreen));
            m_cacheA[i].peBlue =
                static_cast<u8>(((b - m_cacheA[i].peBlue) * pct / 100 + m_cacheA[i].peBlue));
            i++;
        } while (i < end);
    }
    i32 hr = m_palette->SetEntries(0, start, count, m_cacheA + start);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x406, hr);
    }
}

RVA(0x001485b0, 0x162)
i32 CDDPalette::CaptureSystemPalette() {
    HDC hdc = CreateDCA("DISPLAY", 0, 0, 0);
    if (hdc) {
        i32 sizePal = GetDeviceCaps(hdc, SIZEPALETTE);
        i32 half = GetDeviceCaps(hdc, NUMRESERVED) / 2;
        LogPal256 lp;
        lp.palVersion = 0x300;
        lp.palNumEntries = 0x100;
        if (GetSystemPaletteEntries(hdc, 0, half, lp.palPalEntry)
            && GetSystemPaletteEntries(
                hdc,
                sizePal - half,
                half,
                &lp.palPalEntry[lp.palNumEntries - half]
            )) {
            DeleteDC(hdc);
            PALETTEENTRY* dest = m_cacheA;
            if (dest) {
                i32 i;
                for (i = 0; i < half; i++) {
                    dest[i].peRed = lp.palPalEntry[i].peRed;
                    dest[i].peGreen = lp.palPalEntry[i].peGreen;
                    dest[i].peBlue = lp.palPalEntry[i].peBlue;
                }
                for (i = sizePal - half; i < sizePal; i++) {
                    dest[i].peRed = lp.palPalEntry[i].peRed;
                    dest[i].peGreen = lp.palPalEntry[i].peGreen;
                    dest[i].peBlue = lp.palPalEntry[i].peBlue;
                }
                i32 rc = SetAndNotify(0, 0x100, dest, 0);
                if (rc == 0) {
                    return 1;
                }
                CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x495, rc);
            }
        }
    }
    return 0;
}

RVA(0x00148720, 0x9f)
i32 BlackoutSystemPalette() {
    HDC hdc = GetDC(0);
    if (hdc != NULL) {
        LogPal256 lp;
        lp.palVersion = 0x300;
        lp.palNumEntries = 0x100;
        for (i32 i = 0; i < 0x100; i++) {
            lp.palPalEntry[i].peRed = 0;
            lp.palPalEntry[i].peGreen = 0;
            lp.palPalEntry[i].peBlue = 0;
            lp.palPalEntry[i].peFlags = 4;
        }
        HPALETTE hpal = CreatePalette(&lp.m_lp);
        if (hpal != NULL) {
            HPALETTE(WINAPI * pSelect)(HDC, HPALETTE, BOOL) = SelectPalette;
            HPALETTE old = pSelect(hdc, hpal, 0);
            RealizePalette(hdc);
            DeleteObject(pSelect(hdc, old, 0));
            ReleaseDC(0, hdc);
            return 1;
        }
        ReleaseDC(0, hdc);
    }
    return 0;
}
