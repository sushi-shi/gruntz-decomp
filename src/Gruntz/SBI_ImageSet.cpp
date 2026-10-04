#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/SBI_ImageSet.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Globals.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

// @early-stop
RVA(0x000e72f0, 0xc4)
i32 CSBI_ImageSet::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rect,
    const char* frameSetName,
    i32 frameIndex,
    i32 extra
) {
    CDDrawWorker* frames;

    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    Initialize(owner, tab, host);

    m_rect = rect;
    m_cmd = cmd;
    if (frameSetName == NULL) {
        return 0;
    }
    frames = host->FindWorker(frameSetName);
    m_frameSet = frames;
    if (frames == NULL) {
        goto fail;
    }
    i32 initialFrameIndex;
    initialFrameIndex = frameIndex;
    if (initialFrameIndex == -1) {
        initialFrameIndex = frames->GetMinIndex();
    }
    m_frameIndex = initialFrameIndex;

    SetFrame(frames->GetAt(initialFrameIndex));
    return 1;
fail:
    return 0;
}

RVA(0x000e7400, 0x9)
void CSBI_ImageSet::Reset() {
    m_frameSet = NULL;
    SetFrame(NULL);
}

RVA(0x000e7420, 0x8)
i32 CSBI_ImageSet::Refresh(i32) {
    return 1;
}

RVA(0x000e7440, 0x5e)
i32 CSBI_ImageSet::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        i32 frameIndex = m_frameIndex;
        CDDrawWorker* frames = m_frameSet;
        CImage* image = frames->GetAt(frameIndex);
        SetFrame(image);
        if (image != NULL) {
            i32 y = image->GetAnchorY() + m_rect.top;
            i32 x = image->GetAnchorX() + m_rect.left;
            image->RenderFrame(g_gameReg->World()->GetDrawTarget()->m_backPair, x, y, 0);
        }
    }
    return 1;
}

RVA(0x000e74c0, 0x16)
void CSBI_ImageSet::SetFrameIndex(i32 frameIndex) {
    if (frameIndex != -1) {
        m_frameIndex = frameIndex;
    }
    m_redrawFrames = 2;
}

RVA(0x000e74f0, 0x152)
i32 CSBI_ImageSet::SerializeFields(
    CFileMemBase* archive,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (archive == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* world = g_gameReg->World();
    if (world == NULL) {
        return 0;
    }
    char frameSetName[SERIAL_NAME_LEN];
    switch (mode) {
        case SERIAL_LOAD:
            archive->Read(&m_frameIndex, sizeof(m_frameIndex));
            g_serialCounter++;
            archive->Read(frameSetName, SERIAL_NAME_LEN);
            if (strlen(frameSetName)) {
                CDDrawWorker* frames;

                frames = world->FindWorker(frameSetName);
                m_frameSet = frames;
            } else {
                m_frameSet = NULL;
            }
            break;
        case SERIAL_SAVE:
            archive->Write(&m_frameIndex, sizeof(m_frameIndex));
            g_serialCounter++;
            memset(frameSetName, 0, SERIAL_NAME_LEN);
            if (m_frameSet) {
                strcpy(frameSetName, m_frameSet->GetName());
            }
            archive->Write(frameSetName, SERIAL_NAME_LEN);
            break;
    }

    return CSBI_Image::SerializeFields(archive, mode, typeId, payload) != 0;
}
