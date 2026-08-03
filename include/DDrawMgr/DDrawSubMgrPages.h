#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H

#include <rva.h>

#include <Enums.h>
#include <Gruntz/Loadable.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

class CDDrawSurfaceMgr;
class CDDSurface;
class CDDrawSurfacePair;
class CDDrawSurfaceChildA;

class CDDrawSubMgrPages : public CLoadable {
public:
    CDDrawSubMgrPages(CDDrawSurfaceMgr* owner) : CLoadable(owner, 0, 0) {
        m_frontPair = 0;
        m_backPair = 0;
        m_overlayPair = 0;
    }
    virtual ~CDDrawSubMgrPages() OVERRIDE;

    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    RVA(0x001574a0, 0x6)
    virtual LoadableClassId GetClassId() OVERRIDE {
        return CLASSID_SUBMGRPAGES;
    }
    virtual i32 CreateChildren(i32 w, i32 h, i32 bpp, i32 flags);

    i32 ResolvePageImage(char* name, i32 pageIndex);
    i32 LoadPageImage(struct CParseSource* src, i32 pageIndex);
    void FlipAndNotify();
    i32 PagesReady();
    i32 ResizePages(i32 w, i32 h, i32 bpp);
    i32 CreateOverlay(i32 copyFromBack, i32 createFlag);
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
SIZE(0x1c);

class CDrawSubWorker : public CLoadable {
public:
    CDrawSubWorker(CDDrawSurfaceMgr* owner, i32 id, i32 flags);

protected:
    enum InlineCtorTag {
        INLINE_CTOR
    };
    CDrawSubWorker(InlineCtorTag, CDDrawSurfaceMgr* owner, i32 id, i32 flags)
        : CLoadable(owner, id, flags) {
        m_width = 0;
    }

public:
    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetGeometry(i32 w, i32 h, i32 bpp);

    virtual i32 SetGeom(i32 w, i32 h, i32 bpp);

    i32 Probe();

    virtual ~CDrawSubWorker() OVERRIDE {
        m_width = 0;
    }

    i32 m_width;
    i32 m_height;
    i32 m_bpp;
    i32 m_srcRect[4];
    CDDSurface* m_surface;
};
SIZE(0x30);

class CDDrawSurfaceChildA : public CDrawSubWorker {
public:
    CDDrawSurfaceChildA(CDDrawSurfaceMgr* owner, i32 id, i32 flags)
        : CDrawSubWorker(owner, id, flags) {
        m_surface = 0;
    }

    virtual ~CDDrawSurfaceChildA() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;
    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetGeometry(i32 w, i32 h, i32 bpp) OVERRIDE;
    virtual i32 SetGeom(i32 w, i32 h, i32 bpp) OVERRIDE;
};
SIZE(0x30);

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRPAGES_H
