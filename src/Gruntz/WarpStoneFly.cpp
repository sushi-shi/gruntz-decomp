#include <rva.h>

#include <Gruntz/WarpStoneFly.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/StatusBarDock.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/StatusBarTab.h>
#include <Gruntz/TriggerMgr.h>
#include <Image/CImage.h>
#include <Ints.h>

RVA(0x00109bb0, 0xb)
CWarpStoneFly::CWarpStoneFly() {
    m_sprite = NULL;
    m_owner = NULL;
}

// @early-stop
RVA(0x0010a0f0, 0x184)
i32 CWarpStoneFly::Tick(u32 dt) {
    i32 cellY = static_cast<i32>(m_currentY);
    i32 cellX = static_cast<i32>(m_currentX);
    if (cellX == m_targetX && cellY == m_targetY) {
        i32 mode = m_arrivalMode;
        CByteArray* arr = &g_gameReg->m_cmdGrid->m_byteArr;
        arr->SetAtGrow(arr->GetSize(), static_cast<BYTE>(mode));
        m_owner->m_hlBusy = 0;
        if (m_owner->m_position != STATUSBAR_HIDDEN && m_owner->m_activeTab == TAB_GAME) {
            m_owner->ResetWidgets(0);
            m_owner->TryActivate();
        }
        CStatusBarMgr* owner = m_owner;
        if (owner->m_retabNotify != NULL) {
            ::operator delete(owner->m_retabNotify);
            owner->m_retabNotify = NULL;
        }
        return 1;
    }

    double t = static_cast<double>(dt);
    double newX = m_currentX + (t * m_velocityScale) * m_xDirection;
    double newY = m_currentY + (t * m_yDirection) * m_velocityScale;
    m_currentX = newX;
    m_currentY = newY;

    if (m_xDirection > 0.0) {
        if (static_cast<i32>(newX) > m_targetX) {
            m_currentX = static_cast<double>(m_targetX);
        }
    } else if (m_xDirection < 0.0) {
        if (static_cast<i32>(newX) < m_targetX) {
            m_currentX = static_cast<double>(m_targetX);
        }
    }

    if (m_yDirection > 0.0) {
        if (static_cast<i32>(newY) > m_targetY) {
            goto clampY;
        }
    } else if (m_yDirection < 0.0) {
        if (static_cast<i32>(newY) < m_targetY) {
            goto clampY;
        }
    }
    return 1;
clampY:
    m_currentY = static_cast<double>(m_targetY);
    return 1;
}

RVA(0x0010a2f0, 0x35)
i32 CWarpStoneFly::Draw() {
    m_sprite->RenderFrame(
        g_gameReg->m_world->m_drawTarget->m_backPair,
        static_cast<i32>(m_currentX),
        static_cast<i32>(m_currentY),
        0
    );
    return 1;
}
