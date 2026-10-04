#include <StdAfx.h>

#include <Ints.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <DDrawMgr/DDrawSubMgrPages.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawPaletteRegistry.h>
#include <DDrawMgr/DDrawPlacedWorker.h>
#include <DDrawMgr/DDrawSubMgr.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StateId.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <SafeDelete.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

i32 CDDrawSubMgrPages::CreateChildren(i32 w, i32 h, ColorDepth bpp, i32 flags) {

    m_frontSurface = new CDDrawFrontSurface(m_ownerCtx, 0, 0);
    m_backPair = new CDDrawSurfacePair(m_ownerCtx, IDX(DDRAW_PAGE_BACK), 0);
    m_overlayPair = new CDDrawSurfacePair(m_ownerCtx, IDX(DDRAW_PAGE_OVERLAY), 0);

    if (m_frontSurface->SetGeometry(w, h, bpp) == BPP_UNSET) {
        OwnerMgr()->SetInitError(WORLDERR_FRONT_SURFACE);
        return 0;
    }
    if (m_backPair->Create(w, h, bpp, 0) == BPP_UNSET) {
        OwnerMgr()->SetInitError(WORLDERR_BACK_SURFACE);
        return 0;
    }
    if (!HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_SKIP_OVERLAY)) {
        if (m_overlayPair->Create(w, h, bpp, 0) == BPP_UNSET) {
            OwnerMgr()->SetInitError(WORLDERR_OVERLAY_SURFACE);
            return 0;
        }
    }
    return 1;
}

void CDDrawSubMgrPages::Unload() {
    SAFE_DELETE(m_frontSurface);
    SAFE_DELETE(m_backPair);
    SAFE_DELETE(m_overlayPair);
}

i32 CDDrawSubMgrPages::ResolvePageImage(char* name, DDrawPageKind pageIndex) {
    CDDrawSurfacePair* p;
    if (pageIndex == DDRAW_PAGE_OVERLAY) {
        p = m_overlayPair;
        if (!p) {
            return 0;
        }
    } else {
        p = m_backPair;
        if (!p) {
            return 0;
        }
    }
    return p->ResolveImageName(name);
}

i32 CDDrawSubMgrPages::LoadPageImage(CRezItm* src, DDrawPageKind pageIndex) {
    CDDrawSurfacePair* p;
    if (pageIndex == DDRAW_PAGE_OVERLAY) {
        p = m_overlayPair;
        if (!p) {
            return 0;
        }
    } else {
        p = m_backPair;
        if (!p) {
            return 0;
        }
    }
    return p->LoadImage(src);
}

void CDDrawSubMgrPages::BltDirtyChildrenEx() {
    OwnerMgr()->ChildGroup()->BltDirtyChildrenEx(m_frontSurface, m_backPair, m_overlayPair);
}

void CDDrawSubMgrPages::FlipAndNotify() {
    m_frontSurface->GetSurface()->Flip(NULL);
    CDDrawSurfaceMgr* n = OwnerMgr();
    CDDrawChildGroup* c = n->ChildGroup();
    CDDrawSubMgrPages* s = n->GetDrawTarget();
    c->BltDirtyChildren(s->GetBackPair(), s->m_overlayPair);
}

i32 CDDrawSubMgrPages::PagesReady() {
    if (m_frontSurface && !m_frontSurface->Probe()) {
        return 0;
    }
    if (m_overlayPair && !m_overlayPair->RestoreIfLost()) {
        return 0;
    }
    return 1;
}

i32 CDDrawSubMgrPages::ResizePages(i32 w, i32 h, ColorDepth bpp) {
    CDDrawFrontSurface* p = m_frontSurface;
    if (p->GetWidth() != w || p->GetHeight() != h || p->m_bpp != bpp) {
        if (!m_frontSurface->SetGeom(w, h, bpp)) {
            return 0;
        }
        if (!m_backPair->SetGeom(w, h, bpp)) {
            return 0;
        }
        if (m_overlayPair && m_overlayPair->IsLoaded()) {
            if (!m_overlayPair->SetGeom(w, h, bpp)) {
                return 0;
            }
        }
    }
    return 1;
}

i32 CDDrawSubMgrPages::BlitPage(CDDrawSurfacePair* dst) {
    if (!m_frontSurface) {
        return 0;
    }
    CDDSurface* s = m_frontSurface->GetSurface();
    if (!s) {
        return 0;
    }
    CDDSurface* d = dst->GetSurface();
    if (!d) {
        return 0;
    }
    i32 hr = d->Blt(s);
    return hr == 0;
}

i32 CDDrawSubMgrPages::CreateOverlay(i32 copyFromBack, i32 createFlag) {
    if (m_overlayPair->IsLoaded()) {
        return 0;
    }
    CDDrawSurfacePair* backBuffer = m_backPair;
    if (!m_overlayPair->Create(
            backBuffer->GetWidth(),
            backBuffer->GetHeight(),
            backBuffer->m_bpp,
            createFlag
        )) {
        return 0;
    }
    if (copyFromBack) {
        BLT_SURFACE_PAIR_SELF(m_overlayPair, m_backPair);
    }
    return 1;
}

