#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <Gruntz/UserLogic.h>

extern "C" {
    i32 LogicHitFactory(CGameObject* obj);
    i32 LogicAttackFactory(CGameObject* obj);
    i32 LogicBumpFactory(CGameObject* obj);
}

// Byte-for-byte the same body as the out-of-line copy in UserLogic.cpp
// (0x8a40) - one function, one shape.  The `Find()` spelling this used to carry
// is a DIFFERENT shape: retail expands the CMapStringToOb::Lookup into a local
// at every one of the three tests.
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

#endif // GRUNTZ_LOGICTYPETABLEINLINE_H
