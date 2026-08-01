#define CIMAGE_INLINE_DTOR // DrawScreenTextImage folds ~CImage; see the header note
#include <Mfc.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/ParseSource.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <rva.h>
#include <Pix16.h> // the byte-cursor unions (RecordBytes / Pix16Ptr)
#include <DDrawMgr/DDrawPtrCollections.h>

#include <Gruntz/ResolveNode.h> // canonical CResolveNode (Init @0x1647e0, ctor @0x1549d0)
#include <Image/CImage.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>  // m_ownerCtx's real class (ex the CImageParent pad-view)
#include <DDrawMgr/DDrawSubMgrPages.h> // m_ownerCtx->m_drawTarget->m_frontPair->m_bpp
#include <Gruntz/GameLevel.h>          // CGameLevel (the node m_level hop) + CTileImageSet
#include <Wwd/WwdFile.h> // CDDrawWorkerHost::WrapCoord (m_level->m_mainPlane origin remap)

#include <DDrawMgr/DDSurface.h> // canonical CDDSurface (m_surface geometry/Fill/Blt/Reload/m_8 COM)
#include <DDrawMgr/DDrawShadeBlit.h>   // canonical CDDrawShadeBlit (m_owned: new/Build/Teardown)
#include <DDrawMgr/DDrawSurfacePair.h> // the blit destination (dst->m_surface/m_width/m_height)
#include <Win32.h>                     // windows.h base types (ddraw.h needs them first)
#include <ddraw.h>                     // real IDirectDrawSurface dispatch (m_8->IsLost/Restore)

#include <Gruntz/State.h>         // CState (DrawScreenTextImage's receiver - see its note below)
#include <Bute/SymTab.h>          // CSymTab::ResolveQualified
#include <stdio.h>                // sprintf (the "\\SCREENZ\\%sTEXT" key)
#include <Image/ImageFormatTag.h> // the shared 4-char format codes (ex this TU's local enum)

