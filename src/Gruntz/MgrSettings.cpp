#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SerialCounter.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/WarpStoneFly.h>
#include <Image/CImage.h>
#include <Ints.h>
#include <Io/FileMem.h>

#include <string.h>

RVA(0x00109e00, 0x245)
i32 CWarpStoneFly::Sync(CFileMemBase* arc, SerialMode mode, LogicTypeId typeId, i32 pObj) {
    if (arc == NULL) {
        return 0;
    }
    CDDrawSurfaceMgr* lvl = g_gameReg->m_world;
    if (lvl == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            arc->Read(&m_arrivalMode, sizeof(m_arrivalMode));
            arc->Read(&m_targetX, sizeof(m_targetX));
            arc->Read(&m_targetY, sizeof(m_targetY));
            arc->Read(&m_currentX, sizeof(m_currentX));
            arc->Read(&m_currentY, sizeof(m_currentY));
            arc->Read(&m_velocityScale, sizeof(m_velocityScale));
            arc->Read(&m_xDirection, sizeof(m_xDirection));
            arc->Read(&m_yDirection, sizeof(m_yDirection));
            g_serialCounter++;

            char name[SERIAL_NAME_LEN];
            i32 index;
            arc->Read(name, SERIAL_NAME_LEN);
            arc->Read(&index, sizeof(index));
            if (strlen(name) != 0) {
                i32 i = index;
                CObject* out = 0;
                lvl->m_imageRegistry->m_10map.Lookup(name, out);
                CDDrawWorker* rec = static_cast<CDDrawWorker*>(out);
                CImage* r;
                if (rec != NULL && i >= rec->m_minIndex && i <= rec->m_maxIndex) {
                    r = static_cast<CImage*>(rec->m_items.GetAt(i));
                } else {
                    r = NULL;
                }
                m_sprite = r;
            } else {
                m_sprite = NULL;
            }
            return 1;
        }
        case SERIAL_SAVE: {

            arc->Write(&m_arrivalMode, sizeof(m_arrivalMode));
            arc->Write(&m_targetX, sizeof(m_targetX));
            arc->Write(&m_targetY, sizeof(m_targetY));
            arc->Write(&m_currentX, sizeof(m_currentX));
            arc->Write(&m_currentY, sizeof(m_currentY));
            arc->Write(&m_velocityScale, sizeof(m_velocityScale));
            arc->Write(&m_xDirection, sizeof(m_xDirection));
            arc->Write(&m_yDirection, sizeof(m_yDirection));
            g_serialCounter++;

            CImage* obj = m_sprite;
            char name[SERIAL_NAME_LEN];
            i32 index = 0;
            memset(name, 0, SERIAL_NAME_LEN);
            if (obj != NULL) {
                lvl->m_imageRegistry->AnyValueMatches(obj, name, &index);
            }
            arc->Write(name, SERIAL_NAME_LEN);
            arc->Write(&index, sizeof(index));
            break;
        }
    }
    return 1;
}
