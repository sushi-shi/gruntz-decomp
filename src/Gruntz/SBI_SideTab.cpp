#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/SBI_SideTab.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntPickupInline.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthGlyph.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialWorkerRefMacros.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StatusBarItem.h>
#include <Gruntz/StatusBarMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Image/CImage.h>
#include <Image/ImageSet.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

RVA(0x000e9600, 0x18c)
i32 CSBI_SideTab::Initialize(
    CStatusBarMgr* parent,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* unused,
    i32 playerIndex,
    i32 unitIndex,
    StatusSampleMode sampleMode,
    i32 onLeft
) {
    static_cast<void>(unused);
    if (host == NULL) {
        goto fail;
    }
    if (parent == NULL) {
        goto fail;
    }
    // Keep the conditional enable branch after the rectangle copy.
    m_host = host;
    m_tab = tab;
    m_owner = parent;
    m_rect = rc;
    m_redrawFrames = 0;
    m_cmd = cmd;

    if (sampleMode != STATUS_SAMPLE_NONE) {
        SetEnabled(1);
    } else {
        SetEnabled(0);
    }
    m_playerIndex = playerIndex;
    m_unitIndex = unitIndex;
    m_onLeft = onLeft;

    if (onLeft != 0) {
        m_backgroundImage =
            g_gameReg->World()->FindFrame("GAME_STATUSBAR_TABZ_STATZTAB_TABONLEFT", 1);
        m_drawPosition.m_x = parent->GetBarRect()->left - (rc.right - rc.left) / 2;
        m_iconOffsetX = 1;
    } else {
        m_backgroundImage =
            g_gameReg->World()->FindFrame("GAME_STATUSBAR_TABZ_STATZTAB_TABONRIGHT", 1);
        m_drawPosition.m_x = (rc.right - rc.left) / 2 + parent->GetBarRect()->right;
        m_iconOffsetX = -1;
    }
    m_drawPosition.m_y = unitIndex * 0x12 + 0xd1;
    if (m_backgroundImage == NULL) {
        goto fail;
    }
    m_sampleMode = sampleMode;
    m_iconIndex = -1;
    m_hasSample = UpdateSampleIcon();
    return 1;
fail:
    return 0;
}

RVA(0x000e9800, 0x9)
void CSBI_SideTab::Reset() {
    m_backgroundImage = NULL;
    m_iconImage = NULL;
}

RVA(0x000e9820, 0x11)
i32 CSBI_SideTab::Refresh(i32 unused) {
    m_hasSample = UpdateSampleIcon();
    return 0;
}

// @early-stop
RVA(0x000e9850, 0x111)
i32 CSBI_SideTab::UpdateSampleIcon() {
    StatusSampleMode mode = m_sampleMode;
    if (mode == STATUS_SAMPLE_NONE) {
        return 0;
    }
    CGrunt* unit = g_gameReg->GetTriggerMgr()->UnitAt(m_playerIndex, m_unitIndex);
    if (unit == NULL) {
        m_owner->ClearUnitSample(m_unitIndex);
        return 0;
    }
    i32 val;
    if (mode == STATUS_SAMPLE_TOOL) {
        PickupType level = unit->GetEquippedToolType();
        val = IDX(level);
        if (level == PICKUP_NONE) {
            m_sampleMode = STATUS_SAMPLE_HEALTH;
        }
    } else if (mode == STATUS_SAMPLE_TOY) {
        val = IDX(unit->GetCarriedToyType());
        if (unit->GetCarriedToyType() == PICKUP_NONE) {
            m_sampleMode = STATUS_SAMPLE_HEALTH;
        }
    }
    if (m_sampleMode == STATUS_SAMPLE_HEALTH) {
        val = HealthGlyphIndex(unit->GetHealth());
    }
    if (m_iconIndex == val) {
        return 1;
    }
    CImage* glyph = g_gameReg->World()->FindFrame("GAME_STATUSBAR_TABZ_STATZTAB_SMALLICONZ", val);
    m_iconIndex = val;
    m_iconImage = glyph;
    return 1;
}

RVA(0x000e99c0, 0x4c)
i32 CSBI_SideTab::Render() {
    if (m_hasSample) {
        CRenderBuffer* ctx = g_gameReg->World()->GetDisplayBuffers()->GetBackBuffer();
        m_backgroundImage->RenderFrame(ctx, m_drawPosition.m_x, m_drawPosition.m_y, 0);
        m_iconImage->RenderFrame(ctx, m_drawPosition.m_x + m_iconOffsetX, m_drawPosition.m_y, 0);
    }
    return 1;
}

RVA(0x000e9a30, 0x31e)
i32 CSBI_SideTab::SerializeFields(
    CFileMemBase* s,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (s == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* reg = g_gameReg->World();
    if (reg == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];

    switch (mode) {
        case SERIAL_SAVE: {
            i32 v;

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_backgroundImage);

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_iconImage);

            s->Write(&m_iconIndex, sizeof(m_iconIndex));
            s->Write(&m_playerIndex, sizeof(m_playerIndex));
            s->Write(&m_unitIndex, sizeof(m_unitIndex));
            s->Write(&m_sampleMode, sizeof(m_sampleMode));
            s->Write(&m_drawPosition, sizeof(m_drawPosition));
            s->Write(&m_iconOffsetX, sizeof(m_iconOffsetX));
            s->Write(&m_onLeft, sizeof(m_onLeft));
            s->Write(&m_hasSample, sizeof(m_hasSample));
            break;
        }

        case SERIAL_LOAD: {
            CObject* out;
            i32 idx;

            GS_IDXREF(m_backgroundImage);

            GS_IDXREF(m_iconImage);

            s->Read(&m_iconIndex, sizeof(m_iconIndex));
            s->Read(&m_playerIndex, sizeof(m_playerIndex));
            s->Read(&m_unitIndex, sizeof(m_unitIndex));
            s->Read(&m_sampleMode, sizeof(m_sampleMode));
            s->Read(&m_drawPosition, sizeof(m_drawPosition));
            s->Read(&m_iconOffsetX, sizeof(m_iconOffsetX));
            s->Read(&m_onLeft, sizeof(m_onLeft));
            s->Read(&m_hasSample, sizeof(m_hasSample));
            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, payload) != 0 ? 1 : 0;
}

RVA_COMPGEN(0x001051d0, 0x1e, ??_GCSBI_SideTab@@UAEPAXI@Z)
RVA(0x00105200, 0x55)
CSBI_SideTab::~CSBI_SideTab() {
    Reset();
}