DATA(0x002bf28c)
i32 g_imageClipRect[4] = {0}; // 0x2bf28c  (owner-TU definition)
DATA(0x002bf318)
DDBLTFX g_bltFx = {0}; // 0x2bf318  (the ex-"g_bltFxScratch" i32[25] - it IS a DDBLTFX)
DATA(0x002bf37c)
i32 g_resourceInstallActive = 0; // 0x2bf37c
DATA(0x002bf380)
i32 g_surfaceColorKey = 0; // 0x2bf380
VTBL(CImage, 0x001eaa2c);  // vtable_names -> code (RTTI game class)

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
//       (CImage its sole non-overriding user); LoadImageHelper is a CImage-family loader
//       helper (xref: calls CImage::Resolve/FreeAll/RenderFrame + CSymTab/sprintf);
//       0xd5d70 is the linker-kept ??1CLoadable COMDAT (bound in DDrawWorkerRegistry.cpp).
// Splitting any of these into a foreign RVA-named .cpp would MISATTRIBUTE CImage's
// own methods. No split warranted; the flag is a false-positive (flag_outliers
// _POOLED_RE does not recognise the GetClassId/Slot1N/IsLoaded pooled-virtual names).
// ===========================================================================
// ---------------------------------------------------------------------------
// CState::DrawScreenTextImage (0x0d5c10) - render the "\SCREENZ\<name>TEXT" PID page
// onto the back page at (0x140, 0x158) through a throwaway stack CImage. Homed here by
// RVA neighbourhood (it sits between LevelTileValidation, which ends at 0xd5bdb, and
// this file's low-RVA CImage COMDAT block at 0xd5e20+).
//
// IDENTITY RESOLVED 2026-07-29 - the old note said "its exact identity is TBD" and
// guessed a "CImage-family leaf image-loader helper" from the callee list. It is not a
// CImage method at all; CImage is what it USES. The receiver is named by its own two
// field reads: [this+0x2c] is the CSymTab it calls ResolveQualified on, and [this+0x0c]
// is the CDDrawSurfaceMgr it both seeds the stack CImage's m_ownerCtx with and walks
// ->m_drawTarget(+0x04)->m_backPair(+0x14) through. Exactly one class in the tree pairs
// a CSymTab at +0x2c with a CDDrawSurfaceMgr at +0x0c: CState (m_2c and m_world). Its
// direct sibling CState::FadeInTitle @0xfa1f0 has the identical opening - sprintf a
// "\SCREENZ\..." key into a stack buffer, then SymTab2c()->ResolveQualified(buf, tag).
// No caller exists anywhere in the image, which is what stalled the earlier chase; the
// callee/field direction settles it without one.
//
// The stack CImage's eight seed stores ARE the ordinary two-arg ctor with index 0:
// CWapObj(0, world) writes m_id/m_flags/m_ownerCtx, then the derived vptr stamp, then
// m_width/m_height/m_surface/m_owned - retail's exact order. The two later
// vptr-restamp + FreeAll pairs at trylevel 1 and 2 are its inlined destructor on the
// failure and success paths.
// EXACT. Retail INLINES ~CImage at both teardown sites - `mov [img],??_7CImage;
// lea ecx,[img]; call FreeAll` - with /O2 dead-storing the dtor's m_id/m_flags/
// m_ownerCtx resets and the ~CObject grand restamp (all unobservable on a dying stack
// object; the vptr store survives because cl never kills that one), so its EH trylevel
// steps 0 -> 1 -> 2 through the two partial-teardown states instead of dropping to -1.
// The out-of-line `virtual ~CImage()` emitted `lea ecx,[img]; call ??1CImage` + trylevel
// -1 instead. Closed by the OPT-IN inline dtor: <Image/CImage.h> carries the body under
// `#ifdef CIMAGE_INLINE_DTOR`, this TU defines that macro (so only cimage.obj folds it -
// no other TU is reshaped) and the standalone at 0xd5e80 is now cl's own COMDAT copy,
// RVA_COMPGEN-pinned above and still byte-exact.
RVA(0x000d5c10, 0x10d)
i32 CState::DrawScreenTextImage(const char* name) {
    char buf[0x40];
    sprintf(buf, "\\SCREENZ\\%sTEXT", name);
    CParseSource* src = SymTab2c()->ResolveQualified(buf, IMGTAG_DIP);
    if (src == 0) {
        return 0;
    }
    CDDrawSurfaceMgr* world = m_world;
    CDDrawSurfacePair* page = world->m_drawTarget->m_backPair;
    if (page == 0) {
        return 0;
    }
    CImage img(0, world);
    if (img.Resolve(src, 1) == 0) {
        return 0;
    }
    img.RenderFrame(page, 0x140, 0x158, 0);
    return 1;
}

// ---------------------------------------------------------------------------
// 0x0d5d70 - ??1CLoadable@@UAE@XZ: the ONE linker-kept COMDAT copy of the
// canonical CLoadable inline dtor (<Gruntz/Loadable.h>): m_04=-1, m_08/m_0c=0 +
// the single surviving ??_7CObject re-stamp (0x5e8cb4; the intermediate stamps
// dead-store-eliminated). Its ??_G pair 0x155720 (DDrawWorkerRegistry-band obj)
// calls it via the ILT thunk 0x429b. Because C++ allows no second out-of-line
// definition of an inline member, the fn is not spelled here - cl auto-emits the
// byte-identical COMDAT (verified llvm-objdump -dr) in every CLoadable-using obj,
// and it is RVA_COMPGEN-bound in src/DDrawMgr/DDrawWorkerRegistry.cpp (whose base
// obj emits both halves of the pair). Was the fabricated `CDDrawSubMgrFar :
// CObject` view with four body-less placeholder virtuals - dissolved.

// CWapObj::IsReady (0x000d5da0): the slot-6 base default - `return 1`. Reached by
// the whole family through the 0x001c08 ILT thunk; every kind inherits it unless it
// overrides. Un-phantoms the shared slot-6 declaration.
RVA(0x000d5da0, 0x6)
i32 CWapObj::IsReady() {
    return 1;
}

RVA(0x000d5dc0, 0xb)
i32 CWapObj::IsLoaded() {
    // the base default peeks the derived +0x10 slot - for the image family (whose
    // TU this body lives in) that is CImage::m_width: loaded == has a width
    return (static_cast<CImage*>(this))->m_width > 0;
}

