#include <Gruntz/StateDispatch.h>
#include <rva.h>

#include <Gruntz/LevelTimeDtor.h>

class CUserLogic;

RVA(0x0009b770, 0xf1)
i32 StateDispatch(CGameObject* obj) {
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
            aux->m_logic->UserLogicVfunc9();
            break;
        case 0x1e:
            aux->m_logic->UserLogicVfunc8();
            break;
        case 0x50:
            aux->m_logic->UserLogicVfuncC();
            break;
        case 0x51:
            aux->m_logic->UserLogicVfuncB();
            break;
        case 0x52:
            aux->m_logic->UserLogicVfuncA();
            break;
        case 0x53:
            aux->m_logic->UserLogicVfuncD();
            break;
        case 0x3e8:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}
