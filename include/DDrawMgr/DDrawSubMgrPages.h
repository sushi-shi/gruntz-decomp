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
class CDDrawSurfacePair;
class CDDrawSurfaceChildA;

GZ_ENUM_BEGIN(DDrawPageKind)
    DDRAW_PAGE_BACK = 1,
    DDRAW_PAGE_OVERLAY = 2
GZ_ENUM_END(DDrawPageKind)

class CDDrawSubMgrPages : public CWapObj {
public:
    CDDrawSubMgrPages(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0) {
        m_frontPair = NULL;
        m_backPair = NULL;
        m_overlayPair = NULL;
    }
    virtual ~CDDrawSubMgrPages() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    RVA(0x001574a0, 0x6)
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_SUBMGRPAGES;
    }
    virtual i32 CreateChildren(i32 w, i32 h, ColorDepth bpp, i32 flags);

    i32 ResolvePageImage(char* name, DDrawPageKind pageIndex);
    i32 LoadPageImage(struct CParseSource* src, DDrawPageKind pageIndex);
    void BltDirtyChildrenEx();
    void FlipAndNotify();
    i32 PagesReady();
    i32 ResizePages(i32 w, i32 h, ColorDepth bpp);
    i32 CreateOverlay(i32 copyFromBack, i32 createFlag);
    void UnloadOverlay();
    void ClearAllPages(u32 color);
    i32 BlitPage(CDDrawSurfacePair* dst);
    i32 HasOverlay();
    i32 PresentBackPage();
    i32 TransEnter();
    i32 TransTitle();
    i32 TransExit();

    CDDrawSurfaceChildA* m_frontPair;
    CDDrawSurfacePair* m_backPair;
    CDDrawSurfacePair* m_overlayPair;
};

class CDrawSubWorker : public CWapObj {
public:
    CDrawSubWorker(CDDrawSurfaceMgr* owner, i32 id, i32 flags);

protected:
    enum InlineCtorTag {
        INLINE_CTOR
    };
    CDrawSubWorker(InlineCtorTag, CDDrawSurfaceMgr* owner, i32 id, i32 flags)
        : CWapObj(owner, id, flags) {
        m_width = 0;
    }

public:
    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetGeometry(i32 w, i32 h, ColorDepth bpp);

    virtual i32 SetGeom(i32 w, i32 h, ColorDepth bpp);

    i32 Probe();

    virtual ~CDrawSubWorker() OVERRIDE {
        m_width = 0;
    }

    i32 m_width;
    i32 m_height;
    ColorDepth m_bpp;
    RECT m_srcRect;
    CDDSurface* m_surface;
};

class CDDrawSurfaceChildA : public CDrawSubWorker {
public:
    CDDrawSurfaceChildA(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
        : CDrawSubWorker(owner, id, flags) {
        m_surface = NULL;
    }

    virtual ~CDDrawSurfaceChildA() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetGeometry(i32 w, i32 h, ColorDepth bpp) OVERRIDE;
    virtual i32 SetGeom(i32 w, i32 h, ColorDepth bpp) OVERRIDE;
};

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
