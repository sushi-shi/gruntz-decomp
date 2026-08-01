#ifndef SBI_WELLGOO_H
#define SBI_WELLGOO_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SBI_Image.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Image/ImageSet.h>
#include <Gruntz/SpriteRefTable.h>

class CFileMemBase;

class CImage;
class CDDSurface;
class CDDrawShadeBlit;

class CSBI_WellGoo : public CSBI_Image {
public:
    CSBI_WellGoo() {
        m_kind = 7;
        m_frame = 0;
    }

    virtual ~CSBI_WellGoo() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* arc, i32 mode, i32 typeId, i32 pObj) OVERRIDE;
    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        i32 cmd,
        i32 tab,
        RECT rc,
        const char* key,
        i32 fillScale
    ) OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 a) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    CDDSurface* m_gooSrc;
    CDDrawShadeBlit* m_blitter;
    CImage* m_fgFrame;
    CImage* m_baseFrame;
    i32 m_fillScale;
    i32 m_drawX;

    RECT m_srcRect;
    RECT m_dstRect;
};
SIZE_UNKNOWN();

#endif // SBI_WELLGOO_H
