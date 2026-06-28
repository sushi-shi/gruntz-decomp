// ImagePool.cpp - the engine image/palette resource pool (C:\Proj\...\Image).
// The pool keeps two MFC CObLists - a list of GDI surface nodes (+0x10) and a
// list of palette nodes (+0x2c) - plus a teardown/add/remove API over them. The
// node payloads are the image-surface + palette helper objects already modeled
// (under placeholder names) in src/Stub/ApiCallers.cpp and src/Image/Image.cpp:
//
//   GdiOwner_175c90    (surface node)  - Cleanup() @0x175c90 releases its GDI
//                       object + buffer; SetPalette() @0x176ad0 (here) latches the
//                       associated palette node ptr (+0x458) and a scalar (+0x454).
//   PalBuilder_176df0  (palette node)  - Build() @0x176df0 realizes an HPALETTE
//                       from a PALETTEENTRY[256]; ProcessPal/ParseDispatch/
//                       ParsePaletteTail (here) are the format front-ends that
//                       fill that array from a packed-RGB / dispatched / trailing-
//                       768-byte source then forward to Build.
//   DeleteObjHost_177070 (palette node, free view) - Run() @0x177070 deletes the
//                       realized HPALETTE before the node is freed.
//   CImageExtLoader    - LoadByExtension() @0x176f90 (Image.cpp) loads a palette
//                       node from a file by extension.
//
// Those four callees are external (reloc-masked): named here exactly as their
// owning TUs spell them so the pool's calls reloc-mask. The pool walks/edits the
// CObLists through the real MFC CObList (AddTail/RemoveAll/RemoveAt are NAFXCW,
// reloc-masked via library labels) and (de)allocates nodes through the engine
// RezAlloc/RezFree. Field names are placeholders; only OFFSETS + code bytes are
// load-bearing.
#include <Mfc.h>        // CObList / POSITION + <windows.h> PALETTEENTRY
#include <Rez/RezMgr.h> // RezAlloc/RezFree (_RezAlloc 0x1b9b46 / _RezFree 0x1b9b82)
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
    // The GDI surface node (list +0x10). Cleanup is the foreign teardown; SetPalette
    // (this TU) latches the associated palette node ptr.
    struct GdiOwner_175c90 {
        char m_pad0[0x428];                // +0x000..+0x427
        i32 m_428;                         // +0x428
        i32 m_42c;                         // +0x42c
        i32 m_430;                         // +0x430
        i32 m_434;                         // +0x434
        i32 m_438;                         // +0x438
        i32 m_43c;                         // +0x43c
        i32 m_440;                         // +0x440 (not zeroed by the inlined ctor)
        i32 m_444;                         // +0x444
        i32 m_448;                         // +0x448
        i32 m_listPosition;                // +0x44c  cached AddTail POSITION
        i32 m_450;                         // +0x450 (not zeroed by the inlined ctor)
        i32 m_paletteValue;                // +0x454  associated scalar
        void* m_paletteNode;               // +0x458  associated palette node
        void Cleanup();                    // 0x175c90 (foreign, reloc-masked)
        void SetPalette(void* pal, i32 a); // 0x176ad0 (this TU)
    };

    // The palette node (list +0x2c). Build is the foreign realizer; the three
    // front-ends are reconstructed in this TU.
    struct PalBuilder_176df0 {
        i32 m_paletteHandle;                         // +0x000  realized HPALETTE
        char m_pad4[0x40c - 0x4];                    // +0x004..+0x40b
        i32 m_40c;                                   // +0x40c
        void* m_listPosition;                        // +0x410  cached AddTail POSITION
        i32 Build(PALETTEENTRY* entries, i32 flags); // 0x176df0 (foreign, reloc-masked)
        i32 ProcessPal(void* rgb, i32 flags);        // 0x176e70 (this TU)
        i32 ParseDispatch(void* buf, u32 size, i32 type, i32 ctrl); // 0x177040 (this TU)
        i32 ParsePaletteTail(void* buf, u32 size, i32 ctrl);        // 0x177400 (this TU)
    };

    // The palette node's free view: Run deletes the realized HPALETTE.
    struct DeleteObjHost_177070 {
        void Run(); // 0x177070 (foreign, reloc-masked)
    };
} // namespace ApiCallerStubs

