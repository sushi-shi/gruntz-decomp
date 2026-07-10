// DDPalette.cpp - CDDPalette (C:\Proj\DDrawMgr\DIRPAL.CPP), the DirectDraw palette
// wrapper. Split out of the DirectDrawMgr god-TU: one contiguous retail .text block
// (0x147390-0x1485a9) == one retail .obj (DIRPAL). The palette loaders open files
// through the engine's MFC-derived CFileIO, so DIRPAL.CPP was compiled /GX (the "eh"
// profile): LoadBmp/LoadPcx/LoadPal carry real C++ EH frames for their stack CFileIO
// (this is why the god-TU's "base" profile capped them). <Io/FileStream.h> pulls
// <Mfc.h> first (afx brings <windows.h> the controlled way) before <ddraw.h>.
// CDDPalette's ctor is inline in DDrawPtrCollections.cpp (emission anchor).
//
// Locals are placeholders; the switch/loader dispatch, the DIRPAL.CPP line numbers
// in the GetErrorString (file, line, hr) tuples, and the code bytes are load-bearing.
#include <Io/FileStream.h>

#include <DDrawMgr/DirectDrawMgr.h>
#include <ddraw.h> // real DirectDraw SDK (IDirectDraw2, IDirectDrawPalette, LPPALETTEENTRY)
#include <rva.h>
#include <stdio.h>
#include <string.h>  // strrchr / _stricmp / inline memcpy
#include <Globals.h> // g_DirectDrawMgr (the singleton whose device the notify waits on)

#define DIRPAL_FILE "C:\\Proj\\DDrawMgr\\DIRPAL.CPP"

// The Rez heap allocator/free + operator new (reloc-masked engine leaves).
extern "C" void* RezAlloc(unsigned int); // 0x1b9b46
extern "C" void RezFree(void* p);        // 0x1b9b82
void* operator new(u32);                 // engine allocator (reloc-masked rel32)

// The cached frame-clock fn-ptr (retail _g_pTimeGetTime @ 0x6c4650); FadeRange times
// through `call ds:[0x6c4650]`, NOT the WINMM import.
extern "C" u32(WINAPI* g_pTimeGetTime)(); // 0x6c4650

// The three file-extension literals the LoadFromFile dispatcher stricmp-ladders over
// (reloc-masked .rdata globals; .BMP/.PCX share the Image.cpp addresses). File-scope so
// each `push OFFSET` matches the binary's direct-address push.
static const char s_extBmp[] = ".BMP";
static const char s_extPcx[] = ".PCX";
static const char s_extPal[] = ".PAL";

// CDDPalette::Create (__thiscall, ret 0xc => 3 args). Caches a copy of the
// PALETTEENTRY array (m_cacheA), allocates a second cache (m_cacheB), then creates the
// DirectDraw palette via IDirectDraw::CreatePalette into m_palette.
RVA(0x00147390, 0x78)
i32 CDDPalette::Create(IDirectDraw2* dd, void* entries, u32 flags) {
    m_cacheA = (u8*)operator new(0x400);
    // Plateau note: byte-for-byte except the copy loop's SIB base/index roles
    // (retail encodes [entries+i]/[m_cacheA+i] with i as the index; MSVC here makes i
    // the base) - a 1-byte-per-insn encoding choice, semantically identical.
    for (i32 i = 0; i < 0x400; i += 4) {
        *(i32*)(m_cacheA + i) = *(i32*)((char*)entries + i);
    }
    m_cacheB = (u8*)operator new(0x400);
    i32 hr = dd->CreatePalette(flags, (LPPALETTEENTRY)entries, &m_palette, 0);
    if (hr == 0) {
        return 1;
    }
    CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x4b, hr);
    return 0;
}

// CDDPalette::LoadDefault (0x1479e0) is the no-extension / unresolved-extension
// fallback loader - a separate DIRPAL.CPP method defined in another base obj. It
// is declared on the class (header) but NOT defined here, so the dispatcher's
// tail call to it reloc-masks (resolved by the engine_label_stubs unit).

// CDDPalette::LoadFromFile (__thiscall, ret 0xc => 3 args). Pick the palette
// loader by file extension: take ext = strrchr(filename,'.') then a stricmp
// ladder on .BMP/.PCX/.PAL, forwarding (dd, filename, flags) verbatim to the
// matching loader; no/unresolved extension -> LoadDefault. Same idiom as
// CImage::LoadFromRez. Each branch re-tests `ext != 0` (the target's per-case
// `test esi; je default`).
RVA(0x00147410, 0xbc)
i32 CDDPalette::LoadFromFile(IDirectDraw2* dd, char* filename, u32 flags) {
    char* ext = strrchr(filename, '.');
    if (ext && _stricmp(ext, s_extBmp) == 0) {
        return LoadBmp(dd, filename, flags);
    } else if (ext && _stricmp(ext, s_extPcx) == 0) {
        return LoadPcx(dd, filename, flags);
    } else if (ext && _stricmp(ext, s_extPal) == 0) {
        return LoadPal(dd, filename, flags);
    }
    return LoadDefault(dd, filename, flags);
}

