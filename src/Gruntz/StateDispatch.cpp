#include <rva.h>

#include <Gruntz/StateDispatch.h>

#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/LevelTime.h>
#include <Wwd/AnimWorkerAct.h>

class CUserLogic;

VTBL(CLevelTime, 0x001e801c);

// CLevelTime's ctor is defined HERE, not in LevelTime.cpp: retail's copy sits at
// 0x9b8b0, immediately behind this TU's CreateLevelTime (0x9b770), so this is the
// TU that emits it - along with the rest of the class's COMDAT set (vtable, RTTI,
// ??_G, ??1), which constructing the object pulls in. Spelling it out-of-line
// here is required: as a header inline our cl flattens it into CreateLevelTime,
// while retail keeps a standalone 0x18f-byte body.
RVA_COMPGEN(0x00011a20, 0x1e, ??_GCLevelTime@@UAEPAXI@Z)
RVA_COMPGEN(0x00011a50, 0x44, ??1CLevelTime@@UAE@XZ)

// @identity-TODO _CreateLevelTime (241 B) sits outside this TU's block at 0x9b770, between
// IsSameWorld (areamgr) and ?0CLevelTime (statedispatch). No size-family and too
// large for a dtor pool - the placement is UNEXPLAINED; find its real owner.
RVA(0x0009b770, 0xf1)
i32 CreateLevelTime(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    AnimWorkerAct act = aux->WorkerAct();
    switch (act) {
        case ACT_UNINITIALISED: {
            aux->SetWorkerAct(ACT_LIVE);
            CLevelTime* h = new CLevelTime(obj);
            h->Activate();
            aux->m_logic = h;
            break;
        }
        case ACT_OBJECT_REMOVED:
            aux->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            aux->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            aux->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            aux->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            aux->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            aux->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            ProjTypeXfer(aux->m_logic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x0009b8b0, 0x18f)
CLevelTime::CLevelTime(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->m_flags |= 2;
}
