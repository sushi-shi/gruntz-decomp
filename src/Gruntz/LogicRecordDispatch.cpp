#include <Ints.h>
#include <rva.h>

#include <Gruntz/StaticHazard.h>
#include <Gruntz/TimeBomb.h>
#include <Gruntz/Projectile.h>
#include <Gruntz/Boomerang.h>
#include <Gruntz/XferArchive.h>
#include <Gruntz/UserLogic.h>

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

inline void LogicSubDefault_16e4f0(CUserLogic* sub) {
    ProjTypeXfer(sub);
}

RVA(0x000de8a0, 0xf4)
i32 LogicDispatchE(CGameObject* owner) {
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
            rec->m_logic->UserLogicVfunc9();
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000de9e0, 0xf4)
i32 LogicDispatchBoomerang(CGameObject* owner) {
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
            rec->m_logic->UserLogicVfunc9();
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000deb20, 0xf1)
i32 LogicDispatchD(CGameObject* owner) {
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
            rec->m_logic->UserLogicVfunc9();
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000fb660, 0xf1)
i32 LogicDispatchA(CGameObject* owner) {
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
            rec->m_logic->UserLogicVfunc9();
            break;
        case kLogicStateOp1e:
            rec->m_logic->UserLogicVfunc8();
            break;
        case kLogicStateOp50:
            rec->m_logic->UserLogicVfuncC();
            break;
        case kLogicStateOp51:
            rec->m_logic->UserLogicVfuncB();
            break;
        case kLogicStateOp52:
            rec->m_logic->UserLogicVfuncA();
            break;
        case kLogicStateOp53:
            rec->m_logic->UserLogicVfuncD();
            break;
        case kLogicStateBuilt:
            break;
        default:
            LogicSubDefault_16e4f0(rec->m_logic);
            break;
    }
    return 1;
}
