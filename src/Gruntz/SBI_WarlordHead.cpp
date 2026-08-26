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

RVA(0x000eb7e0, 0x67)
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
    if (CSBI_ImageSet::SetupImage(owner, host, cmd, tab, rc, key, frame, extra) == SBICMD_NONE) {
        return 0;
    }
    SetState(0);
    return 1;
}

RVA(0x000eb870, 0xb3)
i32 CSBI_WarlordHead::ShowFrames(ShadeMode show, CShadeTable* palDescr) {
    if (m_frameSet == NULL) {
        return 0;
    }

    CImage* f = m_frameSet->GetAt(1);
    if (f == NULL) {
        return 0;
    }
    if (f->m_owned) {
        f->m_owned->Select(show, NULL);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }

    f = m_frameSet->GetAt(2);
    if (f == NULL) {
        return 0;
    }
    if (f->m_owned) {
        f->m_owned->Select(show, NULL);
    }
    if (palDescr && f->m_owned) {
        f->m_owned->m_palDescr = palDescr;
    }
    return 1;
}

RVA(0x000eb960, 0x31)
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
RVA(0x000eb9b0, 0xbd)
i32 CSBI_WarlordHead::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        CDDrawSurfacePair* target = g_gameReg->m_world->m_drawTarget->m_backPair;

        CDDrawWorker* cfg = m_frameSet;
        CImage* f;
        if (m_direction == 1) {
            f = cfg->GetAt(3);
        } else {
            f = cfg->GetAt(4);
        }
        if (f) {
            f->RenderFrame(target, m_rect.left + f->m_anchorX, m_rect.top + f->m_anchorY, 0);
        }

        cfg = m_frameSet;
        i32 idx = m_frameIndex;
        CImage* g = cfg->GetAt(idx);
        SetFrame(g);
        if (g) {
            g->RenderFrame(target, m_rect.left + g->m_anchorX, m_rect.top + g->m_anchorY, 0);
        }
    }
    return 1;
}

RVA(0x000ebaa0, 0x72)
i32 CSBI_WarlordHead::SerializeFields(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
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
    return CSBI_ImageSet::SerializeFields(s, mode, typeId, payload) != 0;
}

RVA_COMPGEN(0x00104b00, 0x1e, ??_GCSBI_WarlordHead@@UAEPAXI@Z)
RVA(0x00104b30, 0x94)
CSBI_WarlordHead::~CSBI_WarlordHead() {
    Reset();
}
