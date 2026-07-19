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

#include <Gruntz/ResolveNode.h> // canonical CResolveNode (Init @0x1647e0, ctor @0x1549d0)
#include <Image/CImage.h>
#include <Image/CBlitInfo.h> // canonical CBlitInfo/CBlitXform (RenderImage selector arg)
#include <Wwd/WwdFile.h>     // CPlaneRender::WrapCoord (the m_xform origin remap)

#include <DDrawMgr/DDSurface.h> // canonical CDDSurface (m_surface geometry/Fill/Blt/Reload/m_8 COM)
#include <DDrawMgr/DDrawShadeBlit.h> // canonical CDDrawShadeBlit (m_owned: new/Build/Teardown)
#include <Win32.h>                   // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>                   // real IDirectDrawSurface dispatch (m_8->IsLost/Restore)

// The engine heap free is the NAFXCW global operator delete (??3@YAXPAX@Z @0x1b9b82,
// declared by <Mfc.h>); reloc-masked rel32.

// The per-frame draw-delta mirror (BSS @0x6bf3bc) the RenderImage animate path
// consumes to advance/wrap the request's m_44 counter. Canonical binding in
// Projectile.cpp; declared address-pinned here (reloc-masked).
extern "C" u32 g_engineFrameDelta;

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

// ===========================================================================
// wave4-L (dossier #15): the 0x0d5xxx block below are COMDAT-at-usage exiles of
// this TU's class, kept at the play-region obj (which first-references the CImage
// vtable and carries its /GR RTTI); the C block proper is 0x152e90-0x1549c5.
// File-head position keeps the intra-file RVA order strictly ascending.
//
// DRAIN NOTE (matcher-2 D6, verified): the flag_outliers "5@0xd5c10" cluster is
// NOT a foreign obj and NOT a second CImage .cpp - it is a COMDAT-FOLD POOL, and
// the whole cluster is pooled-member (doctrine (a) -> LEAVE), NOT conflation. Proof:
//   (1) the 0xd5xxx members are 512 KB from CImage's main obj (0x152e90) and
//       interleaved with FOUR other TUs' functions (Play::AddLevelGruntz/ResetGoals,
//       PlayPlaneScan::ScanBuildTiles, LevelTileValidation::PositionBridgeToggle,
//       WwdFile::GetTileHandle) - a linker COMDAT pool, not a contiguous obj run;
//   (2) a CRT COMDAT (??_G__non_rtti_object @0xd5e50) is interleaved BETWEEN Slot17
//       (0xd5e20) and ~CImage (0xd5e80), so the CImage COMDATs are not even
//       contiguous among themselves -> unsplittable into one clean obj;
//   (3) GetClassId/Slot16/Slot17 are CImage inline virtuals declared polymorphic in
//       CImage.h (cl COMDAT-emits them); IsLoaded is CWapObj's base inline virtual
//       (CImage its sole non-overriding user); Gap_0d5c10 is a CImage-family loader
//       helper (xref: calls CImage::Resolve/FreeAll/RenderFrame + CSymTab/sprintf);
//       0xd5d70 is the linker-kept ??1CLoadable COMDAT (bound in DDrawWorkerRegistry.cpp).
// Splitting any of these into a foreign RVA-named .cpp would MISATTRIBUTE CImage's
// own methods. No split warranted; the flag is a false-positive (flag_outliers
// _POOLED_RE does not recognise the GetClassId/Slot1N/IsLoaded pooled-virtual names).
// ===========================================================================
// @early-stop
// 0x0d5c10 (269 B) - homed from src/Stub/GapFunctions.cpp (matcher-5) by RVA
// neighbourhood: it sits between LevelTileValidation (ends 0xd5bdb) and this file's
// low-RVA CImage block (0xd5e20+). A CImage-family leaf image-loader helper (xref-
// confirmed: calls CImage::Resolve/FreeAll/RenderFrame, CSymTab::ResolveQualified,
// sprintf); homed pending leaf-first reconstruction (its exact identity is TBD).
RVA(0x000d5c10, 0x10d)
i32 Gap_0d5c10(void) {
    return 0;
}

