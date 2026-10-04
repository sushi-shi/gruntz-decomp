#include <StdAfx.h>

#include <rva.h>

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

RVA(0x001588f0, 0x1c5)
i32 CDisplayBuffers::CreateChildren(i32 w, i32 h, ColorDepth bpp, i32 flags) {

    m_frontSurface = new CDDrawFrontSurface(m_world, 0, 0);
    m_backBuffer = new CRenderBuffer(m_world, IDX(DDRAW_PAGE_BACK), 0);
    m_overlayBuffer = new CRenderBuffer(m_world, IDX(DDRAW_PAGE_OVERLAY), 0);

    if (m_frontSurface->SetGeometry(w, h, bpp) == BPP_UNSET) {
        GetWorld()->SetInitError(WORLDERR_FRONT_SURFACE);
        return 0;
    }
    if (m_backBuffer->Create(w, h, bpp, 0) == BPP_UNSET) {
        GetWorld()->SetInitError(WORLDERR_BACK_SURFACE);
        return 0;
    }
    if (!HAS(static_cast<DDrawSurfaceMgrFlags>(flags), SURFACEMGR_SKIP_OVERLAY)) {
        if (m_overlayBuffer->Create(w, h, bpp, 0) == BPP_UNSET) {
            GetWorld()->SetInitError(WORLDERR_OVERLAY_SURFACE);
            return 0;
        }
    }
    return 1;
}

RVA(0x00158ac0, 0x44)
void CDisplayBuffers::Unload() {
    SAFE_DELETE(m_frontSurface);
    SAFE_DELETE(m_backBuffer);
    SAFE_DELETE(m_overlayBuffer);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00158b10, 0x2c)
i32 CDisplayBuffers::ResolvePageImage(char* name, DDrawPageKind pageIndex) {
    CRenderBuffer* p;
    if (pageIndex == DDRAW_PAGE_OVERLAY) {
        p = m_overlayBuffer;
        if (!p) {
            return 0;
        }
    } else {
        p = m_backBuffer;
        if (!p) {
            return 0;
        }
    }
    return p->ResolveImageName(name);
}

RVA(0x00158b40, 0x2c)
i32 CDisplayBuffers::LoadPageImage(CRezItm* src, DDrawPageKind pageIndex) {
    CRenderBuffer* p;
    if (pageIndex == DDRAW_PAGE_OVERLAY) {
        p = m_overlayBuffer;
        if (!p) {
            return 0;
        }
    } else {
        p = m_backBuffer;
        if (!p) {
            return 0;
        }
    }
    return p->LoadImage(src);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00158b70, 0x1c)
void CDisplayBuffers::BltDirtyChildrenEx() {
    GetWorld()->ChildGroup()->BltDirtyChildrenEx(m_frontSurface, m_backBuffer, m_overlayBuffer);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00158b90, 0x28)
void CDisplayBuffers::FlipAndNotify() {
    m_frontSurface->GetSurface()->Flip(NULL);
    CGameWorld* n = GetWorld();
    CDDrawChildGroup* c = n->ChildGroup();
    CDisplayBuffers* s = n->GetDisplayBuffers();
    c->BltDirtyChildren(s->GetBackBuffer(), s->GetOverlayBuffer());
}

RVA(0x00158bc0, 0x2e)
i32 CDisplayBuffers::RestoreLostSurfaces() {
    if (m_frontSurface && !m_frontSurface->RestoreIfLost()) {
        return 0;
    }
    if (m_overlayBuffer && !m_overlayBuffer->RestoreIfLost()) {
        return 0;
    }
    return 1;
}

RVA(0x00158bf0, 0x7f)
i32 CDisplayBuffers::ResizePages(i32 w, i32 h, ColorDepth bpp) {
    CDDrawFrontSurface* p = m_frontSurface;
    if (p->GetWidth() != w || p->GetHeight() != h || p->m_bpp != bpp) {
        if (!m_frontSurface->SetGeom(w, h, bpp)) {
            return 0;
        }
        if (!m_backBuffer->SetGeom(w, h, bpp)) {
            return 0;
        }
        if (m_overlayBuffer && m_overlayBuffer->IsLoaded()) {
            if (!m_overlayBuffer->SetGeom(w, h, bpp)) {
                return 0;
            }
        }
    }
    return 1;
}

