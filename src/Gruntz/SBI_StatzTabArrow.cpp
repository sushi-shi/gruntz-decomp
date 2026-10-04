#include <StdAfx.h>

#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_SideTab.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

RVA(0x000ea0f0, 0x5c)
void CSBI_StatzTabArrow::SetUnsampledDirection(StatusBarDock position, b32 animate) {
    if (position == STATUSBAR_DOCK_RIGHT) {
        if (animate == false) {
            SetRange(4, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, 1, 0, -1);
        }
    } else {
        if (animate == false) {
            SetRange(1, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, -1, 0, -1);
        }
    }
}

RVA(0x000ea170, 0x5c)
void CSBI_StatzTabArrow::SetSampledDirection(StatusBarDock position, b32 animate) {
    if (position == STATUSBAR_DOCK_RIGHT) {
        if (animate == false) {
            SetRange(1, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, -1, 0, -1);
        }
    } else {
        if (animate == false) {
            SetRange(4, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, 1, 0, -1);
        }
    }
}

// @early-stop
RVA(0x000ea1f0, 0x1fa)
i32 CSBI_StatzTabGruntBar::Initialize(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rect,
    const char* iconSetName,
    i32 playerIndex,
    i32 unitIndex,
    i32 showSelectionGroup
) {
    CDDrawSurfaceMgr* world;
    CDDrawWorker* iconFrames;

    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    world = host;
    CStatusBarItem::Initialize(owner, tab, world);

    m_rect = rect;

    m_cmd = cmd;
    iconFrames = world->FindWorker(iconSetName);
    m_iconFrames = iconFrames;
    if (iconFrames == NULL) {
        return 0;
    }
    CImage* healthBackground;
    healthBackground = iconFrames->GetAt(0x21);
    m_healthBackgroundImage = healthBackground;
    if (healthBackground == NULL) {
        return 0;
    }
    CImage* toolBackground;
    toolBackground = iconFrames->GetAt(0x22);
    m_toolBackgroundImage = toolBackground;
    if (toolBackground == NULL) {
        return 0;
    }

    CImage* toyBackground;
    if (showSelectionGroup != 0) {
        CDDrawWorker* selectionFrames =
            m_host->FindWorker("GAME_STATUSBAR_TABZ_STATZTAB_SELECTEDBAR");
        m_selectionFrames = selectionFrames;
        if (selectionFrames == NULL) {
            return 0;
        }
        CImage* groupBackground = m_iconFrames->GetAt(0x23);
        m_groupBackgroundImage = groupBackground;
        if (groupBackground == NULL) {
            return 0;
        }
        toyBackground = m_iconFrames->GetAt(0x22);
    } else {
        CDDrawWorker* selectionFrames =
            m_host->FindWorker("GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_SELECTEDBAR");
        m_selectionFrames = selectionFrames;
        if (selectionFrames == NULL) {
            return 0;
        }
        toyBackground = m_iconFrames->GetAt(0x23);
    }
    m_toyBackgroundImage = toyBackground;
    if (toyBackground == NULL) {
        goto fail;
    }
    m_playerIndex = playerIndex;
    m_unitIndex = unitIndex;
    m_selectionFrameIndex = -1;
    m_toyIconIndex = -1;
    m_toolIconIndex = -1;
    m_healthIconIndex = -1;
    m_groupMarker = 0;
    m_selectionAnimationClock.Clear();
    UpdateIcons();
    return 1;
fail:
    return 0;
}
