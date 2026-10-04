#ifndef GRUNTZ_SBI_SIDETAB_H
#define GRUNTZ_SBI_SIDETAB_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusSampleMode.h>
#include <Image/CImage.h>
#include <Ints.h>

#include <stddef.h>

class CStatusBarMgr;

class CSBI_SideTab : public CStatusBarItem {
public:
    CSBI_SideTab() {
        m_backgroundImage = NULL;
        m_iconImage = NULL;
        m_iconIndex = -1;
        m_sampleMode = STATUS_SAMPLE_UNINITIALIZED;
    }
    virtual ~CSBI_SideTab() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 deltaMs) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    i32 Initialize(
        CStatusBarMgr* parent,
        CGameWorld* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rc,
        const char* unused,

        i32 playerIndex,
        i32 unitIndex,
        StatusSampleMode sampleMode,
        i32 onLeft
    );

    i32 UpdateSampleIcon();

    CImage* m_backgroundImage;
    CImage* m_iconImage;
    i32 m_iconIndex;
    i32 m_playerIndex;
    i32 m_unitIndex;
    StatusSampleMode m_sampleMode;
    Coord m_drawPosition;
    i32 m_iconOffsetX;
    i32 m_onLeft;
    i32 m_hasSample;
};

#endif // GRUNTZ_SBI_SIDETAB_H
