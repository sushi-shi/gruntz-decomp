// ImagePool.cpp - the engine image/palette resource pool + the CRezImage DIB
// surface class (C:\Proj\...\Image) - ONE original TU (interval dossier
// 0x174e90-0x177476: imagepool + rezimage + scanlinesurface + imagevflip +
// scanlinesurfacesave + imagerectfill all hold methods of the same
// CImagePool/CRezImage/CImagePaletteNode classes, totally
// interleaved - merged wave1-E, strict retail-RVA order).
//
// The pool keeps two MFC CObLists - a list of GDI surface nodes (+0x10) and a
// list of palette nodes (+0x2c) - plus a teardown/add/remove API over them. The
// node payloads:
//
//   CRezImage           (surface node)  - the shared DIB-surface class in
//                       <Image/Image.h>. DecodeBmpHeader is the shared plane
//                       allocator (fills the BITMAPINFOHEADER, CreateDIBSections
//                       the pixel plane + builds the bottom-up per-row offset
//                       table); DecodeResData/RidData hand pre-decoded pixels to
//                       the shared blitter DecodeBlit; DecodePcxData/PidData
//                       RLE-decode .PCX/.PID into the plane. The .PID format
//                       (flags@4, width@8, height@0xc, COMPRESSION=0x20) matches
//                       Monolith's WAP32 layout (libwap32 wap32/pid.h). Free()
//                       @0x175c90 releases its GDI object + buffer; SetPalette()
//                       @0x176ad0 latches the palette node ptr (+0x458) and a
//                       scalar (+0x454).
//   CImagePaletteNode   (palette node)  - Build() @0x176df0 realizes an HPALETTE
//                       from a PALETTEENTRY[256]; ProcessPal/ParseDispatch/
//                       ParsePaletteTail are the format front-ends that fill that
//                       array from a packed-RGB / dispatched / trailing-768-byte
//                       source then forward to Build. Run() @0x177070 deletes the
//                       HPALETTE.
//   CImagePaletteNode   - LoadByExtension() @0x176f90 loads a palette node from a
//                       file by extension (.BMP/.PCX/.PAL -> sibling loaders); the
//                       former CImageExtLoader view is folded in (proven same `this`).
//
// The pool walks/edits the CObLists through the real MFC CPtrList (AddTail/
// RemoveAll/RemoveAt are NAFXCW, reloc-masked via library labels) and
// (de)allocates nodes through the engine RezAlloc/RezFree. The CFileIO/CFile
// stack objects in the file loaders carry dtors -> C++ EH frames -> this TU
// builds with /GX. Field names are placeholders; only OFFSETS + code bytes are
// load-bearing.
#include <Mfc.h>         // CPtrList / POSITION / CFile + <windows.h> PALETTEENTRY
#include <Image/Image.h> // CRezImage - the shared DIB-surface class (the pool's surface node)
#include <Image/ImagePaletteNode.h> // the canonical CImagePaletteNode (this TU owns most bodies)
#include <Image/ImagePool.h>        // the canonical CImagePool (this TU owns its bodies)
#include <Rez/RezMgr.h>             // RezAlloc/RezFree (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82)
#include <rva.h>
// <string.h>: strrchr (find the ext dot) / _stricmp (the case-insensitive ext
// compare) + memset/memcpy in the RLE decoders.
#include <string.h>

// The .PID/.PCX-via-RezMgr flags word (PidFlags) now comes from the shared
// <DDrawMgr/DDSurface.h> (via Image.h) - wave4-K dedup.

// The .PID on-disk image header (PidHeader, 0x20 B) is the shared format struct in
// <DDrawMgr/DDSurface.h> (via Image.h), homed next to the PidFlags it carries at +0x04
// (2026-07-14); DecodePidData reads it by name at the head of the RezMgr payload.

// The five file-extension string LITERALS (??_C@ .rdata constants, pooled + shared
// with the DirPal.cpp / Image.cpp loaders; reloc-masked, so each `push OFFSET` pairs).

// The resource module handle the .DEFAULT loader pulls RT_BITMAP resources from
// and the pool's AddSurfaceRez/AddImageFile latch before a file-backed add
// (reloc-masked .data global; 0 until the engine records the instance handle).
// DEFINED HERE (owner TU; PaletteBmp only reads it). It was extern on both sides, so the
// resource-module handle had no storage at all. .bss, zero-init.
DATA(0x002bf6e0)
extern "C" {
    HINSTANCE g_hResModule = 0; // 0x6bf6e0
}

// ---------------------------------------------------------------------------
// The palette node payload type. Declared with the EXACT class/namespace/
// signature its RVA-keyed bodies (below) use. Only the offsets the pool touches
// are pinned.
// ---------------------------------------------------------------------------
namespace ApiCallerStubs {
    // The palette list node CImagePaletteNode is the ONE canonical class in
    // <Image/ImagePaletteNode.h> (included above): this TU owns its Build/ProcessPal*/
    // Parse*/Load* bodies (below); PaletteBmp.cpp owns LoadBmpFile/Apply. The former
    // identical .cpp-local definition here is DISSOLVED onto that header (2026-07-14).

    // Two free GDI palette helpers PalBuilder::Build/Tune funnel through:
    // 0x1770a0 probes display-palette support; 0x177160 resets the screen
    // palette to all-black. __cdecl.
    i32 winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps();
    void winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD();
} // namespace ApiCallerStubs

