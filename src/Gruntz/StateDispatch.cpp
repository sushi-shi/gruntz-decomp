#include <rva.h>

#include <Gruntz/LevelTimeDtor.h> // canonical CLevelTime : CTileLogic : CUserLogic (+ CGameObject/AnimWorkerObj)

class CUserLogic;
i32 ProjTypeXfer(CUserLogic* logic); // 0x16e4f0 (the "archive" IS the logic)

RVA(0x0009b770, 0xf1)
i32 StateDispatch(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_7c;
    // aux->m_1c doubles as the state id here (0 / 0x1d / 0x1e / 0x50..0x53 / 0x3e8)
    // - the same proven-heterogeneous aux slot other sprite classes use as a
    // lookup-node pointer; kept generically typed in the canonical (documented
    // variant), read/written through the int view at this site.
    switch (reinterpret_cast<u32>(aux->m_1c)) {
        case 0: {
            aux->m_1c = reinterpret_cast<void*>(0x3e8);
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
