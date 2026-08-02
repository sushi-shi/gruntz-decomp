#include <rva.h>

#include <Gruntz/Boomerang.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/StaticHazard.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/XferArchive.h>
#include <Ints.h>

enum LogicRecordState {
    kLogicStateInit = 0,
    kLogicStateOp1d = 0x1d,
    kLogicStateOp1e = 0x1e,
    kLogicStateOp50 = 0x50,
    kLogicStateOp51 = 0x51,
    kLogicStateOp52 = 0x52,
    kLogicStateOp53 = 0x53,
    kLogicStateBuilt = 0x3e8,
};

inline void DispatchLogicAction(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

RVA(0x000de8a0, 0xf4)
i32 CreateProjectile(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case kLogicStateInit:
            rec->SetActKey(kLogicStateBuilt);
            {
                CUserLogic* obj = new CProjectile(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case kLogicStateOp1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case kLogicStateOp50:
            rec->m_logic->PrepareSave();
            break;
        case kLogicStateOp51:
            rec->m_logic->AfterSave();
            break;
        case kLogicStateOp52:
            rec->m_logic->AfterLoad();
            break;
        case kLogicStateOp53:
            rec->m_logic->AfterLoadReferences();
            break;
        case kLogicStateBuilt:
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
        case kLogicStateInit:
            rec->SetActKey(kLogicStateBuilt);
            {
                CUserLogic* obj = new CBoomerang(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case kLogicStateOp1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case kLogicStateOp50:
            rec->m_logic->PrepareSave();
            break;
        case kLogicStateOp51:
            rec->m_logic->AfterSave();
            break;
        case kLogicStateOp52:
            rec->m_logic->AfterLoad();
            break;
        case kLogicStateOp53:
            rec->m_logic->AfterLoadReferences();
            break;
        case kLogicStateBuilt:
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
        case kLogicStateInit:
            rec->SetActKey(kLogicStateBuilt);
            {
                CUserLogic* obj = new CTimeBomb(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case kLogicStateOp1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case kLogicStateOp50:
            rec->m_logic->PrepareSave();
            break;
        case kLogicStateOp51:
            rec->m_logic->AfterSave();
            break;
        case kLogicStateOp52:
            rec->m_logic->AfterLoad();
            break;
        case kLogicStateOp53:
            rec->m_logic->AfterLoadReferences();
            break;
        case kLogicStateBuilt:
            break;
        default:
            DispatchLogicAction(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000fb660, 0xf1)
i32 CreateStaticHazard(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case kLogicStateInit:
            rec->SetActKey(kLogicStateBuilt);
            {
                CUserLogic* obj = new CStaticHazard(owner);
                obj->Activate();
                rec->m_logic = obj;
            }
            break;
        case kLogicStateOp1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case kLogicStateOp1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case kLogicStateOp50:
            rec->m_logic->PrepareSave();
            break;
        case kLogicStateOp51:
            rec->m_logic->AfterSave();
            break;
        case kLogicStateOp52:
            rec->m_logic->AfterLoad();
            break;
        case kLogicStateOp53:
            rec->m_logic->AfterLoadReferences();
            break;
        case kLogicStateBuilt:
            break;
        default:
            DispatchLogicAction(rec->m_logic);
            break;
    }
    return 1;
}
