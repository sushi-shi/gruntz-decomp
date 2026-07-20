#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

extern "C" {
    i32 LogicHitFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)    // 0x56e4c0
    i32 LogicAttackFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant) // 0x56e4d0
    i32 LogicBumpFactory(CGameObject* obj); // GameObjNotifyFn ABI (CreateWorker registrant)   // 0x56e4e0
}

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>

inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    // Each block re-reads world->m_workerCache for BOTH the Find and the
    // CreateWorker call (retail reloads the chain at each site - do not hoist).
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