// The "BM" BITMAPFILEHEADER magic (0x61aabc, .data). SaveBmp copies 14 bytes up front
// (over-reading past the 4-byte "BM\0\0" into the adjacent DIRPAL.CPP path literal -
// only the leading "BM" bfType matters; bfSize/bfOffBits are patched after). DEFINED
// here (owner TU): this is the SINGLE datum FileImage's `g_imageTag` alias folded onto
// (both were 0x21aabc). Reference extern stays in <Globals.h>. (REHOME DD-G)
DATA(0x0021aabc)
char g_bmpHeaderTemplate[4] = "BM"; // 0x61aabc  = 42 4d 00 00

// The engine's cached GDI fn-ptr globals (retail reaches GDI via `call ds:[0x6c44xx]`);
// DATA-bound once in DirPal.cpp, re-declared here (extern only) so the calls reloc-mask.

// The palette object handed to Convert8To16: an 8-byte header then the 256-entry
// RGB table (one u32 per palette index) the 8bpp pixels look up.
struct ScanlinePalette {
    char m_pad0[8];    // +0x00  header
    u32 m_colors[256]; // +0x08  RGB table (indexed by the 8bpp pixel)
};
SIZE_UNKNOWN(ScanlinePalette); // partial view of the foreign palette object (pointer-passed)

// SaveBmp writes through a plain destructible MFC CFile stack temp (ctor 0x1befd7,
// Open 0x1bf200, Write 0x1bf362, dtor 0x1bf121 - all NAFXCW, reloc-masked via <Mfc.h>).
// The former BmpFile/Stream/Opener view read Open on file+0xc / Write on file+0x8 as
// "sub-objects"; that was a MISREADING - the retail `lea ecx,[esp+0x30]`/`[esp+0x2c]`
// are AFTER the arg pushes shifted esp, so the effective `this` is the SAME file base
// for ctor/Open/Write/dtor. Dissolved to `CFile file;` (2026-07-14).

// CImagePool is the canonical class in <Image/ImagePool.h> (included above); this
// TU owns its method bodies. The five surface factories below RezAlloc a fresh
// CRezImage node (0x45c bytes) then forward to one of its decoders (DecodeBmpHeader/
// DecodeBlit/LoadFromRez/DispatchDecode/Convert8To16, defined below in this TU).

// ===========================================================================
// CImagePool::SetHandles (ret 0xc) - seed the three head scalars.
// ===========================================================================
RVA(0x00174e90, 0x1c)
i32 CImagePool::SetHandles(i32 a, i32 b, i32 c) {
    m_resourceModuleHandle = (HINSTANCE)a;
    m_08 = c;
    m_sourceHwnd = (HWND)b;
    return 1;
}

// ===========================================================================
// CImagePool::Clear - tear down both lists, zero the head scalars.
// ===========================================================================
RVA(0x00174eb0, 0x1b)
void CImagePool::Clear() {
    ClearSurfaces();
    ClearPalettes();
    m_resourceModuleHandle = 0;
    m_sourceHwnd = 0;
    m_08 = 0;
}

// ===========================================================================
// CImagePool::Free (ret 4, re-homed from BoundaryUpper2) - discard a surface
// node: if it currently owns a palette (m_paletteNode && m_paletteScalar),
// RemovePalette it and repoint (B(null, 0, 0)); unlink the node from the +0x10
// surface list by its cached POSITION (m_listPosition), run CRezImage::Free
// (0x175c90) and RezFree it.
// ===========================================================================
RVA(0x00174ed0, 0x5d)
void CImagePool::Free(CRezImage* node) {
    if (!node) {
        return;
    }
    if (node->m_paletteNode && node->m_paletteScalar) {
        RemovePalette((CImagePaletteNode*)node->m_paletteNode);
        B(0, 0, 0);
    }
    if (node->m_listPosition) {
        m_surfaces.RemoveAt(node->m_listPosition);
    }
    node->Free();
    ::operator delete(node);
}

// ===========================================================================
// CImagePool::RemovePalette (ret 4) - unlink one palette node from
// the +0x2c list (by its cached POSITION), delete its HPALETTE, free it.
// ===========================================================================
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

// ===========================================================================
// CImagePool::ClearSurfaces - delete every surface node, RemoveAll.
// ===========================================================================
RVA(0x00174f60, 0x37)
void CImagePool::ClearSurfaces() {
    POSITION pos = m_surfaces.GetHeadPosition();
    while (pos) {
        CRezImage* item = (CRezImage*)m_surfaces.GetNext(pos);
        if (item) {
            item->Free();
            ::operator delete(item);
        }
    }
    m_surfaces.RemoveAll();
}

