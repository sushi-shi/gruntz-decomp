#include <Mfc.h>         // CPtrList / POSITION / CFile + <windows.h> PALETTEENTRY
#include <Image/Image.h> // CRezImage - the shared DIB-surface class (the pool's surface node)
#include <Image/ImagePaletteNode.h> // the canonical CImagePaletteNode (this TU owns most bodies)
#include <Image/ImagePool.h>        // the canonical CImagePool (this TU owns its bodies)
#include <Rez/RezMgr.h>             // RezAlloc/RezFree (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82)
#include <rva.h>
#include <string.h>
#include <DDrawMgr/DDSurface.h>     // PidHeader - .PID and .RID share the header
#include <Image/FileImageRecords.h> // the real on-disk PcxHeader

DATA(0x002bf6e0)
HINSTANCE g_hResModule = 0; // 0x6bf6e0

namespace ApiCallerStubs {
    // The palette list node CImagePaletteNode is the ONE canonical class in
    // <Image/ImagePaletteNode.h> (included above): this TU owns its Build/ProcessPal*/
    // Parse*/Load* bodies (below); PaletteBmp.cpp owns LoadBmpFile/Apply. The former
    // identical .cpp-local definition here is DISSOLVED onto that header (2026-07-14).

    // Two free GDI palette helpers PalBuilder::Build/Tune funnel through:
    // 0x1770a0 probes display-palette support; 0x177160 resets the screen
    // palette to all-black. __cdecl.
} // namespace ApiCallerStubs

DATA(0x0021aabc)
char g_bmpHeaderTemplate[4] = "BM"; // 0x61aabc  = 42 4d 00 00

RVA(0x00174e90, 0x1c)
i32 CImagePool::SetHandles(HINSTANCE resModule, HWND src, i32 c) {
    m_resourceModuleHandle = resModule;
    m_sourceHwnd = src;
    m_08 = c;
    return 1;
}

RVA(0x00174eb0, 0x1b)
void CImagePool::Clear() {
    ClearSurfaces();
    ClearPalettes();
    m_resourceModuleHandle = 0;
    m_sourceHwnd = 0;
    m_08 = 0;
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
    m_48 = 0;
}

