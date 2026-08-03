#include <rva.h>

#include <DDrawMgr/LogicRecord.h>

#include <Mfc.h>

#include <DDrawMgr/AnimWorkerObj.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>

// @early-stop
RVA(0x00164830, 0xec)
i32 AnimWorkerObj::Dispatch(CFileMemBase* a, SerialMode mode, LogicTypeId c, void* d) {
    if (a == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_PRESAVE:
            m_targetId = 0;
            if (m_target) {
                m_targetId = m_target->m_objectId;
            }
            break;
        case SERIAL_SAVE:

            if (Save(a) == 0) {
                return 0;
            }
            break;
        case SERIAL_LOAD:

            if (Load(a) == 0) {
                return 0;
            }
            break;
        case SERIAL_POSTLOAD:
            if (m_targetId) {
                void* out = 0;
                CMapPtrToPtr* res = &m_ownerCtx->m_childGroup->m_map48;
                m_target = MapLookupById(*res, m_targetId, out) ? static_cast<CGameObject*>(out)
                                                                : static_cast<CGameObject*>(0);
            }
            break;
        default:
            break;
    }
    if (m_logic) {
        if (m_logic->SerializeMove(a, mode, c, static_cast<CGameObject*>(d)) == 0) {
            return 0;
        }
    }
    return 1;
}

RVA(0x00164920, 0x35)
i32 AnimWorkerObj::CacheTargetId(void* a) {
    if (a == NULL) {
        return 0;
    }
    m_targetId = 0;
    if (m_target) {
        m_targetId = m_target->m_objectId;
    }
    return 1;
}

RVA(0x00164960, 0x41a)
i32 AnimWorkerObj::Save(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Write(&m_actKey, 4);
    ar->Write(&m_timeDelay, 4);
    ar->Write(&m_frameDelay, 4);
    ar->Write(&m_userFlags, 4);
    ar->Write(&m_minX, 4);
    ar->Write(&m_maxX, 4);
    ar->Write(&m_minY, 4);
    ar->Write(&m_maxY, 4);
    ar->Write(&m_pad3c, 4);
    ar->Write(&m_reserved40, 4);
    ar->Write(&m_tweakX, 4);
    ar->Write(&m_tweakY, 4);
    ar->Write(&m_scrollTargetX, 4);
    ar->Write(&m_scrollTargetY, 4);
    ar->Write(&m_pad54, 4);
    ar->Write(&m_reserved58, 4);
    ar->Write(&m_reserved5c, 4);
    ar->Write(&m_reserved60, 4);
    ar->Write(&m_user1, 4);
    ar->Write(&m_user2, 4);
    ar->Write(&m_user3, 4);
    ar->Write(&m_user4, 4);
    ar->Write(&m_user5, 4);
    ar->Write(&m_user6, 4);
    ar->Write(&m_user7, 4);
    ar->Write(&m_user8, 4);
    ar->Write(&m_reserved84, 4);
    ar->Write(&m_reserved88, 4);
    ar->Write(&m_reserved8c, 4);
    ar->Write(&m_reserved90, 4);
    ar->Write(&m_reserved94, 4);
    ar->Write(&m_reserved98, 4);
    ar->Write(&m_reserved9c, 4);
    ar->Write(&m_reserveda0, 4);
    ar->Write(&m_reserveda4, 4);
    ar->Write(&m_reserveda8, 4);
    ar->Write(&m_reservedac, 4);
    ar->Write(&m_reservedb0, 4);
    ar->Write(&m_reservedb4, 4);
    ar->Write(&m_counter, 4);
    ar->Write(&m_speed, 4);
    ar->Write(&m_padc0, 4);
    ar->Write(&m_reservedc4, 4);
    ar->Write(&m_width, 4);
    ar->Write(&m_height, 4);
    ar->Write(&m_reservedd0, 16);
    ar->Write(&m_reservede0, 16);
    ar->Write(&m_userRect1, 16);
    ar->Write(&m_userRect2, 16);
    ar->Write(&m_pad110, 16);
    ar->Write(&m_reserved120, 16);
    ar->Write(&m_sparkleDelay, 4);
    ar->Write(&m_pad134, 4);
    ar->Write(&m_reserved138, 4);
    ar->Write(&m_reserved13c, 4);
    ar->Write(&m_reserved140, 4);
    ar->Write(&m_reserved144, 4);
    ar->Write(&m_reserved148, 4);
    ar->Write(&m_reserved14c, 4);
    ar->Write(&m_reserved150, 4);
    ar->Write(&m_reserved154, 4);
    ar->Write(&m_reserved158, 4);
    ar->Write(&m_reserved15c, 4);
    ar->Write(&m_reserved160, 4);
    ar->Write(&m_reserved164, 4);
    ar->Write(&m_targetId, 4);
    ar->Write(&m_payloadSize, 4);
    void* payload = m_payload;
    if (payload && m_payloadSize > 0) {
        ar->Write(payload, m_payloadSize);
    }
    return 1;
}