RVA(0x000d5de0, 0x6)
i32 CImage::GetClassId() {
    return CLASSID_IMAGE;
}

RVA(0x000d5e00, 0x3)
void CImage::FlipHorizontal(void*) {}

RVA(0x000d5e20, 0x1b)
void CImage::FlipBoth(void* arg) {
    FlipVertical(arg);
    FlipHorizontal(arg);
}

RVA_COMPGEN(0x000d5e50, 0x1e, ??_GCImage@@UAEPAXI@Z)
// ~CImage's body is IN-CLASS (<Image/CImage.h>, CIMAGE_INLINE_DTOR): retail folds it
// into DrawScreenTextImage's two teardown arms, so this TU takes the inline form and
// the standalone at 0xd5e80 is cl's own COMDAT copy.
RVA_COMPGEN(0x000d5e80, 0x5b, ??1CImage@@UAE@XZ)

// ---------------------------------------------------------------------------
// (vtable slot 12): Create. Load a surface BY FILE NAME: hand the path to the parent
// pool's 3-arg create (Createa58_3 @0x142560) as (path, capArg, flagsArg) - where
// flagsArg = keyed ? g_surfaceColorKey : -1 and capArg = g_resourceInstallActive ?
// 0x800 : 0 - then cache the surface geometry (w/h, halved) and clear the
// m_originX/m_originY origin. __thiscall, ret 8 (2 stack args).
//
// SETTLED 2026-07-27 (the cast here carried an open identity TODO): arg1 is a
// `char* path`, not a descriptor. Retail 0x152ea9 loads [esp+0x10] (= arg1) and pushes
// it LAST of the three -> it is Createa58_3's FIRST parameter. Createa58_3 @0x1425c5
// then pushes that same slot last into `call [eax+0x28]` (vtable 0x5efa58 slot 10 =
// CFileImageSurface::LoadByExt @0x148940), whose first act is
// `push 0x2e / push arg2 / call strrchr` followed by _stricmp against
// ".BMP" / ".PCX" / ".PID". A path, end to end.
// ---------------------------------------------------------------------------
RVA(0x00152e90, 0x8b)
i32 CImage::Create(char* path, i32 keyed) {
    i32 flagsArg = (keyed != 0) ? g_surfaceColorKey : -1;
    i32 capArg = 0;
    if (g_resourceInstallActive != 0) {
        capArg = 0x800;
    }
    CDDSurface* item = m_ownerCtx->m_ptrColl->Createa58_3(path, capArg, flagsArg);
    m_surface = item;
    if (item == 0) {
        return 0;
    }
    // No w/h locals HERE (the two sibling loaders below keep theirs): the halves are
    // re-read from the members just written, which is what makes cl put the width temp
    // in edx and the height temp in ecx.  Naming the pair swaps that assignment.
    m_width = item->m_width;
    m_height = item->m_height;
    m_anchorX = m_width >> 1;
    m_anchorY = m_height >> 1;
    if (item->m_hasColorKey != 0) {
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
    char* resolved = src->BeginParse();
    if (resolved == 0) {
        return 0;
    }
    // The 3rd argument is the blob LENGTH (m_length) - the independent corroboration
    // that LoadDispatch's slot-10 `size` parameter is a byte count, not a pointer.
    RecordBytes blob;
    blob.m_chars = resolved;
    i32 result = this->LoadDispatch(
        // BeginParse is a PROVEN-heterogeneous slot - it yields a PID/BMP blob here, a
        // WWD block in CGameLevel::LoadFromSource and a raw size in DDrawSubMgrLeafScan -
        // so its return stays the generic handle <Gruntz/ParseSource.h> declares and each
        // consumer names the concrete record at its own seam. This is that one seam.
        static_cast<PidHeader*>(blob.m_rec),
        static_cast<u32>(index),
        src->m_length,
        arg
    );
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
// (ex-wall note: this function is now EXACT - the text below is HISTORY, not a
// current claim. Retired by the stale-marker sweep.)
// 99.86% - all 108 instructions byte-identical to retail (verified llvm-objdump
// base vs target). The lone residual is the objdiff reloc-typing scoring artifact
// on the two g_resourceInstallActive/B DIR32 refs (REL32-vs-DIR32 against differently-
// named symbols); the code bytes match. See MEMORY objdiff-reloc-scoring.
// ---------------------------------------------------------------------------
RVA(0x00152fb0, 0x123)
i32 CImage::LoadDispatch(PidHeader* desc, u32 mode, u32 size, i32 keyed) {
    if (mode != 1 && mode != 2 && mode != 3 && mode != 4) {
        return 0;
    }
    // The skip/fill grammar is what routes to the slot-13 builder: BuildSlot13 ->
    // Build decodes exactly that stream (m_rleData/m_rleLen).
    if (mode == 4 && (desc->flags & PID_GRAMMAR_SKIPRUN)) {
        if (!BuildSlot13(desc, size)) {
            return 0;
        }
        // 0x153006 `test BYTE PTR [edi+4],0x40` gates the shade-type select.
        if (m_owned != 0 && (desc->flags & PID_SRC_8BPP_SHADE)) {
            m_owned->Select(2, 0);
            return 1;
        }
        return 1;
    }
    i32 flagsArg = (keyed != 0) ? g_surfaceColorKey : -1;
    if (mode == 4 || mode == 3) {
        i32 g10 = desc->offsetX;
        i32 g14 = desc->offsetY;
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
    // CreateA is MONOMORPHIC (the old "polymorphic first arg" note was a conflation
    // with the neighbouring CreateB, which really does take a width/height pair via
    // LoadKeyed -> BlitSurf @0x13e0d0). CreateA's a1 lands in ResolveEx's `buf`, which
    // 0x1457a0 reads at +0x04/+0x08/+0x0c => this very desc; its a3 lands in ResolveEx's
    // `size` and thence in DecodePcxData's, gated by `cmp eax,0x300 / jbe fail` at
    // 0x145847 followed by `lea eax,[eax+edi-0x300]` => a byte count with the 768-byte
    // palette at the blob tail. SETTLED 2026-07-27: the ex-`void* a` parameter IS that
    // count (this argument used to be a `void*` carrying an open identity TODO).
    CDDSurface* item =
        m_ownerCtx->m_ptrColl->CreateA(desc, static_cast<i32>(mode), size, capArg, flagsArg);
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
    if (item->m_hasColorKey != 0) {
        m_loadResult = 0x11;
        return 1;
    }
    m_loadResult = 0x10;
    return 1;
}

// (vtable slot 9): Create24. Allocate a BLANK surface of the given geometry from the
// parent pool's 5-arg create (CreateB @0x1423c0), then cache back the surface's own
// w/h. SETTLED 2026-07-27: arg1/arg2 are a WIDTH and a HEIGHT, not a descriptor+mode.
// Retail 0x153107 pushes [esp+0xc]=arg1 last and [esp+0x14]=arg2 next-to-last into
// CreateB; CreateB @0x142425 forwards them as LoadKeyed's a2/a3 (`call [eax+0x2c]`,
// vtable 0x5efa58 slot 11 = 0x148840), which forwards them again to
// CDDSurface::BlitSurf @0x13e0d0 - and 0x13e0f1/0x13e0f8 store them into the
// surface's +0x18/+0x1c, the exact two fields this function reads back below as
// m_height/m_width. (`0` in the 3rd CreateB slot is BlitSurf's format arg.)
RVA(0x001530e0, 0x92)
i32 CImage::Create24(i32 width, i32 height, i32 keyed) {
    i32 flagsArg = (keyed != 0) ? g_surfaceColorKey : -1;
    i32 capArg = 0;
    if (g_resourceInstallActive != 0) {
        capArg = 0x800;
    }
    CDDSurface* item = m_ownerCtx->m_ptrColl->CreateB(width, height, 0, capArg, flagsArg);
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
    if (item->m_hasColorKey != 0) {
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
RVA(0x00153180, 0xda)
i32 CImage::BuildSlot13(PidHeader* desc, u32 size) {
    CDDrawShadeBlit* owned = new CDDrawShadeBlit();
    m_owned = owned;
    if (owned == 0) {
        return 0;
    }
    // The cross-class reinterpret that used to sit on `desc` here is GONE: Build's
    // source and this slot's descriptor are the same `struct PidHeader` (the
    // CImageFrameDesc/CImageBuildDesc pair was two padded views of it).
    if (!owned->Build(desc, static_cast<i32>(size), m_ownerCtx->m_drawTarget->m_frontPair->m_bpp)) {
        return 0;
    }
    i32 w = m_owned->m_width;
    m_width = w;
    i32 h = m_owned->m_height;
    m_height = h;
    m_loadResult = 0x11;
    m_anchorX = w >> 1;
    m_anchorY = h >> 1;
    m_originX = desc->offsetX;
    m_originY = desc->offsetY;
    return 1;
}

RVA(0x00153260, 0x41)
void CImage::FreeAll() {
    m_width = 0;
    m_height = 0;
    if (m_surface != 0) {
        m_ownerCtx->m_ptrColl->RemoveItemA(m_surface);
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
i32 CImage::SetOrigin(PidHeader* desc, i32 mode) {
    if (mode == 4 || mode == 3) {
        i32 oy = desc->offsetY;
        i32 ox = desc->offsetX;
        m_originX = ox;
        m_originY = oy;
    } else {
        m_originX = 0;
        m_originY = 0;
    }
    return 1;
}

RVA(0x00153370, 0xf)
void CImage::FlipVertical(void*) {
    if (m_surface) {
        m_surface->FlipVertical();
    }
}

RVA(0x00153380, 0xeb)
i32 CImage::Reload(CParseSource* src, i32 arg) {
    // m_surface is RE-READ into the local before the Restore: retail chains the two
    // loads through one register (`mov eax,[esi+0x2c]; mov eax,[eax+8]`), which is what
    // a reassigned named local lowers to; reading `m_surface->m_ddSurface` as one
    // expression gives the value its own register instead (edx + eax).
    CDDSurface* surf = m_surface;
    if (surf == 0) {
        return 1;
    }
    IDirectDrawSurface* s = surf->m_ddSurface;
    if (s != 0) {
        if (s->IsLost() == 0) {
            return 1;
        }
    }
    surf = m_surface;
    if (surf->m_ddSurface->Restore() != 0) {
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
    char* resolved = src->BeginParse();
    if (resolved == 0) {
        return 0;
    }
    if (src->m_length == 0) {
        return 0;
    }
    // CDDSurface::Resolve(surf, buf, type, size, surf2): resolved is the decoded buffer,
    // src->m_length its byte size, g_surfaceColorKey lands in the (PID-only) surf2 slot.
    return m_surface->Resolve(
        m_ownerCtx->m_ptrColl,
        resolved,
        index,
        static_cast<u32>(src->m_length),
        g_surfaceColorKey
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
RVA(0x00153470, 0x31a)
void CImage::RenderImage(CResolveNode* info, CDDrawSurfacePair* dst) {
    i32 mode = info->m_stateFlags;
    if (mode & 1) {
        info->m_dirty.m_armed = -1;
        return;
    }
    if (mode & 8) {
        if (g_engineFrameDelta >= info->m_44) {
            info->m_44 = info->m_48;
            mode ^= 0x10000000;
            info->m_stateFlags = mode;
        } else {
            info->m_44 -= g_engineFrameDelta;
        }
        mode = info->m_stateFlags;
        if (!(mode & 0x10000000)) {
            info->m_dirty.m_armed = -1;
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
    LONG x = m_originX - m_anchorX + info->m_plotDX + info->m_screenX;
    LONG y = m_originY - m_anchorY + info->m_plotDY + info->m_screenY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    i32 dleft = x;
    i32 dtop = y;
    i32 dright = right;
    i32 dbottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect srcClip = m_ownerCtx->m_level->m_planeCtx;
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
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
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
        if (x < info->m_clip.left) {
            dleft = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            dright = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            dtop = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            dbottom = info->m_clip.bottom;
        }
    }
    i32 w = dright - dleft + 1;
    i32 h = dbottom - dtop + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    RECT s;
    s.left = dleft - x;
    s.top = dtop - y;
    s.right = s.left + w;
    s.bottom = s.top + h;
    dst->m_surface->BltFast(dleft, dtop, m_surface, &s, m_loadResult);
    info->m_dirty.m_lastX = dleft;
    info->m_dirty.m_rect.left = dleft;
    info->m_dirty.m_lastY = dtop;
    info->m_dirty.m_w = w;
    info->m_dirty.m_rect.top = dtop;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
    info->m_dirty.m_rect.right = dright;
    info->m_dirty.m_rect.bottom = dbottom;
}

RVA(0x00153790, 0x6a)
void CImage::RenderFrame(CDDrawSurfacePair* target, i32 x, i32 y, i32 flags) {
    static CResolveNode clip; // magic-static guard @0x6bf314, ctor 0x1549d0 + atexit
    if (clip.Init(m_ownerCtx, 0, x, y, flags, 0)) {
        this->RenderImage(&clip, target);
    }
}

// The shared clip/resolve singleton is the canonical CResolveNode (class in
// <Gruntz/ResolveNode.h>). Its default ctor (0x1549d0) + Init (0x1647e0) are external
// engine __thiscall callees; the magic-static init + the Init call reloc-mask, and the
// node's virtual ~CResolveNode drives the compiler-emitted atexit thunk.
//
// The +0x38 render virtual (slot 14, RenderImage @0x153470, reconstructed above) is
// dispatched on `this` as an ordinary virtual call (`this->RenderImage(...)` ->
// `mov ecx,this; call [vptr+0x38]`). The `clip` CResolveNode IS the blit request
// request the RESOLVE method fills in (same physical layout); the cast is transitional
// (the ex-CBlitInfo view - unified onto CResolveNode).
//
// The magic-static trio cl5 emits for `static CResolveNode clip;` has NO source VarDecl
// to hang DATA() on, so the three compiler-minted symbols are pinned to their retail
// addresses verbatim (the DATA_SYMBOL/RVA_COMPGEN carriers). Per local static:
//   the object   .bss  0x2bf2a0 / 0x2bf228
//   the once-guard .bss  0x2bf314 / 0x2bf29c   (`$S<n>` byte)
//   the atexit dtor thunk .text 0x153800 / 0x1538b0 (`$E<n>`, pushed to atexit)
// The `$S<5-digit>` tail on the .bss names is cl5's per-TU COMDAT sequence number: it
// SHIFTS if CImage.cpp's earlier statics change, so re-read it from the base obj
// (`llvm-nm build/objdiff/base/cimage.obj`) if labels.py reports "not in base obj".

RVA(0x00153810, 0x95)
void CImage::RenderFrameClipped(
    CDDrawSurfacePair* target,
    i32 x,
    i32 y,
    RECT* clipRect,
    i32 flags
) {
    static CResolveNode clip; // magic-static guard @0x6bf29c, ctor 0x1549d0 + atexit
    if (clip.Init(m_ownerCtx, 0, x, y, flags, 0)) {
        if (clipRect != 0) {
            g_imageClipRect[0] = clipRect->left;
            g_imageClipRect[1] = clipRect->top;
            g_imageClipRect[2] = clipRect->right;
            g_imageClipRect[3] = clipRect->bottom;
        }
        this->RenderImage(&clip, target);
    }
}

// ---------------------------------------------------------------------------
// No flip, surface blit (BltEx, blend mode 6).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x001538c0, 0x257)
void CImage::BlitNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_originX - info->m_plotDX - m_anchorX;
    LONG y = info->m_screenY - m_originY - info->m_plotDY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
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
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
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
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT | DDBLTFX_MIRRORUPDOWN; // 6
    d.right += 1;
    d.bottom += 1;
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// ---------------------------------------------------------------------------
// Vertical flip, surface blit (BltEx, blend mode 2).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x00153b20, 0x270)
void CImage::BlitFlipV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - info->m_plotDX - m_anchorX - m_originX;
    LONG y = m_originY - m_anchorY + info->m_plotDY + info->m_screenY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
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
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
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
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORLEFTRIGHT; // 2
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// ---------------------------------------------------------------------------
// Horizontal flip, surface blit (BltEx, blend mode 4).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x00153d90, 0x259)
void CImage::BlitFlipH(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_plotDX - m_anchorX + m_originX + info->m_screenX;
    LONG y = info->m_screenY - m_originY - m_anchorY - info->m_plotDY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    RECT d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
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
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
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
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    RECT s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w;
    s.bottom = s.top + h;
    d.right += 1;
    d.bottom += 1;
    g_bltFx.dwDDFX = DDBLTFX_MIRRORUPDOWN; // 4
    dst->m_surface->BltEx(&d, m_surface, &s, 0x8800, &g_bltFx);
    d.right -= 1;
    d.bottom -= 1;
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// ---------------------------------------------------------------------------
// X+Y flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 0/0).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x00153ff0, 0x280)
void CImage::BlitShadeFlipHV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_anchorX + m_originX + info->m_plotDX;
    LONG y = info->m_screenY - m_anchorY + m_originY + info->m_plotDY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
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
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
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
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 0);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// ---------------------------------------------------------------------------
// No flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 1/1).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x00154270, 0x257)
void CImage::BlitShadeNorm(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_originX - m_anchorX - info->m_plotDX;
    LONG y = info->m_screenY - m_originY - m_anchorY - info->m_plotDY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
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
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
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
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 1, 1);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// ---------------------------------------------------------------------------
// Vertical flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 1/0).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x001544d0, 0x275)
void CImage::BlitShadeFlipV(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_screenX - m_anchorX - info->m_plotDX - m_originX;
    LONG y = m_originY + info->m_plotDY + info->m_screenY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
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
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
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
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 1, 0);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}

// ---------------------------------------------------------------------------
// X flip, shaded blit (CDDrawShadeBlit::Blit, sel/p4 = 0/1).
// ---------------------------------------------------------------------------
// @early-stop
RVA(0x00154750, 0x275)
void CImage::BlitShadeFlipH(CResolveNode* info, CDDrawSurfacePair* dst) {
    LONG x = info->m_plotDX + m_originX + info->m_screenX - m_anchorX;
    LONG y = info->m_screenY - m_originY - info->m_plotDY - m_anchorY;
    if (info->m_flags & 0x40000) {
        info->m_level->m_mainPlane->WrapCoord(&x, &y);
    }
    i32 right = m_width + x - 1;
    i32 bottom = m_height + y - 1;
    ShadeRect d;
    d.left = x;
    d.top = y;
    d.right = right;
    d.bottom = bottom;
    if (info->m_flags & 0x40000) {
        BlitRect clipA = m_ownerCtx->m_level->m_planeCtx;
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
    } else if (info->m_clip.left == static_cast<i32>(0x80000000)) {
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
        if (x < info->m_clip.left) {
            d.left = info->m_clip.left;
        }
        if (right > info->m_clip.right) {
            d.right = info->m_clip.right;
        }
        if (y < info->m_clip.top) {
            d.top = info->m_clip.top;
        }
        if (bottom > info->m_clip.bottom) {
            d.bottom = info->m_clip.bottom;
        }
    }
    i32 w = d.right - d.left + 1;
    i32 h = d.bottom - d.top + 1;
    if (w <= 0 || h <= 0) {
        info->m_dirty.m_armed = -1;
        return;
    }
    ShadeRect s;
    s.left = right - d.right;
    s.top = bottom - d.bottom;
    s.right = s.left + w - 1;
    s.bottom = s.top + h - 1;
    if (info->m_drawActive) {
        m_owned->Select(info->m_drawFillCmd, info->m_drawFillArg);
    }
    m_owned->Blit(&d, dst->m_surface, &s, 0, 1);
    info->m_dirty.m_lastX = d.left;
    info->m_dirty.m_lastY = d.top;
    info->m_dirty.m_rect = *(&d);
    info->m_dirty.m_w = w;
    info->m_dirty.m_h = h;
    info->m_dirty.m_armed = 0;
}