// ===========================================================================
// CImagePool::ClearPalettes - delete every palette node, RemoveAll,
// then clear +0x48.
// ===========================================================================
RVA(0x00174fa0, 0x3e)
void CImagePool::ClearPalettes() {
    POSITION pos = m_palettes.GetHeadPosition();
    while (pos) {
        CImagePaletteNode* item = (CImagePaletteNode*)m_palettes.GetNext(pos);
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
// (tried node=0 pre-init -> 94%).
// ===========================================================================
// @early-stop
// regalloc tie-break: node should be edi / zero should be ebx (retail); recompile
// swaps the two callee-saved regs. Code byte-identical otherwise.
RVA(0x00174fe0, 0xfe)
CRezImage* CImagePool::AddSurfaceBmp(i32 width, i32 height, i32 bitCount, i32 flag) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node;
    CRezImage* raw = (CRezImage*)::operator new(0x45c);
    if (raw) {
        raw->m_dibSection = 0;
        raw->m_pixels = 0;
        raw->m_rowOffsets = 0;
        raw->m_434 = 0;
        raw->m_width = 0;
        raw->m_height = 0;
        raw->m_stride = 0;
        raw->m_rowPad = 0;
        raw->m_listPosition = 0;
        raw->m_paletteScalar = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (node->DecodeBmpHeader((void*)hdc, width, height, bitCount, (void*)flag) == 0) {
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
CRezImage* CImagePool::AddSurfaceBlit(i32 src, i32 width, i32 height, i32 bitCount, i32 flag) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node;
    CRezImage* raw = (CRezImage*)::operator new(0x45c);
    if (raw) {
        raw->m_dibSection = 0;
        raw->m_pixels = 0;
        raw->m_rowOffsets = 0;
        raw->m_434 = 0;
        raw->m_width = 0;
        raw->m_height = 0;
        raw->m_stride = 0;
        raw->m_rowPad = 0;
        raw->m_listPosition = 0;
        raw->m_paletteScalar = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (node->DecodeBlit((void*)src, (void*)hdc, width, height, bitCount, (void*)flag) == 0) {
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
    CRezImage* node;
    CRezImage* raw = (CRezImage*)::operator new(0x45c);
    if (raw) {
        raw->m_dibSection = 0;
        raw->m_pixels = 0;
        raw->m_rowOffsets = 0;
        raw->m_434 = 0;
        raw->m_width = 0;
        raw->m_height = 0;
        raw->m_stride = 0;
        raw->m_rowPad = 0;
        raw->m_listPosition = 0;
        raw->m_paletteScalar = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (node->DispatchDecode(buf, kind, (void*)hdc, (void*)ctrl) == 0) {
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
CRezImage* CImagePool::AddSurfaceRez(i32 name, i32 ctrl) {
    HDC hdc = GetDC(m_sourceHwnd);
    g_hResModule = m_resourceModuleHandle;
    CRezImage* node;
    CRezImage* raw = (CRezImage*)::operator new(0x45c);
    if (raw) {
        raw->m_dibSection = 0;
        raw->m_pixels = 0;
        raw->m_rowOffsets = 0;
        raw->m_434 = 0;
        raw->m_width = 0;
        raw->m_height = 0;
        raw->m_stride = 0;
        raw->m_rowPad = 0;
        raw->m_listPosition = 0;
        raw->m_paletteScalar = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (node->LoadFromRez((char*)name, (void*)hdc, (void*)ctrl) == 0) {
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
CRezImage* CImagePool::AddSurfaceConvert(i32 src, i32 pal) {
    HDC hdc = GetDC(m_sourceHwnd);
    CRezImage* node;
    CRezImage* raw = (CRezImage*)::operator new(0x45c);
    if (raw) {
        raw->m_dibSection = 0;
        raw->m_pixels = 0;
        raw->m_rowOffsets = 0;
        raw->m_434 = 0;
        raw->m_width = 0;
        raw->m_height = 0;
        raw->m_stride = 0;
        raw->m_rowPad = 0;
        raw->m_listPosition = 0;
        raw->m_paletteScalar = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (node->Convert8To16((void*)hdc, (CRezImage*)src, (void*)pal) == 0) {
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
RVA(0x001754f0, 0x7b)
CImagePaletteNode* CImagePool::AddPaletteEntries(PALETTEENTRY* entries, i32 flags) {
    CImagePaletteNode* node;
    CImagePaletteNode* raw = (CImagePaletteNode*)::operator new(0x414);
    if (raw) {
        raw->m_palette = 0;
        raw->m_systemTuned = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
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
    CImagePaletteNode* node;
    CImagePaletteNode* raw = (CImagePaletteNode*)::operator new(0x414);
    if (raw) {
        raw->m_palette = 0;
        raw->m_systemTuned = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
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
    CImagePaletteNode* node;
    CImagePaletteNode* raw = (CImagePaletteNode*)::operator new(0x414);
    if (raw) {
        raw->m_palette = 0;
        raw->m_systemTuned = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
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
    CImagePaletteNode* node;
    CImagePaletteNode* raw = (CImagePaletteNode*)::operator new(0x414);
    if (raw) {
        raw->m_palette = 0;
        raw->m_systemTuned = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
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

// ===========================================================================
// CImagePool::EnsureSurface (0x175710, ret 0x14) - GetDC the pool's source window,
// (re)size `img` to (w,h,bitCount) through CRezImage::EnsureSize, restore + clear any
// selected palette, ReleaseDC, and return the resize result (0 if img is null).
// ===========================================================================
RVA(0x00175710, 0x69)
i32 CImagePool::EnsureSurface(CRezImage* img, i32 w, i32 h, i32 bitCount, void* flag) {
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

// ===========================================================================
// CImagePool::B (ret 0xc, re-homed from BoundaryUpper2) - re-point a surface
// node's palette: if it currently owns one, detach it (RemovePalette +
// CRezImage::SetPalette(0,0)), then latch the new (a,b) via SetPalette.
// ===========================================================================
RVA(0x00175780, 0x3f)
void CImagePool::B(CRezImage* node, i32 a, i32 b) {
    if (node->m_paletteNode && node->m_paletteScalar) {
        RemovePalette((CImagePaletteNode*)node->m_paletteNode);
        node->SetPalette(0, 0);
    }
    node->SetPalette((void*)a, b);
}

// ---------------------------------------------------------------------------
// CRezImage::DecodeBmpHeader
// The plane allocator/setup shared by every format. Records the image geometry
// (width/abs(height)/bitcount and the aligned destination stride) into the
// engine fields at this+0x434.., builds an in-place BITMAPINFOHEADER (this+0)
// with an identity DIB_PAL_COLORS table for 8bpp, CreateDIBSections the pixel
// plane (HBITMAP @+0x428, bits @+0x42c), and operator-new's the bottom-up
// per-row byte-offset table (this+0x430). Returns 0 if CreateDIBSection fails.
RVA(0x001757c0, 0x16f)
i32 CRezImage::DecodeBmpHeader(void* a2, i32 width, i32 height, i32 bitcount, void* a3) {
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
    m_bih.biBitCount = (WORD)m_bitCount;
    m_bih.biSize = sizeof(BITMAPINFOHEADER);
    m_bih.biHeight = height;
    m_bih.biPlanes = 1;
    m_bih.biCompression = 0;
    m_bih.biSizeImage = 0;
    m_bih.biClrUsed = 0;
    m_bih.biClrImportant = 0;
    if (m_bitCount == 8) {
        for (i32 i = 0; i < 256; i++) {
            m_pal[i] = (u16)i;
        }
        m_dibSection =
            CreateDIBSection((HDC)a2, (BITMAPINFO*)&m_bih, DIB_PAL_COLORS, (void**)&m_pixels, 0, 0);
    } else {
        m_dibSection =
            CreateDIBSection((HDC)a2, (BITMAPINFO*)&m_bih, DIB_RGB_COLORS, (void**)&m_pixels, 0, 0);
    }
    if (!m_dibSection) {
        return 0;
    }
    m_rowOffsets = (i32*)::operator new(m_height * 4);
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
i32 CRezImage::DecodeBlit(void* src, void* a2, i32 width, i32 height, i32 bitcount, void* a3) {
    if (!DecodeBmpHeader(a2, width, height, bitcount, a3)) {
        return 0;
    }
    if (m_rowPad == 0) {
        memcpy(m_pixels, src, (u32)(m_stride * m_height * bitcount) >> 3);
        return 1;
    }
    char* s = (char*)src;
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
i32 CRezImage::DispatchDecode(void* buf, i32 kind, void* dc, void* ctrl) {
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

// ---------------------------------------------------------------------------
// CRezImage::LoadFromRez
// ext = strrchr(name,'.'); dispatch on .BMP/.PCX/.RID/.PID, else default. Each
// branch re-tests `ext != 0` (the target's `test esi; je default` per case) and
// forwards (name,a2,a3); a matched ext returns its loader's result directly.
RVA(0x00175a90, 0xee)
i32 CRezImage::LoadFromRez(char* name, void* a2, void* a3) {
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
    u32* palette = ((ScanlinePalette*)pal)->m_colors;
    if (palette == 0) {
        return 0;
    }
    if (!DecodeBmpHeader(dc, src->m_width, src->m_height, 0x10, 0)) {
        return 0;
    }
    for (i32 y = 0; y < m_height; y++) {
        u8* sp = (u8*)src->m_pixels + y * src->m_stride;
        u16* dp = (u16*)((u8*)m_pixels + y * m_stride * 2);
        for (i32 x = 0; x < m_width; x++) {
            u32 c = palette[*sp];
            u32 r = c & 0xff;
            u32 g = (c >> 8) & 0xff;
            u32 b = (c >> 16) & 0xff;
            *dp = (u16)(((((r & 0xf8) << 5) | (g & 0xf8)) << 2) | (b >> 3));
            dp++;
            sp++;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CRezImage::Free (0x175c90): release the cached GDI object + Rez buffer and
// clear the slots. The pool's node teardown IS the same Free the loaders'
// EnsureSize calls - one shared DIB-surface class.
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

// ---------------------------------------------------------------------------
// Keep the surface at `w`x`h`. If the DIB object is live and already that
// size, reuse it (return TRUE); otherwise Free + reallocate via DecodeBmpHeader.
RVA(0x00175ce0, 0x6b)
i32 CRezImage::EnsureSize(void* dc, i32 w, i32 h, i32 bitCount, void* flag) {
    if (m_dibSection && m_pixels && m_rowOffsets && m_width == w && m_height == h) {
        return 1;
    }
    Free();
    return DecodeBmpHeader(dc, w, h, bitCount, flag);
}

// ---------------------------------------------------------------------------
// Fill every pixel with the low byte of `value`. Contiguous buffers
// (m_rowPad == 0) get one flat fill; padded buffers fill row by row.
// @early-stop
// memset-inline LICM wall: the contiguous fast path is byte-exact, but retail hoists
// the per-scanline fill's value-mask out of the loop (`value &= 0xff` once + reload)
// while our recompile re-reads the byte each iteration; an explicit `value &= 0xff`
// is folded away as redundant before memset. ~91.4%.
RVA(0x00175d50, 0xad)
void CRezImage::Fill(i32 value) {
    if (m_rowPad == 0) {
        memset(m_pixels, value, m_stride * m_height);
    } else {
        for (i32 y = 0; y < m_height; y++) {
            memset((u8*)m_pixels + m_rowOffsets[y], value, m_width);
        }
    }
}

// ---------------------------------------------------------------------------
// CRezImage::DecodeResData
// The RT_BITMAP / .DEFAULT decoder: `buf` points at a packed DIB
// (BITMAPINFOHEADER + palette + pixels). Pull biWidth/biHeight/biBitCount, point
// `src` at the pixel bytes (for 8bpp the 256-entry RGBQUAD palette pushes them to
// buf+biSize+0x400, else right after the 0x28 header + the 4 RGBQUAD masks at
// buf+0x2c) and hand it to the shared blitter.
RVA(0x00175e00, 0x3d)
i32 CRezImage::DecodeResData(void* buf, void* a2, void* a3) {
    BITMAPINFOHEADER* ih = (BITMAPINFOHEADER*)buf;
    i32 bitcount = ih->biBitCount;
    i32 height = ih->biHeight;
    i32 width = ih->biWidth;
    void* src = (u8*)buf + 0x2c;
    if (bitcount == 8) {
        src = (u8*)buf + ih->biSize + 0x400;
    }
    return DecodeBlit(src, a2, width, height, bitcount, a3);
}

// ---------------------------------------------------------------------------
// CRezImage::LoadBmp
// The .BMP loader: open the file, read the 14-byte BITMAPFILEHEADER and the
// 40-byte BITMAPINFOHEADER, hand the parsed (width, height, bitcount, a2, a3)
// to the decode helper that allocates the CRezImage's pixel plane, then Seek to
// bfOffBits and Read exactly (bitcount/8)*stride*height pixel bytes into the
// plane. Returns 1 on a full read, 0 on any I/O / decode failure. The CFileIO
// stack object's dtor runs on every exit -> the C++ EH frame.
// @early-stop
// TU-merge regalloc ripple (~98%; was 100% in the pre-merge rezimage TU with the
// IDENTICAL source): the `size = (bitcount/8)*m_stride*height` block reschedules
// (retail keeps bitcount/8 as the esi accumulator + `imul esi,[m_stride]` memory
// operand; the merged-TU compile hoists m_stride into esi first). Not source-
// steerable (bpp-local spelling is byte-identical); everything else exact.
RVA(0x00175e40, 0x1b3)
i32 CRezImage::LoadBmp(char* name, void* a2, void* a3) {
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

// ---------------------------------------------------------------------------
// CRezImage::DecodePcxData
// The .PCX decoder: parse the ZSoft header (width = Xmax-Xmin+1, height =
// Ymax-Ymin+1; bail unless BitsPerPixel==8), allocate the plane via
// DecodeBmpHeader (bitcount = NPlanes*8), then RLE-decode each scanline into a
// scratch buffer (filled back-to-front) and emit it into the plane row, either
// straight (1 plane) or interleaving 3 planes into RGB triples.
RVA(0x00176000, 0x18f)
i32 CRezImage::DecodePcxData(void* buf, void* a2, void* a3) {
    u8* hdr = (u8*)buf;
    i32 width = *(i16*)(hdr + 8) - *(i16*)(hdr + 4) + 1;
    i32 height = *(i16*)(hdr + 0xa) - *(i16*)(hdr + 6) + 1;
    if (hdr[3] != 8) {
        return 0;
    }
    if (!DecodeBmpHeader(a2, width, height, (i8)hdr[0x41] * 8, a3)) {
        return 0;
    }

    u8* src = hdr + 0x80;
    i32 scanBytes = (width * (i8)hdr[0x41] * (i8)hdr[3] + 7) / 8;
    u8* scan = (u8*)::operator new(scanBytes);

    for (i32 y = 0; y < height; y++) {
        u8* dst = m_pixels + m_rowOffsets[y];
        i32 n = width * (i8)hdr[0x41];
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

        if ((i8)hdr[0x41] == 1) {
            for (i32 x = width; x != 0; x--) {
                *dst++ = scan[x - 1];
            }
        } else if ((i8)hdr[0x41] == 3) {
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

// ---------------------------------------------------------------------------
// CRezImage::LoadPcx
// Open the file, GetLength(); if zero return 0. `operator new` a buffer of that
// size; if it fails return 0. Read the whole file, hand the buffer (+a2,a3) to
// the PCX decode helper, free the buffer and return the decoder's result.
RVA(0x00176190, 0x126)
i32 CRezImage::LoadPcx(char* name, void* a2, void* a3) {
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

// ---------------------------------------------------------------------------
// CRezImage::DecodeRidData
// The .RID decoder: the header at buf+8 carries (width, height) and the raw
// 8bpp pixels begin at buf+0x20; hand them straight to the blitter. a3's low bit
// gates the transparency flag at this+0x450 (cleared when not set).
RVA(0x001762c0, 0x42)
i32 CRezImage::DecodeRidData(void* buf, void* a2, void* a3) {
    i32* hdr = (i32*)((char*)buf + 8);
    i32 width = hdr[0];
    i32 height = hdr[1];
    i32 ok = DecodeBlit((char*)buf + 0x20, a2, width, height, 8, a3);
    if (!((i32)a3 & 1)) {
        m_transparent = 0;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// CRezImage::LoadRid
// Byte-identical to LoadPcx except for the per-format decode helper (the .RID
// reader DecodeRidData).
RVA(0x00176310, 0x126)
i32 CRezImage::LoadRid(char* name, void* a2, void* a3) {
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

// ---------------------------------------------------------------------------
// CRezImage::DecodePidData
// The .PID decoder. The header carries a flags word at buf+4 and geometry at
// buf+8/buf+0xc; raw run data starts at buf+0x20. After allocating the plane
// (DecodeBmpHeader, bitcount 8) two decode modes are selected by flags:
//   flags&0x20  -> a horizontal skip/fill RLE: each opcode either repeats a fill
//                  colour (high bit set, count = c-0x80) or copies `c` literal
//                  bytes, advancing x across rows by the source width (m_width).
//   else        -> a per-row PCX-style RLE ((c&0xc0)==0xc0 => run of `c&0x3f`).
// flags&0x100 masks the fill colour (buf+0x18) to a low word, else it is zeroed.
// a3's low bit gates the transparency flag at this+0x450.
RVA(0x00176440, 0x25d)
i32 CRezImage::DecodePidData(void* buf, void* a2, void* a3) {
    PidHeader* hdr = (PidHeader*)buf;
    u8* src = (u8*)(hdr + 1); // pixel stream at buf + 0x20
    i32 width = hdr->width;
    i32 height = hdr->height;
    i32 flags = hdr->flags;
    i32 fill = hdr->fill;

    if (!DecodeBmpHeader(a2, width, height, 8, a3)) {
        return 0;
    }
    if (!((i32)a3 & 1)) {
        m_transparent = 0;
    }

    if (flags & 0x100) {
        fill &= 0xffff;
    } else {
        fill = 0;
    }

    if (flags & PID_COMPRESSION) {
        m_transparent = 1;
        u8* dstRow = m_pixels + m_rowOffsets[0];
        i32 x = 0;
        i32 y = 0;
        i32 i = 0;
        while (y < m_height) {
            u8 c = src[i];
            if (c & 0x80) {
                i32 count = (c & 0xff) - 0x80;
                memset(dstRow + x, (u8)fill, count);
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
        for (i32 y = 0; (u32)y < (u32)height; y++) {
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

// ---------------------------------------------------------------------------
// CRezImage::LoadPid
// Byte-identical to LoadPcx/LoadRid except for the .PID decode helper.
RVA(0x001766a0, 0x126)
i32 CRezImage::LoadPid(char* name, void* a2, void* a3) {
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

// ---------------------------------------------------------------------------
// CRezImage::LoadDefault
// The fallback (no/unknown extension): pull the named RT_BITMAP resource from
// the engine's resource module and decode it in place. Returns 0 unless the
// module handle is set and FindResource/LoadResource/LockResource all succeed.
RVA(0x001767d0, 0x64)
i32 CRezImage::LoadDefault(char* name, void* a2, void* a3) {
    HINSTANCE hModule = g_hResModule;
    if (!hModule) {
        return 0;
    }
    HRSRC hRsrc = FindResourceA(hModule, name, (LPCSTR)RT_BITMAP);
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
// strength-reduction / merged-induction wall (~43.6%). Logic verified exact: guard
// (m_height<=1), ::operator new(m_width) scratch, the height/2 row-pair swap (scratch=bot;
// bot=top; top=scratch), RezFree. Retail's /O2 fuses the three per-pair byte copies
// into merged inductions where one register is BOTH the scratch index and the source
// offset (`[eax+ecx-1]`, `[edx+ebp-1]`) and accumulates the row offsets in stack spill
// slots; the wine MSVC5 keeps separate `x` index + `top`/`bot` pointer accumulators.
// Tried per-iteration multiply vs pointer-accumulator forms (40.3 -> 43.6); the merged
// 3-loop induction shape is not source-expressible. Bytes for the prologue/guard/alloc/
// free + the swap ordering match; the residue is the inner-loop addressing selection.
RVA(0x00176840, 0x288)
void CRezImage::FlipVertical() {
    if (m_height <= 1) {
        return;
    }
    u8* scratch = (u8*)::operator new(m_width);
    if (scratch == 0) {
        return;
    }
    i32 pairs = m_height / 2;
    u8* top = m_pixels;
    u8* bot = m_pixels + (m_height - 1) * m_width;
    i32 x;
    for (i32 i = 0; i < pairs; i++) {
        for (x = 0; x < m_width; x++) {
            scratch[x] = bot[x];
        }
        for (x = 0; x < m_width; x++) {
            bot[x] = top[x];
        }
        for (x = 0; x < m_width; x++) {
            top[x] = scratch[x];
        }
        top += m_width;
        bot -= m_width;
    }
    ::operator delete(scratch);
}

// ---------------------------------------------------------------------------
// CRezImage::SetPalette (0x176ad0) - store the palette node + scalar.
RVA(0x00176ad0, 0x17)
void CRezImage::SetPalette(void* paletteNode, i32 scalar) {
    m_paletteNode = paletteNode;
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
        default:
            return 0;
    }
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
// zero-register-pinning regalloc wall (~57%). Fixed a REAL bug first: the CFile temp
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
    char info[0x428];         // BITMAPINFOHEADER + 256-entry RGBQUAD table ([esp+0x34])
    BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)info;
    memset(info, 0, 0x428);
    bih->biSize = 0x28;
    bih->biWidth = m_width;
    bih->biHeight = m_height;
    bih->biPlanes = 1;
    bih->biBitCount = 8;
    bih->biCompression = 0;
    bih->biSizeImage = 0;

    u8* pal = (u8*)obj + 8;
    if (pal == 0) {
        return 0;
    }
    // De-interleave the source RGBQUADs into the colour table (BMP BGR order).
    u8* ct = (u8*)info + 0x28;
    for (i32 i = 0x100; i != 0; i--) {
        ct[0] = pal[0];
        *(ct - 1) = pal[1];
        *(ct - 2) = pal[2];
        ct += 4;
        pal += 4;
    }

    // Copy the 14-byte file-header template, then patch bfSize / bfOffBits.
    char* fh = (char*)&fileHdr;
    for (i32 b = 0; b < 0xe; b++) {
        fh[b] = g_bmpHeaderTemplate[b];
    }
    fileHdr.bfSize = m_width * m_height + 0x436;
    fileHdr.bfOffBits = 0x436;

    CFile file;
    if (file.Open(filename, 0x1001, 0) == 0) {
        return 0;
    }
    file.Write(&fileHdr, 0xe);
    file.Write(info, 0x428);
    for (i32 row = m_height - 1; row >= 0; row--) {
        file.Write((u8*)m_pixels + m_rowOffsets[row], m_width);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CRezImage::FillRect (0x176d20) - fill scanlines [top..bottom] of the rect with
// `color`, each row base being m_pixels + m_rowOffsets[y] + rect.left, `width`
// bytes. 100% (the earlier 91% read-order wall in BoundaryTail dissolved once
// folded into this complete CRezImage TU with its caller).
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
// @early-stop
// ~66% regalloc-pressure wall (extra ebx spill vs retail's tighter esi/edi pick).
// This function's 100% is TU-shape-dependent: ANY change to the imagepool TU's type
// graph reschedules the whole /O2 regalloc and flips it 100<->66. It last re-surfaced
// when CImageExtLoader was folded into ApiCallerStubs::CImagePaletteNode, and again
// when CFileImageSurface (Image.h) was reparented onto its real CDDSurface base (R53
// reloc-fidelity fix), and again (100->66.44) when m_surfaces/m_palettes were retyped
// from CObList to their real class CPtrList (LIST-AUDIT: every CImagePool list call
// targets the band whose vtable 0x1eb054 slot-0 CRuntimeClass names "CPtrList"), and
// again (2026-07-14) when SaveBmp dropped the BmpFile view for a real CFile + modeled
// fileHdr/info as BITMAPFILEHEADER/BITMAPINFOHEADER (same TU-type-graph reschedule).
// Proven causal + isolated: reverting ONLY this TU's two files restores it to 0-diff.
// Body byte-faithful; kept per the clean-room mandate (a correct de-view /
// reloc-fidelity fix outranks a collateral regalloc %).
RVA(0x00176da0, 0x4b)
void CRezImage::FillRectAt(i32 dx, i32 dy, CRezFillRect* src, i32 color) {
    CRezFillRect r;
    r.left = dx;
    r.top = dy;
    r.right = src->right + dx - src->left;
    r.bottom = src->bottom - src->top + dy;
    FillRect(&r, color);
}

// ---------------------------------------------------------------------------
// CImagePaletteNode::Build (0x176df0): build a 256-entry LOGPALETTE from src,
// optionally reserve the system range, then realize it into m_palette.
RVA(0x00176df0, 0x71)
i32 ApiCallerStubs::CImagePaletteNode::Build(PALETTEENTRY* src, i32 flags) {
    m_flags = flags;
    m_pal.palNumEntries = 0x100;
    m_pal.palVersion = 0x300;
    DWORD* s = (DWORD*)src;
    PALETTEENTRY* d = m_pal.palPalEntry;
    i32 i = 0x100;
    do {
        *(DWORD*)d = *s++;
        d->peFlags = 0;
        d++;
    } while (--i);
    if (winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() && !(flags & 1)) {
        Tune1770e0();
        m_systemTuned = 1;
    }
    m_palette = CreatePalette(&m_pal);
    return m_palette != 0;
}

// ===========================================================================
// CImagePaletteNode::ProcessPal (ret 8) - expand a packed RGB-triple
// source into a PALETTEENTRY[256] (peFlags untouched) then realize it.
// ===========================================================================
RVA(0x00176e70, 0x4e)
i32 ApiCallerStubs::CImagePaletteNode::ProcessPal(void* rgb, i32 flags) {
    PALETTEENTRY pal[256];
    u8* s = (u8*)rgb;
    PALETTEENTRY* d = pal;
    for (i32 i = 0; i < 256; i++) {
        d->peRed = *s++;
        d->peGreen = *s++;
        d->peBlue = *s++;
        d++;
    }
    return Build(pal, flags);
}

// ===========================================================================
// CImagePaletteNode::ProcessPalQuad (0x176ec0, ret 8) - same R/B swap as
// ProcessPalBGR but the source is a 4-byte-per-entry RGBQUAD array (DIB palette
// order: blue, green, red, reserved); stride 4, peFlags untouched, then realize.
// @early-stop
// stride-4 strength-reduction wall (~62.7%). Body/logic identical to ProcessPalBGR
// (95.6%) apart from `s += 4` vs `s += 3`. That stride-4 makes this wine MSVC5 split
// the source into TWO induction registers (a kept `s` in esi plus a walking edx, one
// byte fetched via a merged `[esi+eax]` dst-relative address) instead of the clean
// single-induction `[eax+1]/[eax-4]/[eax-5]` retail (and ProcessPalBGR) emit. Not
// source-steerable (load-into-locals, `pal[i]` indexing, reordered stores all keep the
// two-IV split) and the permuter finds no operand-order win. Prologue/counter/Build
// tail + the palette bytes produced are exact.
// ===========================================================================
RVA(0x00176ec0, 0x64)
i32 ApiCallerStubs::CImagePaletteNode::ProcessPalQuad(void* bgr, i32 flags) {
    PALETTEENTRY pal[256];
    u8* s = (u8*)bgr;
    PALETTEENTRY* d = pal;
    for (i32 i = 0; i < 256; i++) {
        d->peRed = s[2];
        d->peGreen = s[1];
        d->peBlue = s[0];
        s += 4;
        d++;
    }
    return Build(pal, flags);
}

// ===========================================================================
// CImagePaletteNode::ProcessPalBGR (0x176f30, ret 8) - same as ProcessPal but the
// source triples are BGR-ordered (peBlue = s[0] ... peRed = s[2]); expand into a
// PALETTEENTRY[256] (peFlags untouched) then realize.
// @early-stop
// dst-pointer-anchor tie (~95.6%): source side byte-identical (eax bias +1, the
// [eax+1]/[eax-3]/[eax-4] BGR reads, the add eax,3 mid-body); the sole residue is the
// dst induction anchor - retail centres edx on peGreen (lea edx,[esp+1], writes
// [edx-1]/[edx]/[edx+1]), the recompile centres on peBlue (lea edx,[esp+2]). A pure
// MSVC5 strength-reduction bias choice; the permuter + read-interleave reshuffles
// don't flip it. Logic complete.
// ===========================================================================
RVA(0x00176f30, 0x51)
i32 ApiCallerStubs::CImagePaletteNode::ProcessPalBGR(void* bgr, i32 flags) {
    PALETTEENTRY pal[256];
    u8* s = (u8*)bgr;
    PALETTEENTRY* d = pal;
    for (i32 i = 0; i < 256; i++) {
        d->peRed = s[2];
        d->peGreen = s[1];
        d->peBlue = s[0];
        s += 3;
        d++;
    }
    return Build(pal, flags);
}

// ---------------------------------------------------------------------------
// CImagePaletteNode::LoadByExtension  @0x176f90 - see the class comment above.
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

// ===========================================================================
// CImagePaletteNode::ParseDispatch  @0x177040 (ret 0x10) - format-3 -> the
// trailing-palette path; anything else fails.
// ===========================================================================
RVA(0x00177040, 0x23)
i32 ApiCallerStubs::CImagePaletteNode::ParseDispatch(void* buf, u32 size, i32 type, i32 ctrl) {
    if (type == 3) {
        return ParsePaletteTail(buf, size, ctrl);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CImagePaletteNode::Run (0x177070): delete the owned GDI object, then clear a
// far flag.
RVA(0x00177070, 0x22)
void ApiCallerStubs::CImagePaletteNode::Run() {
    if (m_palette) {
        DeleteObject(m_palette);
        m_palette = 0;
    }
    m_flags = 0;
}

// ---------------------------------------------------------------------------
// 0x1770a0: does the display device support a palette? (RC_PALETTE bit)
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

// ---------------------------------------------------------------------------
// 0x1770e0: snapshot the reserved system-palette entries, marking the interior
// animatable range PC_RESERVED (peFlags=1).
RVA(0x001770e0, 0x7c)
void ApiCallerStubs::CImagePaletteNode::Tune1770e0() {
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

// ---------------------------------------------------------------------------
// 0x177160: realize an all-black 256-entry palette on the screen DC to reset it.
RVA(0x00177160, 0x81)
void ApiCallerStubs::winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD() {
    char buf[4 + 256 * sizeof(PALETTEENTRY)];
    LOGPALETTE* lp = (LOGPALETTE*)buf;
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

// ---------------------------------------------------------------------------
// CImagePaletteNode::LoadPalFile (0x1771f0) - load a raw 768-byte (.PAL) palette:
// open the file, require length == 0x300, Read the 256*3 RGB bytes into a stack
// buffer, hand it to ProcessPal(buf, arg). Any I/O failure returns 0. The CFileIO
// stack object forces the /GX EH frame. __thiscall, ret 8.
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
// and hand it to BuildPalette(table, arg). The CFileIO stack object forces the
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
    return Build((PALETTEENTRY*)rgbq, arg);
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
    u8* s = (u8*)buf + size - 0x300;
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
