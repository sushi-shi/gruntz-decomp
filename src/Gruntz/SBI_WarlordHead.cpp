#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/SBI_WarlordHead.h>

#include <DDrawMgr/DDrawShadeBlit.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Globals.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/Sprite.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>

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
    if (CSBI_ImageSet::SetupImage(owner, host, cmd, tab, rc, key, frame, extra) == SBICMD_NONE) {
        return 0;
    }
    SetDisplayState(0);
    return 1;
}

RVA(0x000eb740, 0xb3)
i32 CSBI_WarlordHead::SetHeadShading(ShadeMode shadeMode, CShadeTable* shadeTable) {
    if (m_frameSet == NULL) {
        return 0;
    }

    CImage* f = m_frameSet->GetAt(1);
    if (f == NULL) {
        return 0;
    }
    if (f->GetShadeBlitter()) {
        f->GetShadeBlitter()->Select(shadeMode, NULL);
    }
    if (shadeTable && f->GetShadeBlitter()) {
        f->GetShadeBlitter()->m_palDescr = shadeTable;
    }

    f = m_frameSet->GetAt(2);
    if (f == NULL) {
        return 0;
    }
    if (f->GetShadeBlitter()) {
        f->GetShadeBlitter()->Select(shadeMode, NULL);
    }
    if (shadeTable && f->GetShadeBlitter()) {
        f->GetShadeBlitter()->m_palDescr = shadeTable;
    }
    return 1;
}

RVA(0x000eb830, 0x31)
i32 CSBI_WarlordHead::SetDisplayState(i32 state) {
    if (state == 0 || state == 1) {
        m_displayState = state;
        m_frameIndex = 1;
        return 1;
    }
    m_displayState = state;
    m_frameIndex = 2;
    return 1;
}

RVA(0x000eb880, 0xbd)
i32 CSBI_WarlordHead::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        CRenderBuffer* target = g_gameReg->m_world->m_drawTarget->GetBackPair();

        CImage* f;
        if (m_displayState == 1) {
            f = m_frameSet->GetAt(3);
        } else {
            f = m_frameSet->GetAt(4);
        }
        if (f) {
            f->RenderFrame(target, m_rect.left + f->GetAnchorX(), m_rect.top + f->GetAnchorY(), 0);
        }

        CImage* g = m_frameSet->GetAt(m_frameIndex);
        SetFrame(g);
        if (g) {
            g->RenderFrame(target, m_rect.left + g->GetAnchorX(), m_rect.top + g->GetAnchorY(), 0);
        }
    }
    return 1;
}

RVA(0x000eb970, 0x72)
i32 CSBI_WarlordHead::SerializeFields(
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
            s->Read(&m_displayState, sizeof(m_displayState));
            break;
        case SERIAL_SAVE:
            s->Write(&m_displayState, sizeof(m_displayState));
            break;
    }
    return CSBI_ImageSet::SerializeFields(s, mode, typeId, payload) != 0;
}

RVA_COMPGEN(0x001049d0, 0x1e, ??_GCSBI_WarlordHead@@UAEPAXI@Z)
RVA(0x00104a00, 0x94)
CSBI_WarlordHead::~CSBI_WarlordHead() {
    Reset();
}