// ---------------------------------------------------------------------------
// 0x0d5d70 - ??1CLoadable@@UAE@XZ: the ONE linker-kept COMDAT copy of the
// canonical CLoadable inline dtor (<Gruntz/Loadable.h>): m_04=-1, m_08/m_0c=0 +
// the single surviving ??_7CObject re-stamp (0x5e8cb4; the intermediate stamps
// dead-store-eliminated). Its ??_G pair 0x155720 (DDrawWorkerRegistry-band obj)
// calls it via the ILT thunk 0x429b. Because C++ allows no second out-of-line
// definition of an inline member, the fn is not spelled here - cl auto-emits the
// byte-identical COMDAT (verified llvm-objdump -dr) in every CLoadable-using obj,
// and it is @rva-symbol-bound in src/DDrawMgr/DDrawWorkerRegistry.cpp (whose base
// obj emits both halves of the pair). Was the fabricated `CDDrawSubMgrFar :
// CObject` view with four body-less placeholder virtuals - dissolved.

// ---------------------------------------------------------------------------
// CWapObj::IsLoaded (slot 5, 0x0d5dc0) - the SHARED base default the whole
// CWapObj family inherits unless it overrides: the first derived field (@+0x10 -
// a count/size) is nonzero. CImage inherits it unchanged (its m_width lands at
// +0x10). Emitted in this (CImage) TU, its sole non-overriding user. `mov edx,
// [ecx+0x10]; xor eax,eax; test edx,edx; setg al; ret`.
RVA(0x000d5dc0, 0xb)
i32 CWapObj::IsLoaded() {
    return *reinterpret_cast<i32*>((reinterpret_cast<char*>(this) + 0x10)) > 0;
}

// ---------------------------------------------------------------------------
// CImage::GetClassId (slot 8, 0x0d5de0): the class type tag - CImage returns 10.
RVA(0x000d5de0, 0x6)
i32 CImage::GetClassId() {
    return 10;
}

// ---------------------------------------------------------------------------
// CImage::Slot16 (slot 16, 0x0d5e00): a no-op sink (Slot17 forwards its arg here
// and to Slot15). __thiscall, one arg, ret 4.
RVA(0x000d5e00, 0x3)
void CImage::FlipHorizontal(void*) {}

