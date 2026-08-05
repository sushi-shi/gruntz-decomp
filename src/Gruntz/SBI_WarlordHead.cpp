#include <rva.h>

#include <Gruntz/SBI_WarlordHead.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>

VTBL(CSBI_WarlordHead, 0x001ead24);

RVA(0x000eb6b0, 0x67)
i32 CSBI_WarlordHead::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 frame,
    i32 extra
) {
    if (CSBI_ImageSet::SetupImage(owner, host, cmd, tab, rc, key, frame, extra) == 0) {
        return 0;
    }
    SetState(0);
    return 1;
}

// @early-stop
RVA(0x000eb740, 0xb3)
i32 CSBI_WarlordHead::ShowFrames(ShadeMode show, CShadeTable* palDescr) {
    CDDrawWorker* cfg = m_frameSet;
    if (cfg == NULL) {
        return 0;
    }

    CImage* f = (cfg->m_minIndex <= 1 && cfg->m_maxIndex >= 1)
                    ? static_cast<CImage*>(cfg->m_items.GetAt(1))
                    : 0;
    if (f == NULL) {
        return 0;
    }
    if (f->m_owned) {
        f->m_owned->Select(show, 0);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }

    f = (cfg->m_minIndex <= 2 && cfg->m_maxIndex >= 2) ? static_cast<CImage*>(cfg->m_items.GetAt(2))
                                                       : 0;
    if (f == NULL) {
        return 0;
    }
    if (f->m_owned) {
        f->m_owned->Select(show, 0);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }
    return 1;
}

RVA(0x000eb830, 0x31)
i32 CSBI_WarlordHead::SetState(i32 dir) {
    if (dir == 0 || dir == 1) {
        m_direction = dir;
        m_frameIndex = 1;
        return 1;
    }
    m_direction = dir;
    m_frameIndex = 2;
    return 1;
}

// @early-stop
RVA(0x000eb880, 0xbd)
i32 CSBI_WarlordHead::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        CDDrawSurfacePair* target = g_gameReg->m_world->m_drawTarget->m_backPair;

        CDDrawWorker* cfg = m_frameSet;
        CImage* f;
        if (m_direction == 1) {
            f = (cfg->m_minIndex > 3 || cfg->m_maxIndex < 3)
                    ? 0
                    : static_cast<CImage*>(cfg->m_items.GetAt(3));
        } else {
            f = (cfg->m_minIndex > 4 || cfg->m_maxIndex < 4)
                    ? 0
                    : static_cast<CImage*>(cfg->m_items.GetAt(4));
        }
        if (f) {
            f->RenderFrame(target, m_rect14.left + f->m_anchorX, m_rect14.top + f->m_anchorY, 0);
        }

        cfg = m_frameSet;
        i32 idx = m_frameIndex;
        CImage* g = (idx < cfg->m_minIndex || idx > cfg->m_maxIndex)
                        ? 0
                        : static_cast<CImage*>(cfg->m_items.GetAt(idx));
        m_frame = g;
        if (g) {
            g->RenderFrame(target, m_rect14.left + g->m_anchorX, m_rect14.top + g->m_anchorY, 0);
        }
    }
    return 1;
}

RVA(0x000eb970, 0x72)
i32 CSBI_WarlordHead::SerializeFields(
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
            s->Read(&m_direction, sizeof(m_direction));
            break;
        case SERIAL_SAVE:
            s->Write(&m_direction, sizeof(m_direction));
            break;
    }
    return CSBI_ImageSet::SerializeFields(s, mode, typeId, pObj) != 0;
}

RVA_COMPGEN(0x001049d0, 0x1e, ??_GCSBI_WarlordHead@@UAEPAXI@Z)
RVA(0x00104a00, 0x94)
CSBI_WarlordHead::~CSBI_WarlordHead() {
    Reset();
}
