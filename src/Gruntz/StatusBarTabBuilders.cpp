#include <StdAfx.h>

#include <rva.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SBI_GruntMachine.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/SBI_SideTab.h>
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

// @early-stop
RVA(0x000e8a70, 0x18c)
i32 CSBI_GruntMachine::Initialize(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rect,
    const char* frameSetName,
    i32 leftFrameIndex,
    i32 rightFrameIndex
) {

    CDDrawSurfaceMgr* world;
    CDDrawWorker* rec;
    CImage* backgroundImage;
    CDDrawWorker* machineFrames;
    CImage* leftImage;
    CShadeTable* shadeTable;
    CImage* rightImage;

    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    world = host;
    CStatusBarItem::Initialize(owner, tab, world);

    m_rect = rect;

    m_cmd = cmd;
    backgroundImage = world->FindFrame("GAME_STATUSBAR_TABZ_RESOURCETAB_MACHINEBACKGROUND", 1);
    m_backgroundImage = backgroundImage;
    if (backgroundImage == NULL) {
        return 0;
    }
    machineFrames = m_host->FindWorker(frameSetName);
    m_machineFrames = machineFrames;
    if (machineFrames == NULL) {
        return 0;
    }
    m_leftFrameIndex = leftFrameIndex;
    m_rightFrameIndex = rightFrameIndex;
    leftImage = m_machineFrames->GetAt(leftFrameIndex);
    m_leftFrame = leftImage;
    if (leftImage == NULL) {
        goto fail;
    }
    shadeTable = g_gameReg->GruntPalettes()->GetShadeTable(
        IDX(g_gameReg->GetPlayer(g_curPlayer).GetColor()),
        0
    );
    if (shadeTable == NULL) {
        shadeTable = g_gameReg->GruntPalettes()->GetShadeTable(1, 0);
    }
    m_machineFrames->SetAllTypes(SHADE_PAL_16);
    m_machineFrames->SetAllFormats(shadeTable);
    rightImage = m_machineFrames->GetAt(m_rightFrameIndex);
    m_rightFrame = rightImage;
    return rightImage != NULL;
fail:
    return 0;
}

RVA(0x000e8c70, 0xc)
void CSBI_GruntMachine::Reset() {
    m_leftFrame = NULL;
    m_rightFrame = NULL;
    m_machineFrames = NULL;
}

RVA(0x000e8c90, 0x8)
i32 CSBI_GruntMachine::Refresh(i32) {
    return 1;
}

RVA(0x000e8cb0, 0xc4)
i32 CSBI_GruntMachine::Render() {
    if (m_redrawFrames > 0) {
        i32 idx = m_leftFrameIndex;
        m_redrawFrames--;
        CDDrawWorker* cfg = m_machineFrames;

        m_leftFrame = cfg->GetAt(idx);
        idx = m_rightFrameIndex;
        m_rightFrame = cfg->GetAt(idx);

        CDDrawSurfacePair* ctx = g_gameReg->World()->GetDrawTarget()->m_backPair;

        CImage* f = m_backgroundImage;
        if (f) {
            f->RenderFrame(ctx, m_rect.left + f->GetAnchorX(), m_rect.top + f->GetAnchorY(), 0);
        }
        f = m_rightFrame;
        if (f) {
            f->RenderFrame(
                ctx,
                m_rect.left + f->GetAnchorX() + 0x2c,
                m_rect.top + f->GetAnchorY(),
                0
            );
        }
        f = m_leftFrame;
        if (f) {
            f->RenderFrame(ctx, m_rect.left + f->GetAnchorX(), m_rect.top + f->GetAnchorY(), 0);
        }
    }
    return 1;
}

RVA(0x000e8dc0, 0x22)
void CSBI_GruntMachine::SetFrames(i32 leftFrameIndex, i32 rightFrameIndex) {
    if (leftFrameIndex != -1) {
        m_leftFrameIndex = leftFrameIndex;
    }
    if (rightFrameIndex != -1) {
        m_rightFrameIndex = rightFrameIndex;
    }
    m_redrawFrames = 2;
}

// Each LOAD group scopes its own `idx`: cl 5.0 then overlays that home onto the
// `reg` spill, which is dead once the first group has hoisted it into ESI.
RVA(0x000e8e00, 0x41a)
i32 CSBI_GruntMachine::SerializeFields(
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

            SERIAL_WRITE_WORKER(s, buf, m_machineFrames);
            s->Write(&m_leftFrameIndex, sizeof(m_leftFrameIndex));

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_leftFrame);
            s->Write(&m_rightFrameIndex, sizeof(m_rightFrameIndex));

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_rightFrame);

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_backgroundImage);
            break;
        }

        case SERIAL_LOAD: {
            CObject* out;

            GS_NAMEREF(m_machineFrames);
            s->Read(&m_leftFrameIndex, sizeof(m_leftFrameIndex));

            {
                i32 idx;
                GS_IDXREF(m_leftFrame);
            }
            s->Read(&m_rightFrameIndex, sizeof(m_rightFrameIndex));

            {
                i32 idx;
                GS_IDXREF(m_rightFrame);
            }

            {
                i32 idx;
                GS_IDXREF(m_backgroundImage);
            }

            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, payload) != 0 ? 1 : 0;
}
