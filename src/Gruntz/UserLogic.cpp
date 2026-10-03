#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/UserLogic.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <DDrawMgr/LogicRecordRegistryFindInline.h>
#include <Enums.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <Utils/MapTyped.h>

#include <string.h>

RVA(0x00008a40, 0xc8)
void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
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

RVA(0x00008b50, 0x3)
void CUserLogic::StepBehavior(char* animationActName) {}

RVA(0x00008b70, 0x3)
void CUserLogic::FireActivation(i32) {}

RVA(0x00008b90, 0x40)
void CUserLogic::FinalizeStep(char*) {
    if (m_deferredCallback == NULL) {
        return;
    }
    if (m_gatedCallback != NULL && m_logicRecord->EventCode() == m_gatedCallbackCode) {
        (this->*m_gatedCallback)();
        m_gatedCallback = NULL;
    }
    (this->*m_deferredCallback)();
    m_deferredCallback = NULL;
    m_gatedCallbackCode = IDX(ACT_NONE);
}