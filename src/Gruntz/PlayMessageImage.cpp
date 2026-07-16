// PlayMessageImage.cpp - the two CPlay "GAME_MESSAGEZ" overlay renderers.
//
// Both look up the GAME_MESSAGEZ image set in the image registry
// (m_c->m_imageRegistry->m_10map, an MFC CMapStringToOb name->object map), pull a frame out
// of the set by a signed index, and blit it centered:
//   * DrawMessageFrame (0x0d1650, non-virtual): index + useFront are caller args;
//     the frame is blit into the active viewport via the shared 0x115300 layer
//     blit helper (LayerBlitFrame, glyphstr), centered on the viewport rect.
//   * Vslot23 (0x0cfef0, vtable slot 35): presents the state's message screen -
//     Present(0x3c), pick frame 3 (or 4 when Update()==7), CImage::RenderFrame it
//     centered on the draw surface, then Flip.
//
// Kept in its own TU so the CImageSet/CImage/CDDSurface includes do not perturb the
// matched Play.cpp regalloc. Only offsets + code bytes are load-bearing.
#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/Play.h>
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_24: planeCtx viewport rect) // CPlay + CDDrawSurfaceMgr/CImageRegistry/CDDrawSubMgrPages (m_c->m_imageRegistry/m_24/m_drawTarget)
#include <Image/ImageSet.h>   // CImageSet::GetAt (m_frames/m_minIndex/m_maxIndex) + CImageFrame
#include <Image/CImage.h>     // CImage::RenderFrame (0x153790)
#include <DDrawMgr/DDSurface.h>        // CDDSurface::Flip (0x13e850)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)
#include <Globals.h>                   // s_GameMessagez ("GAME_MESSAGEZ" @0x611ab8)

// The shared __cdecl layer-blit helper (0x115300, in src/Gruntz/GlyphStringDraw.cpp):
// blit a CImage frame into the active draw-target layer. `m_c` is a CDDrawSurfaceMgr,
// the same real class ResMgr.h models as CDDrawSurfaceMgr (the helper's host param), so the cast
// documents that conflation; `frame` is a CImage (passed as-is).
i32 LayerBlitFrame(CDDrawSurfaceMgr*, CImage*, i32, i32, i32, i32); // 0x115300

// (the PresentHost_faec0 placeholder is GONE - identity RECOVERED. 0xfaec0 is
// CState::Present (declared in <Gruntz/State.h>, defined in Attract.cpp): its only other
// caller, CGruntzMgr::RunModalDialog, invokes it as `mov ecx,[esi+0x2c]; call 0x1ec9` and
// CGruntzMgr+0x2c is m_curState, a CState* - so the receiver Vslot23 was casting `this` to
// was simply CState, which CPlay already IS. The cast fell out with the placeholder.)

// ===========================================================================
// CPlay::DrawMessageFrame (0x0d1650) - draw the GAME_MESSAGEZ image `index`
// centered in the active viewport. useFront selects the front/back layer node.
// ===========================================================================
// @early-stop
// ~88%: logic byte-faithful (lookup + inlined GetAt bounds/frame + centered blit
// all match). Wall is a regalloc register-CLASS coin-flip: retail keeps m_c in the
// caller-saved ecx (leaving esi/edi/ebx free to read all 4 viewport corners UP FRONT
// into registers), while cl assigns m_c to callee-saved esi (reusing `this`'s reg),
// which forces the two corner reads to be scheduled lazily around the /2 divides.
// Same instructions + operands, only the register names/read-order differ (verified
// sema disasm --diff); tried l/t/r/b locals, cx/cy locals, fresh Lookup local - none
// tip m_c out of esi. Not source-steerable (permuter: no change).
RVA(0x000d1650, 0x90)
void CPlay::DrawMessageFrame(i32 index, i32 useFront) {
    CObject* set_ob = 0;
    m_c->m_imageRegistry->m_10map.Lookup(s_GameMessagez, set_ob);
    CImageSet* set = (CImageSet*)set_ob;
    if (set != 0) {
        CImage* frame = set->GetAt(index);
        if (frame != 0) {
            LevelCoordRect& vp = m_c->m_level->m_planeCtx;
            i32 cx = vp.minX + (vp.maxX - vp.minX) / 2;
            i32 cy = vp.minY + (vp.maxY - vp.minY) / 2;
            LayerBlitFrame(m_c, frame, cx, cy, useFront, 1);
        }
    }
}

// ===========================================================================
// CPlay::Vslot23 (0x0cfef0, slot 35) - present the state's GAME_MESSAGEZ screen.
// ===========================================================================
// @early-stop
// ~95%: logic byte-faithful. The escaped Lookup out-param is copied to a fresh local
// (`set`) so cl promotes it into the callee-saved ebx across the Update() virtual
// call - matching retail. Residual is a 2-instruction /O2 SCHEDULING swap: retail
// emits the `out = 0` init AFTER pushing the Lookup args (so the slot sits at
// [esp+0x14]); cl emits it before (at [esp+0xc]). Not source-steerable (the fresh
// local is required for the ebx promotion; permuter: no change). The 4*esi/esi*4 and
// `sar eax`/`sar eax,1` diff rows are disasm-formatting only, not real byte diffs.
RVA(0x000cfef0, 0xbc)
i32 CPlay::Vslot23() {
    Present(0x3c);

    CObject* lookup_ob = 0;
    m_c->m_imageRegistry->m_10map.Lookup(s_GameMessagez, lookup_ob);
    CImageSet* lookup = (CImageSet*)lookup_ob;
    CImageSet* set = lookup;
    if (set == 0) {
        return 0;
    }

    i32 index = 3;
    if (Update() == 7) {
        index = 4;
    }
    CImage* frame = set->GetAt(index);
    if (frame == 0) {
        return 0;
    }

    CDDrawSurfacePair* surf = m_c->m_drawTarget->m_backPair;
    if (surf == 0) {
        return 0;
    }
    ((CImage*)frame)->RenderFrame(surf, (void*)(surf->m_width / 2), (void*)(surf->m_height / 2), 0);
    m_c->m_drawTarget->m_frontPair->m_surface->Flip((CDDSurface*)0);
    return 1;
}
