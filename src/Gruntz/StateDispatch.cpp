#include <rva.h>

#include <Gruntz/StateDispatch.h>

#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/LevelTimeDtor.h>

class CUserLogic;

RVA(0x0009b770, 0xf1)
i32 CreateLevelTime(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;

    switch (static_cast<u32>(aux->ActKey())) {
        case 0: {
            aux->SetActKey(0x3e8);
            CLevelTime* h = new CLevelTime(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case 0x1d:
            aux->m_logic->OnObjectRemoved();
            break;
        case 0x1e:
            aux->m_logic->OnLeaveActiveRegion();
            break;
        case 0x50:
            aux->m_logic->PrepareSave();
            break;
        case 0x51:
            aux->m_logic->AfterSave();
            break;
        case 0x52:
            aux->m_logic->AfterLoad();
            break;
        case 0x53:
            aux->m_logic->AfterLoadReferences();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}
