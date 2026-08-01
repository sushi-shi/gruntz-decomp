#include <Gruntz/PlayMessageImage.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/Play.h>
#include <Gruntz/GameLevel.h>
#include <Image/ImageSet.h>
#include <Image/CImage.h>
#include <DDrawMgr/DDSurface.h>
#include <DDrawMgr/DDrawSurfacePair.h>

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
