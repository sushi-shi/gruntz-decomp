// ImagePool.cpp - the engine image/palette resource pool (C:\Proj\...\Image).
// The pool keeps two MFC CObLists - a list of GDI surface nodes (+0x10) and a
// list of palette nodes (+0x2c) - plus a teardown/add/remove API over them. The
// node payloads are the two engine node classes below (former placeholder views
// GdiOwner_175c90 / PalBuilder_176df0 / DeleteObjHost_177070 folded to real names):
//
//   CRezImage           (surface node)  - the shared DIB-surface class in
//                       <Image/Image.h> (former pool-local CImageSurfaceNode view folded
//                       onto it). Free() @0x175c90 releases its GDI object + buffer;
//                       SetPalette() @0x176ad0 (here) latches the palette node ptr
//                       (+0x458) and a scalar (+0x454). The decoders @0x1757c0 (Create/
//                       DecodeBmpHeader) / 0x175b80 (Convert8To16) / 0x175a00
//                       (DispatchDecode) are its methods, defined in Image.cpp /
//                       CScanlineSurface.cpp - one class across all three TUs.
//   CImagePaletteNode   (palette node)  - Build() @0x176df0 realizes an HPALETTE
//                       from a PALETTEENTRY[256]; ProcessPal/ParseDispatch/
//                       ParsePaletteTail (here) are the format front-ends that fill
//                       that array from a packed-RGB / dispatched / trailing-768-
//                       byte source then forward to Build. Run() @0x177070 (former
//                       DeleteObjHost free view, folded in) deletes the HPALETTE.
//   CImgPoolExtLoader   - LoadByExtension() @0x176f90 (Image.cpp) loads a palette
//                       node from a file by extension.
//
// The decoder callees are external (reloc-masked): named here exactly as their
// owning TUs spell them so the pool's calls reloc-mask. The pool walks/edits the
// CObLists through the real MFC CObList (AddTail/RemoveAll/RemoveAt are NAFXCW,
// reloc-masked via library labels) and (de)allocates nodes through the engine
// RezAlloc/RezFree. Field names are placeholders; only OFFSETS + code bytes are
// load-bearing.
#include <Mfc.h>             // CObList / POSITION + <windows.h> PALETTEENTRY
#include <Image/Image.h>     // CRezImage - the shared DIB-surface class (the pool's surface node)
#include <Image/ImagePool.h> // the canonical CImagePool (this TU owns its bodies)
#include <Rez/RezMgr.h>      // RezAlloc/RezFree (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82)
#include <rva.h>

// The selected-image resource module handle, latched into the engine global
// _g_hResModule (0x6bf6e0) before a file-backed add. External, reloc-masked.
extern "C" void* g_hResModule;

// ---------------------------------------------------------------------------
// The node payload types. Declared with the EXACT class/namespace/signature the
// owning TUs use so the pool's calls into them reloc-mask. Only the offsets the
// pool touches are pinned.
// ---------------------------------------------------------------------------
namespace ApiCallerStubs {
    struct CImagePaletteNode; // fwd (CRezImage::m_paletteNode points at one)

    // The surface list node (+0x10 list) IS CRezImage, the shared DIB-surface class in
    // <Image/Image.h> (former pool-local CImageSurfaceNode view folded onto it): Free
    // @0x175c90 (== the loaders' Free), and the decoders @0x1757c0/0x175b80/0x175a00 are
    // CRezImage::DecodeBmpHeader/Convert8To16/DispatchDecode - one class across Image.cpp /
    // CScanlineSurface.cpp / here. Free releases the GDI DIB object + the row-offset table;
    // SetPalette (@0x176ad0, defined below) latches the associated palette node.

    // The palette list node (+0x2c list). Build realizes the HPALETTE from a 256-entry
    // LOGPALETTE it assembles in-place; the front-ends fill that array. Run (former
    // CImagePaletteNode free view, folded in) deletes the realized HPALETTE before
    // the node is freed.
    struct CImagePaletteNode {
        HPALETTE m_palette;                     // +0x000  realized HPALETTE (CreatePalette)
        LOGPALETTE m_pal;                       // +0x004  header + entry[0]
        char m_padEntries[0x408 - (4 + 4 + 4)]; // entry[1..255] -> +0x408
        i32 m_flags;                            // +0x408  Build's stored flags (Run zeroes it)
        i32 m_systemTuned;                      // +0x40c  1 when reserved system range snapshotted
        POSITION m_listPosition;                // +0x410  cached AddTail POSITION
        i32 Build(PALETTEENTRY* entries, i32 flags);                // 0x176df0 (this TU)
        void Tune1770e0();                                          // 0x1770e0 (this TU)
        i32 ProcessPal(void* rgb, i32 flags);                       // 0x176e70 (this TU)
        i32 ParseDispatch(void* buf, u32 size, i32 type, i32 ctrl); // 0x177040 (this TU)
        i32 ParsePaletteTail(void* buf, u32 size, i32 ctrl);        // 0x177400 (this TU)
        void Run();                                                 // 0x177070 (this TU)
    };

    // Two free GDI palette helpers PalBuilder::Build/Tune funnel through (defined at
    // EOF): 0x1770a0 probes display-palette support; 0x177160 resets the screen
    // palette to all-black. __cdecl.
    i32 winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps();
    void winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD();
} // namespace ApiCallerStubs

// The file-backed palette loader (Image.cpp). LoadByExtension is foreign here.
SIZE_UNKNOWN(CImageExtLoader);
// The real (header-less image-unit) CImageExtLoader; LoadByExtension @0x176f90.
class CImageExtLoader {
public:
    i32 LoadByExtension(char* path, i32 arg);
};

