#include <rva.h>

#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/ToyPeek.h>
#include <Gruntz/WorkerHandler.h>
#include <Wwd/AnimWorkerAct.h>

RVA(0x00095750, 0xf4)
i32 CreateInGameIcon(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (rec->WorkerAct()) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
            CUserLogic* sub = new CInGameIcon(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x00095890, 0xf1)
i32 CreateInGameText(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (rec->WorkerAct()) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
            CUserLogic* sub = new CInGameText(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x000959d0, 0xf1)
i32 CreateToyPeek(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (rec->WorkerAct()) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
            CUserLogic* sub = new CToyPeek(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}