// ===========================================================================
// The five surface-node factories @0x174fe0/0x1750e0/0x1751f0/0x1752f0/0x1753f0.
// Each GetDC's the pool HWND (+0x04), RezAlloc's a 0x45c-byte surface node and
// zeroes its handle/dim/POSITION block, then forwards to one foreign decoder; on
// success AddTail's the node onto the surface list (+0x10) caching the POSITION at
// node+0x44c; either way it restores the selected palette (+0x0c) and ReleaseDC's,
// returning the node (or, on decode failure, Free()'ing + RezFree'ing it -> 0).
//
// All five sit at ~96% on one regalloc tie-break: retail enregisters the node in
// edi and the zero constant in ebx, the recompile swaps them (node=ebx/zero=edi),
// which also flips the epilogue `mov eax,node` placement. Verified byte-identical
// after canonicalizing edi<->ebx (llvm-objdump base vs target). Same register-
// assignment wall class as the palette siblings below; not source-steerable
// (tried node=0 pre-init -> 94%; and 2026-07-28, byte-NEUTRAL: modelling the seed as
// a real `new CRezImage()` ctor instead of the spelled-out `::operator new(0x45c)` +
// 11 stores + hand-written null guard, which is what
// docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md prescribes - the ctor form
// is the correct model so it is KEPT, but it does not move the edi/ebx tie-break).
// ===========================================================================
// @early-stop
// regalloc tie-break: node should be edi / zero should be ebx (retail); recompile
// swaps the two callee-saved regs. Code byte-identical otherwise.
RVA(0x00174fe0, 0xfe)
CRezImage* CImagePool::AddSurfaceBmp(i32 width, i32 height, i32 bitCount, i32 flag) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DecodeBmpHeader(static_cast<void*>(hdc), width, height, bitCount, flag) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = 0;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail(node);
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = 0;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// @early-stop
// regalloc tie-break: node<->edi / zero<->ebx swap vs retail (see AddSurfaceBmp).
RVA(0x001750e0, 0x103)
CRezImage* CImagePool::AddSurfaceBlit(void* src, i32 width, i32 height, i32 bitCount, i32 flag) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DecodeBlit(src, static_cast<void*>(hdc), width, height, bitCount, flag) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = 0;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail(node);
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = 0;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// @early-stop
// regalloc tie-break: node<->edi / zero<->ebx swap vs retail (see AddSurfaceBmp).
RVA(0x001751f0, 0xf9)
CRezImage* CImagePool::AddSurfaceOp(void* buf, i32 kind, i32 ctrl) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->DispatchDecode(buf, kind, static_cast<void*>(hdc), ctrl) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = 0;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail(node);
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = 0;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// @early-stop
// regalloc tie-break: node<->edi / zero<->ebx swap vs retail (see AddSurfaceBmp).
RVA(0x001752f0, 0xfc)
CRezImage* CImagePool::AddSurfaceRez(char* name, i32 ctrl) {
    HDC hdc = GetDC(m_sourceHwnd);
    g_hResModule = m_resourceModuleHandle;
    CRezImage* node = new CRezImage();
    if (node->LoadFromRez(name, static_cast<void*>(hdc), ctrl) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = 0;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail(node);
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = 0;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// @early-stop
// regalloc tie-break: node<->edi / zero<->ebx swap vs retail (see AddSurfaceBmp).
RVA(0x001753f0, 0xf4)
CRezImage* CImagePool::AddSurfaceConvert(CRezImage* src, void* pal) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node = new CRezImage();
    if (node->Convert8To16(static_cast<void*>(hdc), src, pal) == 0) {
        if (m_selectedPalette) {
            SelectPalette(hdc, m_selectedPalette, FALSE);
            m_selectedPalette = 0;
        }
        ReleaseDC(m_sourceHwnd, hdc);
        if (node) {
            node->Free();
            ::operator delete(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail(node);
    if (m_selectedPalette) {
        SelectPalette(hdc, m_selectedPalette, FALSE);
        m_selectedPalette = 0;
    }
    ReleaseDC(m_sourceHwnd, hdc);
    return node;
}

// ===========================================================================
// The four palette-node factories @0x1754f0/0x175570/0x1755f0/0x175680. Each
// RezAlloc's a 0x414-byte node, zeroes its handle/POSITION header, runs the
// matching parse front-end, and on success AddTail's it onto the +0x2c list
// (caching the POSITION at node+0x410); on parse failure it deletes + frees the
// node and returns 0.
// ===========================================================================
// @early-stop
// this-register regalloc wall (~99%): every instruction matches except retail
// pins `this` in edi while the recompile picks ebx (perturbing the prologue
// pushes + the AddTail `lea 0x2c(this)`). The sibling AddImageFile reaches 100%
// only because its early m_resourceModuleHandle read pins `this`=edi; with `this` first used at the
// tail there is no source lever. Logic byte-identical.
// Also tried (2026-07-28) and byte-NEUTRAL: modelling the node seed as a real
// `new CImagePaletteNode()` ctor instead of the spelled-out `::operator new(0x414)` +
// three stores + hand-written null guard, which is what
// docs/patterns/ctor-vptr-interleave-vs-spelled-out-init.md prescribes for exactly this
// wrong-`this`-role symptom. It does not move the pin - but the ctor form is the correct
// model, so it is KEPT (the spelled-out construction is gone from all four factories).
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
    node->m_listPosition = m_palettes.AddTail(node);
    return node;
}

// @early-stop
// this-register regalloc wall (~99%): same as AddPaletteEntries (this in ebx vs
// retail edi). Logic byte-identical.
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
    node->m_listPosition = m_palettes.AddTail(node);
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
    node->m_listPosition = m_palettes.AddTail(node);
    return node;
}

// @early-stop
// this-register regalloc wall (~99%): same as AddPaletteEntries (this in ebx vs
// retail edi). Logic byte-identical.
RVA(0x00175680, 0x85)
CImagePaletteNode* CImagePool::AddImageDispatch(void* buf, u32 size, i32 type, i32 ctrl) {
    CImagePaletteNode* node = new CImagePaletteNode();
    if (node->ParseDispatch(buf, size, type, ctrl) == 0) {
        if (node) {
            node->Run();
            ::operator delete(node);
        }
        return 0;
    }
    node->m_listPosition = m_palettes.AddTail(node);
    return node;
}

RVA(0x00175710, 0x69)
i32 CImagePool::EnsureSurface(CRezImage* img, i32 w, i32 h, i32 bitCount, i32 flag) {
    if (img == 0) {
        return 0;
    }
    HDC dc = ::GetDC(m_sourceHwnd);
    i32 result = img->EnsureSize(dc, w, h, bitCount, flag);
    if (m_selectedPalette) {
        ::SelectPalette(dc, m_selectedPalette, FALSE);
        m_selectedPalette = 0;
    }
    ::ReleaseDC(m_sourceHwnd, dc);
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
i32 CRezImage::DecodeBmpHeader(void* a2, i32 width, i32 height, i32 bitcount, i32 a3) {
    m_434 = 0;
    m_width = width;
    m_height = (height < 0) ? -height : height;
    m_bitCount = bitcount;
    if (bitcount == 8) {
        m_stride = ((width + 3) / 4) * 4;
    } else {
        m_stride = width;
    }
    m_rowPad = m_stride - width;
    m_paletteScalar = 0;
    m_paletteNode = 0;
    m_transparent = 1;
    memset(&m_bih, 0, sizeof(BITMAPINFOHEADER));
    m_bih.biWidth = m_width;
    m_bih.biBitCount = static_cast<WORD>(m_bitCount);
    m_bih.biSize = sizeof(BITMAPINFOHEADER);
    m_bih.biHeight = height;
    m_bih.biPlanes = 1;
    m_bih.biCompression = 0;
    m_bih.biSizeImage = 0;
    m_bih.biClrUsed = 0;
    m_bih.biClrImportant = 0;
    // `pal` is a real dev local: retail computes `lea ecx,[esi+0x28]` BEFORE the
    // bitcount branch (a sunk `&m_pal[0]` would land inside it), and walks it with a
    // post-increment (`mov [ecx],ax; add ecx,2; inc eax`) instead of the indexed
    // strength-reduction `m_pal[i]` produces.
    u16* pal = m_pal;
    if (m_bitCount == 8) {
        for (i32 i = 0; i < 256; i++) {
            *pal++ = static_cast<u16>(i);
        }
        m_dibSection = CreateDIBSection(
            static_cast<HDC>(a2),
            // API-forced: BITMAPINFO == BITMAPINFOHEADER + the colour table that
            // follows it in the same buffer (m_bih at +0x00, m_pal at +0x28)
            reinterpret_cast<BITMAPINFO*>(&m_bih),
            DIB_PAL_COLORS,
            // API-forced: CreateDIBSection's ppvBits out-param is void**
            reinterpret_cast<void**>(&m_pixels),
            0,
            0
        );
    } else {
        m_dibSection = CreateDIBSection(
            static_cast<HDC>(a2),
            // API-forced: BITMAPINFO == BITMAPINFOHEADER + the colour table that
            // follows it in the same buffer (m_bih at +0x00, m_pal at +0x28)
            reinterpret_cast<BITMAPINFO*>(&m_bih),
            DIB_RGB_COLORS,
            // API-forced: CreateDIBSection's ppvBits out-param is void**
            reinterpret_cast<void**>(&m_pixels),
            0,
            0
        );
    }
    if (!m_dibSection) {
        return 0;
    }
    m_rowOffsets = static_cast<i32*>(::operator new(m_height * 4));
    for (i32 i = 0; i < m_height; i++) {
        m_rowOffsets[i] = (m_height - i - 1) * (m_bitCount / 8) * m_stride;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CRezImage::DecodeBlit - the shared plane blitter the format decoders
// call. (Re)allocate/decode the plane via DecodeBmpHeader (fail -> return 0),
// then copy `src` into it: contiguous rep-movs of (m_stride*m_height*bitcount)/8
// bytes when m_rowPad==0, else row-by-row through the m_rowOffsets table (each
// row m_width bytes from the running source).
// @early-stop
// shrink-wrapped callee-save push wall (~83%): body byte-identical otherwise. Retail
// saves only ebx/esi at entry and defers `push edi`/`push ebp` past the DecodeBmpHeader
// early-out (which restores just esi/ebx); cl pushes all four upfront. Not source-
// steerable; docs/patterns/shrink-wrapped-callee-save-push.md. Final sweep.
RVA(0x00175930, 0xc6)
i32 CRezImage::DecodeBlit(void* src, void* a2, i32 width, i32 height, i32 bitcount, i32 a3) {
    if (!DecodeBmpHeader(a2, width, height, bitcount, a3)) {
        return 0;
    }
    if (m_rowPad == 0) {
        memcpy(m_pixels, src, static_cast<u32>((m_stride * m_height * bitcount)) >> 3);
        return 1;
    }
    char* s = static_cast<char*>(src);
    for (i32 row = 0; row < m_height; row++) {
        memcpy(m_pixels + m_rowOffsets[row], s, m_width);
        s += m_width;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CRezImage::DispatchDecode - select one of four CRezImage format decoders keyed by
// `kind` (2..5 -> DecodePcxData/DecodeResData/DecodeRidData/DecodePidData), forwarding
// `this` (kept in ecx) plus (buf, a2, a3); unknown kind -> 0. __thiscall, ret 0x10.
// The 2nd stack arg is the selector (`this` is the ecx-passed surface node); the four
// pass-through args match the AddSurfaceOp call site (buf, kind, hdc, ctrl).
// @early-stop
// jump-table-data-overlap scoring artifact (docs/patterns/jumptable-data-overlap.md):
// retail's inline jump table is scored against our $L-symbol table; plus a per-case
// arg-forward schedule (regalloc). Dispatch + cases logically exact; relocs now align
// to the real CRezImage decoders.
RVA(0x00175a00, 0x74)
i32 CRezImage::DispatchDecode(void* buf, i32 kind, void* dc, i32 ctrl) {
    switch (kind) {
        case 2:
            return DecodePcxData(buf, dc, ctrl);
        case 3:
            return DecodeResData(buf, dc, ctrl);
        case 4:
            return DecodeRidData(buf, dc, ctrl);
        case 5:
            return DecodePidData(buf, dc, ctrl);
    }
    return 0;
}

RVA(0x00175a90, 0xee)
i32 CRezImage::LoadFromRez(char* name, void* a2, i32 a3) {
    char* ext = strrchr(name, '.');

    if (ext && _strcmpi(ext, ".BMP") == 0) {
        return LoadBmp(name, a2, a3);
    } else if (ext && _strcmpi(ext, ".PCX") == 0) {
        return LoadPcx(name, a2, a3);
    } else if (ext && _strcmpi(ext, ".RID") == 0) {
        return LoadRid(name, a2, a3);
    } else if (ext && _strcmpi(ext, ".PID") == 0) {
        return LoadPid(name, a2, a3);
    }

    return LoadDefault(name, a2, a3);
}

// ---------------------------------------------------------------------------
// Build a fresh 16bpp RGB555 copy of the 8bpp `src` surface through the
// `pal` 256-entry RGB table (8 bytes in). Returns TRUE on success.
// @early-stop
// regalloc wall: retail pins `palette` in ebp across the whole function while our
// recompile spills it to the stack and reloads it in the inner loop; that cascades
// into different register encodings throughout. Conversion logic is byte-faithful.
RVA(0x00175b80, 0x105)
i32 CRezImage::Convert8To16(void* dc, CRezImage* src, void* pal) {
    if (pal == 0) {
        return 0;
    }
    u32* palette = (static_cast<ScanlinePalette*>(pal))->m_colors;
    if (palette == 0) {
        return 0;
    }
    if (!DecodeBmpHeader(dc, src->m_width, src->m_height, 0x10, 0)) {
        return 0;
    }
    for (i32 y = 0; y < m_height; y++) {
        u8* sp = src->m_pixels + y * src->m_stride;
        // byte-forced: m_pixels is the 8bpp byte plane, but DecodeBmpHeader(...,0x10,...)
        // above just reconfigured `this` to 16bpp, so the same member now carries u16
        // pixels at the m_stride*2 byte pitch (retail `lea edx,[edi+edx*2]`).
        u16* dp = reinterpret_cast<u16*>((m_pixels + y * m_stride * 2));
        for (i32 x = 0; x < m_width; x++) {
            u32 c = palette[*sp];
            u32 r = c & 0xff;
            u32 g = (c >> 8) & 0xff;
            u32 b = (c >> 16) & 0xff;
            *dp = static_cast<u16>((((((r & 0xf8) << 5) | (g & 0xf8)) << 2) | (b >> 3)));
            dp++;
            sp++;
        }
    }
    return 1;
}

RVA(0x00175c90, 0x45)
void CRezImage::Free() {
    if (m_dibSection) {
        DeleteObject(m_dibSection);
        m_dibSection = 0;
    }
    if (m_rowOffsets) {
        ::operator delete(m_rowOffsets);
        m_rowOffsets = 0;
    }
    m_pixels = 0;
    m_paletteNode = 0;
}

RVA(0x00175ce0, 0x6b)
i32 CRezImage::EnsureSize(void* dc, i32 w, i32 h, i32 bitCount, i32 flag) {
    if (m_dibSection && m_pixels && m_rowOffsets && m_width == w && m_height == h) {
        return 1;
    }
    Free();
    return DecodeBmpHeader(dc, w, h, bitCount, flag);
}

// ---------------------------------------------------------------------------
// Fill every pixel with the low byte of `value`. Contiguous buffers
// (m_rowPad == 0) get one flat fill; padded buffers fill row by row.
// The `value &= 0xff` is REAL dev source, not a redundancy: retail loads the full
// dword and masks (`mov eax,[esp+0x14]; and eax,0xff`) where a bare memset arg would
// just byte-load (`mov al,[esp+0x14]`), and the padded path writes the masked value
// BACK to the parameter's home slot before the loop - the signature of an assignment
// to the parameter that MSVC sinks into each use block.
// @early-stop
// 94.1%, two residues, both compiler-canonicalisation: (1) the size multiply -
// cl loads the LOWER-offset operand (`mov ecx,[m_height]; imul ecx,[m_stride]`)
// whichever way the source spells it (both operand orders tried, identical output),
// retail loads m_stride; (2) the loop preheader's mask - cl picks the memory RMW
// `and [esp+0x14],0xff` where retail computes it in eax and `jmp`s past the body's
// reload; a `masked` temp for the value is folded back into the RMW.
RVA(0x00175d50, 0xad)
void CRezImage::Fill(i32 value) {
    if (m_rowPad == 0) {
        value &= 0xff;
        memset(m_pixels, value, m_stride * m_height);
    } else {
        // The mask sits in the loop PREHEADER (retail: after the zero-trip guard, once,
        // written back to the parameter's home slot at [esp+0x14]), which is what the
        // guard + do/while spelling produces; a plain `for` with the mask above it
        // folds the mask into the guard block instead.
        i32 y = 0;
        if (y < m_height) {
            value &= 0xff;
            do {
                memset(m_pixels + m_rowOffsets[y], value, m_width);
                y++;
            } while (y < m_height);
        }
    }
}

// @early-stop
// 96.7%: instruction-for-instruction identical (the header field READ order was the
// real bug and is fixed - retail loads biHeight before biWidth). Residual is the
// two-register role swap `buf`/`bitcount`: cl puts buf in eax and the movzx zero in
// edx, retail the reverse, which also flips the `lea esi,[buf+biSize+0x400]` SIB
// base/index. Tried: reading bitcount first through an inline cast, declaring it
// last, reordering width/height - the role assignment does not move.
RVA(0x00175e00, 0x3d)
i32 CRezImage::DecodeResData(void* buf, void* a2, i32 a3) {
    BITMAPINFOHEADER* ih = static_cast<BITMAPINFOHEADER*>(buf);
    i32 width = ih->biWidth;
    i32 height = ih->biHeight;
    i32 bitcount = ih->biBitCount;
    void* src = static_cast<u8*>(buf) + sizeof(BITMAPINFOHEADER) + 4; // header + 1 quad
    if (bitcount == 8) {
        src = static_cast<u8*>(buf) + ih->biSize + 0x400;
    }
    return DecodeBlit(src, a2, width, height, bitcount, a3);
}

// ---------------------------------------------------------------------------
// CRezImage::LoadBmp
// The .BMP loader: open the file, read the 14-byte BITMAPFILEHEADER and the
// 40-byte BITMAPINFOHEADER, hand the parsed (width, height, bitcount, a2, a3)
// to the decode helper that allocates the CRezImage's pixel plane, then Seek to
// bfOffBits and Read exactly (bitcount/8)*stride*height pixel bytes into the
// plane. Returns 1 on a full read, 0 on any I/O / decode failure. The CFile
// stack object's dtor runs on every exit -> the C++ EH frame.
RVA(0x00175e40, 0x1b3)
i32 CRezImage::LoadBmp(char* name, void* a2, i32 a3) {
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
    i32 bitcount = ih.biBitCount & 0xffff;
    if (!DecodeBmpHeader(a2, width, height, bitcount, a3)) {
        return 0;
    }

    file.Seek(fh.bfOffBits, 0);
    u32 size = (bitcount / 8) * m_stride * height;
    if (file.Read(m_pixels, size) != size) {
        return 0;
    }
    return 1;
}

RVA(0x00176000, 0x18f)
i32 CRezImage::DecodePcxData(void* buf, void* a2, i32 a3) {
    PcxHeader* hdr = static_cast<PcxHeader*>(buf);
    i32 width = hdr->m_xMax - hdr->m_xMin + 1;
    i32 height = hdr->m_yMax - hdr->m_yMin + 1;
    if (hdr->m_bitsPerPixel != 8) {
        return 0;
    }
    if (!DecodeBmpHeader(a2, width, height, static_cast<i8>(hdr->m_planes) * 8, a3)) {
        return 0;
    }

    u8* src = hdr->m_pixels; // the RLE stream at +0x80
    i32 scanBytes =
        (width * static_cast<i8>(hdr->m_planes) * static_cast<i8>(hdr->m_bitsPerPixel) + 7) / 8;
    u8* scan = static_cast<u8*>(::operator new(scanBytes));

    for (i32 y = 0; y < height; y++) {
        u8* dst = m_pixels + m_rowOffsets[y];
        i32 n = width * static_cast<i8>(hdr->m_planes);
        while (n > 0) {
            u8 c = *src++;
            if ((c & 0xc0) == 0xc0) {
                i32 count = c & 0x3f;
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

        if (static_cast<i8>(hdr->m_planes) == 1) {
            for (i32 x = width; x != 0; x--) {
                *dst++ = scan[x - 1];
            }
        } else if (static_cast<i8>(hdr->m_planes) == 3) {
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

    ::operator delete(scan);
    return 1;
}

RVA(0x00176190, 0x126)
i32 CRezImage::LoadPcx(char* name, void* a2, i32 a3) {
    CFile file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = ::operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    i32 result = DecodePcxData(buf, a2, a3);
    ::operator delete(buf);
    return result;
}

RVA(0x001762c0, 0x42)
i32 CRezImage::DecodeRidData(void* buf, void* a2, i32 a3) {
    // .RID shares .PID's 0x20-byte header: width/height at +0x08/+0x0c and the pixel
    // stream immediately after it. The old RID_HEADER_SIZE = 0x20 was just
    // sizeof(PidHeader) spelled again, so it is gone with the offset arithmetic; only
    // the pixel ENCODING differs between the two formats.
    PidHeader* hdr = static_cast<PidHeader*>(buf);
    i32 width = hdr->width;
    i32 height = hdr->height;
    // hdr->pixels, NOT `hdr + 1`: the trailing `u8 pixels[1]` pads sizeof(PidHeader)
    // to 0x24, so `hdr + 1` skipped 4 bytes too many (retail's cursor lands on +0x20).
    i32 ok = DecodeBlit(hdr->pixels, a2, width, height, 8, a3);
    if (!(a3 & 1)) {
        m_transparent = 0;
    }
    return ok;
}

RVA(0x00176310, 0x126)
i32 CRezImage::LoadRid(char* name, void* a2, i32 a3) {
    CFile file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = ::operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    i32 result = DecodeRidData(buf, a2, a3);
    ::operator delete(buf);
    return result;
}

// @early-stop
// 72.8% is CODEGEN RESIDUE, NOT A LOGIC BUG - do not go hunting for one. Both
// grammars were re-derived from the bytes and differentially tested against the
// 29,798 shipped PID resources (tools/gruntz-oracle): the flag test, the token
// grammars, the row-end rule and the fill semantics below all reproduce retail
// exactly. Two things here look like mistakes and are faithful:
//
//   * the 0xC0 arm does NOT clamp a run to the scanline. `n -= count` may go
//     negative and `dst += count` may spill into the next row - that is retail
//     (0x176646 `rep stos` of the full count, unclamped), and it is what makes
//     this decoder DIFFER from CDDSurface::RunDecode1 @0x145270, which clamps
//     and carries the remainder into the next row. The two consume a DIFFERENT
//     number of tokens once a run crosses a row, so they desynchronise for
//     good. No shipped sprite contains such a run, which is why nothing has
//     ever noticed. Do not "fix" this arm to match RunDecode1.
//   * the row advance is `x >= m_width` (0x176597 `cmp edx,[eax+0x438]`), NOT
//     `m_width - 1`. CDDrawShadeBlit::EncodeRle16 @0x149694 uses `width - 1` on
//     its own stream; see the note there before reconciling them.
//
// Residual is register/scheduling: retail keeps the row cursor and the token
// index in esi/ebp across both arms and re-derives `this` from the frame after
// each rep-stos, where cl spills them.
RVA(0x00176440, 0x25d)
i32 CRezImage::DecodePidData(void* buf, void* a2, i32 a3) {
    PidHeader* hdr = static_cast<PidHeader*>(buf);
    // the pixel stream is the header's trailing member (+0x20). NOT `hdr + 1`: the
    // `u8 pixels[1]` tail pads sizeof(PidHeader) to 0x24, so that spelling skipped
    // 4 bytes of stream.
    u8* src = hdr->pixels;
    i32 width = hdr->width;
    i32 height = hdr->height;
    i32 flags = hdr->flags;
    i32 fill = hdr->fill;

    if (!DecodeBmpHeader(a2, width, height, 8, a3)) {
        return 0;
    }
    if (!(a3 & 1)) {
        m_transparent = 0;
    }

    if (flags & PID_FILL_IS_WORD) {
        fill &= 0xffff;
    } else {
        fill = 0;
    }

    if (flags & PID_GRAMMAR_SKIPRUN) {
        m_transparent = 1;
        u8* dstRow = m_pixels + m_rowOffsets[0];
        i32 x = 0;
        i32 y = 0;
        i32 i = 0;
        while (y < m_height) {
            u8 c = src[i];
            if (c & 0x80) {
                i32 count = (c & 0xff) - 0x80;
                memset(dstRow + x, static_cast<u8>(fill), count);
                x += (src[i] & 0xff) - 0x80;
                i++;
            } else {
                i32 count = c & 0xff;
                memcpy(dstRow + x, &src[i + 1], count);
                x += src[i];
                i += src[i] + 1;
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
                u8 c = *src++;
                if ((c & 0xc0) == 0xc0) {
                    i32 count = c & 0x3f;
                    u8 v = *src++;
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
i32 CRezImage::LoadPid(char* name, void* a2, i32 a3) {
    CFile file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = ::operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    i32 result = DecodePidData(buf, a2, a3);
    ::operator delete(buf);
    return result;
}

RVA(0x001767d0, 0x64)
i32 CRezImage::LoadDefault(char* name, void* a2, i32 a3) {
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
    return DecodeResData(data, a2, a3);
}

// ---------------------------------------------------------------------------
// CRezImage::FlipVertical (0x176840): flip the tightly-packed 8bpp pixel plane
// (m_pixels, rows of m_width bytes, m_height rows) top-to-bottom by swapping
// row i with row (m_height-1-i) through a m_width-byte scratch row.
// @early-stop
// 47.36% (was 41.61). Fix 2026-07-28 (jcc_sieve OTHER `jl -> jne`): the row width is
// HOISTED into a local. `x < m_width` re-reads [esi+0x438] every iteration, which pins the
// guard as `cmp edx,ecx / jl`; with the width in a register cl strength-reduces the first
// two byte loops to retail's `dec edi / jne` trip count. Logic verified exact: guard
// (m_height<=1), ::operator new(m_width) scratch, the height/2 row-pair swap, RezFree.
// Residue (2 branches): retail keeps the THIRD byte loop and the outer pair loop as
// UP-counted index compares (`cmp edi,esi / jl`, `cmp eax,ecx / jl`) while cl down-counts
// both, and fuses the copies into merged inductions where one register is BOTH the scratch
// index and the source offset (`[eax+ecx-1]`, `[edx+ebp-1]`) with the row offsets in spill
// slots. Tried per-iteration multiply vs pointer-accumulator forms (40.3 -> 43.6).
RVA(0x00176840, 0x11f)
void CRezImage::FlipVertical() {
    if (m_height <= 1) {
        return;
    }
    u8* scratch = static_cast<u8*>(::operator new(m_width));
    if (scratch == 0) {
        return;
    }
    // The row width is HOISTED: retail's three byte loops end `dec edi / jne` off a
    // register trip count, where `x < m_width` re-reads [esi+0x438] every iteration and
    // forces `cmp edx,ecx / jl` instead.
    i32 wid = m_width;
    i32 pairs = m_height / 2;
    u8* top = m_pixels;
    u8* bot = m_pixels + (m_height - 1) * wid;
    i32 x;
    for (i32 i = 0; i < pairs; i++) {
        for (x = 0; x < wid; x++) {
            scratch[x] = bot[x];
        }
        for (x = 0; x < wid; x++) {
            bot[x] = top[x];
        }
        for (x = 0; x < wid; x++) {
            top[x] = scratch[x];
        }
        top += wid;
        bot -= wid;
    }
    ::operator delete(scratch);
}

RVA(0x00176ad0, 0x17)
void CRezImage::SetPalette(void* paletteNode, i32 scalar) {
    // The generic setter takes the node as void* (CImagePool::B threads it through as an
    // int handle); store it typed so the reads in Free/B are cast-free.
    m_paletteNode = static_cast<CImagePaletteNode*>(paletteNode);
    m_paletteScalar = scalar;
}

// ---------------------------------------------------------------------------
// CRezImage::Save(filename, paletteObj) - the format-guard dispatch: only 8bpp
// surfaces are BMP-writable; 16bpp (and anything else) return 0.
// @early-stop
// codegen block-merge divergence (~62.6%). Logic is exact: retail is a switch on
// m_bitCount (+0x440) with `case 8 -> SaveBmp`, `case 0x10 -> return 0`, default
// `return 0`. Retail MSVC5 keeps the identical `case 0x10` and default `return 0`
// as two separate blocks (emits `cmp 0x10; je`); this wine MSVC5 proves them equal
// and drops the `cmp 0x10` comparison entirely. Not source-steerable (every
// spelling - switch/no-default/if-chain - collapses the redundant block) and the
// permuter only reorders operands, it can't un-merge a block. First 4 insns + the
// SaveBmp tail match byte-exact; only the dropped 16bpp block differs.
RVA(0x00176b00, 0x2c)
i32 CRezImage::Save(const char* filename, void* paletteObj) {
    switch (m_bitCount) {
        case 8:
            return SaveBmp(filename, paletteObj);
        case 0x10:
            return 0;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CRezImage::SaveBmp(filename, paletteObj), __thiscall (ret 8) - the 8bpp software
// surface's "write me out as an 8-bit BMP" path: build a BITMAPFILEHEADER (copied
// from the 14-byte template @0x61aabc, then the bfSize/bfOffBits slots patched) +
// a zeroed BITMAPINFOHEADER and 256-entry colour table (de-interleaved from the
// source palette object's RGBQUADs), open the file and Write the two headers then
// the scanlines bottom-up (m_pixels + m_rowOffsets[row], width bytes each). The
// destructible stack CFile temp forces the exception frame (push -1 / handler / fs:0).
// @early-stop
// zero-register-pinning regalloc wall (58.17 -> 73.13 after the 2026-07-27 record fix:
// the 0x428 byte buffer became the declared `Bmp256Info` (BITMAPINFOHEADER + the full
// 256-entry table), the colour-table de-interleave was writing TWO BYTES LOW (ct[0]/
// ct[-1]/ct[-2] off the table base, corrupting biClrImportant - retail's three `lea`
// bases at esp+0x5c/0x5b/0x5a run against a cursor seeded at pal+2, i.e. rgbRed/
// rgbGreen/rgbBlue of entry i), and the file header is an inline strcpy of the
// 0x61aabc "BM" template over a zeroed record, not a 14-byte copy loop). Fixed a REAL
// bug earlier too: the CFile temp
// is 0x10 B on the frame (ctor@[esp+0x24], info@[esp+0x34]=file+0x10), not 0x440 -
// the oversized view had inflated the frame to sub esp,0x878; now 0x448 (retail 0x44c,
// off by one consequent spill slot). The EH prologue matches (push -1 + scope-table
// reloc + old-fs, reloc-masked). Residual: retail pins esi=0 as a whole-function zero
// register and holds `this` in ebp, so every null-check is `cmp esi,edx` and every
// BITMAPINFOHEADER zero-store reads esi; this build keeps `this` in esi + uses
// `test`/immediate-0, diverging ~40% of the body. A documented regalloc coin-flip
// (docs/patterns/zero-register-pinning.md), not source-steerable; logic + reloc-masked
// CFile ctor/Open/Write/dtor all faithful. The +4 frame is the extra spill slot.
RVA(0x00176b30, 0x1e5)
i32 CRezImage::SaveBmp(const char* filename, void* paletteObj) {
    void* obj = paletteObj;
    if (obj == 0) {
        obj = m_paletteNode; // +0x458 default palette object
        if (obj == 0) {
            return 0;
        }
    }

    BITMAPFILEHEADER fileHdr; // real 0xe-byte packed file header ([esp+0x10])
    Bmp256Info info;          // BITMAPINFOHEADER + the 256-entry RGBQUAD table ([esp+0x34])
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = 0x28;
    info.bmiHeader.biWidth = m_width;
    info.bmiHeader.biHeight = m_height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 8;
    info.bmiHeader.biCompression = 0;
    info.bmiHeader.biSizeImage = 0;

    // +0x008 of the palette node is its LOGPALETTE entry table (Build assembles it there).
    PALETTEENTRY* pal = static_cast<CImagePaletteNode*>(obj)->m_pal.palPalEntry;
    if (pal == 0) {
        return 0;
    }
    // De-interleave the node's R,G,B entries into the colour table's BGR order:
    // retail stores red to [ct+2], green to [ct+1], blue to [ct+0] (the three
    // `lea` bases at esp+0x5c/0x5b/0x5a against a cursor seeded at pal+2).
    RGBQUAD* ct = info.bmiColors;
    for (i32 i = 0x100; i != 0; i--) {
        ct->rgbRed = pal->peRed;
        ct->rgbGreen = pal->peGreen;
        ct->rgbBlue = pal->peBlue;
        ct++;
        pal++;
    }

    // Zero the packed 0xe-byte on-disk header, stamp "BM" over it, then patch
    // bfSize / bfOffBits. Retail inlines a strcpy here (repnz scasb over the
    // 0x61aabc template, then rep movsd/movsb), not a 14-byte copy loop.
    memset(&fileHdr, 0, sizeof(fileHdr));
    // API-forced: strcpy takes char*, and the target is a packed on-disk header.
    strcpy(reinterpret_cast<char*>(&fileHdr), g_bmpHeaderTemplate);
    fileHdr.bfSize = m_width * m_height + 0x436;
    fileHdr.bfOffBits = 0x436;

    CFile file;
    if (file.Open(filename, 0x1001, 0) == 0) {
        return 0;
    }
    file.Write(&fileHdr, 0xe);
    file.Write(&info, 0x428);
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

// ---------------------------------------------------------------------------
// CRezImage::FillRectAt (0x176da0) - build a translated fill rect at origin
// (dx,dy) sized from `src` (right = dx + src.width, bottom = dy + src.height)
// and scanline-fill it.
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
i32 ApiCallerStubs::CImagePaletteNode::Build(PALETTEENTRY* src, i32 flags) {
    m_flags = flags;
    m_pal.palNumEntries = 0x100;
    m_pal.palVersion = 0x300;
    PALETTEENTRY* s = src;
    PALETTEENTRY* d = m_pal.palPalEntry;
    i32 i = 0x100;
    do {
        *d = *s++; // 4-byte POD copy -> retail's single `mov ebp,[ecx] / mov [eax-3],ebp`
        d->peFlags = 0;
        d++;
    } while (--i);
    if (winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() && !(flags & 1)) {
        Tune();
        m_systemTuned = 1;
    }
    m_palette = CreatePalette(&m_pal);
    return m_palette != 0;
}

RVA(0x00176e70, 0x4e)
i32 ApiCallerStubs::CImagePaletteNode::ProcessPal(void* rgb, i32 flags) {
    PALETTEENTRY pal[256];
    u8* s = static_cast<u8*>(rgb);
    // indexed (pal[i].field), NOT a walking `d` pointer: cl anchors the strength-
    // reduced dst cursor on peGreen (`lea edx,[esp+1]`) for the indexed spelling and
    // on peBlue (`lea edx,[esp+2]`) for the pointer-bump one.
    for (i32 i = 0; i < 256; i++) {
        pal[i].peRed = *s++;
        pal[i].peGreen = *s++;
        pal[i].peBlue = *s++;
    }
    return Build(pal, flags);
}

// ===========================================================================
// CImagePaletteNode::ProcessPalQuad (0x176ec0, ret 8) - same R/B swap as
// ProcessPalBGR but the source is a 4-byte-per-entry RGBQUAD array (DIB palette
// order: blue, green, red, reserved); stride 4, peFlags untouched, then realize.
// Same indexed-dst / in-loop source cursor as ProcessPalBGR; at stride 4 that is
// what makes cl base-difference two of the three dst writes off the source cursor
// (esi/edi = pal - bgr), which is exactly retail's three-address-mode loop.
// ===========================================================================
RVA(0x00176ec0, 0x64)
i32 ApiCallerStubs::CImagePaletteNode::ProcessPalQuad(void* bgr, i32 flags) {
    PALETTEENTRY pal[256];
    for (i32 i = 0; i < 256; i++) {
        u8* s = static_cast<u8*>(bgr) + i * 4;
        pal[i].peRed = s[2];
        pal[i].peGreen = s[1];
        pal[i].peBlue = s[0];
    }
    return Build(pal, flags);
}

// ===========================================================================
// CImagePaletteNode::ProcessPalBGR (0x176f30, ret 8) - same as ProcessPal but the
// source triples are BGR-ordered (peBlue = s[0] ... peRed = s[2]); expand into a
// PALETTEENTRY[256] (peFlags untouched) then realize.
// Indexed dst (pal[i]) centres the cursor on peGreen (lea edx,[esp+1], writes
// [edx-1]/[edx]/[edx+1]) as retail does; a walking `d` pointer centres on peBlue.
// The source cursor is derived INSIDE the loop (base + i*3, strength-reduced back to
// a walking pointer) - a hoisted `u8* s` schedules its `inc eax` bias ahead of the
// dst `lea`, which is the only thing retail orders the other way round.
// ===========================================================================
RVA(0x00176f30, 0x51)
i32 ApiCallerStubs::CImagePaletteNode::ProcessPalBGR(void* bgr, i32 flags) {
    PALETTEENTRY pal[256];
    for (i32 i = 0; i < 256; i++) {
        u8* s = static_cast<u8*>(bgr) + i * 3;
        pal[i].peRed = s[2];
        pal[i].peGreen = s[1];
        pal[i].peBlue = s[0];
    }
    return Build(pal, flags);
}

RVA(0x00176f90, 0xa4)
i32 ApiCallerStubs::CImagePaletteNode::LoadByExtension(char* path, i32 arg) {
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
i32 ApiCallerStubs::CImagePaletteNode::ParseDispatch(void* buf, u32 size, i32 type, i32 ctrl) {
    if (type == 3) {
        return ParsePaletteTail(buf, size, ctrl);
    }
    return 0;
}

RVA(0x00177070, 0x22)
void ApiCallerStubs::CImagePaletteNode::Run() {
    if (m_palette) {
        DeleteObject(m_palette);
        m_palette = 0;
    }
    m_flags = 0;
}

RVA(0x001770a0, 0x3a)
i32 ApiCallerStubs::winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() {
    HDC ic = CreateICA("DISPLAY", 0, 0, 0);
    if (ic) {
        i32 caps = GetDeviceCaps(ic, RASTERCAPS) & RC_PALETTE;
        DeleteDC(ic);
        return caps;
    }
    return 0;
}

RVA(0x001770e0, 0x7c)
void ApiCallerStubs::CImagePaletteNode::Tune() {
    winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD();
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
void ApiCallerStubs::winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD() {
    // API-forced: LOGPALETTE is a GDI variable-length header, so the SDK's own
    // idiom is a byte buffer overlaid with it.
    char buf[4 + 256 * sizeof(PALETTEENTRY)];
    LOGPALETTE* lp = reinterpret_cast<LOGPALETTE*>(buf);
    HDC hdc = GetDC(0);
    lp->palVersion = 0x300;
    lp->palNumEntries = 256;
    for (i32 i = 0; i < 256; i++) {
        lp->palPalEntry[i].peRed = 0;
        lp->palPalEntry[i].peGreen = 0;
        lp->palPalEntry[i].peBlue = 0;
        lp->palPalEntry[i].peFlags = 4;
    }
    HPALETTE hpal = CreatePalette(lp);
    if (hpal) {
        HPALETTE old = SelectPalette(hdc, hpal, FALSE);
        RealizePalette(hdc);
        DeleteObject(SelectPalette(hdc, old, FALSE));
    }
    ReleaseDC(0, hdc);
}

RVA(0x001771f0, 0xe2)
i32 ApiCallerStubs::CImagePaletteNode::LoadPalFile(char* path, i32 arg) {
    CFile file;
    char rgb[0x300];

    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    if (file.GetLength() != 0x300) {
        return 0;
    }
    file.Read(rgb, 0x300);
    return ProcessPal(rgb, arg);
}

// ---------------------------------------------------------------------------
// CImagePaletteNode::LoadPcxFile (0x1772e0) - load the trailing palette of a .PCX:
// seek 0x300 bytes back from EOF, Read the 256*3 RGB triples; on a short read
// return 0. Expand the triples in place into a 256-entry RGBQUAD table (R,G,B,0)
// and hand it to BuildPalette(table, arg). The CFile stack object forces the
// /GX EH frame. __thiscall, ret 8.
// @early-stop
// 93.9% de-interleave-loop induction-phase wall: the EH frame + open/seek/read +
// BuildPalette call are byte-exact, but retail phases the dst induction variable at
// base+1 (`add ecx,4` after the FIRST byte store, the four writes at [iv-1]/[iv-4]/
// [iv-3]/[iv-2], the zero-store LAST) while clean C reorders the +4 and the zero
// store; not source-steerable. Logic 100% correct (256 RGB triples -> RGBQUAD).
RVA(0x001772e0, 0x117)
i32 ApiCallerStubs::CImagePaletteNode::LoadPcxFile(char* path, i32 arg) {
    CFile file;
    u8 rgb[0x300];
    u8 rgbq[0x400];

    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    if (file.Read(rgb, 0x300) == 0) {
        return 0;
    }

    u8* src = rgb;
    u8* dst = rgbq;
    for (i32 i = 0x100; i != 0; i--) {
        dst[0] = *src++;
        dst[1] = *src++;
        dst[2] = *src++;
        dst[3] = 0;
        dst += 4;
    }
    // API-forced: the 0x400 palette blob is handed to the SDK-typed entry API
    return Build(reinterpret_cast<PALETTEENTRY*>(rgbq), arg);
}

// ===========================================================================
// CImagePaletteNode::ParsePaletteTail (ret 0xc) - extract the
// trailing 768-byte VGA palette from the end of `buf` into a PALETTEENTRY[256]
// (peFlags = 0) and realize it; needs at least 0x300 bytes.
// ===========================================================================
// @early-stop
// byte-copy-loop scheduling wall (~88.6%): the prologue/size-check/tail all match
// and the loop produces the identical palette, but MSVC schedules the four per-
// entry byte stores (R/G/B + the peFlags=0) in a different order and picks a
// different `d` base displacement than retail (retail: R,G,B,flags from d=buf+1;
// recompile: R,flags,G,B from d=buf+2). Not source-steerable (cf. the entropy-
// prone decoder tails in Image.cpp). Logic byte-faithful.
RVA(0x00177400, 0x76)
i32 ApiCallerStubs::CImagePaletteNode::ParsePaletteTail(void* buf, u32 size, i32 ctrl) {
    PALETTEENTRY pal[256];
    if (size < 0x300) {
        return 0;
    }
    u8* s = static_cast<u8*>(buf) + size - 0x300;
    PALETTEENTRY* d = pal;
    for (i32 i = 0; i < 256; i++) {
        d->peRed = *s++;
        d->peGreen = *s++;
        d->peBlue = *s++;
        d->peFlags = 0;
        d++;
    }
    return Build(pal, ctrl);
}
