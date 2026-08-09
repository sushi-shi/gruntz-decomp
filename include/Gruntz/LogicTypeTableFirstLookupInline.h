#ifndef GRUNTZ_GRUNTZ_LOGICTYPETABLEFIRSTLOOKUPINLINE_H
#define GRUNTZ_GRUNTZ_LOGICTYPETABLEFIRSTLOOKUPINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/UserLogic.h>

inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    CObject* found = 0;
    obj->OwnerMgr()->m_workerCache->m_workers.Lookup("LogicHit", found);
    if (!found) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicHitFactory, "LogicHit", 2);
    }
    if (!obj->OwnerMgr()->m_workerCache->Find("LogicAttack")) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicAttackFactory, "LogicAttack", 2);
    }
    if (!obj->OwnerMgr()->m_workerCache->Find("LogicBump")) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicBumpFactory, "LogicBump", 2);
    }
}

#endif // GRUNTZ_GRUNTZ_LOGICTYPETABLEFIRSTLOOKUPINLINE_H