// CDDPalette::CreateRGB (__thiscall, ret 0xc => 3 args). Takes a packed 256x3
// RGB-triplet array, expands it on the stack into PALETTEENTRY[256] (peFlags=0),
// then forwards to Create. The 0x400-byte stack buffer drives `sub esp,0x400`.
RVA(0x001474d0, 0x60)
i32 CDDPalette::CreateRGB(IDirectDraw2* dd, void* rgb, u32 flags) {
    u8 entries[0x400];
    u8* src = (u8*)rgb;
    for (i32 i = 0; i < 0x100; i++) {
        entries[i * 4 + 0] = src[0];
        entries[i * 4 + 1] = src[1];
        entries[i * 4 + 2] = src[2];
        entries[i * 4 + 3] = 0;
        src += 3;
    }
    return Create(dd, entries, flags);
}

// CDDPalette::Destroy (__thiscall, no args). Nulls m_0/m_palette/m_8, frees the three
// owned buffers (m_cacheA/m_cacheB/m_18) via operator delete, clears m_34. m_palette is
// only nulled (not Released) here.
RVA(0x00147530, 0x54)
void CDDPalette::Destroy() {
    m_0 = 0;
    m_8 = 0;
    if (m_palette != 0) {
        m_palette = 0;
    }
    if (m_cacheA != 0) {
        operator delete(m_cacheA);
        m_cacheA = 0;
    }
    if (m_cacheB != 0) {
        operator delete(m_cacheB);
        m_cacheB = 0;
    }
    if (m_18 != 0) {
        operator delete(m_18);
        m_18 = 0;
    }
    m_34 = 0;
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
    CFileIO file;
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
    CFileIO file;
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
    u8* src = (u8*)data + size - 0x300;
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
    CFileIO file;
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
    i32* cache = (i32*)m_cacheA;
    for (i32 i = 0; i < count; i++) {
        cache[start + i] = data[i];
    }
    if (g_DirectDrawMgr != 0) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    return m_palette->SetEntries(0, start, count, (LPPALETTEENTRY)data);
}

// CDDPalette::SetEntriesQuad (0x147b10, __thiscall, ret 0x10 => 4 args). Allocate
// a count*4 working buffer, expand `count` RGBQUAD entries (R/B swapped, as in
// LoadBmp) into PALETTEENTRY, SetAndNotify the range, free, return the HRESULT.
RVA(0x00147b10, 0x8b)
i32 CDDPalette::SetEntriesQuad(i32 start, i32 count, u8* quads, i32 a4) {
    u8* buf = (u8*)RezAlloc(count * 4);
    if (buf == 0) {
        return 0x80070057;
    }
    for (i32 i = 0; i < count; i++) {
        buf[i * 4 + 0] = quads[i * 4 + 2];
        buf[i * 4 + 1] = quads[i * 4 + 1];
        buf[i * 4 + 2] = quads[i * 4 + 0];
        buf[i * 4 + 3] = 0;
    }
    i32 hr = SetAndNotify(start, count, (i32*)buf, a4);
    RezFree(buf);
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
    u8* buf = (u8*)RezAlloc(count * 4);
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
    i32 hr = SetAndNotify(start, count, (i32*)buf, a4);
    RezFree(buf);
    return hr;
}

// CDDPalette::GetEntries (__thiscall, ret 0 => no args). Lazily allocates the
// readback cache (m_cacheB), then reads all 256 entries; reports a bad HRESULT.
RVA(0x00147c30, 0x4d)
i32 CDDPalette::GetEntries() {
    // Lazily allocates the readback cache, then reads all 256 entries; reports a
    // bad HRESULT. Plateau note: retail's body falls off the end (no return -
    // the int symbol returns whatever sits in eax), so it omits the trailing
    // `xor eax,eax` this `return 0` emits. MSVC 5.0 C++ forbids fall-off
    // (C2561), so the one-instruction `xor` gap can't be reproduced from clean C.
    if (m_cacheB == 0) {
        m_cacheB = (u8*)operator new(0x400);
        if (m_cacheB == 0) {
            return 0;
        }
    }
    i32 hr = m_palette->GetEntries(0, 0, 0x100, (LPPALETTEENTRY)m_cacheB);
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
        *(i32*)(m_cacheA + i) = *(i32*)(readback + i);
    }
    if (g_DirectDrawMgr != 0) {
        IDirectDraw2* dd = g_DirectDrawMgr->m_device;
        dd->WaitForVerticalBlank(1, 0);
    }
    m_palette->SetEntries(0, 0, 0x100, (LPPALETTEENTRY)readback);
}