// CImagePool is the canonical class in <Image/ImagePool.h> (included above); this
// TU owns its method bodies. The five surface factories below RezAlloc a fresh
// CRezImage node (0x45c bytes) then forward to one of its decoders (DecodeBmpHeader/
// DecodeBlit/LoadFromRez/DispatchDecode/Convert8To16, defined in Image.cpp /
// CScanlineSurface.cpp) - external here, the `call rel32` reloc-masks.

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
        m_surfaces.RemoveAt((POSITION)node->m_listPosition);
    }
    node->Free();
    RezFree(node);
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
    ((CImagePaletteNode*)node)->Run();
    RezFree(node);
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
            RezFree(item);
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
            RezFree(item);
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
    CRezImage* raw = (CRezImage*)RezAlloc(0x45c);
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
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail((CObject*)node);
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
    CRezImage* raw = (CRezImage*)RezAlloc(0x45c);
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
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail((CObject*)node);
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
    CRezImage* raw = (CRezImage*)RezAlloc(0x45c);
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
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail((CObject*)node);
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
    CRezImage* raw = (CRezImage*)RezAlloc(0x45c);
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
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail((CObject*)node);
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
    CRezImage* raw = (CRezImage*)RezAlloc(0x45c);
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
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_surfaces.AddTail((CObject*)node);
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
    CImagePaletteNode* raw = (CImagePaletteNode*)RezAlloc(0x414);
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
            ((CImagePaletteNode*)node)->Run();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_palettes.AddTail((CObject*)node);
    return node;
}

// @early-stop
// this-register regalloc wall (~99%): same as AddPaletteEntries (this in ebx vs
// retail edi). Logic byte-identical.
RVA(0x00175570, 0x7b)
CImagePaletteNode* CImagePool::AddPaletteRGB(void* rgb, i32 flags) {
    CImagePaletteNode* node;
    CImagePaletteNode* raw = (CImagePaletteNode*)RezAlloc(0x414);
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
            ((CImagePaletteNode*)node)->Run();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_palettes.AddTail((CObject*)node);
    return node;
}

RVA(0x001755f0, 0x82)
CImagePaletteNode* CImagePool::AddImageFile(char* path, i32 arg) {
    g_hResModule = m_resourceModuleHandle;
    CImagePaletteNode* node;
    CImagePaletteNode* raw = (CImagePaletteNode*)RezAlloc(0x414);
    if (raw) {
        raw->m_palette = 0;
        raw->m_systemTuned = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (((CImageExtLoader*)node)->LoadByExtension(path, arg) == 0) {
        if (node) {
            ((CImagePaletteNode*)node)->Run();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_palettes.AddTail((CObject*)node);
    return node;
}

// @early-stop
// this-register regalloc wall (~99%): same as AddPaletteEntries (this in ebx vs
// retail edi). Logic byte-identical.
RVA(0x00175680, 0x85)
CImagePaletteNode* CImagePool::AddImageDispatch(void* buf, u32 size, i32 type, i32 ctrl) {
    CImagePaletteNode* node;
    CImagePaletteNode* raw = (CImagePaletteNode*)RezAlloc(0x414);
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
            ((CImagePaletteNode*)node)->Run();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_palettes.AddTail((CObject*)node);
    return node;
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

// CRezImage::SetPalette (0x00176ad0) is now an inline member in the header.


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

// ===========================================================================
// The node-teardown / palette-realize methods + the two GDI helpers, re-homed from
// src/Stub/ApiCallers.cpp (CImagePool owns these node types). All GDI callees are
// GDI32 imports (reloc-masked); RezFree is the pool allocator (0x1b9b82).
// ===========================================================================
// 0x175c90: release the cached GDI object + Rez buffer and clear the slots.
// The pool's node teardown IS CRezImage::Free (the same 0x175c90 the loaders' EnsureSize
// calls) - one shared DIB-surface class.
RVA(0x00175c90, 0x45)
void CRezImage::Free() {
    if (m_dibSection) {
        DeleteObject(m_dibSection);
        m_dibSection = 0;
    }
    if (m_rowOffsets) {
        RezFree(m_rowOffsets);
        m_rowOffsets = 0;
    }
    m_pixels = 0;
    m_paletteNode = 0;
}

namespace ApiCallerStubs {
    // 0x176df0: build a 256-entry LOGPALETTE from src, optionally reserve the system
    // range, then realize it into m_palette.
    RVA(0x00176df0, 0x71)
    i32 CImagePaletteNode::Build(PALETTEENTRY* src, i32 flags) {
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

    // 0x177070: delete the owned GDI object, then clear a far flag.
    RVA(0x00177070, 0x22)
    void CImagePaletteNode::Run() {
        if (m_palette) {
            DeleteObject(m_palette);
            m_palette = 0;
        }
        m_flags = 0;
    }

    // 0x1770a0: does the display device support a palette? (RC_PALETTE bit)
    RVA(0x001770a0, 0x3a)
    i32 winapi_1770a0_CreateICA_DeleteDC_GetDeviceCaps() {
        HDC ic = CreateICA("DISPLAY", 0, 0, 0);
        if (ic) {
            i32 caps = GetDeviceCaps(ic, RASTERCAPS) & RC_PALETTE;
            DeleteDC(ic);
            return caps;
        }
        return 0;
    }

    // 0x1770e0: snapshot the reserved system-palette entries, marking the interior
    // animatable range PC_RESERVED (peFlags=1).
    RVA(0x001770e0, 0x7c)
    void CImagePaletteNode::Tune1770e0() {
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

    // 0x177160: realize an all-black 256-entry palette on the screen DC to reset it.
    RVA(0x00177160, 0x81)
    void winapi_177160_CreatePalette_DeleteObject_GetDC_RealizePalette_ReleaseD() {
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
} // namespace ApiCallerStubs
