#include <rva.h>

#include <Gruntz/Boomerang.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicRecordState.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/StaticHazard.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

inline void DispatchUnhandledLogicEvent(CUserLogic* sub) {
    DispatchLogicEvent(sub);
}

RVA(0x000de8a0, 0xf4)
i32 DispatchProjectileLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (static_cast<u32>(record->EventCode())) {
        case LOGICREC_INIT:
            record->SetEventCode(LOGICREC_BUILT);
            {
                CUserLogic* obj = new CProjectile(owner);
                obj->Activate();
                record->m_userLogic = obj;
            }
            break;
        case LOGICREC_OP_1D:
            record->m_userLogic->OnObjectRemoved();
            break;
        case LOGICREC_OP_1E:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case LOGICREC_OP_50:
            record->m_userLogic->PrepareSave();
            break;
        case LOGICREC_OP_51:
            record->m_userLogic->AfterSave();
            break;
        case LOGICREC_OP_52:
            record->m_userLogic->AfterLoad();
            break;
        case LOGICREC_OP_53:
            record->m_userLogic->AfterLoadReferences();
            break;
        case LOGICREC_BUILT:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x000de9e0, 0xf4)
i32 DispatchBoomerangLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (static_cast<u32>(record->EventCode())) {
        case LOGICREC_INIT:
            record->SetEventCode(LOGICREC_BUILT);
            {
                CUserLogic* obj = new CBoomerang(owner);
                obj->Activate();
                record->m_userLogic = obj;
            }
            break;
        case LOGICREC_OP_1D:
            record->m_userLogic->OnObjectRemoved();
            break;
        case LOGICREC_OP_1E:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case LOGICREC_OP_50:
            record->m_userLogic->PrepareSave();
            break;
        case LOGICREC_OP_51:
            record->m_userLogic->AfterSave();
            break;
        case LOGICREC_OP_52:
            record->m_userLogic->AfterLoad();
            break;
        case LOGICREC_OP_53:
            record->m_userLogic->AfterLoadReferences();
            break;
        case LOGICREC_BUILT:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x000deb20, 0xf1)
i32 DispatchTimeBombLogic(CGameObject* owner) {
    CLogicRecord* record = owner->m_logicRecord;
    switch (static_cast<u32>(record->EventCode())) {
        case LOGICREC_INIT:
            record->SetEventCode(LOGICREC_BUILT);
            {
                CUserLogic* obj = new CTimeBomb(owner);
                obj->Activate();
                record->m_userLogic = obj;
            }
            break;
        case LOGICREC_OP_1D:
            record->m_userLogic->OnObjectRemoved();
            break;
        case LOGICREC_OP_1E:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case LOGICREC_OP_50:
            record->m_userLogic->PrepareSave();
            break;
        case LOGICREC_OP_51:
            record->m_userLogic->AfterSave();
            break;
        case LOGICREC_OP_52:
            record->m_userLogic->AfterLoad();
            break;
        case LOGICREC_OP_53:
            record->m_userLogic->AfterLoadReferences();
            break;
        case LOGICREC_BUILT:
            break;
        default:
            DispatchUnhandledLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}
