#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawSubMgr.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerNode.h>
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
#include <Gruntz/ParseSource.h>
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
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RVA(0x001588f0, 0x1c5)
i32 CDDrawSubMgrPages::CreateChildren(i32 w, i32 h, ColorDepth bpp, i32 flags) {

    m_frontPair = new CDDrawSurfaceChildA(m_ownerCtx, 0, 0);
    m_backPair = new CDDrawSurfacePair(m_ownerCtx, 1, 0);
    m_overlayPair = new CDDrawSurfacePair(m_ownerCtx, 2, 0);

    if (m_frontPair->SetGeometry(w, h, bpp) == BPP_UNSET) {
        if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
            OwnerMgr()->m_lastError = WORLDERR_FRONT_SURFACE;
        }
        return 0;
    }
    if (m_backPair->Create(w, h, bpp, 0) == BPP_UNSET) {
        if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
            OwnerMgr()->m_lastError = WORLDERR_BACK_SURFACE;
        }
        return 0;
    }
    if (!(flags & 1)) {
        if (m_overlayPair->Create(w, h, bpp, 0) == BPP_UNSET) {
            if (OwnerMgr()->m_lastError == WORLDERR_NONE) {
                OwnerMgr()->m_lastError = WORLDERR_OVERLAY_SURFACE;
            }
            return 0;
        }
    }
    return 1;
}

RVA(0x00158ac0, 0x44)
void CDDrawSubMgrPages::Unload() {
    if (m_frontPair != NULL) {
        delete m_frontPair;
        m_frontPair = NULL;
    }
    if (m_backPair != NULL) {
        delete m_backPair;
        m_backPair = NULL;
    }
    if (m_overlayPair != NULL) {
        delete m_overlayPair;
        m_overlayPair = NULL;
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00158b10, 0x2c)
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

RVA(0x00158b40, 0x2c)
i32 CDDrawSubMgrPages::LoadPageImage(CParseSource* src, DDrawPageKind pageIndex) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00158b70, 0x1c)
void CDDrawSubMgrPages::BltDirtyChildrenEx() {
    OwnerMgr()->m_childGroup->BltDirtyChildrenEx(m_frontPair, m_backPair, m_overlayPair);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00158b90, 0x28)
void CDDrawSubMgrPages::FlipAndNotify() {
    m_frontPair->m_surface->Flip(NULL);
    CDDrawSurfaceMgr* n = OwnerMgr();
    CDDrawChildGroup* c = n->m_childGroup;
    CDDrawSubMgrPages* s = n->m_drawTarget;
    c->BltDirtyChildren(s->m_backPair, s->m_overlayPair);
}

RVA(0x00158bc0, 0x2e)
i32 CDDrawSubMgrPages::PagesReady() {
    if (m_frontPair && !m_frontPair->Probe()) {
        return 0;
    }
    if (m_overlayPair && !m_overlayPair->RestoreIfLost()) {
        return 0;
    }
    return 1;
}

