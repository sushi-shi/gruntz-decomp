#include <rva.h>

#include <Gruntz/SBI_Image.h>

#include <Mfc.h>
#include <MfcWin.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
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

RVA(0x000e6c80, 0xc3)
i32 CSBI_Image::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    SbiCommandId cmd,
    StatusBarTab tab,
    RECT rc,
    const char* key,
    i32 frame,
    i32 extra
) {
    if (host != NULL && owner != NULL) {
        m_owner = owner;
        m_tab = tab;
        m_host = host;
        m_redrawFrames = 0;
        SetEnabled(0);
        m_rect = rc;
        m_cmd = cmd;
        if (key != NULL) {
            CObject* found = NULL;
            host->m_imageRegistry->m_workersByName.Lookup(key, found);
            CDDrawWorker* rec = static_cast<CDDrawWorker*>(found);
            CImage* val;
            if (rec == NULL || !rec->ContainsFrame(1)) {
                val = NULL;
            } else {
                val = rec->FrameAtUnchecked(1);
            }
            SetFrame(val);
            return val != NULL;
        }
    }
    return 0;
}

RVA(0x000e6d90, 0x8)
void CSBI_Image::Reset() {
    SetFrame(NULL);
}

RVA(0x000e6db0, 0x8)
i32 CSBI_Image::Refresh(i32) {
    return 1;
}

RVA(0x000e6dd0, 0x45)
i32 CSBI_Image::Render() {
    if (m_redrawFrames > 0) {
        m_redrawFrames--;
        CImage* cel = m_frame;
        if (cel != NULL) {
            CPoint position(m_rect.left + cel->m_anchor.x, m_rect.top + cel->m_anchor.y);
            cel->RenderFrame(
                g_gameReg->m_world->m_drawTarget->m_backPair,
                position.x,
                position.y,
                0
            );
        }
    }
    return 1;
}

// @early-stop
RVA(0x000e6e40, 0x17c)
i32 CSBI_Image::SerializeFields(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    i32 payload
) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == NULL) {
        return 0;
    }

    char name[SERIAL_NAME_LEN];
    i32 idx;
    i32 v;
    switch (mode) {
        case SERIAL_LOAD:

            g_serialCounter++;
            ar->Read(name, SERIAL_NAME_LEN);
            ar->Read(&idx, sizeof(idx));
            if (strlen(name) != 0) {
                i32 frameIndex = idx;
                CObject* r_ob = NULL;
                mgr->m_imageRegistry->m_workersByName.Lookup(name, r_ob);
                CDDrawWorker* r = static_cast<CDDrawWorker*>(r_ob);
                if (r && r->ContainsFrame(frameIndex)) {
                    SetFrame(r->FrameAtUnchecked(frameIndex));
                } else {
                    SetFrame(NULL);
                }
            } else {
                SetFrame(NULL);
            }
            break;
        case SERIAL_SAVE:

            v = 0;
            g_serialCounter++;
            memset(name, 0, sizeof(name));
            if (m_frame) {
                mgr->m_imageRegistry->AnyValueMatches(m_frame, name, &v);
            }
            ar->Write(name, SERIAL_NAME_LEN);
            ar->Write(&v, sizeof(v));
            break;
    }

    return CStatusBarItem::SerializeFields(ar, mode, typeId, payload) != 0;
}
