#include <rva.h>

#include <Gruntz/Boomerang.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicRecordState.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/StaticHazard.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h>
#include <Ints.h>

inline void DispatchLogicAction(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

RVA(0x000de8a0, 0xf4)
i32 CreateProjectile(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case LOGICREC_INIT:
            rec->SetActKey(LOGICREC_BUILT);
            {
                CUserLogic* obj = new CProjectile(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case LOGICREC_OP_1D:
            rec->m_logic->OnObjectRemoved();
            break;
        case LOGICREC_OP_1E:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case LOGICREC_OP_50:
            rec->m_logic->PrepareSave();
            break;
        case LOGICREC_OP_51:
            rec->m_logic->AfterSave();
            break;
        case LOGICREC_OP_52:
            rec->m_logic->AfterLoad();
            break;
        case LOGICREC_OP_53:
            rec->m_logic->AfterLoadReferences();
            break;
        case LOGICREC_BUILT:
            break;
        default:
            DispatchLogicAction(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000de9e0, 0xf4)
i32 CreateBoomerang(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case LOGICREC_INIT:
            rec->SetActKey(LOGICREC_BUILT);
            {
                CUserLogic* obj = new CBoomerang(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case LOGICREC_OP_1D:
            rec->m_logic->OnObjectRemoved();
            break;
        case LOGICREC_OP_1E:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case LOGICREC_OP_50:
            rec->m_logic->PrepareSave();
            break;
        case LOGICREC_OP_51:
            rec->m_logic->AfterSave();
            break;
        case LOGICREC_OP_52:
            rec->m_logic->AfterLoad();
            break;
        case LOGICREC_OP_53:
            rec->m_logic->AfterLoadReferences();
            break;
        case LOGICREC_BUILT:
            break;
        default:
            DispatchLogicAction(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000deb20, 0xf1)
i32 CreateTimeBomb(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case LOGICREC_INIT:
            rec->SetActKey(LOGICREC_BUILT);
            {
                CUserLogic* obj = new CTimeBomb(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case LOGICREC_OP_1D:
            rec->m_logic->OnObjectRemoved();
            break;
        case LOGICREC_OP_1E:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case LOGICREC_OP_50:
            rec->m_logic->PrepareSave();
            break;
        case LOGICREC_OP_51:
            rec->m_logic->AfterSave();
            break;
        case LOGICREC_OP_52:
            rec->m_logic->AfterLoad();
            break;
        case LOGICREC_OP_53:
            rec->m_logic->AfterLoadReferences();
            break;
        case LOGICREC_BUILT:
            break;
        default:
            DispatchLogicAction(rec->m_logic);
            break;
    }
    return 1;
}
