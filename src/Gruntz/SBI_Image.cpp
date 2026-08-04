#include <rva.h>

#include <Gruntz/SBI_Image.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/Sprite.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

VTBL(CSBI_RectOnly, 0x001eab8c);
VTBL(CStatusBarItem, 0x001eabcc);
VTBL(CSBI_Image, 0x001eac0c);

// @early-stop
RVA(0x000e6c80, 0xc3)
i32 CSBI_Image::SetupImage(
    CStatusBarMgr* owner,
    CDDrawSurfaceMgr* host,
    i32 cmd,
    i32 tab,
    RECT rc,
    const char* key,
    i32 frame,
    i32 extra
) {
    if (host == NULL) {
        return 0;
    }
    if (owner == NULL) {
        return 0;
    }
    m_owner = owner;
    m_tab = tab;
    m_host = host;
    m_redrawFrames = 0;
    m_enabled = 0;
    m_rect14.left = rc.left;
    m_rect14.top = rc.top;
    m_rect14.right = rc.right;
    m_rect14.bottom = rc.bottom;
    m_cmd = cmd;
    if (key == NULL) {
        m_frame = NULL;
        return 0 != 0;
    }
    CObject* found = 0;
    host->m_imageRegistry->m_10map.Lookup(key, found);
    CDDrawWorker* rec = static_cast<CDDrawWorker*>(found);
    if (rec == NULL || rec->m_minIndex > 1 || rec->m_maxIndex < 1) {
        m_frame = NULL;
        return 0 != 0;
    }
    CImage* val = static_cast<CImage*>(rec->m_items.GetAt(1));
    m_frame = val;
    return val != NULL;
}

RVA(0x000e6d90, 0x8)
void CSBI_Image::Reset() {
    m_frame = NULL;
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
            cel->RenderFrame(
                g_gameReg->m_world->m_drawTarget->m_backPair,
                m_rect14.left + cel->m_anchorX,
                m_rect14.top + cel->m_anchorY,
                0
            );
        }
    }
    return 1;
}

// @early-stop
RVA(0x000e6e40, 0x17c)
i32 CSBI_Image::SerializeFields(CFileMemBase* ar, SerialMode kind, LogicTypeId a, i32 b) {
    if (ar == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* mgr = g_gameReg->m_world;
    if (mgr == NULL) {
        return 0;
    }

    char name[SERIAL_NAME_LEN];
    i32 idx;
    switch (kind) {
        case SERIAL_LOAD:

            g_serialCounter++;
            ar->Read(name, SERIAL_NAME_LEN);
            ar->Read(&idx, sizeof(idx));
            if (strlen(name) != 0) {
                CObject* r_ob = 0;
                mgr->m_imageRegistry->m_10map.Lookup(name, r_ob);
                CDDrawWorker* r = static_cast<CDDrawWorker*>(r_ob);
                if (r && idx >= r->m_minIndex && idx <= r->m_maxIndex) {
                    m_frame = static_cast<CImage*>(r->m_items.GetAt(idx));
                } else {
                    m_frame = NULL;
                }
            } else {
                m_frame = NULL;
            }
            break;
        case SERIAL_SAVE:

            idx = 0;
            g_serialCounter++;
            memset(name, 0, sizeof(name));
            if (m_frame) {
                mgr->m_imageRegistry->AnyValueMatches(m_frame, name, &idx);
            }
            ar->Write(name, SERIAL_NAME_LEN);
            ar->Write(&idx, sizeof(idx));
            break;
    }

    return CStatusBarItem::SerializeFields(ar, kind, a, b) != 0;
}
