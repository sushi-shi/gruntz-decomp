#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
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
void CSBI_StatzTabArrow::SetDirection(StatusBarDock position, i32 animate) {
    if (position == STATUSBAR_DOCK_RIGHT) {
        if (animate == 0) {
            SetRange(4, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, 1, 0, -1);
        }
    } else {
        if (animate == 0) {
            SetRange(1, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, -1, 0, -1);
        }
    }
}

RVA(0x000ea170, 0x5c)
void CSBI_StatzTabArrow::SetDirectionAlt(StatusBarDock position, i32 animate) {
    if (position == STATUSBAR_DOCK_RIGHT) {
        if (animate == 0) {
            SetRange(1, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, -1, 0, -1);
        }
    } else {
        if (animate == 0) {
            SetRange(4, -1, 0, 0, -1);
        } else {
            SetRange(-1, -1, 1, 0, -1);
        }
    }
}

// @early-stop
RVA(0x000ea1f0, 0x1fa)
i32 CSBI_StatzTabGruntBar::BuildMultiplayerTabStatusBar(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT g,
    const char* key,
    i32 unitRow,
    i32 unitCol,
    i32 selMode
) {
    CDDrawSurfaceMgr* h;
    CObject* found;
    CDDrawWorker* head;

    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    h = host;
    m_owner = owner;
    m_tab = tab;
    m_host = h;
    m_redrawFrames = 0;
    m_enabled = 1;

    m_rect14 = g;

    found = NULL;
    m_cmd = cmd;
    h->m_imageRegistry->m_workersByName.Lookup(key, found);
    head = static_cast<CDDrawWorker*>(found);
    m_glyphMap = head;
    if (head == NULL) {
        return 0;
    }
    CImage* v;
    v = head->GetAt(0x21);
    m_statusGlyph = v;
    if (v == NULL) {
        return 0;
    }
    CImage* w;
    w = head->GetAt(0x22);
    m_abilityGlyph = w;
    if (w == NULL) {
        return 0;
    }

    CImage* val;
    if (selMode != 0) {
        found = NULL;
        m_host->m_imageRegistry->m_workersByName.Lookup(
            "GAME_STATUSBAR_TABZ_STATZTAB_SELECTEDBAR",
            found
        );
        CDDrawWorker* sel = static_cast<CDDrawWorker*>(found);
        m_timerGlyphMap = sel;
        if (sel == NULL) {
            return 0;
        }
        CImage* x = m_glyphMap->GetAt(0x23);
        m_selectKey = x;
        if (x == NULL) {
            return 0;
        }
        val = m_glyphMap->GetAt(0x22);
    } else {
        found = NULL;
        m_host->m_imageRegistry->m_workersByName.Lookup(
            "GAME_STATUSBAR_TABZ_MULTIPLAYERTAB_SELECTEDBAR",
            found
        );
        CDDrawWorker* sel = static_cast<CDDrawWorker*>(found);
        m_timerGlyphMap = sel;
        if (sel == NULL) {
            return 0;
        }
        val = m_glyphMap->GetAt(0x23);
    }
    m_overrideGlyph = val;
    if (val == NULL) {
        goto fail;
    }
    m_unitRow = unitRow;
    m_unitCol = unitCol;
    m_timerValue = -1;
    m_overrideValue = -1;
    m_abilityValue = -1;
    m_statusValue = -1;
    m_selectValue = 0;
    m_timerAnchorLo = 0;
    m_timerWindowLo = 0;
    m_timerAnchorHi = 0;
    m_timerWindowHi = 0;
    Update();
    return 1;
fail:
    return 0;
}
