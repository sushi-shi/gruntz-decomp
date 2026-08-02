

#include <Gruntz/UserLogicCtorEmit.h>
#include <Mfc.h>
#include <Gruntz/UserLogic.h>
#include <rva.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>

RVA_COMPGEN(0x000138d0, 0x4b, ??0CUserLogic@@QAE@XZ)

// @early-stop
RVA_COMPGEN(0x00058cd0, 0x195, ??0CUserLogic@@QAE@PAUCGameObject@@@Z)

i32 LogicHitFactory(CGameObject* obj);

inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_workers.Lookup("LogicHit", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicHitFactory, "LogicHit", 2);
        }
    }
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_workers.Lookup("LogicAttack", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicAttackFactory, "LogicAttack", 2);
        }
    }
    {
        CObject* found = 0;
        obj->OwnerMgr()->m_workerCache->m_workers.Lookup("LogicBump", found);
        if (!found) {
            obj->OwnerMgr()->m_workerCache->CreateWorker(LogicBumpFactory, "LogicBump", 2);
        }
    }
}

static CUserLogic* volatile g_forceEmitSink;
#pragma inline_depth(0)
void ForceEmitCUserLogicNoArg() {
    g_forceEmitSink = new CUserLogic();
}

#pragma inline_depth(0)
void ForceEmitCUserLogic1Arg(CGameObject* o) {
    g_forceEmitSink = new CUserLogic(o);
}
#pragma inline_depth()
