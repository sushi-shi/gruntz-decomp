#include <Gruntz/PlayMessageImage.h>      // this TU's external declarations
#include <DDrawMgr/DDrawSubMgrPages.h>    // the m_drawTarget pages (full def)
#include <DDrawMgr/DDrawWorkerRegistry.h> // m_imageRegistry (full def)
#include <Gruntz/Play.h>
#include <Gruntz/GameLevel.h> // canonical CGameLevel (m_24: planeCtx viewport rect) // CPlay + CDDrawSurfaceMgr/CDDrawWorkerRegistry/CDDrawSubMgrPages (m_c->m_imageRegistry/m_24/m_drawTarget)
#include <Image/ImageSet.h>   // CDDrawWorker::GetAt (m_frames/m_minIndex/m_maxIndex) + CImageFrame
#include <Image/CImage.h>     // CImage::RenderFrame (0x153790)
#include <DDrawMgr/DDSurface.h>        // CDDSurface::Flip (0x13e850)
#include <DDrawMgr/DDrawSurfacePair.h> // the CDDrawSubMgrPages pages (real class of m_10/m_14/m_18)

// ===========================================================================
// CPlay::DrawMessageFrame (0x0d1650) - draw the GAME_MESSAGEZ image `index`
// centered in the active viewport. useFront selects the front/back layer node.
// ===========================================================================
// @early-stop
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
    (static_cast<CImage*>(frame))->RenderFrame(surf, surf->m_width / 2, surf->m_height / 2, 0);
    m_world->m_drawTarget->m_frontPair->m_surface->Flip(static_cast<CDDSurface*>(0));
    return 1;
}
