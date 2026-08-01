#include <Mfc.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Rez/RezAlloc.h>
#include <rva.h>
#include <Ints.h>
#include <Gruntz/StatusBarMgr.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Image/CImage.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/WarpStoneFly.h>

RVA(0x00109bb0, 0xb)
CWarpStoneFly::CWarpStoneFly() {
    m_sprite = 0;
    m_owner = 0;
}

// @early-stop
RVA(0x0010a0f0, 0x184)
i32 CWarpStoneFly::Tick(i32 dt) {
    if (static_cast<i32>(m_currentX) == m_targetX && static_cast<i32>(m_currentY) == m_targetY) {
        CByteArray* arr = &g_gameReg->m_cmdGrid->m_byteArr;
        arr->SetAtGrow(arr->GetSize(), static_cast<BYTE>(m_arrivalMode));
        m_owner->m_hlBusy = 0;
        if (m_owner->m_position != 2 && m_owner->m_activeTab == 5) {
            m_owner->ResetWidgets(0);
            m_owner->TryActivate();
        }
        if (m_owner->m_retabNotify != 0) {
            RezFree(m_owner->m_retabNotify);
            m_owner->m_retabNotify = 0;
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
