#ifndef SRC_IMAGE_CIMAGE_H
#define SRC_IMAGE_CIMAGE_H

#include <rva.h>

#include <DDrawMgr/DDSurface.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

struct CParseSource;

class CDDrawDeviceManager;

class CString;
class CResolveNode;
class CDDrawSurfacePair;

class CDDrawSurfaceMgr;

class CDDSurface;

class CDDrawShadeBlit;

typedef struct tagRECT BlitRect;

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

    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual LoadableClassId GetClassId() OVERRIDE;

    virtual i32 CreateBlankSurface(i32 width, i32 height, i32 keyed);
    virtual i32 LoadDispatch(PidHeader* desc, FileImageFormat mode, u32 size, i32 keyed);
    virtual i32 Resolve(CParseSource* src, i32 keyed);
    virtual i32 Create(char* path, i32 keyed);
    virtual i32 Reload(CParseSource* src, i32 keyed);
    virtual void RenderImage(CResolveNode* info, CDDrawSurfacePair* dst);
    virtual void FlipVertical(void* unused);
    virtual void FlipHorizontal(void* unused);
    virtual void FlipBoth(void* unused);

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

// 0xd5e80 (RVA_COMPGEN pin at the emitting keeper, Play.cpp - an RVA() here
// would annotate BOTH cl dtor variants and collide with ??_GCImage@0xd5e50).
inline CImage::~CImage() {
    Unload();
}

RVA(0x000d5dc0, 0xb)
inline i32 CImage::IsLoaded() {
    return m_width > 0;
}

RVA(0x000d5de0, 0x6)
inline LoadableClassId CImage::GetClassId() {
    return CLASSID_IMAGE;
}

RVA(0x000d5e00, 0x3)
inline void CImage::FlipHorizontal(void*) {}

RVA(0x000d5e20, 0x1b)
inline void CImage::FlipBoth(void* unused) {
    FlipVertical(unused);
    FlipHorizontal(unused);
}

struct _DDBLTFX;
extern _DDBLTFX g_bltFx;
#endif // SRC_IMAGE_CIMAGE_H