i32 CDDrawSubMgrPages::HasOverlay() {
    if (!m_overlayPair) {
        return 0;
    }
    return m_overlayPair->IsLoaded() != 0;
}

void CDDrawSubMgrPages::UnloadOverlay() {
    if (m_overlayPair != NULL) {
        m_overlayPair->Unload();
    }
}

void CDDrawSubMgrPages::ClearAllPages(u32 color) {
    m_backPair->GetSurface()->Fill(color);
    m_frontSurface->GetSurface()->Flip(NULL);
    m_backPair->GetSurface()->Fill(color);
    m_frontSurface->GetSurface()->Flip(NULL);
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(OwnerMgr()->m_flags), SURFACEMGR_TRIPLE_BUFFER)) {
        m_backPair->GetSurface()->Fill(color);
        m_frontSurface->GetSurface()->Flip(NULL);
    }
}

i32 CDDrawSubMgrPages::PresentBackPage() {
    CDDrawFrontSurface* front = m_frontSurface;
    CDDrawSurfacePair* back = m_backPair;
    b32 ok;
    if (front == NULL) {
        ok = false;
    } else {
        CDDSurface* frontBuffer = front->GetSurface();
        if (frontBuffer == NULL) {
            ok = false;
        } else {
            CDDSurface* backBuffer = back->GetSurface();
            if (backBuffer == NULL) {
                ok = false;
            } else {
                i32 hr = backBuffer->Blt(frontBuffer);
                ok = (hr == 0);
            }
        }
    }
    if (ok
        && HAS(static_cast<DDrawSurfaceMgrFlags>(OwnerMgr()->m_flags), SURFACEMGR_TRIPLE_BUFFER)) {
        m_frontSurface->GetSurface()->Flip(NULL);
        CDDrawSurfacePair* a = m_backPair;
        CDDrawFrontSurface* b = m_frontSurface;
        if (b == NULL) {
            return 0;
        }
        CDDSurface* bs = b->GetSurface();
        if (bs == NULL) {
            return 0;
        }
        CDDSurface* as = a->GetSurface();
        if (as == NULL) {
            return 0;
        }
        i32 hr2 = as->Blt(bs);
        ok = (hr2 == 0);
    }
    return ok;
}

i32 CDDrawSubMgrPages::TransEnter() {
    CDDrawSurfacePair* a;
    CDDrawFrontSurface* b;
    CDDSurface* bs;
    CDDSurface* as;
    i32 hr;

    if (!m_overlayPair) {
        goto fail;
    }
    if (!m_overlayPair->IsLoaded()) {
        goto fail;
    }
    a = m_overlayPair;
    b = m_frontSurface;
    if (!b) {
        return 0;
    }
    bs = b->GetSurface();
    if (!bs) {
        return 0;
    }
    as = a->GetSurface();
    if (!as) {
        return 0;
    }
    hr = as->Blt(bs);
    return hr == 0;
fail:
    return 0;
}

i32 CDDrawSubMgrPages::TransTitle() {
    if (!m_backPair) {
        return 0;
    }
    if (!m_overlayPair) {
        return 0;
    }
    if (!m_overlayPair->IsLoaded()) {
        return 0;
    }
    CDDrawSurfacePair* a = m_backPair;
    CDDrawSurfacePair* b = m_overlayPair;
    BLT_SURFACE_PAIR_SELF(b, a);
    return 1;
}

i32 CDDrawSubMgrPages::TransExit() {
    if (!m_backPair) {
        return 0;
    }
    if (!m_overlayPair) {
        return 0;
    }
    if (!m_overlayPair->IsLoaded()) {
        return 0;
    }
    CDDrawSurfacePair* a = m_overlayPair;
    CDDrawSurfacePair* b = m_backPair;
    BLT_SURFACE_PAIR_SELF(b, a);
    return 1;
}

CDrawSubWorker::CDrawSubWorker(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED) {
    m_width = 0;
}

i32 CDrawSubWorker::IsLoaded() {
    if (m_width <= 0) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

LoadableClassId CDrawSubWorker::GetClassId() {
    return CLASSID_SUBWORKER;
}

void CDrawSubWorker::Unload() {
    m_width = 0;
}

i32 CDDrawSurfacePair::IsLoaded() {
    if (m_surface != NULL && m_width > 0 && m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

LoadableClassId CDDrawSurfacePair::GetClassId() {
    return CLASSID_SURFACEPAIR;
}

CDDrawSurfacePair::~CDDrawSurfacePair() {
    Unload();
}

i32 CDDrawFrontSurface::IsLoaded() {
    if (m_surface != NULL && m_width > 0 && m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

LoadableClassId CDDrawFrontSurface::GetClassId() {
    return CLASSID_FRONT_SURFACE;
}

void CDDrawFrontSurface::Unload() {
    m_width = 0;
}
