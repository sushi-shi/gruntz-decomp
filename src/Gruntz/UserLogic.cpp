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

// Keep this callable body synchronized with LogicTypeTableInline.h. The shared
// inline-only form does not emit this function; see rule-exceptions.tsv.
RVA(0x00008a40, 0xc8)
void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
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