RVA(0x00164d80, 0x421)
i32 AnimWorkerObj::Load(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }
    ar->Read(&m_actKey, 4);
    ar->Read(&m_timeDelay, 4);
    ar->Read(&m_frameDelay, 4);
    ar->Read(&m_userFlags, 4);
    ar->Read(&m_minX, 4);
    ar->Read(&m_maxX, 4);
    ar->Read(&m_minY, 4);
    ar->Read(&m_maxY, 4);
    ar->Read(&m_pad3c, 4);
    ar->Read(&m_reserved40, 4);
    ar->Read(&m_tweakX, 4);
    ar->Read(&m_tweakY, 4);
    ar->Read(&m_scrollTargetX, 4);
    ar->Read(&m_scrollTargetY, 4);
    ar->Read(&m_pad54, 4);
    ar->Read(&m_reserved58, 4);
    ar->Read(&m_reserved5c, 4);
    ar->Read(&m_reserved60, 4);
    ar->Read(&m_user1, 4);
    ar->Read(&m_user2, 4);
    ar->Read(&m_user3, 4);
    ar->Read(&m_user4, 4);
    ar->Read(&m_user5, 4);
    ar->Read(&m_user6, 4);
    ar->Read(&m_user7, 4);
    ar->Read(&m_user8, 4);
    ar->Read(&m_reserved84, 4);
    ar->Read(&m_reserved88, 4);
    ar->Read(&m_reserved8c, 4);
    ar->Read(&m_reserved90, 4);
    ar->Read(&m_reserved94, 4);
    ar->Read(&m_reserved98, 4);
    ar->Read(&m_reserved9c, 4);
    ar->Read(&m_reserveda0, 4);
    ar->Read(&m_reserveda4, 4);
    ar->Read(&m_reserveda8, 4);
    ar->Read(&m_reservedac, 4);
    ar->Read(&m_reservedb0, 4);
    ar->Read(&m_reservedb4, 4);
    ar->Read(&m_counter, 4);
    ar->Read(&m_speed, 4);
    ar->Read(&m_padc0, 4);
    ar->Read(&m_reservedc4, 4);
    ar->Read(&m_width, 4);
    ar->Read(&m_height, 4);
    ar->Read(&m_reservedd0, 16);
    ar->Read(&m_reservede0, 16);
    ar->Read(&m_userRect1, 16);
    ar->Read(&m_userRect2, 16);
    ar->Read(&m_pad110, 16);
    ar->Read(&m_reserved120, 16);
    ar->Read(&m_sparkleDelay, 4);
    ar->Read(&m_pad134, 4);
    ar->Read(&m_reserved138, 4);
    ar->Read(&m_reserved13c, 4);
    ar->Read(&m_reserved140, 4);
    ar->Read(&m_reserved144, 4);
    ar->Read(&m_reserved148, 4);
    ar->Read(&m_reserved14c, 4);
    ar->Read(&m_reserved150, 4);
    ar->Read(&m_reserved154, 4);
    ar->Read(&m_reserved158, 4);
    ar->Read(&m_reserved15c, 4);
    ar->Read(&m_reserved160, 4);
    ar->Read(&m_reserved164, 4);
    ar->Read(&m_targetId, 4);
    ar->Read(&m_payloadSize, 4);
    if (m_payloadSize > 0) {
        m_payload = static_cast<u8*>(::operator new(m_payloadSize));
        ar->Read(m_payload, m_payloadSize);
    }
    return 1;
}

// @early-stop
RVA(0x001651b0, 0x5d)
i32 AnimWorkerObj::ResolveTarget(void* a) {
    if (a == NULL) {
        return 0;
    }
    if (m_targetId) {
        CMapPtrToPtr* res = &m_ownerCtx->m_childGroup->m_map48;
        void* out = 0;
        if (!MapLookupById(*res, m_targetId, out)) {
            m_target = NULL;
        } else {
            m_target = static_cast<CGameObject*>(out);
        }
    }
    return 1;
}