RVA(0x00158c70, 0x36)
i32 CDisplayBuffers::CopyFrontToSurface(CRenderBuffer* dst) {
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

RVA(0x00158cb0, 0x6a)
i32 CDisplayBuffers::CreateOverlay(i32 copyFromBack, i32 createFlag) {
    if (m_overlayBuffer->IsLoaded()) {
        return 0;
    }
    CRenderBuffer* backBuffer = m_backBuffer;
    if (!m_overlayBuffer->Create(
            backBuffer->GetWidth(),
            backBuffer->GetHeight(),
            backBuffer->m_bpp,
            createFlag
        )) {
        return 0;
    }
    if (copyFromBack) {
        COPY_RENDER_BUFFER(m_overlayBuffer, m_backBuffer);
    }
    return 1;
}

RVA(0x00158d20, 0x16)
i32 CDisplayBuffers::HasOverlay() {
    if (!m_overlayBuffer) {
        return 0;
    }
    return m_overlayBuffer->IsLoaded() != 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00158d40, 0xd)
void CDisplayBuffers::UnloadOverlay() {
    if (m_overlayBuffer != NULL) {
        m_overlayBuffer->Unload();
    }
}

RVA(0x00158d50, 0x61)
void CDisplayBuffers::ClearAllPages(u32 color) {
    m_backBuffer->GetSurface()->Fill(color);
    m_frontSurface->GetSurface()->Flip(NULL);
    m_backBuffer->GetSurface()->Fill(color);
    m_frontSurface->GetSurface()->Flip(NULL);
    if (HAS(static_cast<DDrawSurfaceMgrFlags>(GetWorld()->m_flags), SURFACEMGR_TRIPLE_BUFFER)) {
        m_backBuffer->GetSurface()->Fill(color);
        m_frontSurface->GetSurface()->Flip(NULL);
    }
}

RVA(0x00158dc0, 0x7d)
i32 CDisplayBuffers::CopyFrontToBackBuffers() {
    CDDrawFrontSurface* front = m_frontSurface;
    CRenderBuffer* back = m_backBuffer;
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
        && HAS(static_cast<DDrawSurfaceMgrFlags>(GetWorld()->m_flags), SURFACEMGR_TRIPLE_BUFFER)) {
        m_frontSurface->GetSurface()->Flip(NULL);
        CRenderBuffer* a = m_backBuffer;
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

// @early-stop
RVA(0x00158e40, 0x4c)
i32 CDisplayBuffers::CopyFrontToOverlay() {
    CRenderBuffer* a;
    CDDrawFrontSurface* b;
    CDDSurface* bs;
    CDDSurface* as;
    i32 hr;

    if (!m_overlayBuffer) {
        goto fail;
    }
    if (!m_overlayBuffer->IsLoaded()) {
        goto fail;
    }
    a = m_overlayBuffer;
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

RVA(0x00158e90, 0x47)
i32 CDisplayBuffers::CopyBackToOverlay() {
    if (!m_backBuffer) {
        return 0;
    }
    if (!m_overlayBuffer) {
        return 0;
    }
    if (!m_overlayBuffer->IsLoaded()) {
        return 0;
    }
    CRenderBuffer* a = m_backBuffer;
    CRenderBuffer* b = m_overlayBuffer;
    COPY_RENDER_BUFFER(b, a);
    return 1;
}

RVA(0x00158ee0, 0x47)
i32 CDisplayBuffers::CopyOverlayToBack() {
    if (!m_backBuffer) {
        return 0;
    }
    if (!m_overlayBuffer) {
        return 0;
    }
    if (!m_overlayBuffer->IsLoaded()) {
        return 0;
    }
    CRenderBuffer* a = m_overlayBuffer;
    CRenderBuffer* b = m_backBuffer;
    COPY_RENDER_BUFFER(b, a);
    return 1;
}

RVA(0x00158f30, 0x27)
CRenderSurface::CRenderSurface(CGameWorld* owner, i32 id, i32 flags)
    : CWapObj(owner, id, flags, CWapObj::NO_SEED) {
    m_width = 0;
}
RVA(0x00158f60, 0x1d)
i32 CRenderSurface::IsLoaded() {
    if (m_width <= 0) {
        return 0;
    }
    if (m_world != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x00158f80, 0x6)
LoadableClassId CRenderSurface::GetClassId() {
    return CLASSID_RENDER_SURFACE;
}

RVA_COMPGEN(0x00158f90, 0x1e, ??_GCRenderSurface@@UAEPAXI@Z)

RVA_COMPGEN(0x00158fb0, 0x19, ??1CRenderSurface@@UAE@XZ)

RVA(0x00159080, 0x8)
void CRenderSurface::Unload() {
    m_width = 0;
}

RVA(0x00159090, 0x24)
i32 CRenderBuffer::IsLoaded() {
    if (m_surface != NULL && m_width > 0 && m_world != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x001590c0, 0x6)
LoadableClassId CRenderBuffer::GetClassId() {
    return CLASSID_RENDER_BUFFER;
}

RVA_COMPGEN(0x001590d0, 0x1e, ??_GCRenderBuffer@@UAEPAXI@Z)
RVA(0x001590f0, 0x56)
CRenderBuffer::~CRenderBuffer() {
    Unload();
}

RVA(0x00159150, 0x24)
i32 CDDrawFrontSurface::IsLoaded() {
    if (m_surface != NULL && m_width > 0 && m_world != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x00159180, 0x6)
LoadableClassId CDDrawFrontSurface::GetClassId() {
    return CLASSID_FRONT_SURFACE;
}

RVA_COMPGEN(0x00159190, 0x1e, ??_GCDDrawFrontSurface@@UAEPAXI@Z)
RVA_COMPGEN(0x001591b0, 0x19, ??1CDDrawFrontSurface@@UAE@XZ)
RVA(0x001591d0, 0x8)
void CDDrawFrontSurface::Unload() {
    m_width = 0;
}