// The file-backed palette loader (Image.cpp). LoadByExtension is foreign here.
class CImageExtLoader {
public:
    i32 LoadByExtension(char* path, i32 arg); // 0x176f90 (foreign, reloc-masked)
};

using ApiCallerStubs::DeleteObjHost_177070;
using ApiCallerStubs::GdiOwner_175c90;
using ApiCallerStubs::PalBuilder_176df0;

// The surface-node build views. The five surface factories below RezAlloc a fresh
// GdiOwner_175c90 node (0x45c bytes) then forward to one of these foreign decoders
// (each annotated/owned by Image.cpp / CImageBlit.cpp / CScanlineSurface.cpp) - the
// `call rel32` reloc-masks against the owning TU's symbol. Minimal inline decls; the
// mangling depends only on class + method name + param types + convention.
class CImage {
public:
    i32 DecodeBmpHeader(void* a2, i32 width, i32 height, i32 bitcount, void* a3); // 0x1757c0
    i32 DecodeBlit(void*, void*, i32, i32, i32, void*);                           // 0x175930
    i32 LoadFromRez(char* name, void* a2, void* a3);                              // 0x175a90
};
class CScanlineSurface {
public:
    i32 Convert8To16(void* a0, CScanlineSurface* src, void* pal); // 0x175b80
    i32 Dispatch175a00(i32 a0, i32 kind, i32 a2, i32 a3);         // 0x175a00 (thiscall site)
};

// ---------------------------------------------------------------------------
// CImagePool - holds the two node lists + three head scalars.
// ---------------------------------------------------------------------------
class CImagePool {
public:
    i32 SetHandles(i32 a, i32 b, i32 c);                                          // 0x174e90
    void Clear();                                                                 // 0x174eb0
    void RemovePalette(PalBuilder_176df0* node);                                  // 0x174f30
    void ClearSurfaces();                                                         // 0x174f60
    void ClearPalettes();                                                         // 0x174fa0
    PalBuilder_176df0* AddPaletteEntries(PALETTEENTRY* entries, i32 flags);       // 0x1754f0
    PalBuilder_176df0* AddPaletteRGB(void* rgb, i32 flags);                       // 0x175570
    PalBuilder_176df0* AddImageFile(char* path, i32 arg);                         // 0x1755f0
    PalBuilder_176df0* AddImageDispatch(void* buf, u32 size, i32 type, i32 ctrl); // 0x175680

    GdiOwner_175c90* AddSurfaceBmp(i32 a1, i32 a2, i32 a3, i32 a4);          // 0x174fe0
    GdiOwner_175c90* AddSurfaceBlit(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5); // 0x1750e0
    GdiOwner_175c90* AddSurfaceOp(i32 a1, i32 a2, i32 a3);                   // 0x1751f0
    GdiOwner_175c90* AddSurfaceRez(i32 a1, i32 a2);                          // 0x1752f0
    GdiOwner_175c90* AddSurfaceConvert(i32 a1, i32 a2);                      // 0x1753f0

    i32 m_resourceModule; // +0x00  resource module handle
    i32 m_sourceHwnd;     // +0x04  source HWND (GetDC/ReleaseDC)
    i32 m_08;             // +0x08
    i32 m_restorePalette; // +0x0c  selected HPALETTE to restore
    CObList m_surfaces;   // +0x10  GDI surface nodes (m_pHead @+0x14)
    CObList m_palettes;   // +0x2c  palette nodes (m_pHead @+0x30)
    i32 m_48;             // +0x48
};

// ===========================================================================
// CImagePool::SetHandles  @0x174e90 (ret 0xc) - seed the three head scalars.
// ===========================================================================
RVA(0x00174e90, 0x1c)
i32 CImagePool::SetHandles(i32 a, i32 b, i32 c) {
    m_resourceModule = a;
    m_08 = c;
    m_sourceHwnd = b;
    return 1;
}

// ===========================================================================
// CImagePool::Clear  @0x174eb0 - tear down both lists, zero the head scalars.
// ===========================================================================
RVA(0x00174eb0, 0x1b)
void CImagePool::Clear() {
    ClearSurfaces();
    ClearPalettes();
    m_resourceModule = 0;
    m_sourceHwnd = 0;
    m_08 = 0;
}

