#include <rva.h>

#include <Gruntz/SBI_ImageSetAni.h>

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
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarItemInline.h>
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
    i32 b0,
    i32 b1,
    i32 b2,
    i32 b3,
    i32 b4
) {
    CObject* found;
    CDDrawWorker* tbl;

    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    INITIALIZE_STATUS_BAR_ITEM(owner, tab, host)

    m_rect14 = rc;
    m_cmd = cmd;
    if (key == NULL) {
        return 0;
    }
    found = NULL;
    host->m_imageRegistry->m_workersByName.Lookup(key, found);
    tbl = static_cast<CDDrawWorker*>(found);
    m_frameSet = tbl;
    if (tbl == NULL) {
        goto fail;
    }
    m_interval = b2;
    m_loop = b3;
    m_step = b4;

    if (b0 == -1) {
        if (b4 >= 0) {
            m_frameStart = tbl->m_minIndex;
        } else {
            m_frameStart = tbl->m_maxIndex;
        }
    } else {
        m_frameStart = b0;
    }
    if (b1 == -1) {
        if (b4 >= 0) {
            m_frameEnd = tbl->m_maxIndex;
        } else {
            m_frameEnd = tbl->m_minIndex;
        }
    } else {
        m_frameEnd = b1;
    }
    m_frameIndex = m_frameStart;

    CImage* cel;
    if (DDRAW_WORKER_FRAME_OUT_OF_RANGE(tbl, m_frameStart)) {
        cel = NULL;
    } else {
        cel = DDRAW_WORKER_FRAME_AT_UNCHECKED(tbl, m_frameStart);
    }
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
            CDDrawSurfacePair* surfaceCtx = g_gameReg->m_world->m_drawTarget->m_backPair;
            cel->RenderFrame(
                surfaceCtx,
                cel->m_anchorX + m_rect14.left,
                cel->m_anchorY + m_rect14.top,
                0
            );
        }
        u32 now = timeGetTime();
        if (now - static_cast<u32>(m_lastTime) > static_cast<u32>(m_interval)) {
            m_frameIndex += m_step;
            m_lastTime = timeGetTime();
        }
        if (m_step > 0) {
            if (m_frameIndex > m_frameEnd) {
                if (m_loop != 0) {
                    m_frameIndex = m_frameStart;
                    return 1;
                }
                m_redrawFrames--;
                m_frameIndex = m_frameEnd;
                return 1;
            }
        } else if (m_step < 0) {
            if (m_frameIndex < m_frameEnd) {
                if (m_loop != 0) {
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
void CSBI_ImageSetAni::SetRange(i32 start, i32 end, i32 step, i32 loop, i32 interval) {

    if (start == -1) {
        if (step >= 0) {
            m_frameStart = m_frameSet->m_minIndex;
        } else {
            m_frameStart = m_frameSet->m_maxIndex;
        }
    } else {
        m_frameStart = start;
    }
    if (end == -1) {
        if (step >= 0) {
            m_frameEnd = m_frameSet->m_maxIndex;
        } else {
            m_frameEnd = m_frameSet->m_minIndex;
        }
    } else {
        m_frameEnd = end;
    }
    if (interval != -1) {
        m_interval = interval;
    }
    m_step = step;
    m_loop = loop;
    m_frameIndex = m_frameStart;
    m_redrawFrames = 2;
    m_lastTime = timeGetTime();
}

RVA(0x000e7cd0, 0xf8)
i32 CSBI_ImageSetAni::SerializeFields(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 pObj
) {
    if (s == NULL) {
        return 0;
    }
    if (g_gameReg->m_world == NULL) {
        return 0;
    }
    switch (mode) {

        case SERIAL_LOAD:
            s->Read(&m_interval, sizeof(m_interval));
            s->Read(&m_lastTime, sizeof(m_lastTime));
            s->Read(&m_loop, sizeof(m_loop));
            s->Read(&m_step, sizeof(m_step));
            s->Read(&m_frameEnd, sizeof(m_frameEnd));
            s->Read(&m_frameStart, sizeof(m_frameStart));
            break;
        case SERIAL_SAVE:
            s->Write(&m_interval, sizeof(m_interval));
            s->Write(&m_lastTime, sizeof(m_lastTime));
            s->Write(&m_loop, sizeof(m_loop));
            s->Write(&m_step, sizeof(m_step));
            s->Write(&m_frameEnd, sizeof(m_frameEnd));
            s->Write(&m_frameStart, sizeof(m_frameStart));
            break;
    }
    return CSBI_ImageSet::SerializeFields(s, mode, typeId, pObj) != 0;
}
