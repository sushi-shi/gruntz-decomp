#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/UserLogic.h>

inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    if (!obj->OwnerMgr()->m_logicRegistry->FindTemplate("LogicHit")) {
        obj->OwnerMgr()->m_logicRegistry->RegisterLogicType(DispatchLogicHit, "LogicHit", 2);
    }
    if (!obj->OwnerMgr()->m_logicRegistry->FindTemplate("LogicAttack")) {
        obj->OwnerMgr()->m_logicRegistry->RegisterLogicType(DispatchLogicAttack, "LogicAttack", 2);
    }
    if (!obj->OwnerMgr()->m_logicRegistry->FindTemplate("LogicBump")) {
        obj->OwnerMgr()->m_logicRegistry->RegisterLogicType(DispatchLogicBump, "LogicBump", 2);
    }
}

#endif // GRUNTZ_LOGICTYPETABLEINLINE_H
