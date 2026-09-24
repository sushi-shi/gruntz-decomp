#include <rva.h>

#include <Gruntz/AniPlayer.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialRecords.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000e5ad0, 0x84)
i32 CAniPlayer::Start(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 frameStart,
    i32 frameEnd,
    i32 intervalMs,
    i32 loop,
    i32 step
) {
    if (CSBI_ImageSetAni::
            Init(owner, host, cmd, tab, rc, key, frameStart, frameEnd, intervalMs, loop, step)
        == SBICMD_NONE) {
        return 0;
    }
    m_window64 = static_cast<u32>(m_interval);
    m_start64 = g_frameTime;
    return 1;
}

// @early-stop
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000e5b90, 0x51)
i32 CAniPlayer::TickToggle(i32 unused) {
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000e5c10, 0x54)
i32 CAniPlayer::RenderCel() {
    CDDrawWorker* tbl = m_frameSet;
    CImage* cel = tbl->GetAt(m_frameIndex);
    SetFrame(cel);
    if (cel != NULL) {
        CDDrawSurfacePair* surfaceCtx = g_gameReg->m_world->m_drawTarget->m_backPair;
        cel->RenderFrame(
            surfaceCtx,
            cel->m_anchor.x + m_rect.left,
            cel->m_anchor.y + m_rect.top,
            0
        );
    }
    return 1;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000e5c90, 0x87)
i32 CAniPlayer::Serialize(CFileMemBase* arc, SerialMode mode, LogicTypeId typeId, i32 payload) {
    if (arc == NULL) {
        return 0;
    }

    if (CSBI_ImageSetAni::SerializeFields(static_cast<CFileMemBase*>(arc), mode, typeId, payload)
        == 0) {
        return 0;
    }
    SerBandPair(arc, mode, &m_start64);
    return 1;
}
