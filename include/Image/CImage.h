#ifndef SRC_IMAGE_CIMAGE_H
#define SRC_IMAGE_CIMAGE_H

#include <rva.h>

#include <DDrawMgr/DDSurface.h>
#include <Ints.h>
#include <Wap32/WapObj.h>

#include <stddef.h>

struct CRezItm;

class CDDrawDeviceManager;

class CString;
class CRenderState;
class CRenderBuffer;

class CGameWorld;

class CDDSurface;

class CDDrawShadeBlit;

typedef struct tagRECT BlitRect;

struct PidHeader;

extern b32 g_resourceInstallActive;
extern i32 g_surfaceColorKey;

class CRenderState;

class CImage : public CWapObj {
public:
    CImage(i32 index, CGameWorld* parent) : CWapObj(index, parent) {
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
    virtual i32 Resolve(CRezItm* src, i32 keyed);
    virtual i32 Create(char* path, i32 keyed);
    virtual i32 Reload(CRezItm* src, i32 keyed);
    virtual void RenderImage(CRenderState* info, CRenderBuffer* dst);
    virtual void FlipVertical(void* unused);
    virtual void FlipHorizontal(void* unused);
    virtual void FlipBoth(void* unused);

    i32 BuildShadeBlitter(PidHeader* desc, u32 size);
    i32 CopyFrom(CImage* other);
    i32 SetOrigin(PidHeader* desc, FileImageFormat mode);
    void SetBltFastFlags(CDDSurface* surface);
    void RenderFrame(CRenderBuffer* target, i32 x, i32 y, i32 flags);
    void RenderFrameClipped(CRenderBuffer* target, i32 x, i32 y, RECT* clipRect, i32 flags);

    void BlitNorm(CRenderState* info, CRenderBuffer* dst);
    void BlitFlipV(CRenderState* info, CRenderBuffer* dst);
    void BlitFlipH(CRenderState* info, CRenderBuffer* dst);
    void BlitShadeFlipHV(CRenderState* info, CRenderBuffer* dst);
    void BlitShadeNorm(CRenderState* info, CRenderBuffer* dst);
    void BlitShadeFlipV(CRenderState* info, CRenderBuffer* dst);
    void BlitShadeFlipH(CRenderState* info, CRenderBuffer* dst);

    const i32& GetWidth() const {
        return m_width;
    }

    const i32& GetHeight() const {
        return m_height;
    }

    CDDrawShadeBlit* GetShadeBlitter() const {
        return m_owned;
    }

    const i32& GetAnchorX() const {
        return m_anchorX;
    }

    const i32& GetAnchorY() const {
        return m_anchorY;
    }

    i32 m_width;
    i32 m_height;
    i32 m_anchorX;
    i32 m_anchorY;
    i32 m_originX;
    i32 m_originY;
    i32 m_bltFastFlags;
    CDDSurface* m_surface;
    CDDrawShadeBlit* m_owned;
};

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
