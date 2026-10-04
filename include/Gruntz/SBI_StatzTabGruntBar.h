#ifndef SBI_STATZTABGRUNTBAR_H
#define SBI_STATZTABGRUNTBAR_H

#include <rva.h>

#include <Gruntz/ClockInterval.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/StatusBarItem.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <MakeRect.h>

#include <stddef.h>

class CStatusBarMgr;
class CDDrawSurfaceMgr;

class CDDrawWorker;

class CSBI_StatzTabGruntBar : public CStatusBarItem {
public:
    CSBI_StatzTabGruntBar() {
        m_kind = SBI_KIND_STATZ_TAB_GRUNT_BAR;
        m_healthIconImage = NULL;
        m_toolIconImage = NULL;
        m_toyIconImage = NULL;
        m_groupIconImage = NULL;
        m_iconFrames = NULL;
        m_healthBackgroundImage = NULL;
        m_toolBackgroundImage = NULL;
        m_toyBackgroundImage = NULL;
        m_groupBackgroundImage = NULL;
        m_toyIconIndex = -1;
        m_toolIconIndex = -1;
        m_healthIconIndex = -1;
        m_groupMarker = 0;
        m_selectionFrames = NULL;
        m_selectionFrameIndex = -1;
        m_selectionImage = NULL;
    }
    virtual ~CSBI_StatzTabGruntBar() OVERRIDE;

    virtual i32 SerializeFields(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)
        OVERRIDE;
    virtual void Reset() OVERRIDE;
    virtual i32 Refresh(i32 deltaMs) OVERRIDE;
    virtual i32 Render() OVERRIDE;

    i32 Initialize(
        CStatusBarMgr* owner,
        CDDrawSurfaceMgr* host,
        SbiCommandId cmd,
        StatusBarTab tab,
        RECT rect,
        const char* iconSetName,
        i32 playerIndex,
        i32 unitIndex,
        i32 showSelectionGroup
    );

    i32 UpdateIcons();

    CImage* m_healthBackgroundImage;
    CImage* m_healthIconImage;
    i32 m_healthIconIndex;
    CImage* m_toolBackgroundImage;
    CImage* m_toolIconImage;
    i32 m_toolIconIndex;
    CImage* m_toyBackgroundImage;
    CImage* m_toyIconImage;
    i32 m_toyIconIndex;
    CImage* m_groupBackgroundImage;
    CImage* m_groupIconImage;
    i32 m_groupMarker;
    i32 m_playerIndex;
    i32 m_unitIndex;
    CDDrawWorker* m_selectionFrames;
    CImage* m_selectionImage;
    i32 m_selectionFrameIndex;
    CDDrawWorker* m_iconFrames;

    ClockInterval m_selectionAnimationClock;
};

#endif // SBI_STATZTABGRUNTBAR_H
