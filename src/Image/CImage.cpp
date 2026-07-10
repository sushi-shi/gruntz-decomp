#include <Mfc.h>
#include <Gruntz/ParseSource.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <rva.h>
#include <DDrawMgr/DDrawPtrCollections.h>
// CImage.cpp - the RTTI-confirmed polymorphic CImage (`.?AVCImage@@`, primary
// vftable @0x5eaa2c), a surface-backed image element in the DDrawMgr
// image family and a sibling of CDDrawSurfacePair. Three methods (retail-RVA
// order): the cleanup virtual FreeAll (0x153260, slot 7), the /GX destructor
// ~CImage (0x0d5e80), and RenderFrame (0x153790) - the sprite-frame draw that
// resolves a clip rectangle through a shared CResolveNode singleton then dispatches
// the +0x38 render virtual.
//
// See include/Image/CImage.h for the layout + the loader-CImage naming note.
// Field names are placeholders; only the OFFSETS + emitted bytes are load-bearing.
// The TU carries a destructible base subobject -> /GX EH frame (flags="eh").
// ---------------------------------------------------------------------------

#include <Image/CImage.h>
#include <Image/CBlitInfo.h> // canonical CBlitInfo/CBlitXform (RenderImage selector arg)
#include <Wwd/WwdFile.h>     // CPlaneRender::WrapCoord (the m_xform origin remap)

#include <DDrawMgr/DDSurface.h> // canonical CDDSurface (m_surface geometry/Fill/Blt/Reload/m_8 COM)
#include <DDrawMgr/DDrawShadeBlit.h> // canonical CDDrawShadeBlit (m_owned: new/Build/Teardown)
#include <Win32.h>                   // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>                   // real IDirectDrawSurface dispatch (m_8->IsLost/Restore)

// The engine __cdecl deallocator (reloc-masked rel32). _RezFree @0x1b9b82.
extern "C" void RezFree(void* p);

// The per-frame draw-delta mirror (BSS @0x6bf3bc) the RenderImage animate path
// consumes to advance/wrap the request's m_44 counter. Canonical binding in
// Projectile.cpp; declared address-pinned here (reloc-masked).
extern "C" u32 g_6bf3bc;

// The image-source format tag (CParseSource::GetTag, a packed 3-char fourcc) that
// both Resolve and Reload dispatch on to pick the format loader index (1..4). Named
// by the literal tag bytes (low byte first); PMB/XCP are the reversed extension
// (-> BMP / PCX loaders). Same immediates as the bare literals -> matching-neutral;
// the switch key stays (u32) so the compare keeps its unsigned ja/je codegen.
enum ImageFormatTag {
    IMGTAG_PMB = 0x424d50, // "PMB" -> BMP loader (index 1)
    IMGTAG_XCP = 0x504358, // "XCP" -> PCX loader (index 2)
    IMGTAG_DIR = 0x524944, // "DIR" -> loader index 3
    IMGTAG_DIP = 0x504944, // "DIP" -> loader index 4
};

