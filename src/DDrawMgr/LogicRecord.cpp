#include <DDrawMgr/LogicRecord.h>
#include <DDrawMgr/AnimWorkerObj.h>
#include <Utils/MapTyped.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>
#include <rva.h>
#include <Mfc.h>
#include <Io/FileMem.h>

// @early-stop
RVA(0x00164830, 0xec)
i32 AnimWorkerObj::Dispatch(CFileMemBase* a, i32 mode, i32 c, void* d) {
    if (a == 0) {
        return 0;
    }
    switch (mode) {
        case 3:
            m_targetId = 0;
            if (m_target) {
                m_targetId = m_target->m_188;
            }
            break;
        case 4:

            if (Save(a) == 0) {
                return 0;
            }
            break;
        case 7:

            if (Load(a) == 0) {
                return 0;
            }
            break;
        case 8:
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
    if (a == 0) {
        return 0;
    }
    m_targetId = 0;
    if (m_target) {
        m_targetId = m_target->m_188;
    }
    return 1;
}

RVA(0x00164960, 0x41a)
i32 AnimWorkerObj::Save(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }
    ar->Write(&m_1c, 4);
    ar->Write(&m_20, 4);
    ar->Write(&m_24, 4);
    ar->Write(&m_28, 4);
    ar->Write(&m_2c, 4);
    ar->Write(&m_30, 4);
    ar->Write(&m_34, 4);
    ar->Write(&m_38, 4);
    ar->Write(&m_pad3c, 4);
    ar->Write(&m_40, 4);
    ar->Write(&m_44, 4);
    ar->Write(&m_48, 4);
    ar->Write(&m_scrollTargetX, 4);
    ar->Write(&m_scrollTargetY, 4);
    ar->Write(&m_pad54, 4);
    ar->Write(&m_58, 4);
    ar->Write(&m_5c, 4);
    ar->Write(&m_60, 4);
    ar->Write(&m_64, 4);
    ar->Write(&m_68, 4);
    ar->Write(&m_6c, 4);
    ar->Write(&m_70, 4);
    ar->Write(&m_74, 4);
    ar->Write(&m_78, 4);
    ar->Write(&m_7c, 4);
    ar->Write(&m_80, 4);
    ar->Write(&m_84, 4);
    ar->Write(&m_88, 4);
    ar->Write(&m_8c, 4);
    ar->Write(&m_90, 4);
    ar->Write(&m_94, 4);
    ar->Write(&m_98, 4);
    ar->Write(&m_9c, 4);
    ar->Write(&m_a0, 4);
    ar->Write(&m_a4, 4);
    ar->Write(&m_a8, 4);
    ar->Write(&m_ac, 4);
    ar->Write(&m_b0, 4);
    ar->Write(&m_b4, 4);
    ar->Write(&m_b8, 4);
    ar->Write(&m_bc, 4);
    ar->Write(&m_padc0, 4);
    ar->Write(&m_c4, 4);
    ar->Write(&m_c8, 4);
    ar->Write(&m_cc, 4);
    ar->Write(&m_d0, 16);
    ar->Write(&m_e0, 16);
    ar->Write(&m_switchRectA, 16);
    ar->Write(&m_switchRectB, 16);
    ar->Write(&m_pad110, 16);
    ar->Write(&m_120, 16);
    ar->Write(&m_130, 4);
    ar->Write(&m_pad134, 4);
    ar->Write(&m_138, 4);
    ar->Write(&m_13c, 4);
    ar->Write(&m_140, 4);
    ar->Write(&m_144, 4);
    ar->Write(&m_148, 4);
    ar->Write(&m_14c, 4);
    ar->Write(&m_150, 4);
    ar->Write(&m_154, 4);
    ar->Write(&m_158, 4);
    ar->Write(&m_15c, 4);
    ar->Write(&m_160, 4);
    ar->Write(&m_164, 4);
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
    if (ar == 0) {
        return 0;
    }
    ar->Read(&m_1c, 4);
    ar->Read(&m_20, 4);
    ar->Read(&m_24, 4);
    ar->Read(&m_28, 4);
    ar->Read(&m_2c, 4);
    ar->Read(&m_30, 4);
    ar->Read(&m_34, 4);
    ar->Read(&m_38, 4);
    ar->Read(&m_pad3c, 4);
    ar->Read(&m_40, 4);
    ar->Read(&m_44, 4);
    ar->Read(&m_48, 4);
    ar->Read(&m_scrollTargetX, 4);
    ar->Read(&m_scrollTargetY, 4);
    ar->Read(&m_pad54, 4);
    ar->Read(&m_58, 4);
    ar->Read(&m_5c, 4);
    ar->Read(&m_60, 4);
    ar->Read(&m_64, 4);
    ar->Read(&m_68, 4);
    ar->Read(&m_6c, 4);
    ar->Read(&m_70, 4);
    ar->Read(&m_74, 4);
    ar->Read(&m_78, 4);
    ar->Read(&m_7c, 4);
    ar->Read(&m_80, 4);
    ar->Read(&m_84, 4);
    ar->Read(&m_88, 4);
    ar->Read(&m_8c, 4);
    ar->Read(&m_90, 4);
    ar->Read(&m_94, 4);
    ar->Read(&m_98, 4);
    ar->Read(&m_9c, 4);
    ar->Read(&m_a0, 4);
    ar->Read(&m_a4, 4);
    ar->Read(&m_a8, 4);
    ar->Read(&m_ac, 4);
    ar->Read(&m_b0, 4);
    ar->Read(&m_b4, 4);
    ar->Read(&m_b8, 4);
    ar->Read(&m_bc, 4);
    ar->Read(&m_padc0, 4);
    ar->Read(&m_c4, 4);
    ar->Read(&m_c8, 4);
    ar->Read(&m_cc, 4);
    ar->Read(&m_d0, 16);
    ar->Read(&m_e0, 16);
    ar->Read(&m_switchRectA, 16);
    ar->Read(&m_switchRectB, 16);
    ar->Read(&m_pad110, 16);
    ar->Read(&m_120, 16);
    ar->Read(&m_130, 4);
    ar->Read(&m_pad134, 4);
    ar->Read(&m_138, 4);
    ar->Read(&m_13c, 4);
    ar->Read(&m_140, 4);
    ar->Read(&m_144, 4);
    ar->Read(&m_148, 4);
    ar->Read(&m_14c, 4);
    ar->Read(&m_150, 4);
    ar->Read(&m_154, 4);
    ar->Read(&m_158, 4);
    ar->Read(&m_15c, 4);
    ar->Read(&m_160, 4);
    ar->Read(&m_164, 4);
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
    if (a == 0) {
        return 0;
    }
    if (m_targetId) {
        CMapPtrToPtr* res = &m_ownerCtx->m_childGroup->m_map48;
        void* out = 0;
        if (!MapLookupById(*res, m_targetId, out)) {
            m_target = 0;
        } else {
            m_target = static_cast<CGameObject*>(out);
        }
    }
    return 1;
}
