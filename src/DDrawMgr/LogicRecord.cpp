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
    ar->Write(&m_actKey, sizeof(m_actKey));
    ar->Write(&m_timeDelay, sizeof(m_timeDelay));
    ar->Write(&m_frameDelay, sizeof(m_frameDelay));
    ar->Write(&m_userFlags, sizeof(m_userFlags));
    ar->Write(&m_minX, sizeof(m_minX));
    ar->Write(&m_maxX, sizeof(m_maxX));
    ar->Write(&m_minY, sizeof(m_minY));
    ar->Write(&m_maxY, sizeof(m_maxY));
    ar->Write(&m_pad3c, sizeof(m_pad3c));
    ar->Write(&m_reserved40, sizeof(m_reserved40));
    ar->Write(&m_tweakX, sizeof(m_tweakX));
    ar->Write(&m_tweakY, sizeof(m_tweakY));
    ar->Write(&m_scrollTargetX, sizeof(m_scrollTargetX));
    ar->Write(&m_scrollTargetY, sizeof(m_scrollTargetY));
    ar->Write(&m_pad54, sizeof(m_pad54));
    ar->Write(&m_reserved58, sizeof(m_reserved58));
    ar->Write(&m_reserved5c, sizeof(m_reserved5c));
    ar->Write(&m_reserved60, sizeof(m_reserved60));
    ar->Write(&m_user1, sizeof(m_user1));
    ar->Write(&m_user2, sizeof(m_user2));
    ar->Write(&m_user3, sizeof(m_user3));
    ar->Write(&m_user4, sizeof(m_user4));
    ar->Write(&m_user5, sizeof(m_user5));
    ar->Write(&m_user6, sizeof(m_user6));
    ar->Write(&m_user7, sizeof(m_user7));
    ar->Write(&m_user8, sizeof(m_user8));
    ar->Write(&m_reserved84, sizeof(m_reserved84));
    ar->Write(&m_reserved88, sizeof(m_reserved88));
    ar->Write(&m_reserved8c, sizeof(m_reserved8c));
    ar->Write(&m_reserved90, sizeof(m_reserved90));
    ar->Write(&m_reserved94, sizeof(m_reserved94));
    ar->Write(&m_reserved98, sizeof(m_reserved98));
    ar->Write(&m_reserved9c, sizeof(m_reserved9c));
    ar->Write(&m_reserveda0, sizeof(m_reserveda0));
    ar->Write(&m_reserveda4, sizeof(m_reserveda4));
    ar->Write(&m_reserveda8, sizeof(m_reserveda8));
    ar->Write(&m_reservedac, sizeof(m_reservedac));
    ar->Write(&m_reservedb0, sizeof(m_reservedb0));
    ar->Write(&m_reservedb4, sizeof(m_reservedb4));
    ar->Write(&m_counter, sizeof(m_counter));
    ar->Write(&m_speed, sizeof(m_speed));
    ar->Write(&m_padc0, sizeof(m_padc0));
    ar->Write(&m_reservedc4, sizeof(m_reservedc4));
    ar->Write(&m_width, sizeof(m_width));
    ar->Write(&m_height, sizeof(m_height));
    ar->Write(&m_reservedd0, sizeof(m_reservedd0));
    ar->Write(&m_reservede0, sizeof(m_reservede0));
    ar->Write(&m_userRect1, sizeof(m_userRect1));
    ar->Write(&m_userRect2, sizeof(m_userRect2));
    ar->Write(&m_pad110, sizeof(m_pad110));
    ar->Write(&m_reserved120, sizeof(m_reserved120));
    ar->Write(&m_sparkleDelay, sizeof(m_sparkleDelay));
    ar->Write(&m_pad134, sizeof(m_pad134));
    ar->Write(&m_reserved138, sizeof(m_reserved138));
    ar->Write(&m_reserved13c, sizeof(m_reserved13c));
    ar->Write(&m_reserved140, sizeof(m_reserved140));
    ar->Write(&m_reserved144, sizeof(m_reserved144));
    ar->Write(&m_reserved148, sizeof(m_reserved148));
    ar->Write(&m_reserved14c, sizeof(m_reserved14c));
    ar->Write(&m_reserved150, sizeof(m_reserved150));
    ar->Write(&m_reserved154, sizeof(m_reserved154));
    ar->Write(&m_reserved158, sizeof(m_reserved158));
    ar->Write(&m_reserved15c, sizeof(m_reserved15c));
    ar->Write(&m_reserved160, sizeof(m_reserved160));
    ar->Write(&m_reserved164, sizeof(m_reserved164));
    ar->Write(&m_targetId, sizeof(m_targetId));
    ar->Write(&m_payloadSize, sizeof(m_payloadSize));
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
    ar->Read(&m_actKey, sizeof(m_actKey));
    ar->Read(&m_timeDelay, sizeof(m_timeDelay));
    ar->Read(&m_frameDelay, sizeof(m_frameDelay));
    ar->Read(&m_userFlags, sizeof(m_userFlags));
    ar->Read(&m_minX, sizeof(m_minX));
    ar->Read(&m_maxX, sizeof(m_maxX));
    ar->Read(&m_minY, sizeof(m_minY));
    ar->Read(&m_maxY, sizeof(m_maxY));
    ar->Read(&m_pad3c, sizeof(m_pad3c));
    ar->Read(&m_reserved40, sizeof(m_reserved40));
    ar->Read(&m_tweakX, sizeof(m_tweakX));
    ar->Read(&m_tweakY, sizeof(m_tweakY));
    ar->Read(&m_scrollTargetX, sizeof(m_scrollTargetX));
    ar->Read(&m_scrollTargetY, sizeof(m_scrollTargetY));
    ar->Read(&m_pad54, sizeof(m_pad54));
    ar->Read(&m_reserved58, sizeof(m_reserved58));
    ar->Read(&m_reserved5c, sizeof(m_reserved5c));
    ar->Read(&m_reserved60, sizeof(m_reserved60));
    ar->Read(&m_user1, sizeof(m_user1));
    ar->Read(&m_user2, sizeof(m_user2));
    ar->Read(&m_user3, sizeof(m_user3));
    ar->Read(&m_user4, sizeof(m_user4));
    ar->Read(&m_user5, sizeof(m_user5));
    ar->Read(&m_user6, sizeof(m_user6));
    ar->Read(&m_user7, sizeof(m_user7));
    ar->Read(&m_user8, sizeof(m_user8));
    ar->Read(&m_reserved84, sizeof(m_reserved84));
    ar->Read(&m_reserved88, sizeof(m_reserved88));
    ar->Read(&m_reserved8c, sizeof(m_reserved8c));
    ar->Read(&m_reserved90, sizeof(m_reserved90));
    ar->Read(&m_reserved94, sizeof(m_reserved94));
    ar->Read(&m_reserved98, sizeof(m_reserved98));
    ar->Read(&m_reserved9c, sizeof(m_reserved9c));
    ar->Read(&m_reserveda0, sizeof(m_reserveda0));
    ar->Read(&m_reserveda4, sizeof(m_reserveda4));
    ar->Read(&m_reserveda8, sizeof(m_reserveda8));
    ar->Read(&m_reservedac, sizeof(m_reservedac));
    ar->Read(&m_reservedb0, sizeof(m_reservedb0));
    ar->Read(&m_reservedb4, sizeof(m_reservedb4));
    ar->Read(&m_counter, sizeof(m_counter));
    ar->Read(&m_speed, sizeof(m_speed));
    ar->Read(&m_padc0, sizeof(m_padc0));
    ar->Read(&m_reservedc4, sizeof(m_reservedc4));
    ar->Read(&m_width, sizeof(m_width));
    ar->Read(&m_height, sizeof(m_height));
    ar->Read(&m_reservedd0, sizeof(m_reservedd0));
    ar->Read(&m_reservede0, sizeof(m_reservede0));
    ar->Read(&m_userRect1, sizeof(m_userRect1));
    ar->Read(&m_userRect2, sizeof(m_userRect2));
    ar->Read(&m_pad110, sizeof(m_pad110));
    ar->Read(&m_reserved120, sizeof(m_reserved120));
    ar->Read(&m_sparkleDelay, sizeof(m_sparkleDelay));
    ar->Read(&m_pad134, sizeof(m_pad134));
    ar->Read(&m_reserved138, sizeof(m_reserved138));
    ar->Read(&m_reserved13c, sizeof(m_reserved13c));
    ar->Read(&m_reserved140, sizeof(m_reserved140));
    ar->Read(&m_reserved144, sizeof(m_reserved144));
    ar->Read(&m_reserved148, sizeof(m_reserved148));
    ar->Read(&m_reserved14c, sizeof(m_reserved14c));
    ar->Read(&m_reserved150, sizeof(m_reserved150));
    ar->Read(&m_reserved154, sizeof(m_reserved154));
    ar->Read(&m_reserved158, sizeof(m_reserved158));
    ar->Read(&m_reserved15c, sizeof(m_reserved15c));
    ar->Read(&m_reserved160, sizeof(m_reserved160));
    ar->Read(&m_reserved164, sizeof(m_reserved164));
    ar->Read(&m_targetId, sizeof(m_targetId));
    ar->Read(&m_payloadSize, sizeof(m_payloadSize));
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
