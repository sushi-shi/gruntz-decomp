#include <Io/FileStream.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <ddraw.h> // real DirectDraw SDK (IDirectDraw2, IDirectDrawPalette, LPPALETTEENTRY)
#include <rva.h>
#include <DDrawMgr/DirPal.h> // LogPal256 (this TU owns the palette snapshots)
#include <stdio.h>
#include <string.h>  // strrchr / _stricmp / inline memcpy
#include <Globals.h> // g_DirectDrawMgr (the singleton whose device the notify waits on)

#define DIRPAL_FILE "C:\\Proj\\DDrawMgr\\DIRPAL.CPP"

extern HINSTANCE g_resModule;

RVA(0x00147390, 0x78)
i32 CDDPalette::Create(IDirectDraw2* dd, void* entries, u32 flags) {
    m_cacheA = static_cast<u8*>(::operator new(0x400));
    // Plateau note: byte-for-byte except the copy loop's SIB base/index roles
    // (retail encodes [entries+i]/[m_cacheA+i] with i as the index; MSVC here makes i
    // the base) - a 1-byte-per-insn encoding choice, semantically identical.
    for (i32 i = 0; i < 0x400; i += 4) {
        *reinterpret_cast<i32*>((m_cacheA + i)) = *reinterpret_cast<i32*>((reinterpret_cast<char*>(entries) + i));
    }
    m_cacheB = static_cast<u8*>(::operator new(0x400));
    i32 hr = dd->CreatePalette(flags, static_cast<LPPALETTEENTRY>(entries), &m_palette, 0);
    if (hr == 0) {
        return 1;
    }
    CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x4b, hr);
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
    u8 entries[0x400];
    u8* src = static_cast<u8*>(rgb);
    for (i32 i = 0; i < 0x100; i++) {
        entries[i * 4 + 0] = src[0];
        entries[i * 4 + 1] = src[1];
        entries[i * 4 + 2] = src[2];
        entries[i * 4 + 3] = 0;
        src += 3;
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
// (returns 0). The stack CFileIO forces a /GX EH frame. The CFileIO ctor/Open/
// Read/dtor are reloc-masked engine calls.
// @early-stop
// big EH frame (0x17e B): logic/CFG/all four CFileIO calls/the RGBQUAD->PALETTEENTRY
// expand loop/the Create call all reproduced. Residual is the EH stack-slot layout
// of the 0x848-byte scratch frame (retail's exact [esp+N] slot choices for the read
// buffers + the EH-state stores) + the reloc-masked CFileIO/EH symbol names.
// Deferred to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x00147590, 0x17e)
i32 CDDPalette::LoadBmp(IDirectDraw2* dd, char* filename, u32 flags) {
    u8 hdr14[0xe];   // BITMAPFILEHEADER
    u8 pe[0x400];    // expanded PALETTEENTRY[256]
    u8 info[0x428];  // BITMAPINFOHEADER + the in-file palette region
    u8 quads[0x400]; // 256-entry RGBQUAD palette
    CFile file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    if (file.Read(hdr14, 0xe) != 0xe) {
        return 0;
    }
    if (file.Read(info, 0x428) != 0x428) {
        return 0;
    }
    if (file.Read(quads, 0x400) != 0x400) {
        return 0;
    }
    for (i32 i = 0; i < 0x400; i += 4) {
        pe[i + 0] = quads[i + 2]; // R <- RGBQUAD.rgbRed
        pe[i + 1] = quads[i + 1]; // G <- rgbGreen
        pe[i + 2] = quads[i + 0]; // B <- rgbBlue
        pe[i + 3] = 0;            // peFlags
    }
    return Create(dd, pe, flags);
}

// CDDPalette::LoadPcx (__thiscall, ret 0xc => 3 args). Open the .PCX file, Seek
// to -0x300 from EOF (the trailing 768-byte VGA palette), read the 0x300 RGB
// triplets, expand each to a PALETTEENTRY (R,G,B,0), and Create. /GX EH frame for
// the stack CFileIO.
// @early-stop
// big EH frame (0x122 B): the Open/Seek/Read/Create call chain, the 0x300-triplet
// expand loop and the Create forward all reproduced. Residual is the EH stack-slot
// layout of the 0x710-byte scratch frame + the reloc-masked CFileIO/EH symbol
// names. Deferred to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x00147710, 0x122)
i32 CDDPalette::LoadPcx(IDirectDraw2* dd, char* filename, u32 flags) {
    u8 pe[0x400];  // expanded PALETTEENTRY[256]
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
    for (i32 i = 0; i < 0x400; i += 4) {
        pe[i + 0] = src[0];
        pe[i + 1] = src[1];
        pe[i + 2] = src[2];
        pe[i + 3] = 0;
        src += 3;
    }
    return Create(dd, pe, flags);
}

// CDDPalette::CreateFromTrailing (__thiscall, ret 0x10 => 4 args). When `size` is
// at least 0x300 the palette is the trailing 768-byte VGA block (data+size-0x300):
// expand its 256 RGB triplets to a stack PALETTEENTRY[256] (peFlags=0) and Create;
// short data returns 0. No EH frame (no destructible local). The 0x400-byte stack
// buffer drives `sub esp,0x400`.
// @early-stop
// loop-scheduling wall (89.09%): byte-identical to retail EXCEPT the inner expand
// loop's `add edx,4` placement (retail bumps the dst pointer after the first store,
// our cl bumps it at the loop tail) - a code-scheduling choice, not source-steerable.
// Same family as Create's SIB-encoding plateau. docs/patterns/zero-register-pinning.md.
RVA(0x00147840, 0x7e)
i32 CDDPalette::CreateFromTrailing(IDirectDraw2* dd, void* data, u32 size, u32 flags) {
    if (size < 0x300) {
        return 0;
    }
    u8 entries[0x400];
    u8* src = reinterpret_cast<u8*>(data) + size - 0x300;
    // Per-byte src increment (`*src++`) + a running dst pointer reproduce retail's
    // `inc eax`x3 / `add edx,4` loop shape (vs the `src+=3` bulk-add form).
    u8* dst = entries;
    for (i32 i = 0; i < 0x100; i++) {
        dst[0] = *src++;
        dst[1] = *src++;
        dst[2] = *src++;
        dst[3] = 0;
        dst += 4;
    }
    return Create(dd, entries, flags);
}

// CDDPalette::LoadPal (__thiscall, ret 0xc => 3 args). Open the .PAL file, read
// the 0x300-byte RGB-triplet block, expand each to a PALETTEENTRY (R,G,B,0), and
// Create. /GX EH frame for the stack CFileIO.
// @early-stop
// big EH frame (0x112 B): the Open/Read/Create chain, the 0x300-triplet expand
// loop and the Create forward all reproduced. Residual is the EH stack-slot layout
// of the 0x710-byte scratch frame + the reloc-masked CFileIO/EH symbol names.
// Deferred to the final sweep. docs/patterns/zero-register-pinning.md.
RVA(0x001478c0, 0x112)
i32 CDDPalette::LoadPal(IDirectDraw2* dd, char* filename, u32 flags) {
    u8 pe[0x400];  // expanded PALETTEENTRY[256]
    u8 rgb[0x300]; // 256 packed RGB triplets
    CFile file;
    if (file.Open(filename, 0, 0) == 0) {
        return 0;
    }
    if (file.Read(rgb, 0x300) != 0x300) {
        return 0;
    }
    u8* src = rgb;
    for (i32 i = 0; i < 0x400; i += 4) {
        pe[i + 0] = src[0];
        pe[i + 1] = src[1];
        pe[i + 2] = src[2];
        pe[i + 3] = 0;
        src += 3;
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
    char* src = static_cast<char*>(LockResource(hg));
    if (!src) {
        return 0;
    }
    for (i32 i = 0; i < 256; i++) {
        pal[i].peRed = src[0];
        pal[i].peGreen = src[1];
        pal[i].peBlue = src[2];
        pal[i].peFlags = 0;
        src += 3;
    }
    return Create(dd, pal, flags);
}

// CDDPalette::SetAndNotify (__thiscall, ret 0x10 => 4 args; a4 unused). Cache the
// `count` supplied PALETTEENTRYs into m_cacheA starting at `start`, wait for the next
// vertical blank through the global DirectDrawMgr's device (slot 22, @+0x58), then
// push the range straight into the DirectDraw palette via SetEntries(0, start,
// count, data). The notify only fires when the singleton is up.
// @early-stop
// copy-loop SIB / arg-regalloc wall (61.87%): the VBlank-notify + SetEntries call
// chain match, but MSVC5's strength-reduced cache copy loop (`m_cacheA[start+i]=data[i]`)
// picks a different induction-var/SIB form + arg-register assignment than retail.
// Same family as Create's copy-loop plateau. docs/patterns/zero-register-pinning.md.
RVA(0x00147aa0, 0x6a)
i32 CDDPalette::SetAndNotify(i32 start, i32 count, i32* data, i32 a4) {
    i32* cache = reinterpret_cast<i32*>(m_cacheA);
    for (i32 i = 0; i < count; i++) {
        cache[start + i] = data[i];
    }
    if (g_DirectDrawMgr != 0) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    return m_palette->SetEntries(0, start, count, reinterpret_cast<LPPALETTEENTRY>(data));
}

RVA(0x00147b10, 0x8b)
i32 CDDPalette::SetEntriesQuad(i32 start, i32 count, u8* quads, i32 a4) {
    u8* buf = static_cast<u8*>(::operator new(count * 4));
    if (buf == 0) {
        return 0x80070057;
    }
    for (i32 i = 0; i < count; i++) {
        buf[i * 4 + 0] = quads[i * 4 + 2];
        buf[i * 4 + 1] = quads[i * 4 + 1];
        buf[i * 4 + 2] = quads[i * 4 + 0];
        buf[i * 4 + 3] = 0;
    }
    i32 hr = SetAndNotify(start, count, reinterpret_cast<i32*>(buf), a4);
    ::operator delete(buf);
    return hr;
}

// CDDPalette::SetEntriesRGB (0x147ba0, __thiscall, ret 0x10 => 4 args). As above
// but from a packed 3-byte RGB source (straight copy, as in CreateRGB).
// @early-stop
// store-scheduling tail (~93%; permuter confirms no source-steerable gain): body/
// alloc/SetAndNotify/free are byte-exact. Retail schedules the last entry's peBlue
// store before the peFlags=0 store (my cl emits them reversed) + the trailing src++.
// Twin SetEntriesQuad (stride-4 read) IS exact; the stride-3 read reshuffles the tail.
RVA(0x00147ba0, 0x82)
i32 CDDPalette::SetEntriesRGB(i32 start, i32 count, u8* rgb, i32 a4) {
    u8* buf = static_cast<u8*>(::operator new(count * 4));
    if (buf == 0) {
        return 0x80070057;
    }
    u8* src = rgb;
    u8* d = buf;
    for (i32 i = 0; i < count; i++) {
        d[0] = *src++;
        d[1] = *src++;
        d[2] = *src++;
        d[3] = 0;
        d += 4;
    }
    i32 hr = SetAndNotify(start, count, reinterpret_cast<i32*>(buf), a4);
    ::operator delete(buf);
    return hr;
}

RVA(0x00147c30, 0x4d)
i32 CDDPalette::GetEntries() {
    // Lazily allocates the readback cache, then reads all 256 entries; reports a
    // bad HRESULT. Plateau note: retail's body falls off the end (no return -
    // the int symbol returns whatever sits in eax), so it omits the trailing
    // `xor eax,eax` this `return 0` emits. MSVC 5.0 C++ forbids fall-off
    // (C2561), so the one-instruction `xor` gap can't be reproduced from clean C.
    if (m_cacheB == 0) {
        m_cacheB = static_cast<u8*>(::operator new(0x400));
        if (m_cacheB == 0) {
            return 0;
        }
    }
    i32 hr = m_palette->GetEntries(0, 0, 0x100, reinterpret_cast<LPPALETTEENTRY>(m_cacheB));
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x265, hr);
    }
    return 0;
}

