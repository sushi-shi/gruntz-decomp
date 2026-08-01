#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>
#include <Gruntz/UserLogic.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>

extern "C" {
    i32 LogicHitFactory(CGameObject* obj);
    i32 LogicAttackFactory(CGameObject* obj);
    i32 LogicBumpFactory(CGameObject* obj);
}

inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {

    if (!obj->OwnerMgr()->m_workerCache->Find("LogicHit")) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicHitFactory, "LogicHit", 2);
    }
    if (!obj->OwnerMgr()->m_workerCache->Find("LogicAttack")) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicAttackFactory, "LogicAttack", 2);
    }
    if (!obj->OwnerMgr()->m_workerCache->Find("LogicBump")) {
        obj->OwnerMgr()->m_workerCache->CreateWorker(LogicBumpFactory, "LogicBump", 2);
    }
}

#endif // GRUNTZ_LOGICTYPETABLEINLINE_H
