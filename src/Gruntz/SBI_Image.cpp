#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/SBI_Image.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Enums.h>
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
#include <Gruntz/StatusBarMgr.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

i32 CSBI_Image::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const std::string& key,
    i32 frame,
    i32 extra
) {
    if (host != NULL && owner != NULL) {
        Initialize(owner, tab, host, false);
        m_rect = rc;
        m_cmd = cmd;
        if (!key.empty()) {
            CImage* val = host->FindFrame(key, 1);
            SetFrame(val);
            return val != NULL;
        }
    }
    return 0;
}

void CSBI_Image::Reset() {
    SetFrame(NULL);
}

i32 CSBI_Image::Refresh(i32) {
    return 1;
}

i32 CSBI_Image::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        CImage* cel = m_frame;
        if (cel != NULL) {
            i32 y = m_rect.top + cel->m_anchorY;
            i32 x = m_rect.left + cel->m_anchorX;
            cel->RenderFrame(g_gameReg->m_world->GetDrawTarget()->GetBackPair(), x, y, 0);
        }
    }
    return 1;
}

i32 CSBI_Image::SerializeFields(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->World();
    if (mgr == NULL) {
        return 0;
    }

    char name[SERIAL_NAME_LEN];
    switch (mode) {
        case SERIAL_LOAD: {
            i32 idx;
            g_serialCounter++;
            ar->Read(name, SERIAL_NAME_LEN);
            ar->Read(&idx, sizeof(idx));
            if (strlen(name) != 0) {
                SetFrame(mgr->FindFrame(name, idx));
            } else {
                SetFrame(NULL);
            }
            break;
        }
        case SERIAL_SAVE: {
            i32 v = 0;
            g_serialCounter++;
            memset(name, 0, sizeof(name));
            if (m_frame) {
                const FrameReference reference = mgr->m_imageRegistry->FindFrameReference(m_frame);
                if (!copyTextToBuffer(reference.workerName, name, sizeof(name))) return 0;
                v = reference.frameIndex;
            }
            ar->Write(name, SERIAL_NAME_LEN);
            ar->Write(&v, sizeof(v));
            break;
        }
    }

    return CStatusBarItem::SerializeFields(ar, mode, typeId, payload) != 0;
}