// CDDPalette::Apply (__thiscall, ret 4 but no real arg). When the readback cache
// (m_cacheB) is populated, copy it into the working cache (m_cacheA, 0x400 bytes), wait for
// the next vertical blank through the global DirectDrawMgr's device (slot 22,
// @+0x58), then push all 256 entries into the DirectDraw palette via SetEntries(0,
// 0, 0x100, m_cacheB).
// @early-stop
// regalloc coin-flip (97.58%): every code byte matches retail EXCEPT the register
// the m_palette load for SetEntries lands in (retail reuses esi, ours uses eax). Same
// values/stores/order; not source-steerable. docs/patterns/zero-register-pinning.md.
RVA(0x00147c80, 0x4d)
void CDDPalette::Apply(i32 a1) {
    u8* readback = m_cacheB;
    if (readback == 0) {
        return;
    }
    // Byte-offset copy loop (i+=4, cmp 0x400) matches retail's index-walk form.
    for (i32 i = 0; i < 0x400; i += 4) {
        *reinterpret_cast<i32*>((m_cacheA + i)) = *reinterpret_cast<i32*>((readback + i));
    }
    if (g_DirectDrawMgr != 0) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    m_palette->SetEntries(0, 0, 0x100, reinterpret_cast<LPPALETTEENTRY>(readback));
}

