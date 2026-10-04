#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/SBI_ImageSetAni.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

// @early-stop
RVA(0x000e7980, 0x109)
i32 CSBI_ImageSetAni::Init(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 frameStart,
    i32 frameEnd,
    i32 frameDelayMs,
    i32 looping,
    i32 frameStep
) {
    CDDrawWorker* tbl;

    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    Initialize(owner, tab, host);

    m_rect = rc;
    m_cmd = cmd;
    if (key == NULL) {
        return 0;
    }
    tbl = host->FindWorker(key);
    m_frameSet = tbl;
    if (tbl == NULL) {
        goto fail;
    }
    m_frameDelayMs = frameDelayMs;
    m_frameStep = frameStep;
    m_looping = looping;

    if (frameStart == -1) {
        if (frameStep >= 0) {
            m_frameStart = tbl->GetMinIndex();
        } else {
            m_frameStart = tbl->GetMaxIndex();
        }
    } else {
        m_frameStart = frameStart;
    }
    if (frameEnd == -1) {
        if (frameStep >= 0) {
            m_frameEnd = tbl->GetMaxIndex();
        } else {
            m_frameEnd = tbl->GetMinIndex();
        }
    } else {
        m_frameEnd = frameEnd;
    }
    m_frameIndex = m_frameStart;

    CImage* cel;
    cel = tbl->GetAt(m_frameStart);
    SetFrame(cel);
    return cel != NULL;
fail:
    return 0;
}

RVA(0x000e7ae0, 0x8)
i32 CSBI_ImageSetAni::Refresh(i32) {
    return 1;
}

RVA(0x000e7b00, 0xe1)
i32 CSBI_ImageSetAni::Render() {
    if (m_redrawFrames > 0) {
        CImage* cel = m_frameSet->GetAt(m_frameIndex);
        SetFrame(cel);
        if (cel != NULL) {
            CDDrawSurfacePair* surfaceCtx = g_gameReg->World()->m_drawTarget->m_backPair;
            cel->RenderFrame(
                surfaceCtx,
                cel->GetAnchorX() + m_rect.left,
                cel->GetAnchorY() + m_rect.top,
                0
            );
        }
        u32 now = timeGetTime();
        if (now - static_cast<u32>(m_lastFrameTimeMs) > static_cast<u32>(m_frameDelayMs)) {
            m_frameIndex += m_frameStep;
            m_lastFrameTimeMs = timeGetTime();
        }
        if (m_frameStep > 0) {
            if (m_frameIndex > m_frameEnd) {
                if (m_looping != 0) {
                    m_frameIndex = m_frameStart;
                    return 1;
                }
                m_redrawFrames--;
                m_frameIndex = m_frameEnd;
                return 1;
            }
        } else if (m_frameStep < 0) {
            if (m_frameIndex < m_frameEnd) {
                if (m_looping != 0) {
                    m_frameIndex = m_frameStart;
                    return 1;
                }
                m_redrawFrames--;
                m_frameIndex = m_frameEnd;
                return 1;
            }
        } else {
            m_redrawFrames--;
        }
    }
    return 1;
}

RVA(0x000e7c30, 0x7d)
void CSBI_ImageSetAni::SetRange(i32 start, i32 end, i32 frameStep, i32 looping, i32 frameDelayMs) {

    if (start == -1) {
        if (frameStep >= 0) {
            m_frameStart = m_frameSet->GetMinIndex();
        } else {
            m_frameStart = m_frameSet->GetMaxIndex();
        }
    } else {
        m_frameStart = start;
    }
    if (end == -1) {
        if (frameStep >= 0) {
            m_frameEnd = m_frameSet->GetMaxIndex();
        } else {
            m_frameEnd = m_frameSet->GetMinIndex();
        }
    } else {
        m_frameEnd = end;
    }
    if (frameDelayMs != -1) {
        m_frameDelayMs = frameDelayMs;
    }
    m_frameStep = frameStep;
    m_looping = looping;
    m_frameIndex = m_frameStart;
    m_redrawFrames = 2;
    m_lastFrameTimeMs = timeGetTime();
}

RVA(0x000e7cd0, 0xf8)
i32 CSBI_ImageSetAni::SerializeFields(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->World() == NULL) {
        return 0;
    }
    switch (mode) {

        case SERIAL_LOAD:
            s->Read(&m_frameDelayMs, sizeof(m_frameDelayMs));
            s->Read(&m_lastFrameTimeMs, sizeof(m_lastFrameTimeMs));
            s->Read(&m_looping, sizeof(m_looping));
            s->Read(&m_frameStep, sizeof(m_frameStep));
            s->Read(&m_frameEnd, sizeof(m_frameEnd));
            s->Read(&m_frameStart, sizeof(m_frameStart));
            break;
        case SERIAL_SAVE:
            s->Write(&m_frameDelayMs, sizeof(m_frameDelayMs));
            s->Write(&m_lastFrameTimeMs, sizeof(m_lastFrameTimeMs));
            s->Write(&m_looping, sizeof(m_looping));
            s->Write(&m_frameStep, sizeof(m_frameStep));
            s->Write(&m_frameEnd, sizeof(m_frameEnd));
            s->Write(&m_frameStart, sizeof(m_frameStart));
            break;
    }
    return CSBI_ImageSet::SerializeFields(s, mode, typeId, payload) != 0;
}
