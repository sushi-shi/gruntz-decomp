#ifndef GRUNTZ_CDDRAWSURFACEPAIR_H
#define GRUNTZ_CDDRAWSURFACEPAIR_H

#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDSurface.h>
#include <Enums.h>
#include <Ints.h>
#include <MakeRect.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

class CDDSurface;
class CDDrawSurfaceMgr;
struct CRezArchiveEntry;

GZ_ENUM_FLAGS_BEGIN(DDrawSurfacePairFlags, i32)
    SURFACEPAIR_SYSTEM_MEMORY = 0x10000,
    SURFACEPAIR_SKIP_OVERLAY_WORKER_RENDER = 0x20000
GZ_ENUM_FLAGS_END(DDrawSurfacePairFlags, i32)
GZ_ENUM_FLAGS_OPS(DDrawSurfacePairFlags)

class CDDrawSurfacePair : public CDrawSubWorker {
public:
    virtual i32 IsLoaded() OVERRIDE;

    void DrawCount(RECT* rc, i32 n);
    void DrawLabel(RECT* rc, char* text);

public:
    CDDrawSurfacePair(CDDrawSurfaceMgr* mgr, i32 kind, i32 flags)
        : CDrawSubWorker(INLINE_CTOR, mgr, kind, flags) {
        m_surface = NULL;
        m_ownsSurface = true;
    }

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 SetGeom(i32 w, i32 h, ColorDepth bpp) OVERRIDE;
    virtual i32 InitFromSurface(CDDSurface* src);
    virtual i32 Create(i32 w, i32 h, ColorDepth bpp, i32 flags);
    virtual i32 LoadImage(CRezArchiveEntry* src);
    virtual i32 ResolveImageName(char* name);

    virtual ~CDDrawSurfacePair() OVERRIDE;

    void BltSelf(CDDrawSurfacePair* src);
    i32 RestoreIfLost();

    void DrawBox(RECT* rect, i32 color);
    void DrawCross(i32 x, i32 y);

    void BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size);

    b32 m_ownsSurface;
};

inline void CDrawSubWorker::BlitDirtyRect(CDDrawSurfacePair* other, i32* pos, i32* size) {
    RECT rc;
    rc = MakeRect(pos[0], pos[1], pos[0] + size[0], pos[1] + size[1]);
    m_surface->BltEx(&rc, other->m_surface, &rc, DDBLT_WAIT, NULL);
}

#define BLT_SURFACE_PAIR_SELF(dst, src)                                                            \
    (dst)->m_surface->BltFast(0, 0, (src)->m_surface, &(src)->m_srcRect, DDBLTFAST_WAIT)

#endif // GRUNTZ_CDDRAWSURFACEPAIR_H