RVA(0x00147cd0, 0x78)
i32 CDDPalette::SetRange(i32 start, i32 count, u8 r, u8 g, u8 b, u32 flags) {
    for (i32 i = start; i < start + count; i++) {
        m_cacheA[i * 4 + 0] = r;
        m_cacheA[i * 4 + 1] = g;
        m_cacheA[i * 4 + 2] = b;
    }
    i32 hr = m_palette->SetEntries(flags, start, count, reinterpret_cast<LPPALETTEENTRY>((m_cacheA + start * 4)));
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x2a3, hr);
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
// @early-stop
// timing-loop scheduling tail (~97%; permuter no gain): the snapshot copy, the
// per-channel (target-cur)*t/duration lerp, the recompute-only-on-tick guard, the
// SetEntries/SetRange calls and the final RezFree are all byte-faithful. Residual is
// MSVC5's exact [esp+N] slot choices + register schedule across the rotated timing
// loop (the elapsed/prev/t0 live-range packing). Not source-steerable.
RVA(0x00147d50, 0x1d2)
void CDDPalette::FadeRange(i32 start, i32 count, i32 r, i32 g, i32 b, i32 durationMs) {
    i32 hr = m_palette->GetEntries(0, 0, 0x100, reinterpret_cast<LPPALETTEENTRY>(m_cacheA));
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x2c0, hr);
    }
    u8* snapshot = static_cast<u8*>(::operator new(0x400));
    for (i32 i = 0; i < 0x400; i += 4) {
        *reinterpret_cast<i32*>((snapshot + i)) = *reinterpret_cast<i32*>((m_cacheA + i));
    }
    i32 t0 = ::timeGetTime();
    i32 prev = 9;
    for (i32 t = 10; t < durationMs; t = ::timeGetTime() - t0) {
        if (t != prev) {
            for (i32 j = start; j < start + count; j++) {
                m_cacheA[j * 4 + 0] =
                    static_cast<u8>((((r & 0xff) - snapshot[j * 4 + 0]) * t / durationMs + snapshot[j * 4 + 0]));
                m_cacheA[j * 4 + 1] =
                    static_cast<u8>((((g & 0xff) - snapshot[j * 4 + 1]) * t / durationMs + snapshot[j * 4 + 1]));
                m_cacheA[j * 4 + 2] =
                    static_cast<u8>((((b & 0xff) - snapshot[j * 4 + 2]) * t / durationMs + snapshot[j * 4 + 2]));
            }
            m_palette->SetEntries(0, start, count, reinterpret_cast<LPPALETTEENTRY>((m_cacheA + start * 4)));
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
    i32 err = m_palette->GetEntries(0, 0, 0x100, reinterpret_cast<LPPALETTEENTRY>(m_cacheA));
    if (err) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x311, err);
    }
    m_firstColorIndex = start;
    m_colorCount = count;
    m_durationMs = durationMs;
    m_startTimeMs = timeGetTime();
    m_lastElapsedMs = -1;
    m_targetPalette = 0;
    m_fixedR = r;
    m_fixedG = g;
    m_fixedB = b;
    if (!m_sourcePalette) {
        m_sourcePalette = static_cast<u8*>(::operator new(0x400));
    }
    for (i32 i = 0; i < 0x400; i += 4) {
        *reinterpret_cast<i32*>((m_sourcePalette + i)) = *reinterpret_cast<i32*>((m_cacheA + i));
    }
    m_active = 1;
    Tick();
}

