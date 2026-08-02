

#include <Gruntz/AniPlayer.h>
#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Image/CImage.h>
#include <Mfc.h>
#include <rva.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/SbiConfig.h>

RVA(0x000e5ad0, 0x84)
i32 CAniPlayer::Start(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT rc,
    const char* key,
    i32 b0,
    i32 b1,
    i32 b2,
    i32 b3,
    i32 b4
) {
    if (Init(owner, host, cmd, tab, rc, key, b0, b1, b2, b3, b4) == 0) {
        return 0;
    }
    m_windowLo = m_interval;
    m_windowHi = 0;
    m_startLo = g_frameTime;
    m_startHi = 0;
    return 1;
}

// @early-stop
RVA(0x000e5b90, 0x51)
i32 CAniPlayer::TickToggle(i32 param) {
    if (static_cast<__int64>(g_frameTime) - m_start64 >= m_window64) {
        m_frameIndex = (m_frameIndex == m_frameStart) ? m_frameEnd : m_frameStart;
        m_windowLo = m_interval;
        m_windowHi = 0;
        m_startLo = g_frameTime;
        m_startHi = 0;
    }
    return 1;
}

// @early-stop
RVA(0x000e5c10, 0x54)
i32 CAniPlayer::RenderCel() {
    CDDrawWorker* tbl = m_frameSet;
    CImage* cel;
    if (m_frameIndex >= tbl->m_minIndex && m_frameIndex <= tbl->m_maxIndex) {
        cel = static_cast<CImage*>(tbl->m_items.GetAt(m_frameIndex));
    } else {
        cel = 0;
    }
    m_frame = cel;
    if (cel != 0) {
        CDDrawSurfacePair* surfaceCtx = g_gameReg->m_world->m_drawTarget->m_backPair;
        cel->RenderFrame(
            surfaceCtx,
            cel->m_anchorX + m_rect14.left,
            cel->m_anchorY + m_rect14.top,
            0
        );
    }
    return 1;
}

// @early-stop
RVA(0x000e5c90, 0x87)
i32 CAniPlayer::Serialize(CFileMemBase* arc, i32 mode, i32 typeId, i32 pObj) {
    if (arc == 0) {
        return 0;
    }

    if (CSBI_ImageSetAni::SerializeFields(static_cast<CFileMemBase*>(arc), mode, typeId, pObj)
        == 0) {
        return 0;
    }
    switch (mode) {
        case 7:
            arc->Read(&m_start64, 8);
            arc->Read(&m_window64, 8);
            break;
        case 4:
            arc->Write(&m_start64, 8);
            arc->Write(&m_window64, 8);
            break;
    }
    return 1;
}