// ---------------------------------------------------------------------------
// (vtable slot 12): Create. The Create24 sibling without the mode args:
// allocate a surface from the parent pool's 3-arg create (CreateC @0x142560) for
// (desc, capArg, flagsArg) - where flagsArg = keyed ? g_surfaceColorKey : -1 and
// capArg = g_resourceInstallActive ? 0x800 : 0 - then cache the surface geometry (w/h,
// halved) and clear the m_originX/m_originY origin. __thiscall, ret 8 (2 stack args).
// @early-stop
// 99.09% - structurally identical to the 100% sibling Create24, plus the two
// g_resourceInstallActive/B DIR32 reloc artifacts. The lone code residual is the geometry
// temp register: retail caches w(item->m_1c) in edx and h in ecx; cl (with one fewer
// call arg than Create24) swaps them to ecx/edx. The 3-vs-5-arg call shifts the post-
// call allocator state - not steerable from Create's own source.
// ---------------------------------------------------------------------------
RVA(0x00152e90, 0x8b)
i32 CImage::Create(CImageFrameDesc* desc, i32 keyed) {
    i32 flagsArg = (keyed != 0) ? g_surfaceColorKey : -1;
    i32 capArg = 0;
    if (g_resourceInstallActive != 0) {
        capArg = 0x800;
    }
    CDDSurface* item = m_parent->m_1c->Createa58_3((i32)desc, capArg, flagsArg);
    m_surface = item;
    if (item == 0) {
        return 0;
    }
    i32 w = item->m_width;
    m_width = w;
    i32 h = item->m_height;
    m_height = h;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    if (item->m_bc != 0) {
        m_loadResult = 0x11;
    } else {
        m_loadResult = 0x10;
    }
    m_originX = 0;
    m_originY = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// (vtable slot 11): Resolve. Read the source's 3-char format tag, map it
// to a format index (BMP=1, "CPX"=2, "RID"=3, "PID"=4), prime the source, then
// dispatch the +0x28 load virtual (LoadDispatch) with (resolved, index, src->m_length,
// arg); finally release the source and return the load result. __thiscall, ret 8.
//
// The tag compare lowers to MSVC's unsigned binary-search switch tree
// (cmp;ja/je against the fourcc constants) -> the key must be unsigned (the source
// returns the tag as a u32). docs/patterns/switch-key-unsigned-ja-vs-jg.md.
// ---------------------------------------------------------------------------
RVA(0x00152f20, 0x86)
i32 CImage::Resolve(CParseSource* src, i32 arg) {
    i32 index;
    switch ((u32)src->GetEntryTag()) {
        case IMGTAG_PMB: // BMP
            index = 1;
            break;
        case IMGTAG_XCP: // PCX
            index = 2;
            break;
        case IMGTAG_DIR:
            index = 3;
            break;
        case IMGTAG_DIP:
            index = 4;
            break;
        default:
            return 0;
    }
    i32 resolved = src->BeginParse();
    if (resolved == 0) {
        return 0;
    }
    i32 result =
        this->LoadDispatch((CImageFrameDesc*)resolved, (u32)index, (void*)src->m_length, arg);
    src->EndParse();
    return result;
}

// ---------------------------------------------------------------------------
// (vtable slot 10): LoadDispatch. The format-index switch (1..4); index 4
// with the desc's 0x20 flag takes the slot-13 build path (an EH /GX builder),
// otherwise the default path allocates a surface from the parent pool (CreateA),
// caching its geometry into m_width..m_loadResult. __thiscall, ret 0x10 (4 stack args).
//
// The 1..4 range-check lowers to a chain of `cmp;je` (no jump table) -> a sequence
// of explicit case tests, key unsigned (matches the cmp;ja in the caller).
// @early-stop
// 99.86% - all 108 instructions byte-identical to retail (verified llvm-objdump
// base vs target). The lone residual is the objdiff reloc-typing scoring artifact
// on the two g_resourceInstallActive/B DIR32 refs (REL32-vs-DIR32 against differently-
// named symbols); the code bytes match. See MEMORY objdiff-reloc-scoring.
// ---------------------------------------------------------------------------
RVA(0x00152fb0, 0x123)
i32 CImage::LoadDispatch(CImageFrameDesc* desc, u32 mode, void* a, i32 b) {
    if (mode != 1 && mode != 2 && mode != 3 && mode != 4) {
        return 0;
    }
    if (mode == 4 && (desc->m_04 & 0x20)) {
        if (!BuildSlot13(desc, a)) {
            return 0;
        }
        if (m_owned != 0 && (desc->m_04 & 0x40)) {
            ImageNotify(2, 0);
            return 1;
        }
        return 1;
    }
    i32 flagsArg = (b != 0) ? g_surfaceColorKey : -1;
    if (mode == 4 || mode == 3) {
        i32 g10 = desc->m_10;
        i32 g14 = desc->m_14;
        m_originX = g10;
        m_originY = g14;
    } else {
        m_originX = 0;
        m_originY = 0;
    }
    i32 capArg = 0;
    if (g_resourceInstallActive != 0) {
        capArg = 0x800;
    }
    CDDSurface* item = m_parent->m_1c->CreateA((i32)desc, (i32)mode, (i32)a, capArg, flagsArg);
    m_surface = item;
    if (item == 0) {
        return 0;
    }
    i32 w = item->m_width;
    m_width = w;
    i32 h = item->m_height;
    m_height = h;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    if (item->m_bc != 0) {
        m_loadResult = 0x11;
        return 1;
    }
    m_loadResult = 0x10;
    return 1;
}

// ---------------------------------------------------------------------------
// (vtable slot 9): Create24. Allocate a surface from the parent pool's
// Create24 variant (CreateB @0x1423c0) for (desc, mode, NULL, capArg, flagsArg) -
// where flagsArg = keyed ? g_surfaceColorKey : -1 and capArg = g_resourceInstallActive ?
// 0x800 : 0 - then cache the surface geometry (w/h, halved) and clear the
// m_originX/m_originY origin. __thiscall, ret 0xc (3 stack args).
// ---------------------------------------------------------------------------
RVA(0x001530e0, 0x92)
i32 CImage::Create24(CImageFrameDesc* desc, i32 mode, i32 keyed) {
    i32 flagsArg = (keyed != 0) ? g_surfaceColorKey : -1;
    i32 capArg = 0;
    if (g_resourceInstallActive != 0) {
        capArg = 0x800;
    }
    CDDSurface* item = m_parent->m_1c->CreateB((i32)desc, mode, 0, capArg, flagsArg);
    m_surface = item;
    if (item == 0) {
        return 0;
    }
    i32 w = item->m_width;
    m_width = w;
    i32 h = item->m_height;
    m_height = h;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    if (item->m_bc != 0) {
        m_loadResult = 0x11;
    } else {
        m_loadResult = 0x10;
    }
    m_originX = 0;
    m_originY = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// The slot-13 build path (non-virtual /GX builder). Allocate the owned
// +0x30 object (a CImageOwned), decode one frame into it (Build) with the parent's
// active surface format, then cache the decoded geometry (w/h, halved) and the
// descriptor's m_10/m_14 origin into the image. m_loadResult = 0x11 on success.
//
// The `new CImageOwned()` carries the C++ EH state machine (the [esp+0x14] try-
// level writes 0 then -1 around the ctor), which puts the /GX frame on this method.
// __thiscall, ret 8 (2 stack args).
// @early-stop
// 98.7% - all 70 instructions present and logic byte-faithful (the 0x3c new, the
// EH try-level machine, the parent-chain fmt, the geometry copy). The residual is
// (1) the reloc/EH scoring artifacts (push Unwind handler, call _RezAlloc vs
// operator new, the fs:0 __except_list writes - all reloc-masked, code bytes match)
// and (2) a 2-3 byte regalloc/scheduling wall: retail loads `a` into edx before
// completing the fmt chain (we finish the chain first), and orders the tail
// geometry copy before the fs:0 restore (we interleave). No source lever flips it
// under /O2 (tried inline vs local for both the chain and the arg). Logic complete;
// deferred to the final sweep.
// ---------------------------------------------------------------------------
RVA(0x00153180, 0xda)
i32 CImage::BuildSlot13(CImageFrameDesc* desc, void* a) {
    CDDrawShadeBlit* owned = new CDDrawShadeBlit();
    m_owned = owned;
    if (owned == 0) {
        return 0;
    }
    if (!owned->Build((CImageBuildDesc*)desc, (i32)a, m_parent->m_04->m_10[0x18 / 4])) {
        return 0;
    }
    i32 w = m_owned->m_width;
    m_width = w;
    i32 h = m_owned->m_height;
    m_height = h;
    m_loadResult = 0x11;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    m_originX = desc->m_10;
    m_originY = desc->m_14;
    return 1;
}

// ---------------------------------------------------------------------------
// The cleanup virtual (vtable slot 7). Remove the held surface (m_surface)
// from the parent collection's surface pool (m_parent->m_1c), then destroy + free the
// owned +0x30 object; both handles cleared. m_width/m_height zeroed up front.
// ---------------------------------------------------------------------------
RVA(0x00153260, 0x41)
void CImage::FreeAll() {
    m_width = 0;
    m_height = 0;
    if (m_surface != 0) {
        m_parent->m_1c->RemoveItemA(m_surface);
        m_surface = 0;
    }
    CDDrawShadeBlit* owned = m_owned;
    if (owned != 0) {
        owned->Teardown();
        RezFree(owned);
        m_owned = 0;
    }
}

// ---------------------------------------------------------------------------
// CopyFrom - clone another image's surface into this one. Fails unless
// both images own a held surface (m_surface) and neither carries an owned object (m_owned),
// and the two geometries match (m_width/m_height). On a match it Prepares the held surface
// (Prepare(0)) then Blts the other image's surface into it, returning whether the
// blit succeeded. __thiscall, ret 4.
//
// The five reject paths each emit the bare `xor eax,eax; pop; pop; ret 4` epilogue
// inline -> SIBLING guards, not nesting (docs/patterns/redundant-sibling-guard-retest).
// ---------------------------------------------------------------------------
RVA(0x001532b0, 0x80)
i32 CImage::CopyFrom(CImage* other) {
    if (other == 0) {
        return 0;
    }
    if (other->m_owned != 0) {
        return 0;
    }
    if (m_surface == 0) {
        return 0;
    }
    if (m_owned != 0) {
        return 0;
    }
    if (m_width != other->m_width) {
        return 0;
    }
    if (m_height != other->m_height) {
        return 0;
    }
    m_surface->Fill(0);
    i32 ok = m_surface->Blt(other->m_surface);
    return ok != 0;
}

// ---------------------------------------------------------------------------
// SetOrigin (0x153330, __thiscall, ret 8). For frame modes 3 and 4 copy the
// descriptor's +0x10/+0x14 origin into the image (m_originX/m_originY); for any
// other mode zero them. Always returns 1.
// ---------------------------------------------------------------------------
RVA(0x00153330, 0x36)
i32 CImage::SetOrigin(CImageFrameDesc* desc, i32 mode) {
    if (mode == 4 || mode == 3) {
        i32 oy = desc->m_14;
        i32 ox = desc->m_10;
        m_originX = ox;
        m_originY = oy;
    } else {
        m_originX = 0;
        m_originY = 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// 0x153380 (vtable slot 13): Reload - refresh the held surface from its parse
// source. If there is no held surface, succeed. If the surface's source sub-object
// (m_surface->m_08) reports still-clean (IsClean, vtbl[0x60]), succeed. Else if it still
// has a live source descriptor (HasSource, vtbl[0x6c]), re-run FreeAll + Resolve
// (the slot-7 + slot-11 virtuals) with the caller's args. Otherwise parse the new
// source directly: read its 3-char tag (BMP=1/CPX=2/RID=3/PID=4), prime it, then
// rebuild the held surface via the pool's Reload (0x13e550) with the resolved
// index/source/install-flag. __thiscall, ret 8.
//
// The tag switch is the same unsigned binary-search tree as Resolve (key u32, the
// ja/je fourcc compares); docs/patterns/switch-key-unsigned-ja-vs-jg.md.
//
// The FreeAll (slot 7, +0x1c) + Resolve (slot 11, +0x2c) re-runs are ordinary virtual
// dispatches on `this` now (CImage declares them virtual), so `this->FreeAll()` /
// `this->Resolve(...)` lower to `call [vptr+0x1c]` / `[vptr+0x2c]`. The parse-source
// clean/has-source probes are virtual calls on the CImageSurfaceSrc sub-object.
// ---------------------------------------------------------------------------
RVA(0x00153380, 0xeb)
i32 CImage::Reload(CParseSource* src, i32 arg) {
    if (m_surface == 0) {
        return 1;
    }
    IDirectDrawSurface* s = m_surface->m_8;
    if (s != 0) {
        if (s->IsLost() == 0) {
            return 1;
        }
    }
    s = m_surface->m_8;
    if (s->Restore() != 0) {
        this->FreeAll();
        return this->Resolve(src, arg);
    }

    i32 index;
    switch ((u32)src->GetEntryTag()) {
        case IMGTAG_PMB: // BMP
            index = 1;
            break;
        case IMGTAG_XCP: // PCX
            index = 2;
            break;
        case IMGTAG_DIR:
            index = 3;
            break;
        case IMGTAG_DIP:
            index = 4;
            break;
        default:
            return 0;
    }
    i32 resolved = src->BeginParse();
    if (resolved == 0) {
        return 0;
    }
    if (src->m_length == 0) {
        return 0;
    }
    // CDDSurface::Resolve(surf, buf, type, size, surf2): resolved is the decoded buffer,
    // src->m_length its byte size, g_surfaceColorKey lands in the (PID-only) surf2 slot.
    return m_surface->Resolve(
        m_parent->m_1c,
        (void*)resolved,
        index,
        (u32)src->m_length,
        (void*)g_surfaceColorKey
    );
}

// ---------------------------------------------------------------------------
// 0x153470 (vtable slot 14): RenderImage - the sprite blit-mode/clip selector.
// Reads the request's m_mode word: bit 1 culls; bit 8 runs the per-frame animate
// step (wrap m_44 against the draw-delta g_6bf3bc, toggling the live bit) and gates
// on bit 0x10000000; bits 2/4 pick the flip variant and m_owned picks surface-vs-
// shaded, dispatching one of the 7 CImage::Blit* routines. The eighth combination
// (no flip, no owned sprite) is the inlined "plain surface" path: compute the on-
// screen sprite rect from the anchor/origin/draw geometry, remap via WrapCoord (bit
// 0x40000), clip against the parent clip rect / worker box / dest extents, then
// BltFast this->m_surface onto dst->m_surface and record the clipped rect back into
// the request. __thiscall, ret 8.
// @early-stop
// Complete + correct - the dispatch selector is byte-exact; the inlined geometry
// path inherits the SAME whole-function regalloc/scheduling wall as its BlitNorm
// siblings (the origin-load this-member->register tie-break + the WrapCoord ILT-thunk
// / CopyRect IAT-import reloc-name operand artifacts). Logic verified against retail;
// the residual is codegen-only.
RVA(0x00153470, 0x31a)
void CImage::RenderImage(CBlitInfo* info, CImage* dst) {
    i32 mode = info->m_mode;
    if (mode & 1) {
        info->m_result = -1;
        return;
    }
    if (mode & 8) {
        if (g_6bf3bc >= info->m_44) {
            info->m_44 = info->m_48;
            mode ^= 0x10000000;
            info->m_mode = mode;
        } else {
            info->m_44 -= g_6bf3bc;
        }
        mode = info->m_mode;
        if (!(mode & 0x10000000)) {
            info->m_result = -1;
            return;
        }
    }
    i32 hFlip = mode & 4;
    i32 vFlip = mode & 2;
    if (vFlip) {
        if (hFlip) {
            if (m_owned) {
                BlitShadeNorm(info, dst);
            } else {
                BlitNorm(info, dst);
            }
        } else {
            if (m_owned) {
                BlitShadeFlipV(info, dst);
            } else {
                BlitFlipV(info, dst);
            }
        }
        return;
    }
    if (hFlip) {
        if (m_owned) {
            BlitShadeFlipH(info, dst);
        } else {
            BlitFlipH(info, dst);
        }
        return;
    }
    if (m_owned) {
        BlitShadeFlipHV(info, dst);
        return;
    }

    // The plain-surface path (no flip, no owned sprite): compute + clip the rect, BltFast.
    i32 x = m_originX - m_anchorX + info->m_adjustX + info->m_drawX;
    i32 y = m_originY - m_anchorY + info->m_adjustY + info->m_drawY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    i32 dleft = x;
    i32 dtop = y;
    i32 dright = right;
    i32 dbottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect srcClip = m_parent->m_24->m_10;
        RECT destClip;
        CopyRect(&destClip, (const RECT*)&srcClip);
        if (x < destClip.left) {
            dleft += destClip.left - x;
        }
        if (right > destClip.right) {
            dright = destClip.right;
        }
        if (y < destClip.top) {
            dtop += destClip.top - y;
        }
        if (bottom > destClip.bottom) {
            dbottom = destClip.bottom;
        }
    } else if (info->m_clipLeft == (i32)0x80000000) {
        if (x < 0) {
            dleft = 0;
        }
        if (right >= dst->m_width) {
            dright = dst->m_width - 1;
        }
        if (y < 0) {
            dtop = 0;
        }
        if (bottom >= dst->m_height) {
            dbottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            dleft = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            dright = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            dtop = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            dbottom = info->m_clipBottom;
        }
    }
    i32 w = dright - dleft + 1;
    i32 h = dbottom - dtop + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = dleft - x;
    s.top = dtop - y;
    s.right = s.left + w;
    s.bottom = s.top + h;
    dst->m_surface->BltFast(dleft, dtop, m_surface, &s, m_loadResult);
    info->m_outLeft = dleft;
    info->m_outRect.m_00 = dleft;
    info->m_outTop = dtop;
    info->m_outWidth = w;
    info->m_outRect.m_04 = dtop;
    info->m_outHeight = h;
    info->m_result = 0;
    info->m_outRect.m_08 = dright;
    info->m_outRect.m_0c = dbottom;
}

// @early-stop
// 0x0d5c10 (269 B) - homed from src/Stub/GapFunctions.cpp (matcher-5) by RVA
// neighbourhood: it sits between LevelTileValidation (ends 0xd5bdb) and this file's
// low-RVA CImage block (0xd5e20+). A leaf image-loader helper, no vtable-ref; homed
// pending leaf-first reconstruction (its identity within the CImage family is TBD).
RVA(0x000d5c10, 0x10d)
i32 Gap_0d5c10(void) {
    return 0;
}

// ---------------------------------------------------------------------------
// 0x0d5d70 - CDDrawSubMgrFar::~CDDrawSubMgrFar: the member-teardown destructor of a
// far sibling of the DDraw surface-manager family (a FamilyMapBase-shaped, CObject-
// derived 5-slot class) whose .text landed in this cimage unit's RVA range. Its ??_G
// scalar-deleting dtor lives at 0x155720 (DDrawSubMgr.cpp) and calls this ~. The empty
// derived-vtable stamp over the inline MFC ~CObject is elided, so the dtor lowers to
// the field resets (m_04=-1, m_08/m_0c=0) followed by the single ??_7CObject re-stamp.
struct CDDrawSubMgrFar : public CObject {
    virtual void s0();          // slot 0
    virtual ~CDDrawSubMgrFar(); // slot 1 (its ??_G is 0x155720 in DDrawSubMgr.cpp)
    virtual void s2();          // slot 2
    virtual void s3();          // slot 3
    virtual void s4();          // slot 4
    i32 m_04;                   // +0x04
    i32 m_08;                   // +0x08
    i32 m_0c;                   // +0x0c
};
RVA(0x000d5d70, 0x16)
CDDrawSubMgrFar::~CDDrawSubMgrFar() {
    m_04 = -1;
    m_08 = 0;
    m_0c = 0;
}

// ---------------------------------------------------------------------------
// slot 17 (0x0d5e20): forward the arg through two later virtuals - Slot15
// (vtable +0x3c) then Slot16 (vtable +0x40). __thiscall, ret 4. Re-homed from
// src/Stub/BoundaryLowerMethods.cpp (was the Cd5e20 placeholder view); the vtable
// slot-17 thunk 0x1d1b jmps here, so this IS CImage's slot-17 virtual.
RVA(0x000d5e20, 0x1b)
void CImage::Slot17(void* arg) {
    Slot15(arg);
    Slot16(arg);
}

// ---------------------------------------------------------------------------
// The virtual destructor. MSVC stamps this class's own vtable
// (??_7CImage, catalog auto-named) at entry, runs the cleanup virtual (FreeAll),
// then the CObject base subobject dtor folds in (sets m_status=-1, zeroes
// m_08/m_parent, stamps the grand-base dtor vtable). Both vptr stamps are
// compiler-implicit now, so they land in the retail "stamp-first" order. The /GX EH
// frame falls out of the non-trivial CObject subobject.
RVA(0x000d5e80, 0x5b)
CImage::~CImage() {
    FreeAll();
    m_status = -1; // base-field resets (precede the folded ~CObject grand stamp)
    m_08 = 0;
    m_parent = 0;
    // ~CObject() folds here: emits only the grand-base vptr re-stamp.
}

// ---------------------------------------------------------------------------
// 0x153790: render a sprite frame. Resolve a clip context through a shared
// CResolveNode singleton (a function-local static, built once via MSVC5's guarded
// magic-static init), feeding it the parent collection (m_parent) and the (b,c,d)
// geometry; on success dispatch the +0x38 render virtual passing the clip context
// and the first arg. __thiscall, ret 0x10 (4 stack args).
// ---------------------------------------------------------------------------

// The shared clip/resolve singleton: a CResolveNode (vtable 0x5efbc0). Its default
// ctor (0x1549d0) + the resolve method (0x1647e0) are external engine __thiscall
// callees, modeled here as a tiny host so the magic-static init + the resolve
// call reloc-mask. The atexit dtor thunk (0x553800) is the static's cleanup.
class CResolveNode {
public:
    CResolveNode();                                                       // 0x1549d0
    i32 Resolve(void* parent, i32 z1, void* b, void* c, void* d, i32 z2); // 0x1647e0
};

// The +0x38 render virtual (slot 14, RenderImage @0x153470, reconstructed above) is
// dispatched on `this` as an ordinary virtual call (`this->RenderImage(...)` ->
// `mov ecx,this; call [vptr+0x38]`). The `clip` CResolveNode IS the CBlitInfo blit
// request the RESOLVE method fills in (same physical layout); the cast is transitional
// pending a CResolveNode<->CBlitInfo unification.

RVA(0x00153790, 0x6a)
void CImage::RenderFrame(void* a, void* b, void* c, void* d) {
    static CResolveNode clip; // magic-static guard @0x6bf314, ctor 0x1549d0 + atexit
    if (clip.Resolve(m_parent, 0, b, c, d, 0)) {
        this->RenderImage((CBlitInfo*)&clip, (CImage*)a);
    }
}

// ---------------------------------------------------------------------------
// 0x153810: render a sprite frame into a caller-supplied clip rect. Same shape as
// RenderFrame (resolve through a shared CResolveNode singleton, then dispatch the
// +0x38 render virtual) but with a second magic-static singleton (guard @0x6bf29c,
// object @0x6bf228) and an extra `rect` arg whose 4 ints are stashed into a static
// clip rect (@0x6bf28c) before the dispatch. __thiscall, ret 0x14 (5 stack args).
// ---------------------------------------------------------------------------

// The static clip rect updated each call (4 consecutive ints @0x6bf28c). File
// scope so it lands in .bss adjacent to the function's singleton; reloc-masked.
static i32 g_imageClipRect[4]; // @0x6bf28c

RVA(0x00153810, 0x95)
void CImage::RenderFrameClipped(void* a, void* b, void* c, void* rect, void* d) {
    static CResolveNode clip; // magic-static guard @0x6bf29c, ctor 0x1549d0 + atexit
    if (clip.Resolve(m_parent, 0, b, c, d, 0)) {
        if (rect != 0) {
            g_imageClipRect[0] = ((i32*)rect)[0];
            g_imageClipRect[1] = ((i32*)rect)[1];
            g_imageClipRect[2] = ((i32*)rect)[2];
            g_imageClipRect[3] = ((i32*)rect)[3];
        }
        this->RenderImage((CBlitInfo*)&clip, (CImage*)a);
    }
}

// ===========================================================================
// Class-metadata annotations (EOF-hosted: CImage.h is included by several /O2
// Image TUs whose leaf decoders are byte-exact-sensitive, so keep the completeness
// typedefs after the last function). VTBL skips (logged): the CObject base
// vtable is the shared grand-base 0x5e8cb4 (the CObject dtor vtable); CParseSource is
// flagged [virtual] only via its polymorphic Gruntz def, not this view. The held surface
// (CDDSurface) and owned sprite (CDDrawShadeBlit) are annotated in their own headers.
// CImage itself is RTTI-catalogued (??_7CImage@@ @0x5eaa2c, cl-emitted from the real
// virtuals declared in CImage.h).
// ===========================================================================
// --- CImage.h header classes ---
SIZE_UNKNOWN(CDDrawSurfaceDesc);
SIZE(BlitRect, 0x10); // {left,top,right,bottom} RECT
SIZE_UNKNOWN(CBlitClipOwner);
SIZE_UNKNOWN(CImageParent);
SIZE_UNKNOWN(CImageFrameDesc);
SIZE_UNKNOWN(CImage);     // RTTI CImage (real-polymorphic; RTTI-vtable catalogued)
VTBL(CImage, 0x001eaa2c); // vtable_names -> code (RTTI game class)
// --- CImage.cpp local views ---
SIZE_UNKNOWN(CResolveNode);