RVA(0x00147ff0, 0xa9)
void CDDPalette::StartFadeToPalette(i32 start, i32 count, u8* target, i32 durationMs) {
    if (m_active) {
        Flush();
    }
    i32 err = m_palette->GetEntries(0, 0, 0x100, reinterpret_cast<LPPALETTEENTRY>(m_cacheA));
    if (err) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x34b, err);
    }
    m_firstColorIndex = start;
    m_colorCount = count;
    m_durationMs = durationMs;
    m_startTimeMs = timeGetTime();
    m_targetPalette = target;
    m_lastElapsedMs = -1;
    if (!m_sourcePalette) {
        m_sourcePalette = static_cast<u8*>(::operator new(0x400));
    }
    for (i32 i = 0; i < 0x400; i += 4) {
        *reinterpret_cast<i32*>((m_sourcePalette + i)) = *reinterpret_cast<i32*>((m_cacheA + i));
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
                    m_cacheA[i * 4] =
                        static_cast<char>((static_cast<i32>(((static_cast<u32>(m_targetPalette[i * 4]) - static_cast<u32>(m_sourcePalette[i * 4]))
                                     * dt))
                               / m_durationMs))
                        + m_sourcePalette[i * 4];
                    m_cacheA[i * 4 + 1] = static_cast<char>((static_cast<i32>(((static_cast<u32>(m_targetPalette[i * 4 + 1])
                                                        - static_cast<u32>(m_sourcePalette[i * 4 + 1]))
                                                       * dt))
                                                 / m_durationMs))
                                          + m_sourcePalette[i * 4 + 1];
                    m_cacheA[i * 4 + 2] = static_cast<char>((static_cast<i32>(((static_cast<u32>(m_targetPalette[i * 4 + 2])
                                                        - static_cast<u32>(m_sourcePalette[i * 4 + 2]))
                                                       * dt))
                                                 / m_durationMs))
                                          + m_sourcePalette[i * 4 + 2];
                    i++;
                } while (i < m_firstColorIndex + m_colorCount);
            }
            m_palette->SetEntries(
                0,
                m_firstColorIndex,
                m_colorCount,
                reinterpret_cast<LPPALETTEENTRY>((m_cacheA + m_firstColorIndex * 4))
            );
        }
    } else {
        if (dt != static_cast<u32>(m_lastElapsedMs)) {
            i32 i = m_firstColorIndex;
            if (i < m_firstColorIndex + m_colorCount) {
                do {
                    m_cacheA[i * 4] =
                        static_cast<char>((static_cast<i32>(((static_cast<u32>(m_fixedR) - static_cast<u32>(m_sourcePalette[i * 4])) * dt))
                               / m_durationMs))
                        + m_sourcePalette[i * 4];
                    m_cacheA[i * 4 + 1] =
                        static_cast<char>((static_cast<i32>(((static_cast<u32>(m_fixedG) - static_cast<u32>(m_sourcePalette[i * 4 + 1])) * dt))
                               / m_durationMs))
                        + m_sourcePalette[i * 4 + 1];
                    m_cacheA[i * 4 + 2] =
                        static_cast<char>((static_cast<i32>(((static_cast<u32>(m_fixedB) - static_cast<u32>(m_sourcePalette[i * 4 + 2])) * dt))
                               / m_durationMs))
                        + m_sourcePalette[i * 4 + 2];
                    i++;
                } while (i < m_firstColorIndex + m_colorCount);
            }
            m_palette->SetEntries(
                0,
                m_firstColorIndex,
                m_colorCount,
                reinterpret_cast<LPPALETTEENTRY>((m_cacheA + m_firstColorIndex * 4))
            );
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
    u8* v = m_targetPalette;
    m_active = 0;
    if (v != 0) {
        SetAndNotify(m_firstColorIndex, m_colorCount, reinterpret_cast<i32*>(v), 0);
        m_targetPalette = 0;
    } else {
        char buf[8];
        *reinterpret_cast<i32*>(buf) = *reinterpret_cast<i32*>(&m_fixedR);
        SetRange(
            m_firstColorIndex,
            m_colorCount,
            *reinterpret_cast<i32*>(buf),
            *reinterpret_cast<i32*>((buf + 1)),
            *reinterpret_cast<i32*>((buf + 2)),
            0
        );
    }
}

// CDDPalette::BlendRange (0x1482c0, __thiscall, ret 0x18 => 6 args). Blend each
// entry in [start,start+count) pct% (0..100) toward the solid color (r,g,b) in a
// single pass and push it straight to the DirectDraw palette via SetEntries.
// @early-stop
// regalloc / arg-slot wall (~94%): logic/CFG/the (target-cur)*pct/100 magic-divide
// blend/the SetEntries+assert tail are byte-faithful. Retail masks r,g,b in place into
// their arg stack slots (reused) after the loop guard + keeps the cache byte in bl;
// this C spelling hoists the masks into fresh temps (sub esp,0xc) + spills the byte.
// The in-place-mask / do-while restructurings scored strictly lower. Not steerable.
RVA(0x001482c0, 0x2e9)
void CDDPalette::BlendRange(i32 pct, i32 start, i32 count, i32 r, i32 g, i32 b) {
    for (i32 i = start; i < start + count; i++) {
        u8 cr = m_cacheA[i * 4 + 0];
        m_cacheA[i * 4 + 0] = static_cast<u8>((((r & 0xff) - cr) * pct / 100 + cr));
        u8 cg = m_cacheA[i * 4 + 1];
        m_cacheA[i * 4 + 1] = static_cast<u8>((((g & 0xff) - cg) * pct / 100 + cg));
        u8 cb = m_cacheA[i * 4 + 2];
        m_cacheA[i * 4 + 2] = static_cast<u8>((((b & 0xff) - cb) * pct / 100 + cb));
    }
    i32 hr = m_palette->SetEntries(0, start, count, reinterpret_cast<LPPALETTEENTRY>((m_cacheA + start * 4)));
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x406, hr);
    }
}

