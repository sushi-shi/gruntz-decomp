#include <Io/FileStream.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <ddraw.h> // real DirectDraw SDK (IDirectDraw2, IDirectDrawPalette, LPPALETTEENTRY)
#include <rva.h>
#include <DDrawMgr/DirPal.h>        // LogPal256 (this TU owns the palette snapshots)
#include <Image/FileImageRecords.h> // Bmp256Info (the on-disk BMP info block LoadBmp reads)
#include <stdio.h>
#include <string.h> // strrchr / _stricmp / inline memcpy

#define DIRPAL_FILE "C:\\Proj\\DDrawMgr\\DIRPAL.CPP"

RVA(0x00147390, 0x78)
i32 CDDPalette::Create(IDirectDraw2* dd, PALETTEENTRY* entries, u32 flags) {
    m_cacheA = static_cast<PALETTEENTRY*>(::operator new(0x400));
    // Retail's `mov edx,[edi+eax*1] / mov [ecx+eax*1],edx / add eax,4 / cmp eax,0x400`
    // IS this entry-wise copy after MSVC strength-reduced i into a byte cursor: the
    // ARRAY stays the SIB base, the derived counter is the index. (Hand-writing the
    // byte cursor instead swaps those roles - that was the old 99.55% plateau.)
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
    // Post-increment reads: retail is `mov bl,[eax] / inc eax` x3 (0x1474e6), not the
    // fixed src[0..2] + `add edx,3` the bulk-add spelling emits.
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
    m_pos = 0;
    m_8 = 0;
    if (m_palette != 0) {
        m_palette = 0;
    }
    if (m_cacheA != 0) {
        ::operator delete(m_cacheA);
        m_cacheA = 0;
    }
    if (m_cacheB != 0) {
        ::operator delete(m_cacheB);
        m_cacheB = 0;
    }
    if (m_sourcePalette != 0) {
        ::operator delete(m_sourcePalette);
        m_sourcePalette = 0;
    }
    m_active = 0;
}