// ---------------------------------------------------------------------------
// slot 17 (0x0d5e20): forward the arg through two later virtuals - Slot15
// (vtable +0x3c) then Slot16 (vtable +0x40). __thiscall, ret 4. Re-homed from
// src/Stub/BoundaryLowerMethods.cpp (was the Cd5e20 placeholder view); the vtable
// slot-17 thunk 0x1d1b jmps here, so this IS CImage's slot-17 virtual.
RVA(0x000d5e20, 0x1b)
void CImage::FlipBoth(void* arg) {
    FlipVertical(arg);
    FlipHorizontal(arg);
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
    CDDSurface* item = m_parent->m_1c->Createa58_3(reinterpret_cast<i32>(desc), capArg, flagsArg);
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
    switch (static_cast<u32>(src->GetEntryTag())) {
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
        this->LoadDispatch(reinterpret_cast<CImageFrameDesc*>(resolved), static_cast<u32>(index), reinterpret_cast<void*>(src->m_length), arg);
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
            m_owned->Select(2, 0);
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
    CDDSurface* item = m_parent->m_1c->CreateA(reinterpret_cast<i32>(desc), static_cast<i32>(mode), reinterpret_cast<i32>(a), capArg, flagsArg);
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
    CDDSurface* item = m_parent->m_1c->CreateB(reinterpret_cast<i32>(desc), mode, 0, capArg, flagsArg);
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
    if (!owned->Build(reinterpret_cast<CImageBuildDesc*>(desc), reinterpret_cast<i32>(a), m_parent->m_04->m_10[0x18 / 4])) {
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
        ::operator delete(owned);
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
    switch (static_cast<u32>(src->GetEntryTag())) {
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
        reinterpret_cast<void*>(resolved),
        index,
        static_cast<u32>(src->m_length),
        reinterpret_cast<void*>(g_surfaceColorKey)
    );
}

// ---------------------------------------------------------------------------
// 0x153470 (vtable slot 14): RenderImage - the sprite blit-mode/clip selector.
// Reads the request's m_mode word: bit 1 culls; bit 8 runs the per-frame animate
// step (wrap m_44 against the draw-delta g_engineFrameDelta, toggling the live bit) and gates
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
        if (g_engineFrameDelta >= info->m_44) {
            info->m_44 = info->m_48;
            mode ^= 0x10000000;
            info->m_mode = mode;
        } else {
            info->m_44 -= g_engineFrameDelta;
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
        CopyRect(&destClip, static_cast<const RECT*>(&srcClip));
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
    } else if (info->m_clipLeft == static_cast<i32>(0x80000000)) {
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
    info->m_outRect.left = dleft;
    info->m_outTop = dtop;
    info->m_outWidth = w;
    info->m_outRect.top = dtop;
    info->m_outHeight = h;
    info->m_result = 0;
    info->m_outRect.right = dright;
    info->m_outRect.bottom = dbottom;
}

// ---------------------------------------------------------------------------
// 0x153790: render a sprite frame. Resolve a clip context through a shared
// CResolveNode singleton (a function-local static, built once via MSVC5's guarded
// magic-static init), feeding it the parent collection (m_parent) and the (b,c,d)
// geometry; on success dispatch the +0x38 render virtual passing the clip context
// and the first arg. __thiscall, ret 0x10 (4 stack args).
// ---------------------------------------------------------------------------

// The shared clip/resolve singleton is the canonical CResolveNode (class in
// <Gruntz/ResolveNode.h>). Its default ctor (0x1549d0) + Init (0x1647e0) are external
// engine __thiscall callees; the magic-static init + the Init call reloc-mask, and the
// node's virtual ~CResolveNode drives the compiler-emitted atexit thunk.
//
// The +0x38 render virtual (slot 14, RenderImage @0x153470, reconstructed above) is
// dispatched on `this` as an ordinary virtual call (`this->RenderImage(...)` ->
// `mov ecx,this; call [vptr+0x38]`). The `clip` CResolveNode IS the CBlitInfo blit
// request the RESOLVE method fills in (same physical layout); the cast is transitional
// pending a CResolveNode<->CBlitInfo unification.
//
// The magic-static trio cl5 emits for `static CResolveNode clip;` has NO source VarDecl
// to hang DATA() on, so the three compiler-minted symbols are pinned to their retail
// addresses verbatim (the @data/@rva-symbol carriers). Per local static:
//   the object   .bss  0x2bf2a0 / 0x2bf228
//   the once-guard .bss  0x2bf314 / 0x2bf29c   (`$S<n>` byte)
//   the atexit dtor thunk .text 0x153800 / 0x1538b0 (`$E<n>`, pushed to atexit)
// The `$S<5-digit>` tail on the .bss names is cl5's per-TU COMDAT sequence number: it
// SHIFTS if CImage.cpp's earlier statics change, so re-read it from the base obj
// (`llvm-nm build/objdiff/base/cimage.obj`) if labels.py reports "not in base obj".
// @data-symbol: _?clip@?1??RenderFrame@CImage@@QAEXPAX000@Z@4VCResolveNode@@A$S26840 0x002bf2a0
// @data-symbol: _?$S28@?1??RenderFrame@CImage@@QAEXPAX000@Z@4EA$S26842 0x002bf314
// @rva-symbol: _$E29 0x00153800 0x10
// @data-symbol: _?clip@?1??RenderFrameClipped@CImage@@QAEXPAX0000@Z@4VCResolveNode@@A$S26863 0x002bf228
// @data-symbol: _?$S30@?1??RenderFrameClipped@CImage@@QAEXPAX0000@Z@4EA$S26865 0x002bf29c
// @rva-symbol: _$E31 0x001538b0 0x10

RVA(0x00153790, 0x6a)
void CImage::RenderFrame(void* a, void* b, void* c, void* d) {
    static CResolveNode clip; // magic-static guard @0x6bf314, ctor 0x1549d0 + atexit
    if (clip.Init(reinterpret_cast<i32>(m_parent), 0, reinterpret_cast<i32>(b), reinterpret_cast<i32>(c), reinterpret_cast<i32>(d), 0)) {
        this->RenderImage(reinterpret_cast<CBlitInfo*>(&clip), static_cast<CImage*>(a));
    }
}

// ---------------------------------------------------------------------------
// 0x153810: render a sprite frame into a caller-supplied clip rect. Same shape as
// RenderFrame (resolve through a shared CResolveNode singleton, then dispatch the
// +0x38 render virtual) but with a second magic-static singleton (guard @0x6bf29c,
// object @0x6bf228) and an extra `rect` arg whose 4 ints are stashed into a static
// clip rect (@0x6bf28c) before the dispatch. __thiscall, ret 0x14 (5 stack args).
// ---------------------------------------------------------------------------

// The static clip rect updated each call (4 consecutive ints @0x2bf28c .bss);
// DATA-bound so the four DIR32 stores pair to the retail address. reloc-masked.
DATA(0x002bf28c)
i32 g_imageClipRect[4] = {0}; // 0x2bf28c  (owner-TU definition)
// The 25-int BltEx fx block ([1] = blend-mode word) + the surface color-key value
// (keyed blits pass it as the fx flags arg). Shared with CDDrawWorkerRegistry /
// GruntzMgr; homed to this image TU (.bss zero-init). Reference externs stay in
// <Globals.h>. (REHOME DD-Drain-1)
DATA(0x002bf318)
i32 g_bltFxScratch[25] = {0}; // 0x2bf318
// The resource-install gate - DEFINED here beside its sibling g_surfaceColorKey (the two
// are declared as a pair in <Image/CImage.h> and gate the same CreateA call: cap 0x800 /
// the flags arg). 10 TUs referenced it and none defined it. .bss, zero-init.
DATA(0x002bf37c)
i32 g_resourceInstallActive = 0; // 0x2bf37c
DATA(0x002bf380)
i32 g_surfaceColorKey = 0; // 0x2bf380

RVA(0x00153810, 0x95)
void CImage::RenderFrameClipped(void* a, void* b, void* c, void* rect, void* d) {
    static CResolveNode clip; // magic-static guard @0x6bf29c, ctor 0x1549d0 + atexit
    if (clip.Init(reinterpret_cast<i32>(m_parent), 0, reinterpret_cast<i32>(b), reinterpret_cast<i32>(c), reinterpret_cast<i32>(d), 0)) {
        if (rect != 0) {
            g_imageClipRect[0] = (static_cast<i32*>(rect))[0];
            g_imageClipRect[1] = (static_cast<i32*>(rect))[1];
            g_imageClipRect[2] = (static_cast<i32*>(rect))[2];
            g_imageClipRect[3] = (static_cast<i32*>(rect))[3];
        }
        this->RenderImage(reinterpret_cast<CBlitInfo*>(&clip), static_cast<CImage*>(a));
    }
}

// ===========================================================================
// The five CImage sprite blit/clip routines (0x1538c0..0x154750), merged from
// ImageSpriteBlit.cpp (same class, same obj - wave4-L C block).
// ===========================================================================
#include <Globals.h> // g_bltFxScratch (the shared BltEx fx block)

// The 25-int g_bltFxScratch block (shared with CDDrawWorkerRegistry); [1] carries
// the BltEx blend-mode word, the base is the DDBLTFX-style fx pointer.

// The blit backends (CDDSurface::BltEx @0x13eef0, CDDrawShadeBlit::Blit @0x1497f0)
// come from the canonical shared headers above; reloc-masked rel32 call symbols.

// ---------------------------------------------------------------------------
// No flip, surface blit (BltEx, blend mode 6).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct. Residual = 4 origin-load insns whose this-member->register
// assignment differs (the loaded members die right after the subtract chain, so
// the pick is a whole-function regalloc tie-break MSVC5 resolves differently than
// retail) + the WrapCoord (0x295a ILT thunk) / CopyRect (IAT import) reloc-name
// operand artifacts. Code bytes otherwise byte-exact (clip + struct-copy end).
RVA(0x001538c0, 0x257)
void CImage::BlitNorm(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_originX - info->m_adjustX - m_anchorX;
    i32 y = info->m_drawY - m_originY - info->m_adjustY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
        RECT clip;
        CopyRect(&clip, static_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFxScratch[1] = 6;
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(&d);
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// Vertical flip, surface blit (BltEx, blend mode 2).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct (formulas verified against retail). The vertical flip makes
// the Y anchor a mixed-sign chain (m_originY - m_anchorY + m_adjustY + m_drawY);
// MSVC5 reassociates it to (m_adjustY + m_originY + m_drawY) - m_anchorY and picks a
// different Y-accumulator base than
// retail, which co-schedules the X subtract chain into different registers. That
// one divergence cascades through the whole function (no source spelling pins the
// reassociation - compound-assign / anchor-temp / x<->y reorder all tried). Plus
// the WrapCoord/CopyRect reloc-name artifacts.
RVA(0x00153b20, 0x270)
void CImage::BlitFlipV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - info->m_adjustX - m_anchorX - m_originX;
    i32 y = m_originY - m_anchorY + info->m_adjustY + info->m_drawY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
        RECT clip;
        CopyRect(&clip, reinterpret_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFxScratch[1] = 2;
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(&d);
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// Horizontal flip, surface blit (BltEx, blend mode 4).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct. Same wall as BlitFlipV: the horizontal flip makes X a
// mixed-sign chain (m_adjustX - m_anchorX + m_originX + m_drawX) that MSVC5 reassociates + reorders
// vs retail, cascading the co-scheduled X/Y register assignment. Plus the
// WrapCoord/CopyRect reloc-name artifacts.
RVA(0x00153d90, 0x259)
void CImage::BlitFlipH(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_adjustX - m_anchorX + m_originX + info->m_drawX;
    i32 y = info->m_drawY - m_originY - m_anchorY - info->m_adjustY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
        RECT clip;
        CopyRect(&clip, reinterpret_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFxScratch[1] = 4;
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, g_bltFxScratch);
    d.right -= 1;
    d.bottom -= 1;
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(&d);
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// X+Y flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 0/0).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct - the fourth member of the shaded family, structurally
// identical to BlitShadeNorm/FlipV. Both anchor axes are flipped (X: the
// m_anchorX/m_originX signs; Y: the m_originY/m_adjustY/m_anchorY mixed-sign chain), so it inherits the
// SAME whole-function regalloc/reassociation wall the other flip variants hit:
// MSVC5 reassociates the mixed-sign X/Y accumulator chains and picks a different
// this-member->register mapping than retail, cascading downstream. Plus the
// WrapCoord (0x295a ILT thunk) / CopyRect (IAT import) / 0x14dd90 pre-notify
// reloc-name operand artifacts. Clip + inclusive-rect struct-copy end match.
RVA(0x00153ff0, 0x280)
void CImage::BlitShadeFlipHV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_anchorX + m_originX + info->m_adjustX;
    i32 y = info->m_drawY - m_anchorY + m_originY + info->m_adjustY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
        RECT clip;
        CopyRect(&clip, reinterpret_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_notify) {
        m_owned->Select(info->m_notifyArg0, reinterpret_cast<ShadeDescr*>(info->m_notifyArg1));
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 0);
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(&d);
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// No flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 1/1).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct, ~99.86%. The 0x14dd90 pre-notify is bound to its real callee
// CDDrawShadeBlit::Select - a plain `m_owned->Select(...)` now that the fake ShadeSelector
// class it used to be bound to is dissolved, so the ((ShadeSelector*)m_owned) reinterpret
// is GONE (the cast was the symptom; the wrong owning class was the cause). It still
// ripples the /O2 origin-load regalloc-tiebreak (ebp/ebx swap in the rect setup).
// Residual = those
// tiebreak insns + the WrapCoord ILT-thunk / CopyRect IAT-import reloc-name artifacts;
// all other code bytes byte-exact. %-hit accepted per structure-recovery doctrine.
RVA(0x00154270, 0x257)
void CImage::BlitShadeNorm(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_originX - m_anchorX - info->m_adjustX;
    i32 y = info->m_drawY - m_originY - m_anchorY - info->m_adjustY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
        RECT clip;
        CopyRect(&clip, reinterpret_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_notify) {
        m_owned->Select(info->m_notifyArg0, reinterpret_cast<ShadeDescr*>(info->m_notifyArg1));
    }
    m_owned->Blit(&d, dst->m_surface, &s, 1, 1);
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(&d);
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// Vertical flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 1/0).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct - the X/Y formulas already match retail's operation order.
// The wall is pure whole-function regalloc: retail pins `this` in EBX (`mov ebx,ecx`,
// push edi later) where our cl picks EDI, and reorders the all-sub X chain to a
// different this-member->register mapping; that one prologue choice shifts every
// downstream register. Plus the WrapCoord/CopyRect/0x14dd90 reloc-name artifacts.
// End-store struct-copy matches retail.
RVA(0x001544d0, 0x275)
void CImage::BlitShadeFlipV(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_drawX - m_anchorX - info->m_adjustX - m_originX;
    i32 y = m_originY + info->m_adjustY + info->m_drawY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
        RECT clip;
        CopyRect(&clip, reinterpret_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_notify) {
        m_owned->Select(info->m_notifyArg0, reinterpret_cast<ShadeDescr*>(info->m_notifyArg1));
    }
    m_owned->Blit(&d, dst->m_surface, &s, 1, 0);
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(&d);
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}

// ---------------------------------------------------------------------------
// X flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 0/1).
// ---------------------------------------------------------------------------
// @early-stop
// Complete + correct - the shaded twin of BlitFlipH. The horizontal flip makes
// X the mixed-sign chain (m_adjustX + m_originX + m_drawX - m_anchorX) that MSVC5 reassociates +
// reorders vs retail, cascading the co-scheduled X/Y register assignment (same
// wall as the surface BlitFlipH). Plus the WrapCoord/CopyRect/0x14dd90 reloc-name
// operand artifacts. Clip + inclusive-rect struct-copy end match retail.
RVA(0x00154750, 0x275)
void CImage::BlitShadeFlipH(CBlitInfo* info, CImage* dst) {
    i32 x = info->m_adjustX + m_originX + info->m_drawX - m_anchorX;
    i32 y = info->m_drawY - m_originY - info->m_adjustY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_xform->m_planeRender->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_parent->m_24->m_10;
        RECT clip;
        CopyRect(&clip, reinterpret_cast<const RECT*>(&clipA));
        if (x < clip.left) {
            d.left += clip.left - x;
        }
        if (right > clip.right) {
            d.right += clip.right - right;
        }
        if (y < clip.top) {
            d.top += clip.top - y;
        }
        if (bottom > clip.bottom) {
            d.bottom += clip.bottom - bottom;
        }
    } else if (info->m_clipLeft == static_cast<i32>(0x80000000)) {
        if (x < 0) {
            d.left = 0;
        }
        if (right >= dst->m_width) {
            d.right = dst->m_width - 1;
        }
        if (y < 0) {
            d.top = 0;
        }
        if (bottom >= dst->m_height) {
            d.bottom = dst->m_height - 1;
        }
    } else {
        if (x < info->m_clipLeft) {
            d.left = info->m_clipLeft;
        }
        if (right > info->m_clipRight) {
            d.right = info->m_clipRight;
        }
        if (y < info->m_clipTop) {
            d.top = info->m_clipTop;
        }
        if (bottom > info->m_clipBottom) {
            d.bottom = info->m_clipBottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_result = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_notify) {
        m_owned->Select(info->m_notifyArg0, reinterpret_cast<ShadeDescr*>(info->m_notifyArg1));
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 1);
    info->m_outLeft = d.left;
    info->m_outTop = d.top;
    info->m_outRect = *(&d);
    info->m_outWidth = w;
    info->m_outHeight = h;
    info->m_result = 0;
}
// Class-metadata annotations (EOF-hosted, /O2 sprite-blit TU).
SIZE_UNKNOWN(CBlitXform);
SIZE_UNKNOWN(CBlitInfo);

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
