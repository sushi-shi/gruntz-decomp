// DDrawPtrCollections.cpp - the 0x148840-0x148cd8 surface-extras pocket, an
// ORIGINAL DDrawMgr TU (wave4-K consolidation; interval dossier #14, pocket
// note): the WOVEN leftover second cores of the former image + ddrawptrcollections
// + fileimageloadbyext units - CDDSurface's LoadKeyed/ResolveEx/LoadByExt/
// UpdateOverlay extras interleaved with the CPoolItem* derived-slot bodies. One
// obj (TU_MIGRATION row `0x148840` WOVEN 0.29); its original file name is
// unrecovered (a fourth DDrawMgr surface file - no __FILE__ anchor). The unit
// keeps the ddrawptrcollections name (delinker packing); the in-band 0x141cc0-
// 0x143c20 pool/factory methods moved to DirectDrawMgr.cpp (the DDRAWMGR.CPP
// obj, dossier #14H).
#include <Mfc.h> // real MFC types (NAFXCW, reloc-masked) - afx-first
#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DDrawPtrCollections.h> // CDDrawPtrCollections + the CPoolItem* family
#include <Image/Image.h>                  // the single-source CDDSurface extras' records
#include <ddraw.h>                        // IDirectDrawSurface (UpdateOverlay passthrough)
#include <string.h>                       // memset (inlined to rep stos at /O2 /Oi)
#include <Globals.h>

// The engine's own strrchr / case-insensitive compare (cdecl C-linkage helpers).
extern "C" char* RezStrrchr(const char* s, i32 c);       // FUN_00120680 (_RezStrrchr)
extern "C" i32 RezStricmp(const char* a, const char* b); // FUN_0011fdf0 (_RezStricmp)

// The RGB low-bit-position / 8-minus-bitcount pair tables InstallColorFormat fills
// (the same six .data words ComputeColorMasks in the DDRAWMGR obj writes).
extern "C" {
    DATA(0x00283ea0)
    extern i32 g_683ea0; // red   low-bit shift
    DATA(0x00283ea4)
    extern i32 g_683ea4; // green low-bit shift
    DATA(0x00283ea8)
    extern i32 g_683ea8; // blue  low-bit shift
    DATA(0x00283eac)
    extern i32 g_683eac; // red   8-minus-count
    DATA(0x00283eb0)
    extern i32 g_683eb0; // green 8-minus-count
    DATA(0x00283eb4)
    extern i32 g_683eb4; // blue  8-minus-count
}

// The post-mask surface-format apply (BuildColorChannelTables @0x13f740, the
// DIRSURF obj); free-fn decl so the bare `call rel32` reloc-masks.
void Boundary_13f740();

