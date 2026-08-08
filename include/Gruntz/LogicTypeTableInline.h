#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/UserLogic.h>

// The INLINED shape.  It is not the same instruction stream as the out-of-line
// copy in UserLogic.cpp (0x8a40) and it must not be: when this body is expanded
// into a derived ctor, retail leaves `CDDrawWorkerCache::Find` as a CALL, while
// the standalone 0x8a40 copy has that same `Find` expanded in place (the
// `add ecx,0x10` + 2-arg `CMapStringToOb::Lookup`).  Same source, two inline
// depths - so this header carries the `Find()` spelling and UserLogic.cpp
// carries the already-expanded one.
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