// CDDPalette::SetRange (__thiscall, ret 0x18 => 6 args).
RVA(0x00147cd0, 0x78)
i32 CDDPalette::SetRange(i32 start, i32 count, u8 r, u8 g, u8 b, u32 flags) {
    for (i32 i = start; i < start + count; i++) {
        m_cacheA[i * 4 + 0] = r;
        m_cacheA[i * 4 + 1] = g;
        m_cacheA[i * 4 + 2] = b;
    }
    i32 hr = m_palette->SetEntries(flags, start, count, (LPPALETTEENTRY)(m_cacheA + start * 4));
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
// frame clock is the cached g_pTimeGetTime fn-ptr.
// @early-stop
// timing-loop scheduling tail (~97%; permuter no gain): the snapshot copy, the
// per-channel (target-cur)*t/duration lerp, the recompute-only-on-tick guard, the
// SetEntries/SetRange calls and the final RezFree are all byte-faithful. Residual is
// MSVC5's exact [esp+N] slot choices + register schedule across the rotated timing
// loop (the elapsed/prev/t0 live-range packing). Not source-steerable.
RVA(0x00147d50, 0x1d2)
void CDDPalette::FadeRange(i32 start, i32 count, i32 r, i32 g, i32 b, i32 durationMs) {
    i32 hr = m_palette->GetEntries(0, 0, 0x100, (LPPALETTEENTRY)m_cacheA);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x2c0, hr);
    }
    u8* snapshot = (u8*)RezAlloc(0x400);
    for (i32 i = 0; i < 0x400; i += 4) {
        *(i32*)(snapshot + i) = *(i32*)(m_cacheA + i);
    }
    i32 t0 = g_pTimeGetTime();
    i32 prev = 9;
    for (i32 t = 10; t < durationMs; t = g_pTimeGetTime() - t0) {
        if (t != prev) {
            for (i32 j = start; j < start + count; j++) {
                m_cacheA[j * 4 + 0] =
                    (u8)(((r & 0xff) - snapshot[j * 4 + 0]) * t / durationMs + snapshot[j * 4 + 0]);
                m_cacheA[j * 4 + 1] =
                    (u8)(((g & 0xff) - snapshot[j * 4 + 1]) * t / durationMs + snapshot[j * 4 + 1]);
                m_cacheA[j * 4 + 2] =
                    (u8)(((b & 0xff) - snapshot[j * 4 + 2]) * t / durationMs + snapshot[j * 4 + 2]);
            }
            m_palette->SetEntries(0, start, count, (LPPALETTEENTRY)(m_cacheA + start * 4));
        }
        prev = t;
    }
    SetRange(start, count, r, g, b, 0);
    RezFree(snapshot);
}

// ===========================================================================
// CDDPalette::Flush (0x148250, re-homed from BoundaryTail) - flush a pending
// blit: if nothing pending (m_34==0) return; clear the pending flag; when a fill
// color m_14 is set dispatch the solid blit SetAndNotify(m_2c,m_30,m_14,0) and
// clear m_14; otherwise dispatch the keyed blit SetRange passing m_1c plus its
// byte-shifted views (the engine re-reads the packed color at +1/+2 byte offsets
// through an 8-byte stack temp). __thiscall.
// ===========================================================================
RVA(0x00148250, 0x61)
void CDDPalette::Flush() {
    if (m_34 == 0) {
        return;
    }
    i32 v = m_14;
    m_34 = 0;
    if (v != 0) {
        SetAndNotify(m_2c, m_30, (i32*)v, 0);
        m_14 = 0;
    } else {
        char buf[8];
        *(i32*)buf = m_1c;
        SetRange(m_2c, m_30, *(i32*)buf, *(i32*)(buf + 1), *(i32*)(buf + 2), 0);
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
        m_cacheA[i * 4 + 0] = (u8)(((r & 0xff) - cr) * pct / 100 + cr);
        u8 cg = m_cacheA[i * 4 + 1];
        m_cacheA[i * 4 + 1] = (u8)(((g & 0xff) - cg) * pct / 100 + cg);
        u8 cb = m_cacheA[i * 4 + 2];
        m_cacheA[i * 4 + 2] = (u8)(((b & 0xff) - cb) * pct / 100 + cb);
    }
    i32 hr = m_palette->SetEntries(0, start, count, (LPPALETTEENTRY)(m_cacheA + start * 4));
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString(DIRPAL_FILE, 0x406, hr);
    }
}
