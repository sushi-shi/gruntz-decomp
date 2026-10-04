#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H

#include <rva.h>

#include <DDrawMgr/ColorDepth.h>
#include <Enums.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDrawSurfaceMgr;
class CDDSurface;
class CRenderBuffer;
class CDDrawFrontSurface;

GZ_ENUM_BEGIN(DDrawPageKind)
    DDRAW_PAGE_BACK = 1,
    DDRAW_PAGE_OVERLAY = 2
GZ_ENUM_END(DDrawPageKind)

// @identity-TODO: original class spelling is unavailable; runtime class is inherited.
class CDisplayBuffers : public CWapObj {
public:
    CDisplayBuffers(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {
        m_frontSurface = NULL;
        m_backBuffer = NULL;
        m_overlayBuffer = NULL;
    }
    virtual ~CDisplayBuffers() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    RVA(0x001574a0, 0x6)
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_DISPLAY_BUFFERS;
    }
    virtual i32 CreateChildren(i32 w, i32 h, ColorDepth bpp, i32 flags);

    CDDrawFrontSurface* GetFrontSurface() {
        return m_frontSurface;
    }

    i32 ResolvePageImage(char* name, DDrawPageKind pageIndex);
    i32 LoadPageImage(struct CRezItm* src, DDrawPageKind pageIndex);
    void BltDirtyChildrenEx();
    void FlipAndNotify();
    i32 RestoreLostSurfaces();
    i32 ResizePages(i32 w, i32 h, ColorDepth bpp);
    i32 CreateOverlay(i32 copyFromBack, i32 createFlag);
    void UnloadOverlay();
    void ClearAllPages(u32 color);
    i32 CopyFrontToSurface(CRenderBuffer* dst);
    i32 HasOverlay();
    i32 CopyFrontToBackBuffers();
    i32 CopyFrontToOverlay();
    i32 CopyBackToOverlay();
    i32 CopyOverlayToBack();

    CRenderBuffer* GetBackBuffer() const {
        return m_backBuffer;
    }

    CDDrawFrontSurface* m_frontSurface;
    CRenderBuffer* m_backBuffer;
    CRenderBuffer* m_overlayBuffer;
};

// @identity-TODO: original class spelling is unavailable; runtime class is inherited.
class CRenderSurface : public CWapObj {
public:
    CRenderSurface(CDDrawSurfaceMgr* owner, i32 id, i32 flags);

protected:
    enum InlineCtorTag {
        INLINE_CTOR
    };
    CRenderSurface(InlineCtorTag, CDDrawSurfaceMgr* owner, i32 id, i32 flags)
        : CWapObj(owner, id, flags) {
        m_width = 0;
    }

public:
    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetGeometry(i32 w, i32 h, ColorDepth bpp);

    virtual i32 SetGeom(i32 w, i32 h, ColorDepth bpp);

    i32 GetWidth() const {
        return m_width;
    }
    i32 GetHeight() const {
        return m_height;
    }
    CDDSurface* const& GetSurface() const {
        return m_surface;
    }

    i32 RestoreIfLost();
    void BlitDirtyRect(CRenderBuffer* other, const POINT& pos, const SIZE& size);

    virtual ~CRenderSurface() OVERRIDE {
        m_width = 0;
    }

    i32 m_width;
    i32 m_height;
    ColorDepth m_bpp;
    RECT m_srcRect;
    CDDSurface* m_surface;
};

RVA(0x00158fd0, 0x41)
inline i32 CRenderSurface::SetGeometry(i32 w, i32 h, ColorDepth bpp) {
    if (w <= 0 || h <= 0) {
        return 0;
    }
    m_width = w;
    m_height = h;
    m_bpp = bpp;
    m_srcRect.left = 0;
    m_srcRect.top = 0;
    m_srcRect.right = w;
    m_srcRect.bottom = h;
    return 1;
}

RVA(0x00159020, 0x55)
inline i32 CRenderSurface::SetGeom(i32 w, i32 h, ColorDepth bpp) {
    if (w <= 0 || h <= 0) {
        return 0;
    }
    if (bpp != BPP_PALETTED_8 && bpp != BPP_RGB_16 && bpp != BPP_RGB_24 && bpp != BPP_RGB_32) {
        return 0;
    }
    m_width = w;
    m_height = h;
    m_bpp = bpp;
    m_srcRect.left = 0;
    m_srcRect.top = 0;
    m_srcRect.right = w;
    m_srcRect.bottom = h;
    return 1;
}

class CDDrawFrontSurface : public CRenderSurface {
public:
    CDDrawFrontSurface(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
        : CRenderSurface(owner, id, flags) {
        m_surface = NULL;
    }

    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetGeometry(i32 w, i32 h, ColorDepth bpp) OVERRIDE;
    virtual i32 SetGeom(i32 w, i32 h, ColorDepth bpp) OVERRIDE;
};

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
