#ifndef SBI_WELLGOO_H
#define SBI_WELLGOO_H

#include <rva.h>

#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_Image.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteRefTable.h>
#include <Image/ImageSet.h>
#include <Ints.h>

#include <stddef.h>

class CFileMemBase;

class CImage;
class CDDSurface;
class CDDrawShadeBlit;

class CSBI_WellGoo : public CSBI_Image {
public:
    CSBI_WellGoo() {
        m_kind = SBI_KIND_WELL_GOO;
        m_gooSrc = NULL;
    }

    virtual ~CSBI_WellGoo() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual i32 Setup(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* key,
        i32 fillScale
    ) OVERRIDE;
    RVA(0x00104c80, 0x1f)
    virtual void Reset() OVERRIDE {
        if (m_gooSrc != NULL) {
            m_host->m_ptrColl->RemoveItemA(m_gooSrc);
            m_gooSrc = NULL;
        }
    }
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

#endif // SBI_WELLGOO_H
