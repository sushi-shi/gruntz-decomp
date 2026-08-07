#ifndef SRC_IMAGE_CIMAGE_H
#define SRC_IMAGE_CIMAGE_H

#include <rva.h>

#include <DDrawMgr/DDSurface.h>
#include <Gruntz/Loadable.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

struct CParseSource;

class CDDrawPtrCollections;

class CString;
class CResolveNode;
class CDDrawSurfacePair;

class CDDrawSurfaceMgr;

class CDDSurface;

class CDDrawShadeBlit;

typedef struct tagRECT BlitRect;
SIZE(0x10);

struct PidHeader;

extern i32 g_resourceInstallActive;
extern i32 g_surfaceColorKey;

class CResolveNode;

class CImage : public CWapObj {
public:
    CImage(i32 index, CDDrawSurfaceMgr* parent) : CWapObj(index, parent) {
        m_width = 0;
        m_height = 0;
        m_surface = NULL;
        m_owned = NULL;
    }

    virtual ~CImage() OVERRIDE;

    virtual void FreeAll();
    virtual LoadableClassId GetClassId();

    virtual i32 CreateBlankSurface(i32 width, i32 height, i32 keyed);
    virtual i32 LoadDispatch(PidHeader* desc, FileImageFormat mode, u32 size, i32 keyed);
    virtual i32 Resolve(CParseSource* src, i32 arg);
    virtual i32 Create(char* path, i32 keyed);
    virtual i32 Reload(CParseSource* src, i32 arg);
    virtual void RenderImage(CResolveNode* info, CDDrawSurfacePair* dst);
    virtual void FlipVertical(void* a);
    virtual void FlipHorizontal(void* a);
    virtual void FlipBoth(void* a);

    i32 BuildShadeBlitter(PidHeader* desc, u32 size);
    i32 CopyFrom(CImage* other);
    i32 SetOrigin(PidHeader* desc, FileImageFormat mode);
    void RenderFrame(CDDrawSurfacePair* target, i32 x, i32 y, i32 flags);
    void RenderFrameClipped(CDDrawSurfacePair* target, i32 x, i32 y, RECT* clipRect, i32 flags);

    void BlitNorm(CResolveNode* info, CDDrawSurfacePair* dst);
    void BlitFlipV(CResolveNode* info, CDDrawSurfacePair* dst);
    void BlitFlipH(CResolveNode* info, CDDrawSurfacePair* dst);
    void BlitShadeFlipHV(CResolveNode* info, CDDrawSurfacePair* dst);
    void BlitShadeNorm(CResolveNode* info, CDDrawSurfacePair* dst);
    void BlitShadeFlipV(CResolveNode* info, CDDrawSurfacePair* dst);
    void BlitShadeFlipH(CResolveNode* info, CDDrawSurfacePair* dst);

    i32 m_width;
    i32 m_height;
    i32 m_anchorX;
    i32 m_anchorY;
    i32 m_originX;
    i32 m_originY;
    i32 m_loadResult;
    CDDSurface* m_surface;
    CDDrawShadeBlit* m_owned;
};
SIZE_UNKNOWN();

inline CImage::~CImage() {
    FreeAll();
    m_id = -1;
    m_flags = 0;
    m_ownerCtx = NULL;
}

struct _DDBLTFX;
extern _DDBLTFX g_bltFx;
extern i32 g_surfaceColorKey;
#endif // SRC_IMAGE_CIMAGE_H
