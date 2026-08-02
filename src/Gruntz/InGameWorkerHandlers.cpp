#include <rva.h>

#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/ToyPeek.h>
#include <Gruntz/WorkerHandler.h>

RVA(0x00095750, 0xf4)
i32 CreateInGameIcon(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CInGameIcon(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            rec->m_logic->PrepareSave();
            break;
        case 0x53:
            rec->m_logic->AfterLoadReferences();
            break;
        case 0x52:
            rec->m_logic->AfterLoad();
            break;
        case 0x51:
            rec->m_logic->AfterSave();
            break;
        case 0x3e8:
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
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CInGameText(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            rec->m_logic->PrepareSave();
            break;
        case 0x53:
            rec->m_logic->AfterLoadReferences();
            break;
        case 0x52:
            rec->m_logic->AfterLoad();
            break;
        case 0x51:
            rec->m_logic->AfterSave();
            break;
        case 0x3e8:
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
    switch (static_cast<u32>(rec->ActKey())) {
        case 0: {
            rec->SetActKey(0x3e8);
            CUserLogic* sub = new CToyPeek(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case 0x1d:
            rec->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            rec->m_logic->PrepareSave();
            break;
        case 0x53:
            rec->m_logic->AfterLoadReferences();
            break;
        case 0x52:
            rec->m_logic->AfterLoad();
            break;
        case 0x51:
            rec->m_logic->AfterSave();
            break;
        case 0x3e8:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}