// ===========================================================================
// CImagePool::RemovePalette  @0x174f30 (ret 4) - unlink one palette node from
// the +0x2c list (by its cached POSITION), delete its HPALETTE, free it.
// ===========================================================================
RVA(0x00174f30, 0x30)
void CImagePool::RemovePalette(PalBuilder_176df0* node) {
    if (!node) {
        return;
    }
    if (node->m_listPosition) {
        m_palettes.RemoveAt((POSITION)node->m_listPosition);
    }
    ((DeleteObjHost_177070*)node)->Run();
    RezFree(node);
}

// ===========================================================================
// CImagePool::ClearSurfaces  @0x174f60 - delete every surface node, RemoveAll.
// ===========================================================================
RVA(0x00174f60, 0x37)
void CImagePool::ClearSurfaces() {
    POSITION pos = m_surfaces.GetHeadPosition();
    while (pos) {
        GdiOwner_175c90* item = (GdiOwner_175c90*)m_surfaces.GetNext(pos);
        if (item) {
            item->Cleanup();
            RezFree(item);
        }
    }
    m_surfaces.RemoveAll();
}

// ===========================================================================
// CImagePool::ClearPalettes  @0x174fa0 - delete every palette node, RemoveAll,
// then clear +0x48.
// ===========================================================================
RVA(0x00174fa0, 0x3e)
void CImagePool::ClearPalettes() {
    POSITION pos = m_palettes.GetHeadPosition();
    while (pos) {
        DeleteObjHost_177070* item = (DeleteObjHost_177070*)m_palettes.GetNext(pos);
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
// returning the node (or, on decode failure, Cleanup'ing + freeing it -> 0).
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
GdiOwner_175c90* CImagePool::AddSurfaceBmp(i32 a1, i32 a2, i32 a3, i32 a4) {
    HDC hdc = GetDC((HWND)m_sourceHwnd);
    GdiOwner_175c90* node;
    GdiOwner_175c90* raw = (GdiOwner_175c90*)RezAlloc(0x45c);
    if (raw) {
        raw->m_428 = 0;
        raw->m_42c = 0;
        raw->m_430 = 0;
        raw->m_434 = 0;
        raw->m_438 = 0;
        raw->m_43c = 0;
        raw->m_444 = 0;
        raw->m_448 = 0;
        raw->m_listPosition = 0;
        raw->m_paletteValue = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (((CImage*)node)->DecodeBmpHeader((void*)hdc, a1, a2, a3, (void*)a4) == 0) {
        if (m_restorePalette) {
            SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
            m_restorePalette = 0;
        }
        ReleaseDC((HWND)m_sourceHwnd, hdc);
        if (node) {
            node->Cleanup();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = (i32)m_surfaces.AddTail((CObject*)node);
    if (m_restorePalette) {
        SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
        m_restorePalette = 0;
    }
    ReleaseDC((HWND)m_sourceHwnd, hdc);
    return node;
}

// @early-stop
// regalloc tie-break: node<->edi / zero<->ebx swap vs retail (see AddSurfaceBmp).
RVA(0x001750e0, 0x103)
GdiOwner_175c90* CImagePool::AddSurfaceBlit(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5) {
    HDC hdc = GetDC((HWND)m_sourceHwnd);
    GdiOwner_175c90* node;
    GdiOwner_175c90* raw = (GdiOwner_175c90*)RezAlloc(0x45c);
    if (raw) {
        raw->m_428 = 0;
        raw->m_42c = 0;
        raw->m_430 = 0;
        raw->m_434 = 0;
        raw->m_438 = 0;
        raw->m_43c = 0;
        raw->m_444 = 0;
        raw->m_448 = 0;
        raw->m_listPosition = 0;
        raw->m_paletteValue = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (((CImage*)node)->DecodeBlit((void*)a1, (void*)hdc, a2, a3, a4, (void*)a5) == 0) {
        if (m_restorePalette) {
            SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
            m_restorePalette = 0;
        }
        ReleaseDC((HWND)m_sourceHwnd, hdc);
        if (node) {
            node->Cleanup();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = (i32)m_surfaces.AddTail((CObject*)node);
    if (m_restorePalette) {
        SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
        m_restorePalette = 0;
    }
    ReleaseDC((HWND)m_sourceHwnd, hdc);
    return node;
}

// @early-stop
// regalloc tie-break: node<->edi / zero<->ebx swap vs retail (see AddSurfaceBmp).
RVA(0x001751f0, 0xf9)
GdiOwner_175c90* CImagePool::AddSurfaceOp(i32 a1, i32 a2, i32 a3) {
    HDC hdc = GetDC((HWND)m_sourceHwnd);
    GdiOwner_175c90* node;
    GdiOwner_175c90* raw = (GdiOwner_175c90*)RezAlloc(0x45c);
    if (raw) {
        raw->m_428 = 0;
        raw->m_42c = 0;
        raw->m_430 = 0;
        raw->m_434 = 0;
        raw->m_438 = 0;
        raw->m_43c = 0;
        raw->m_444 = 0;
        raw->m_448 = 0;
        raw->m_listPosition = 0;
        raw->m_paletteValue = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (((CScanlineSurface*)node)->Dispatch175a00(a1, a2, (i32)hdc, a3) == 0) {
        if (m_restorePalette) {
            SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
            m_restorePalette = 0;
        }
        ReleaseDC((HWND)m_sourceHwnd, hdc);
        if (node) {
            node->Cleanup();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = (i32)m_surfaces.AddTail((CObject*)node);
    if (m_restorePalette) {
        SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
        m_restorePalette = 0;
    }
    ReleaseDC((HWND)m_sourceHwnd, hdc);
    return node;
}

// @early-stop
// regalloc tie-break: node<->edi / zero<->ebx swap vs retail (see AddSurfaceBmp).
RVA(0x001752f0, 0xfc)
GdiOwner_175c90* CImagePool::AddSurfaceRez(i32 a1, i32 a2) {
    HDC hdc = GetDC((HWND)m_sourceHwnd);
    g_hResModule = (void*)m_resourceModule;
    GdiOwner_175c90* node;
    GdiOwner_175c90* raw = (GdiOwner_175c90*)RezAlloc(0x45c);
    if (raw) {
        raw->m_428 = 0;
        raw->m_42c = 0;
        raw->m_430 = 0;
        raw->m_434 = 0;
        raw->m_438 = 0;
        raw->m_43c = 0;
        raw->m_444 = 0;
        raw->m_448 = 0;
        raw->m_listPosition = 0;
        raw->m_paletteValue = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (((CImage*)node)->LoadFromRez((char*)a1, (void*)hdc, (void*)a2) == 0) {
        if (m_restorePalette) {
            SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
            m_restorePalette = 0;
        }
        ReleaseDC((HWND)m_sourceHwnd, hdc);
        if (node) {
            node->Cleanup();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = (i32)m_surfaces.AddTail((CObject*)node);
    if (m_restorePalette) {
        SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
        m_restorePalette = 0;
    }
    ReleaseDC((HWND)m_sourceHwnd, hdc);
    return node;
}

// @early-stop
// regalloc tie-break: node<->edi / zero<->ebx swap vs retail (see AddSurfaceBmp).
RVA(0x001753f0, 0xf4)
GdiOwner_175c90* CImagePool::AddSurfaceConvert(i32 a1, i32 a2) {
    HDC hdc = GetDC((HWND)m_sourceHwnd);
    GdiOwner_175c90* node;
    GdiOwner_175c90* raw = (GdiOwner_175c90*)RezAlloc(0x45c);
    if (raw) {
        raw->m_428 = 0;
        raw->m_42c = 0;
        raw->m_430 = 0;
        raw->m_434 = 0;
        raw->m_438 = 0;
        raw->m_43c = 0;
        raw->m_444 = 0;
        raw->m_448 = 0;
        raw->m_listPosition = 0;
        raw->m_paletteValue = 0;
        raw->m_paletteNode = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (((CScanlineSurface*)node)->Convert8To16((void*)hdc, (CScanlineSurface*)a1, (void*)a2)
        == 0) {
        if (m_restorePalette) {
            SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
            m_restorePalette = 0;
        }
        ReleaseDC((HWND)m_sourceHwnd, hdc);
        if (node) {
            node->Cleanup();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = (i32)m_surfaces.AddTail((CObject*)node);
    if (m_restorePalette) {
        SelectPalette(hdc, (HPALETTE)m_restorePalette, FALSE);
        m_restorePalette = 0;
    }
    ReleaseDC((HWND)m_sourceHwnd, hdc);
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
// only because its early m_resourceModule read pins `this`=edi; with `this` first used at the
// tail there is no source lever. Logic byte-identical.
RVA(0x001754f0, 0x7b)
PalBuilder_176df0* CImagePool::AddPaletteEntries(PALETTEENTRY* entries, i32 flags) {
    PalBuilder_176df0* node;
    PalBuilder_176df0* raw = (PalBuilder_176df0*)RezAlloc(0x414);
    if (raw) {
        raw->m_paletteHandle = 0;
        raw->m_40c = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (node->Build(entries, flags) == 0) {
        if (node) {
            ((DeleteObjHost_177070*)node)->Run();
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
PalBuilder_176df0* CImagePool::AddPaletteRGB(void* rgb, i32 flags) {
    PalBuilder_176df0* node;
    PalBuilder_176df0* raw = (PalBuilder_176df0*)RezAlloc(0x414);
    if (raw) {
        raw->m_paletteHandle = 0;
        raw->m_40c = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (node->ProcessPal(rgb, flags) == 0) {
        if (node) {
            ((DeleteObjHost_177070*)node)->Run();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_palettes.AddTail((CObject*)node);
    return node;
}

RVA(0x001755f0, 0x82)
PalBuilder_176df0* CImagePool::AddImageFile(char* path, i32 arg) {
    g_hResModule = (void*)m_resourceModule;
    PalBuilder_176df0* node;
    PalBuilder_176df0* raw = (PalBuilder_176df0*)RezAlloc(0x414);
    if (raw) {
        raw->m_paletteHandle = 0;
        raw->m_40c = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (((CImageExtLoader*)node)->LoadByExtension(path, arg) == 0) {
        if (node) {
            ((DeleteObjHost_177070*)node)->Run();
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
PalBuilder_176df0* CImagePool::AddImageDispatch(void* buf, u32 size, i32 type, i32 ctrl) {
    PalBuilder_176df0* node;
    PalBuilder_176df0* raw = (PalBuilder_176df0*)RezAlloc(0x414);
    if (raw) {
        raw->m_paletteHandle = 0;
        raw->m_40c = 0;
        raw->m_listPosition = 0;
        node = raw;
    } else {
        node = 0;
    }
    if (node->ParseDispatch(buf, size, type, ctrl) == 0) {
        if (node) {
            ((DeleteObjHost_177070*)node)->Run();
            RezFree(node);
        }
        return 0;
    }
    node->m_listPosition = m_palettes.AddTail((CObject*)node);
    return node;
}

// ===========================================================================
// GdiOwner_175c90::SetPalette  @0x176ad0 (ret 8) - latch the palette node ptr
// (+0x458) and the associated scalar (+0x454).
// ===========================================================================
RVA(0x00176ad0, 0x17)
void ApiCallerStubs::GdiOwner_175c90::SetPalette(void* pal, i32 a) {
    m_paletteNode = pal;
    m_paletteValue = a;
}

// ===========================================================================
// PalBuilder_176df0::ProcessPal  @0x176e70 (ret 8) - expand a packed RGB-triple
// source into a PALETTEENTRY[256] (peFlags untouched) then realize it.
// ===========================================================================
RVA(0x00176e70, 0x4e)
i32 ApiCallerStubs::PalBuilder_176df0::ProcessPal(void* rgb, i32 flags) {
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
// PalBuilder_176df0::ParseDispatch  @0x177040 (ret 0x10) - format-3 -> the
// trailing-palette path; anything else fails.
// ===========================================================================
RVA(0x00177040, 0x23)
i32 ApiCallerStubs::PalBuilder_176df0::ParseDispatch(void* buf, u32 size, i32 type, i32 ctrl) {
    if (type == 3) {
        return ParsePaletteTail(buf, size, ctrl);
    }
    return 0;
}

// ===========================================================================
// PalBuilder_176df0::ParsePaletteTail  @0x177400 (ret 0xc) - extract the
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
i32 ApiCallerStubs::PalBuilder_176df0::ParsePaletteTail(void* buf, u32 size, i32 ctrl) {
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