RVA(0x00158bf0, 0x7f)
i32 CDDrawSubMgrPages::ResizePages(i32 w, i32 h, ColorDepth bpp) {
    CDDrawSurfaceChildA* p = m_frontPair;
    if (p->m_width != w || p->m_height != h || p->m_bpp != bpp) {
        if (!m_frontPair->SetGeom(w, h, bpp)) {
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

RVA(0x00158c70, 0x36)
i32 CDDrawSubMgrPages::BlitPage(CDDrawSurfacePair* dst) {
    if (!m_frontPair) {
        return 0;
    }
    CDDSurface* s = m_frontPair->m_surface;
    if (!s) {
        return 0;
    }
    CDDSurface* d = dst->m_surface;
    if (!d) {
        return 0;
    }
    i32 hr = d->Blt(s);
    return hr == 0;
}

RVA(0x00158cb0, 0x6a)
i32 CDDrawSubMgrPages::CreateOverlay(i32 copyFromBack, i32 createFlag) {
    if (m_overlayPair->IsLoaded()) {
        return 0;
    }
    CDDrawSurfacePair* s14 = m_backPair;
    if (!m_overlayPair->Create(s14->m_width, s14->m_height, s14->m_bpp, createFlag)) {
        return 0;
    }
    if (copyFromBack) {
        BLT_SURFACE_PAIR_SELF(m_overlayPair, m_backPair);
    }
    return 1;
}

RVA(0x00158d20, 0x16)
i32 CDDrawSubMgrPages::HasOverlay() {
    if (!m_overlayPair) {
        return 0;
    }
    return m_overlayPair->IsLoaded() != 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00158d40, 0xd)
void CDDrawSubMgrPages::UnloadOverlay() {
    if (m_overlayPair != NULL) {
        m_overlayPair->Unload();
    }
}

RVA(0x00158d50, 0x61)
void CDDrawSubMgrPages::ClearAllPages(u32 color) {
    m_backPair->m_surface->Fill(color);
    m_frontPair->m_surface->Flip(NULL);
    m_backPair->m_surface->Fill(color);
    m_frontPair->m_surface->Flip(NULL);
    if (OwnerMgr()->m_flags & 2) {
        m_backPair->m_surface->Fill(color);
        m_frontPair->m_surface->Flip(NULL);
    }
}

RVA(0x00158dc0, 0x7d)
i32 CDDrawSubMgrPages::PresentBackPage() {
    CDDrawSurfaceChildA* front = m_frontPair;
    CDDrawSurfacePair* back = m_backPair;
    i32 ok;
    if (front == NULL) {
        ok = 0;
    } else {
        CDDSurface* s10 = front->m_surface;
        if (s10 == NULL) {
            ok = 0;
        } else {
            CDDSurface* s14 = back->m_surface;
            if (s14 == NULL) {
                ok = 0;
            } else {
                i32 hr = s14->Blt(s10);
                ok = (hr == 0);
            }
        }
    }
    if (ok && (OwnerMgr()->m_flags & 2)) {
        m_frontPair->m_surface->Flip(NULL);
        CDDrawSurfacePair* a = m_backPair;
        CDDrawSurfaceChildA* b = m_frontPair;
        if (b == NULL) {
            return 0;
        }
        CDDSurface* bs = b->m_surface;
        if (bs == NULL) {
            return 0;
        }
        CDDSurface* as = a->m_surface;
        if (as == NULL) {
            return 0;
        }
        i32 hr2 = as->Blt(bs);
        ok = (hr2 == 0);
    }
    return ok;
}

// @early-stop
// retail shares ONE return block for the first two guards and keeps a separate
// inline `xor eax,eax; pop esi; ret` for each of the other three. `||` and the
// mid-function `fail:` label (`goto L; if (b) goto ok; L:`) both enter the TOTAL
// cross-jump regime (-> 50.13); with goto-fail our cl elides the second guard's
// xor (IsLoaded's result sits in eax) and splits the pair - all seven xor levers
// measured in docs/patterns/goto-fail-shares-one-exit-block.md.
RVA(0x00158e40, 0x4c)
i32 CDDrawSubMgrPages::TransEnter() {
    CDDrawSurfacePair* a;
    CDDrawSurfaceChildA* b;
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
    b = m_frontPair;
    if (!b) {
        return 0;
    }
    bs = b->m_surface;
    if (!bs) {
        return 0;
    }
    as = a->m_surface;
    if (!as) {
        return 0;
    }
    hr = as->Blt(bs);
    return hr == 0;
fail:
    return 0;
}

RVA(0x00158e90, 0x47)
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

RVA(0x00158ee0, 0x47)
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

RVA(0x00158f30, 0x27)
CDrawSubWorker::CDrawSubWorker(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED) {
    m_width = 0;
}
RVA(0x00158f60, 0x1d)
i32 CDrawSubWorker::IsLoaded() {
    if (m_width <= 0) {
        return 0;
    }
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x00158f80, 0x6)
LoadableClassId CDrawSubWorker::GetClassId() {
    return CLASSID_SUBWORKER;
}

RVA_COMPGEN(0x00158f90, 0x1e, ??_GCDrawSubWorker@@UAEPAXI@Z)

RVA_COMPGEN(0x00158fb0, 0x19, ??1CDrawSubWorker@@UAE@XZ)

RVA(0x00158fd0, 0x41)
i32 CDrawSubWorker::SetGeometry(i32 w, i32 h, ColorDepth bpp) {
    if (w <= 0 || h <= 0) {
        return 0;
    }
    m_width = w;
    m_bpp = bpp;
    m_height = h;
    m_srcRect.bottom = h;
    m_srcRect.left = 0;
    m_srcRect.top = 0;
    m_srcRect.right = w;
    return 1;
}

RVA(0x00159020, 0x55)
i32 CDrawSubWorker::SetGeom(i32 w, i32 h, ColorDepth bpp) {
    if (w <= 0 || h <= 0) {
        return 0;
    }
    if (bpp != BPP_PALETTED_8 && bpp != BPP_RGB_16 && bpp != BPP_RGB_24 && bpp != BPP_RGB_32) {
        return 0;
    }
    m_height = h;
    m_srcRect.bottom = h;
    m_width = w;
    m_bpp = bpp;
    m_srcRect.left = 0;
    m_srcRect.top = 0;
    m_srcRect.right = w;
    return 1;
}

RVA(0x00159080, 0x8)
void CDrawSubWorker::Unload() {
    m_width = 0;
}

RVA(0x00159090, 0x24)
i32 CDDrawSurfacePair::IsLoaded() {
    if (m_surface != NULL && m_width > 0 && m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x001590c0, 0x6)
LoadableClassId CDDrawSurfacePair::GetClassId() {
    return CLASSID_SURFACEPAIR;
}

RVA_COMPGEN(0x001590d0, 0x1e, ??_GCDDrawSurfacePair@@UAEPAXI@Z)
RVA(0x001590f0, 0x56)
CDDrawSurfacePair::~CDDrawSurfacePair() {
    Unload();
}

RVA(0x00159150, 0x24)
i32 CDDrawSurfaceChildA::IsLoaded() {
    if (m_surface != NULL && m_width > 0 && m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x00159180, 0x6)
LoadableClassId CDDrawSurfaceChildA::GetClassId() {
    return CLASSID_SURFACECHILDA;
}

RVA_COMPGEN(0x00159190, 0x1e, ??_GCDDrawSurfaceChildA@@UAEPAXI@Z)
RVA(0x001591b0, 0x19)
CDDrawSurfaceChildA::~CDDrawSurfaceChildA() {}
RVA(0x001591d0, 0x8)
void CDDrawSurfaceChildA::Unload() {
    m_width = 0;
}
