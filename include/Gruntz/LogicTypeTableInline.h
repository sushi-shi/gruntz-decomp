#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/UserLogic.h>

// Keep this selectively visible body synchronized with UserLogic.cpp; removing
// it changes the constructor call boundaries. See rule-exceptions.tsv.
inline void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
    if (!obj->GetWorld()->GetLogicRegistry()->FindTemplate("LogicHit")) {
        obj->GetWorld()->GetLogicRegistry()->RegisterLogicType(DispatchLogicHit, "LogicHit", 2);
    }
    if (!obj->GetWorld()->GetLogicRegistry()->FindTemplate("LogicAttack")) {
        obj->GetWorld()->GetLogicRegistry()->RegisterLogicType(
            DispatchLogicAttack,
            "LogicAttack",
            2
        );
    }
    if (!obj->GetWorld()->GetLogicRegistry()->FindTemplate("LogicBump")) {
        obj->GetWorld()->GetLogicRegistry()->RegisterLogicType(DispatchLogicBump, "LogicBump", 2);
    }
}

#endif // GRUNTZ_LOGICTYPETABLEINLINE_H
