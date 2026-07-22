#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/Play.h>
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_24: planeCtx viewport rect) // CPlay + CDDrawSurfaceMgr/CDDrawWorkerRegistry/CDDrawSubMgrPages (m_c->m_imageRegistry/m_24/m_drawTarget)
#include <Image/ImageSet.h>   // CDDrawWorker::GetAt (m_frames/m_minIndex/m_maxIndex) + CImageFrame
#include <Image/CImage.h>     // CImage::RenderFrame (0x153790)
#include <DDrawMgr/DDSurface.h>        // CDDSurface::Flip (0x13e850)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)

i32 LayerBlitFrame(CDDrawSurfaceMgr*, CImage*, i32, i32, i32, i32); // 0x115300

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
    m_world->m_imageRegistry->m_10map.Lookup("GAME_MESSAGEZ", set_ob);
    CDDrawWorker* set = static_cast<CDDrawWorker*>(set_ob);
    if (set != 0) {
        CImage* frame = set->GetAt(index);
        if (frame != 0) {
            LevelCoordRect& vp = m_world->m_level->m_planeCtx;
            i32 cx = vp.left + (vp.right - vp.left) / 2;
            i32 cy = vp.top + (vp.bottom - vp.top) / 2;
            LayerBlitFrame(m_world, frame, cx, cy, useFront, 1);
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
    m_world->m_imageRegistry->m_10map.Lookup("GAME_MESSAGEZ", lookup_ob);
    CDDrawWorker* lookup = static_cast<CDDrawWorker*>(lookup_ob);
    CDDrawWorker* set = lookup;
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

    CDDrawSurfacePair* surf = m_world->m_drawTarget->m_backPair;
    if (surf == 0) {
        return 0;
    }
    (static_cast<CImage*>(frame))->RenderFrame(surf, reinterpret_cast<void*>((surf->m_width / 2)), reinterpret_cast<void*>((surf->m_height / 2)), 0);
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(static_cast<CDDSurface*>(0));
    return 1;
}
