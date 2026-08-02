#include <rva.h>

#include <Gruntz/SBI_ImageSetAni.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

VTBL(CSBI_ImageSetAni, 0x001ead6c);

// @early-stop
RVA(0x000e7980, 0x109)
i32 CSBI_ImageSetAni::Init(
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

    if (host == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    m_owner = owner;
    m_tab = tab;
    m_host = host;
    m_redrawFrames = 0;
    m_enabled = 1;

    m_rect14 = rc;
    m_cmd = cmd;
    if (key == 0) {
        return 0;
    }
    CObject* found = 0;
    host->m_imageRegistry->m_10map.Lookup(key, found);
    CDDrawWorker* tbl = static_cast<CDDrawWorker*>(found);
    m_frameSet = tbl;
    if (tbl == 0) {
        return 0;
    }
    m_interval = b2;
    m_loop = b3;
    m_step = b4;

    if (b0 == -1) {
        m_frameStart = (b4 >= 0) ? tbl->m_minIndex : tbl->m_maxIndex;
    } else {
        m_frameStart = b0;
    }
    if (b1 == -1) {
        m_frameEnd = (b4 >= 0) ? tbl->m_maxIndex : tbl->m_minIndex;
    } else {
        m_frameEnd = b1;
    }
    m_frameIndex = m_frameStart;

    CImage* cel;
    if (m_frameStart < tbl->m_minIndex || m_frameStart > tbl->m_maxIndex) {
        cel = 0;
    } else {
        cel = static_cast<CImage*>(tbl->m_items.GetAt(m_frameStart));
    }
    m_frame = cel;
    return cel != 0;
}

RVA(0x000e7ae0, 0x8)
i32 CSBI_ImageSetAni::Refresh(i32) {
    return 1;
}

// @early-stop
RVA(0x000e7b00, 0xe1)
i32 CSBI_ImageSetAni::Render() {
    if (m_redrawFrames > 0) {
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
                m_frameIndex = m_frameEnd;
                m_redrawFrames--;
            }
        } else if (m_step < 0) {
            if (m_frameIndex < m_frameEnd) {
                if (m_loop != 0) {
                    m_frameIndex = m_frameStart;
                    return 1;
                }
                m_frameIndex = m_frameEnd;
                m_redrawFrames--;
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
    m_lastTime = ::timeGetTime();
}

RVA(0x000e7cd0, 0xf8)
i32 CSBI_ImageSetAni::SerializeFields(CFileMemBase* s, i32 mode, i32 typeId, i32 pObj) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    switch (mode) {

        case 7:
            s->Read(&m_interval, 4);
            s->Read(&m_lastTime, 4);
            s->Read(&m_loop, 4);
            s->Read(&m_step, 4);
            s->Read(&m_frameEnd, 4);
            s->Read(&m_frameStart, 4);
            break;
        case 4:
            s->Write(&m_interval, 4);
            s->Write(&m_lastTime, 4);
            s->Write(&m_loop, 4);
            s->Write(&m_step, 4);
            s->Write(&m_frameEnd, 4);
            s->Write(&m_frameStart, 4);
            break;
    }
    return CSBI_ImageSet::SerializeFields(s, mode, typeId, pObj) != 0;
}
