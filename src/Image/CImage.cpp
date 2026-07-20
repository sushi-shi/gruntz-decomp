#include <Mfc.h>
#include <Gruntz/ParseSource.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <rva.h>
#include <DDrawMgr/DDrawPtrCollections.h>

#include <Gruntz/ResolveNode.h> // canonical CResolveNode (Init @0x1647e0, ctor @0x1549d0)
#include <Image/CImage.h>
#include <Image/CBlitInfo.h> // canonical CBlitInfo/CBlitXform (RenderImage selector arg)
#include <Wwd/WwdFile.h>     // CPlaneRender::WrapCoord (the m_xform origin remap)

#include <DDrawMgr/DDSurface.h> // canonical CDDSurface (m_surface geometry/Fill/Blt/Reload/m_8 COM)
#include <DDrawMgr/DDrawShadeBlit.h> // canonical CDDrawShadeBlit (m_owned: new/Build/Teardown)
#include <Win32.h>                   // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>                   // real IDirectDrawSurface dispatch (m_8->IsLost/Restore)

extern "C" u32 g_engineFrameDelta;

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

RVA(0x000d5dc0, 0xb)
i32 CWapObj::IsLoaded() {
    return *reinterpret_cast<i32*>((reinterpret_cast<char*>(this) + 0x10)) > 0;
}

RVA(0x000d5de0, 0x6)
i32 CImage::GetClassId() {
    return 10;
}

RVA(0x000d5e00, 0x3)
void CImage::FlipHorizontal(void*) {}

RVA(0x000d5e20, 0x1b)
void CImage::FlipBoth(void* arg) {
    FlipVertical(arg);
    FlipHorizontal(arg);
}

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

DATA(0x002bf28c)
i32 g_imageClipRect[4] = {0}; // 0x2bf28c  (owner-TU definition)
DATA(0x002bf318)
i32 g_bltFxScratch[25] = {0}; // 0x2bf318
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

#include <Globals.h> // g_bltFxScratch (the shared BltEx fx block)

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
SIZE_UNKNOWN(CBlitXform);
SIZE_UNKNOWN(CBlitInfo);

SIZE_UNKNOWN(CDDrawSurfaceDesc);
SIZE(BlitRect, 0x10); // {left,top,right,bottom} RECT
SIZE_UNKNOWN(CBlitClipOwner);
SIZE_UNKNOWN(CImageParent);
SIZE_UNKNOWN(CImageFrameDesc);
SIZE_UNKNOWN(CImage);     // RTTI CImage (real-polymorphic; RTTI-vtable catalogued)
VTBL(CImage, 0x001eaa2c); // vtable_names -> code (RTTI game class)
SIZE_UNKNOWN(CResolveNode);
