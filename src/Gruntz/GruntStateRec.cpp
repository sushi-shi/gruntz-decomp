#include <StdAfx.h>

#include <rva.h>

#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SBI_StatzTabGruntBar.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/SerialWorkerRefMacros.h>
#include <Gruntz/Sprite.h>
#include <Io/FileMem.h>

#include <string.h>

RVA(0x000ea990, 0xa72)
i32 CSBI_StatzTabGruntBar::SerializeFields(
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

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_healthBackgroundImage);
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_healthIconImage);
            s->Write(&m_healthIconIndex, sizeof(m_healthIconIndex));

            SERIAL_WRITE_FRAME(s, reg, buf, v, m_toolBackgroundImage);
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_toolIconImage);
            s->Write(&m_toolIconIndex, sizeof(m_toolIconIndex));
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_toyBackgroundImage);
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_toyIconImage);
            s->Write(&m_toyIconIndex, sizeof(m_toyIconIndex));
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_groupBackgroundImage);
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_groupIconImage);
            s->Write(&m_groupMarker, sizeof(m_groupMarker));
            SERIAL_WRITE_FRAME(s, reg, buf, v, m_selectionImage);
            s->Write(&m_selectionFrameIndex, sizeof(m_selectionFrameIndex));
            s->Write(&m_playerIndex, sizeof(m_playerIndex));
            s->Write(&m_unitIndex, sizeof(m_unitIndex));

            SERIAL_WRITE_WORKER(s, buf, m_iconFrames);

            SERIAL_WRITE_WORKER(s, buf, m_selectionFrames);
            break;
        }

        case SERIAL_LOAD: {
            CObject* out;
            i32 idx;

            GS_IDXREF(m_healthBackgroundImage);
            GS_IDXREF(m_healthIconImage);
            s->Read(&m_healthIconIndex, sizeof(m_healthIconIndex));
            GS_IDXREF(m_toolBackgroundImage);
            GS_IDXREF(m_toolIconImage);
            s->Read(&m_toolIconIndex, sizeof(m_toolIconIndex));
            GS_IDXREF(m_toyBackgroundImage);
            GS_IDXREF(m_toyIconImage);
            s->Read(&m_toyIconIndex, sizeof(m_toyIconIndex));
            GS_IDXREF(m_groupBackgroundImage);
            GS_IDXREF(m_groupIconImage);
            s->Read(&m_groupMarker, sizeof(m_groupMarker));
            GS_IDXREF(m_selectionImage);
            s->Read(&m_selectionFrameIndex, sizeof(m_selectionFrameIndex));
            s->Read(&m_playerIndex, sizeof(m_playerIndex));
            s->Read(&m_unitIndex, sizeof(m_unitIndex));
            GS_NAMEREF(m_iconFrames);
            GS_NAMEREF(m_selectionFrames);
            break;
        }
    }

    return CStatusBarItem::SerializeFields(s, mode, typeId, payload) != 0 ? 1 : 0;
}
