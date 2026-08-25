#include <rva.h>

#include <Gruntz/UserLogic.h>

#include <Mfc.h>

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

// @early-stop
// 96.36: only instruction scheduling around the CMapStringToPtr::Lookup call
// (retail pushes &val before computing the name `lea`).
RVA(0x00008c00, 0x152)
i32 CWapX::Chain(CFileMemBase* arc, SerialMode mode, LogicTypeId unused, CGameObject* obj) {
    char name[SERIAL_NAME_LEN];

    if (arc == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            arc->Read(name, SERIAL_NAME_LEN);
            arc->Read(m_blob, 0x10);
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
            m_ownerLogicRecord = obj->m_logicRecord;
            if (strlen(name) == 0) {
                m_value = NULL;
            } else {
                CMapStringToPtr* map =
                    &m_ownerLogicRecord->m_ownerCtx->m_animRegistry->m_animations;
                CAniElement* value = NULL;
                MapLookup(*map, name, value);
                m_value = value;
            }
            break;
        }
        case SERIAL_SAVE: {

            memset(name, 0, sizeof(name));
            if (m_value != NULL) {
                strcpy(
                    name,
                    static_cast<const char*>(
                        m_ownerLogicRecord->m_ownerCtx->m_animRegistry->FindAnimationKey(m_value)
                    )
                );
            }
            arc->Write(name, SERIAL_NAME_LEN);
            arc->Write(m_blob, 0x10);
            break;
        }
    }
    return 1;
}