// CDDPalette::LoadBmp (__thiscall, ret 0xc => 3 args). Open the .BMP file, read
// the 14-byte BITMAPFILEHEADER then the 0x428-byte info region (BITMAPINFOHEADER
// + the 256-entry RGBQUAD table) then the 0x400-byte RGBQUAD palette, expand each
// RGBQUAD (B,G,R) to a PALETTEENTRY (R,G,B,0), and Create. Any short read fails
// (returns 0). The stack CFile forces a /GX EH frame. The CFile ctor/Open/
// Read/dtor are reloc-masked engine calls.
RVA(0x00147590, 0x17e)
i32 CDDPalette::LoadBmp(IDirectDraw2* dd, char* filename, u32 flags) {
    BITMAPFILEHEADER hdr;   // 0xe B (wingdi packs it to 14)
    PALETTEENTRY pe[0x100]; // expanded palette
    Bmp256Info info;        // BITMAPINFOHEADER + the full 256-entry colour table
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
    // the 256-entry RGBQUAD palette re-read straight over info's palette region
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

// CDDPalette::LoadPcx (__thiscall, ret 0xc => 3 args). Open the .PCX file, Seek
// to -0x300 from EOF (the trailing 768-byte VGA palette), read the 0x300 RGB
// triplets, expand each to a PALETTEENTRY (R,G,B,0), and Create. /GX EH frame for
// the stack CFile.
RVA(0x00147710, 0x122)
i32 CDDPalette::LoadPcx(IDirectDraw2* dd, char* filename, u32 flags) {
    PALETTEENTRY pe[0x100];
    u8 rgb[0x300]; // 256 packed RGB triplets (trailing VGA palette)
    CFile file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    file.Seek(-0x300, 2); // SEEK_END
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

// CDDPalette::CreateFromTrailing (__thiscall, ret 0x10 => 4 args). When `size` is
// at least 0x300 the palette is the trailing 768-byte VGA block (data+size-0x300):
// expand its 256 RGB triplets to a stack PALETTEENTRY[256] (peFlags=0) and Create;
// short data returns 0. No EH frame (no destructible local). The 0x400-byte stack
// buffer drives `sub esp,0x400`.
RVA(0x00147840, 0x7e)
i32 CDDPalette::CreateFromTrailing(IDirectDraw2* dd, void* data, u32 size, u32 flags) {
    if (size < 0x300) {
        return 0;
    }
    PALETTEENTRY entries[0x100];
    u8* src = static_cast<u8*>(data) + size - 0x300;
    // Per-byte src increment (`*src++`) reproduces retail's `inc eax` x3; SUBSCRIPTING
    // the destination (not a `dst++` cursor) is what puts retail's `add edx,4` right
    // after the peRed store instead of at the loop tail.
    for (i32 i = 0; i < 0x100; i++) {
        entries[i].peRed = *src++;
        entries[i].peGreen = *src++;
        entries[i].peBlue = *src++;
        entries[i].peFlags = 0;
    }
    return Create(dd, entries, flags);
}

// CDDPalette::LoadPal (__thiscall, ret 0xc => 3 args). Open the .PAL file, read
// the 0x300-byte RGB-triplet block, expand each to a PALETTEENTRY (R,G,B,0), and
// Create. /GX EH frame for the stack CFile.
RVA(0x001478c0, 0x112)
i32 CDDPalette::LoadPal(IDirectDraw2* dd, char* filename, u32 flags) {
    PALETTEENTRY pe[0x100];
    u8 rgb[0x300]; // 256 packed RGB triplets
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

// CDDPalette::SetAndNotify (__thiscall, ret 0x10 => 4 args; arg4 unused). Cache the
// `count` supplied PALETTEENTRYs into m_cacheA starting at `start`, wait for the next
// vertical blank through the global DirectDrawMgr's device (slot 22, @+0x58), then
// push the range straight into the DirectDraw palette via SetEntries(0, start,
// count, data). The notify only fires when the singleton is up.
RVA(0x00147aa0, 0x6a)
i32 CDDPalette::SetAndNotify(u32 start, u32 count, PALETTEENTRY* data, i32 unused) {
    // The loop walks the DESTINATION index and derives the source from it - retail
    // @0x147ac7 `mov eax,ecx / sub eax,edx` is exactly (i - start)*4 against the
    // fixed `data` base in ebx - and its guard @0x147ab5 is `cmp ebp,edi / jae`,
    // i.e. start/count are UNSIGNED (the DWORDs SetEntries itself takes).
    for (u32 i = start; i < start + count; i++) {
        m_cacheA[i] = data[i - start];
    }
    if (g_DirectDrawMgr != 0) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    return m_palette->SetEntries(0, start, count, data);
}

RVA(0x00147b10, 0x8b)
i32 CDDPalette::SetEntriesQuad(i32 start, i32 count, RGBQUAD* quads, i32 unused) {
    PALETTEENTRY* buf = static_cast<PALETTEENTRY*>(::operator new(count * 4));
    if (buf == 0) {
        return 0x80070057;
    }
    // RGBQUAD -> PALETTEENTRY: the R/B swap the header describes.
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

// CDDPalette::SetEntriesRGB (0x147ba0, __thiscall, ret 0x10 => 4 args). As above
// but from a packed 3-byte RGB source (straight copy, as in CreateRGB).
RVA(0x00147ba0, 0x82)
i32 CDDPalette::SetEntriesRGB(i32 start, i32 count, u8* rgb, i32 unused) {
    PALETTEENTRY* buf = static_cast<PALETTEENTRY*>(::operator new(count * 4));
    if (buf == 0) {
        return 0x80070057;
    }
    // The parameter IS the cursor - retail defers loading it (`mov eax,[esp+0x1c]`)
    // until past the `count <= 0` guard, which a separate `src` local hoists above.
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

// Lazily allocate the readback cache, then read all 256 entries; report a bad
// HRESULT. VOID, not int: retail sets no return value on ANY of its three exits
// (the alloc-fail `je 0x147c7b`, the hr==0 `je 0x147c7b` and the GetErrorString
// tail all fall into the same bare `pop esi / ret` with no `xor eax,eax`), and
// 0x147c30 has no caller in .text to observe one.
RVA(0x00147c30, 0x4d)
void CDDPalette::GetEntries() {
    if (m_cacheB == 0) {
        m_cacheB = static_cast<PALETTEENTRY*>(::operator new(0x400));
        if (m_cacheB == 0) {
            return;
        }
    }
    i32 hr = m_palette->GetEntries(0, 0, 0x100, m_cacheB);
    if (hr != 0) {
        CDDrawPtrCollections::GetErrorString(DIRPAL_FILE, 0x265, hr);
    }
}

// CDDPalette::Apply (__thiscall, ret 4 but no real arg). When the readback cache
// (m_cacheB) is populated, copy it into the working cache (m_cacheA, 0x400 bytes), wait for
// the next vertical blank through the global DirectDrawMgr's device (slot 22,
// @+0x58), then push all 256 entries into the DirectDraw palette via SetEntries(0,
// 0, 0x100, m_cacheB).
RVA(0x00147c80, 0x4d)
void CDDPalette::Apply(i32 unused) {
    PALETTEENTRY* readback = m_cacheB;
    if (readback == 0) {
        return;
    }
    // Unsigned index: retail's strength-reduced guard is `cmp eax,0x400 / jb`.
    for (u32 i = 0; i < 0x100; i++) {
        m_cacheA[i] = readback[i];
    }
    if (g_DirectDrawMgr != 0) {
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

// CDDPalette::FadeRange (0x147d50, __thiscall, ret 0x18 => 6 args). Snapshot the
// current palette (GetEntries into m_cacheA), then over durationMs interpolate
// each entry in [start,start+count) linearly from its snapshot value toward the
// solid color (r,g,b), pushing SetEntries once per changed millisecond. Finally
// SetRange to the exact target. RezAlloc snapshot copy freed at the end. The
// frame clock is the cached ::timeGetTime fn-ptr. (The BLOCKING fade twin of the
// per-frame StartFadeToColor/Tick machinery below.)
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
    i32 t0 = ::timeGetTime();
    i32 prev = 9;
    // Retail compares elapsed t<durationMs UNSIGNED (jb/jbe) but keeps the lerp
    // arithmetic signed (imul/idiv), so t is a signed int with only the loop guard
    // done unsigned (durationMs is a signed param, unchanged).
    for (i32 t = 10; static_cast<u32>(t) < static_cast<u32>(durationMs); t = ::timeGetTime() - t0) {
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
    m_targetPalette = 0;
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
    u32 dt = ::timeGetTime() - m_startTimeMs;
    if (dt >= static_cast<u32>(m_durationMs)) {
        Flush();
        return 0;
    }
    if (m_targetPalette != 0) {
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
    if (v != 0) {
        SetAndNotify(m_firstColorIndex, m_colorCount, v, 0);
        m_targetPalette = 0;
    } else {
        // Retail @0x148289: `mov eax,[esi+0x1c]` (the whole entry as one dword) ->
        // `mov [esp+8],eax` (the copy) -> `mov edx,[esp+0xa]` / `mov ecx,[esp+9]`.
        // Those unaligned dword re-reads are MSVC5 widening a BYTE argument, not
        // source: `pe.peGreen` at +1 / `pe.peBlue` at +2 of the stack copy.
        PALETTEENTRY pe = m_fixedColor;
        SetRange(m_firstColorIndex, m_colorCount, pe.peRed, pe.peGreen, pe.peBlue, 0);
    }
}

// CDDPalette::BlendRange (0x1482c0, __thiscall, ret 0x18 => 6 args). Blend each
// entry in [start,start+count) pct% (0..100) toward the solid color (r,g,b) in a
// single pass and push it straight to the DirectDraw palette via SetEntries.
// The masks are ASSIGNED to the parameters, not spelled `(r & 0xff)` at each use:
// retail's preheader (0x1482e1..0x148307) writes each masked value back into the
// arg's OWN slot ([esp+0x28]/[esp+0x24]/[esp+0x2c]) and the frame is a single
// `push ecx`. The `(r & 0xff)` spelling makes two extra temps -> `sub esp,0xc`.
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

// CDDPalette::CaptureSystemPalette (0x1485b0, __thiscall; the ex-DirPal view) -
// snapshot the Windows system-reserved palette entries (the low + high halves GDI
// keeps for the shell) into the working cache (m_cacheA), then install them.
// Every gate is POSITIVE-form: retail has exactly TWO epilogues - the inline
// `mov eax,1` success and one shared bottom `xor eax,eax` that all four failure
// gates plus the SetAndNotify error tail fall into. The early-return spelling
// gave each gate its own 6-instruction inline epilogue (6 rets vs retail's 2)
// - docs/patterns/positive-gate-enables-shrink-wrap.md.
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

// BlackoutSystemPalette (0x148720, __cdecl) - build an all-black 256-entry
// PC_NOCOLLAPSE logical palette, select+realize it into the screen DC (driving
// the hardware DAC to black), then restore the previous palette and delete the
// black one. Returns 1 on success, 0 if the DC or palette could not be obtained.
RVA(0x00148720, 0x9f)
i32 BlackoutSystemPalette() {
    HDC hdc = ::GetDC(0);
    if (hdc != 0) {
        LogPal256 lp;
        lp.palVersion = 0x300;
        lp.palNumEntries = 0x100;
        for (i32 i = 0; i < 0x100; i++) {
            lp.palPalEntry[i].peRed = 0;
            lp.palPalEntry[i].peGreen = 0;
            lp.palPalEntry[i].peBlue = 0;
            lp.palPalEntry[i].peFlags = 4; // PC_NOCOLLAPSE
        }
        // LOGPALETTE declares palPalEntry[1]; a 256-entry palette needs the larger
        // local, and handing it to the SDK is the API-forced Win32 idiom
        HPALETTE hpal = ::CreatePalette(reinterpret_cast<LOGPALETTE*>(&lp));
        if (hpal != 0) {
            HPALETTE(WINAPI * pSelect)(HDC, HPALETTE, BOOL) = ::SelectPalette;
            HPALETTE old = pSelect(hdc, hpal, 0);
            ::RealizePalette(hdc);
            ::DeleteObject(pSelect(hdc, old, 0));
            ::ReleaseDC(0, hdc);
            return 1;
        }
        ::ReleaseDC(0, hdc);
    }
    return 0;
}
