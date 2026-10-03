#include <StdAfx.h>

#include <Ints.h>

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

i32 CSBI_ImageSet::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab obj,
    RECT rect,
    const char* key,
    i32 frame,
    i32 extra
) {
    CDDrawWorker* rec;

    if (host == NULL) {
        goto fail;
    }
    if (owner == NULL) {
        goto fail;
    }
    Initialize(owner, obj, host);

    m_rect = rect;
    m_cmd = cmd;
    if (key == NULL) {
        return 0;
    }
    rec = host->FindWorker(key);
    m_frameSet = rec;
    if (rec == NULL) {
        goto fail;
    }
    i32 f;
    f = frame;
    if (f == -1) {
        f = rec->GetMinIndex();
    }
    m_frameIndex = f;

    SetFrame(rec->GetAt(f));
    return 1;
fail:
    return 0;
}

void CSBI_ImageSet::Reset() {
    m_frameSet = NULL;
    SetFrame(NULL);
}

i32 CSBI_ImageSet::Refresh(i32) {
    return 1;
}

i32 CSBI_ImageSet::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        i32 idx = m_frameIndex;
        CDDrawWorker* tbl = m_frameSet;
        CImage* cel = tbl->GetAt(idx);
        SetFrame(cel);
        if (cel != NULL) {
            i32 y = cel->m_anchorY + m_rect.top;
            i32 x = cel->m_anchorX + m_rect.left;
            cel->RenderFrame(g_gameReg->m_world->GetDrawTarget()->m_backPair, x, y, 0);
        }
    }
    return 1;
}

void CSBI_ImageSet::Notify(i32 id) {
    if (id != -1) {
        m_frameIndex = id;
    }
    m_redrawFrames = 2;
}

i32 CSBI_ImageSet::SerializeFields(
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
        case SERIAL_LOAD:
            s->Read(&m_frameIndex, sizeof(m_frameIndex));
            g_serialCounter++;
            s->Read(buf, SERIAL_NAME_LEN);
            if (strlen(buf)) {
                CDDrawWorker* out;

                out = reg->FindWorker(buf);
                m_frameSet = out;
            } else {
                m_frameSet = NULL;
            }
            break;
        case SERIAL_SAVE:
            s->Write(&m_frameIndex, sizeof(m_frameIndex));
            g_serialCounter++;
            memset(buf, 0, SERIAL_NAME_LEN);
            if (m_frameSet) {
                strcpy(buf, m_frameSet->m_name);
            }
            s->Write(buf, SERIAL_NAME_LEN);
            break;
    }

    return CSBI_Image::SerializeFields(s, mode, typeId, payload) != 0;
}