// ---------------------------------------------------------------------------
// CDDSurface::LoadKeyed (ret 0x18) - blit a source surface in with the control word
// forced to OR 0x40 (BlitSurf), and on success - unless the colour key is -1 -
// install it via FillPalette. Returns 1.
RVA(0x00148840, 0x47)
i32 CDDSurface::LoadKeyed(void* surf, i32 width, i32 height, i32 a4, i32 a5, i32 key) {
    // Direct (non-virtual) dispatch to the slot-3 body: qualified call suppresses the
    // vtable indirection (retail direct-calls 0x13e0d0 here).
    if (CDDSurface::BlitSurf(surf, width, height, a4, a5 | 0x40) == 0) {
        return 0;
    }
    if (key != -1) {
        FillPalette(key);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CDDSurface::ResolveEx
// The surface-blit format dispatcher: like Resolve but for the *Data decoders
// that blit straight into the destination surface. The control word is OR'd with
// 0x40 up front; `type` (1=BMP, 2=PCX, 4=PID) selects DecodeBmpData/DecodePcxData2/
// DecodePcxData, each handed (surf, buf, size, ctrl); PID also takes the
// transparency colour. After a successful decode the transparency colour is
// installed via FillPalette unless it is "no key" (-1) or the format already
// handled it (PID = type 4). Returns 1 on success, else 0. The case labels lower
// to the running-subtract chain (switch, bodies in retail .text order 4, 2, 1).
RVA(0x00148890, 0xad)
i32 CDDSurface::ResolveEx(void* surf, void* buf, i32 type, u32 size, i32 ctrl, i32 trans) {
    if (size == 0) {
        return 0;
    }
    i32 c = ctrl | 0x40;
    switch (type) {
        case FMT_PID:
            if (!DecodePcxData(surf, buf, size, c, trans)) {
                return 0;
            }
            break;
        case FMT_PCX:
            if (!Decode((CDDrawPtrCollections*)surf, (CFileImageSrc*)buf, (i32)size, c)) {
                return 0;
            }
            break;
        case FMT_BMP:
            if (!DecodeRun((CDDrawPtrCollections*)surf, buf, (i32)size, c)) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    if (trans != -1 && type != FMT_PID) {
        FillPalette(trans);
    }
    return 1;
}

// CDDSurface::LoadByExt - load an image by inspecting its file
// extension. Forces the IMAGEZ flag (|0x40), finds the extension, and dispatches
// to LoadFile2 (.BMP) / LoadFile (.PCX) / DecodePcxEx (.PID) or the default loader
// (Load @0x144270, the same __thiscall `this`). On a successful load (except the
// .PID path) it fills the palette from a4 when a4 != -1.
// @source: string-xref (.BMP/.PCX/.PID extension table)
//
// @early-stop
// scheduling wall (~99.9%): MSVC swaps the ORDER of the two independent tail loads at
// 0xdd - retail emits `mov esi,[esp+0x20]` (a4) then `mov eax,[esp+0x1c]` (doFill);
// the recompile emits them reversed. Same regs, same short-circuit, only the load
// schedule differs; all other code bytes are byte-identical (llvm-objdump -dr base vs
// target). A pure scheduler tie-break that flips on ANY change to the widely-included
// surface symbol table: it had incidentally drifted to 100% at one baseline, was
// re-triggered by the CScanlineSurface/CImageSurfaceNode fold, and flipped back to
// ~99.9% by the CFileImage->CDDSurface 3-name unification (this method now lives on
// the unified CDDSurface, callees renamed + the header pulls <Mfc.h>). Not source-
// steerable (reordering the `&&` -> 96%); the correct unified-class shape is kept
// over the coin-flip byte-match (per the no-multiple-views mandate).
RVA(0x00148940, 0x102)
i32 CDDSurface::LoadByExt(CDDrawPtrCollections* info, char* path, i32 flags, i32 key) {
    flags |= 0x40;
    i32 doFill = 1;
    char* ext = RezStrrchr(path, '.');
    if (ext != 0 && RezStricmp(ext, ".BMP") == 0) {
        if (LoadFile2(info, path, flags) == 0) {
            return 0;
        }
    } else if (ext != 0 && RezStricmp(ext, ".PCX") == 0) {
        if (LoadFile(info, path, flags) == 0) {
            return 0;
        }
    } else if (ext != 0 && RezStricmp(ext, ".PID") == 0) {
        if (DecodePcxEx(info, path, (void*)flags, (void*)key) == 0) {
            return 0;
        }
        doFill = 0;
    } else if (this->Load((i32)info, path, flags) == 0) {
        return 0;
    }
    if (key != -1 && doFill != 0) {
        FillPalette(key);
    }
    return 1;
}

// CPoolItemA88::v24 (0x148a50, slot 9, "Blit7"): build a 0x6c-byte DDSURFACEDESC on the
// stack (mode 7, ddsCaps = a4|0x80, pitch fields), then run the base surface init
// (CDDSurface::Init1 @0x13e0a0, the descriptor-driven Apply) and return success.
// __thiscall, 4 args. (Re-homed from src/Stub/BoundaryUpper2.cpp; ImgOwnedY view
// dissolved onto the real CPoolItemA88.)
// @early-stop
// descriptor-fill scheduling wall (~85%): the Apply path is the 100% Setup path, but into
// a stack-local descriptor; retail hoists the a4 load (or al,0x80) ahead of a2 while MSVC
// loads a2 first, swapping the eax/ecx assignment + a couple store slots. Logic complete.
RVA(0x00148a50, 0x6b)
i32 CPoolItemA88::v24(CDDrawPtrCollections* info, i32 a2, i32 a3, i32 a4) {
    u32 desc[(0x7c - 0x10) / 4]; // 0x6c-byte DDSURFACEDESC scratch
    memset(desc, 0, 0x6c);
    desc[3] = a2;
    desc[0x1a] = a4 | 0x80;
    desc[2] = a3;
    desc[0x10] = 1;
    desc[0x11] = 1;
    desc[0] = 0x6c;
    desc[1] = 7;
    return CDDSurface::Init1(info, (i32)desc) != 0;
}

// ---------------------------------------------------------------------------
// UpdateOverlay (0x148ac0, __thiscall, ret 0x14 => 5 args). Thin passthrough to the
// held IDirectDrawSurface::UpdateOverlay (COM slot 33 / +0x84), routing the source/
// dest rects, the destination wrapper's own held surface, the flags + the overlay FX.
// ---------------------------------------------------------------------------
RVA(0x00148ac0, 0x2b)
i32 CDDSurface::UpdateOverlay(
    void* srcRect,
    CDDSurface* dest,
    void* destRect,
    u32 flags,
    void* fx
) {
    return m_8
        ->UpdateOverlay((LPRECT)srcRect, dest->m_8, (LPRECT)destRect, flags, (LPDDOVERLAYFX)fx);
}

// ---------------------------------------------------------------------------
// CPoolItemAB8::v24 (0x148af0, slot 9, "Setup"): zero the surface's embedded 0x6c-byte
// DDSURFACEDESC (m_ddsd), fill {dwSize, dwFlags=a3, [+0x14]=a4, ddsCaps=a2|0x200}, run
// the base surface init (CDDSurface::Init1 @0x13e0a0, descriptor NULL => use m_ddsd); on
// success run InstallColorFormat (slot 10) and return 1. __thiscall, 4 args. (Re-homed
// from src/Stub/BoundaryUpper2.cpp; ImgOwnedX view dissolved onto the real CPoolItemAB8.)
// Byte-exact.
RVA(0x00148af0, 0x58)
i32 CPoolItemAB8::v24(CDDrawPtrCollections* info, i32 a2, i32 a3, i32 a4) {
    memset(m_ddsd, 0, 0x6c);
    m_ddsd[0] = 0x6c;
    m_ddsd[0x1a] = a2 | 0x200;
    m_ddsd[1] = a3;
    m_ddsd[5] = a4;
    if (!CDDSurface::Init1(info, 0)) {
        return 0;
    }
    InstallColorFormat();
    return 1;
}

// ---------------------------------------------------------------------------
// CPoolItemAB8::Init1 (0x148b50, vtable slot 2 override): run the base
// CDDSurface::Init1, and on success install the color format (slot 10). Folded
// from Stub/BoundaryUpper.cpp (ImgOwned::Commit).
// ---------------------------------------------------------------------------
RVA(0x00148b50, 0x2c)
i32 CPoolItemAB8::Init1(CDDrawPtrCollections* h, i32 a) {
    if (CDDSurface::Init1(h, a) == 0) {
        return 0;
    }
    InstallColorFormat();
    return 1;
}

// ---------------------------------------------------------------------------
// CPoolItemAB8::InstallColorFormat (0x148b80, slot 10, __thiscall, no args).
// The sibling of ComputeColorMasks (0x143b20) below: derives the live screen
// RGB-format shift/loss globals, but reads the channel bitmasks straight from
// this surface's already-cached DDPIXELFORMAT fields (m_rMask/m_gMask/m_bMask)
// rather than a GetSurfaceDesc. The "up" shift is the channel's lowest set-bit
// index; the "down" loss is 8 - popcount. Then re-applies (Boundary_13f740).
// ---------------------------------------------------------------------------
// @early-stop
// entropy tail (~99.7%, permuter-maximized): structurally identical to the EXACT
// ComputeColorMasks; residual is one 8-count register/materialization slot in the
// last channel's store that cl schedules a hair differently. Not source-steerable.
RVA(0x00148b80, 0xb5)
i32 CPoolItemAB8::InstallColorFormat() {
    u32 m = m_rMask;
    i32 count = 0;
    i32 shift;
    shift = -1;
    for (i32 b = 0; b < 0x20; b++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b;
            }
            count++;
        }
        m >>= 1;
    }
    g_683ea0 = shift;
    g_683eac = 8 - count;

    shift = -1;
    m = m_gMask;
    count = 0;
    for (i32 b2 = 0; b2 < 0x20; b2++) {
        if ((1 & m) == 1) {
            if (shift == -1) {
                shift = b2;
            }
            count++;
        }
        m >>= 1;
    }
    g_683eb0 = 8 - count;
    g_683ea4 = shift;

    count = 0;
    m = m_bMask;
    shift = -1;
    for (i32 b3 = 0; b3 < 0x20; b3++) {
        if ((m & 1) == 1) {
            if (shift == -1) {
                shift = b3;
            }
            count++;
        }
        m >>= 1;
    }
    g_683eb4 = 8 - count;
    g_683ea8 = shift;

    Boundary_13f740();
    return 1;
}

// ---------------------------------------------------------------------------
// CPoolItemAE8::v24 (0x148c40, slot 9, "Blit47"): build a 0x6c-byte DDSURFACEDESC on the
// stack (mode 0x47, ddsCaps = a5|a4|0x20000, [+0x18]=a7), then run the base surface init
// (CDDSurface::Init1 @0x13e0a0) and return success. __thiscall, 7 args (a6 unused).
// (Re-homed from src/Stub/BoundaryUpper2.cpp; ImgOwnedY view dissolved onto the real
// CPoolItemAE8.)
// @early-stop
// descriptor-fill scheduling wall (~85%): mirror of CPoolItemA88::v24 (7-arg / mode 0x47).
// Same stack-local-descriptor load/store scheduling divergence. Logic complete.
RVA(0x00148c40, 0x75)
i32 CPoolItemAE8::v24(CDDrawPtrCollections* info, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6, i32 a7) {
    (void)a6;
    u32 desc[(0x7c - 0x10) / 4]; // 0x6c-byte DDSURFACEDESC scratch
    memset(desc, 0, 0x6c);
    desc[3] = a2;
    desc[0x1a] = a5 | a4 | 0x20000;
    desc[6] = a7;
    desc[2] = a3;
    desc[0] = 0x6c;
    desc[1] = 0x47;
    return CDDSurface::Init1(info, (i32)desc) != 0;
}

// ---------------------------------------------------------------------------
// CPoolItemAE8::Init1 (0x148cc0, vtable slot 2 override): boolify the base
// CDDSurface::Init1 result. Folded from Stub/BoundaryUpper.cpp (ImgOwned::Forward).
// ---------------------------------------------------------------------------
RVA(0x00148cc0, 0x18)
i32 CPoolItemAE8::Init1(CDDrawPtrCollections* h, i32 a) {
    return CDDSurface::Init1(h, a) != 0;
}

// --- vtable catalog ---
