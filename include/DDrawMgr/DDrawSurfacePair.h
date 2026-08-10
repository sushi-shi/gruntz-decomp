#ifndef GRUNTZ_CDDRAWSURFACEPAIR_H
#define GRUNTZ_CDDRAWSURFACEPAIR_H

#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Enums.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDSurface;
class CDDrawSurfaceMgr;
struct CParseSource;

class CDDrawSurfacePair : public CDrawSubWorker {
public:
    virtual i32 IsLoaded() OVERRIDE;

    void DrawCount(RECT* rc, i32 n);
    void DrawLabel(RECT* rc, char* text);

public:
    CDDrawSurfacePair(CDDrawSurfaceMgr* mgr, i32 kind, i32 flags)
        : CDrawSubWorker(INLINE_CTOR, mgr, kind, flags) {
        m_surface = NULL;
        m_ownsSurface = 1;
    }

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetGeom(i32 w, i32 h, ColorDepth bpp) OVERRIDE;
    virtual i32 InitFromSurface(CDDSurface* src);
    virtual i32 Create(i32 w, i32 h, ColorDepth bpp, i32 flags);
    virtual i32 LoadImage(CParseSource* src);
    virtual i32 ResolveImageName(char* name);

    virtual ~CDDrawSurfacePair() OVERRIDE;

    void BltSelf(CDDrawSurfacePair* src);
    i32 RestoreIfLost();

    void DrawBox(RECT* rect, i32 color);
    void DrawCross(i32 x, i32 y);

    void BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size);

    i32 m_ownsSurface;
};
SIZE(0x34);

#endif // GRUNTZ_CDDRAWSURFACEPAIR_H
