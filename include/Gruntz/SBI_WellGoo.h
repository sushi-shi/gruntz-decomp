#ifndef SBI_WELLGOO_H
#define SBI_WELLGOO_H

#include <rva.h>

#include <DDrawMgr/DDrawDeviceManager.h>
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
        m_fillSurface = NULL;
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
        i32 fillPercent
    ) OVERRIDE;
    RVA(0x00104c80, 0x1f)
    virtual void Reset() OVERRIDE {
        if (m_fillSurface != NULL) {
            m_host->GetDeviceManager()->RemoveSurface(m_fillSurface);
            m_fillSurface = NULL;
        }
    }
    virtual i32 Refresh(i32 deltaMs) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    CDDSurface* m_fillSurface;
    CDDrawShadeBlit* m_fillBlitter;
    CImage* m_topImage;
    CImage* m_bottomImage;
    i32 m_fillPercent;
    i32 m_centerX;

    RECT m_fillSourceRect;
    RECT m_fillDestRect;
};

#endif // SBI_WELLGOO_H