// CDDPalette::CaptureSystemPalette (0x1485b0, __thiscall; the ex-DirPal view) -
// snapshot the Windows system-reserved palette entries (the low + high halves GDI
// keeps for the shell) into the working cache (m_cacheA), then install them.
// @early-stop
// reloc-typing scoring artifact (~68%, was a 1.6% `return 0` stub). The CODE
// BYTES match retail instruction-for-instruction (verified base-vs-target with
// llvm-objdump -dr: prologue, both GDI-reserved copy loops - cl's strength-
// reduced single-induction/base-biased pointer walk - and the install/error
// tail are all byte-identical). The residual is only that retail reaches GDI
// through its own fn-ptr table (PTR_CreateDCA_006c3e20 etc.) while our base
// emits `ff 15 [__imp_*]`: same `ff 15` DIR32 form, differently-classed reloc
// target, so objdiff scores the ~9 relocated call sites as fuzzy. Routing
// through the named game globals instead scored LOWER (65%), confirming this is
// reloc-typing, not a codegen miss.
RVA(0x001485b0, 0x162)
i32 CDDPalette::CaptureSystemPalette() {
    HDC hdc = CreateDCA("DISPLAY", 0, 0, 0);
    if (!hdc) {
        return 0;
    }
    i32 sizePal = GetDeviceCaps(hdc, SIZEPALETTE);
    i32 half = GetDeviceCaps(hdc, NUMRESERVED) / 2;
    LogPal256 lp;
    lp.palVersion = 0x300;
    lp.palNumEntries = 0x100;
    if (!GetSystemPaletteEntries(hdc, 0, half, lp.palPalEntry)) {
        return 0;
    }
    if (!GetSystemPaletteEntries(
            hdc,
            sizePal - half,
            half,
            &lp.palPalEntry[lp.palNumEntries - half]
        )) {
        return 0;
    }
    DeleteDC(hdc);
    PALETTEENTRY* dest = reinterpret_cast<PALETTEENTRY*>(m_cacheA);
    if (!dest) {
        return 0;
    }
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
    i32 rc = SetAndNotify(0, 0x100, reinterpret_cast<i32*>(dest), 0);
    if (rc != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x495, rc);
        return 0;
    }
    return 1;
}

// BlackoutSystemPalette (0x148720, __cdecl) - build an all-black 256-entry
// PC_NOCOLLAPSE logical palette, select+realize it into the screen DC (driving
// the hardware DAC to black), then restore the previous palette and delete the
// black one. Returns 1 on success, 0 if the DC or palette could not be obtained.
// @early-stop
// reloc-typing scoring artifact (~99%): the CODE BYTES are byte-exact (register/stack
// layout, the palette-build loop, the cached-SelectPalette `call edi`, the tail-merged
// failure paths all match). Residual is only that objdiff scores the ~7 `ff 15` GDI
// calls fuzzy - they route through the cached fn-ptr globals (::GetDC etc.) vs retail's
// differently-classed reloc target. Same family as CDDPalette::CaptureSystemPalette.
RVA(0x00148720, 0x117)
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

SIZE_UNKNOWN(LogPal256);
