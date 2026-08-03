#include <rva.h>

#include <Gruntz/UserLogic.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <Enums.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

#include <string.h>

RVA(0x000087d0, 0x8)
i32 CUserBase::SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) {
    return 1;
}

RVA(0x000087f0, 0x3)
LogicTypeId CUserBase::GetTypeTag() {
    return static_cast<LogicTypeId>(0);
}

RVA(0x00008840, 0x4)
LogicTypeId CUserLogic::GetTypeTag() {
    return LOGIC_NONE;
}

RVA(0x000088d0, 0x1)
void CUserLogic::Activate() {}

RVA(0x000088f0, 0x6)
i32 CUserLogic::AdvanceAnimation() {
    return 1;
}

RVA(0x00008910, 0x6)
i32 CUserLogic::RecordFrameTick() {
    return 1;
}

RVA(0x00008930, 0x6)
i32 CUserLogic::StepAttackFire() {
    return 1;
}

RVA(0x00008950, 0x1)
void CUserLogic::OnLeaveActiveRegion() {}

RVA(0x00008970, 0x1)
void CUserLogic::OnObjectRemoved() {}

RVA(0x00008990, 0x1)
void CUserLogic::AfterLoad() {}

RVA(0x000089b0, 0x1)
void CUserLogic::AfterSave() {}

RVA(0x000089d0, 0x1)
void CUserLogic::PrepareSave() {}

RVA(0x000089f0, 0x1)
void CUserLogic::AfterLoadReferences() {}

// @early-stop
RVA(0x00008a40, 0xc8)
void CUserLogic::BuildLogicTypeTable(CGameObject* obj) {
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

RVA(0x00008b50, 0x3)
void CUserLogic::XferName(char* name) {}

RVA(0x00008b70, 0x3)
void CUserLogic::FireActivation(i32) {}

RVA(0x00008b90, 0x40)
void CUserLogic::FinalizeStep(char*) {
    if (m_deferredCallback == 0) {
        return;
    }
    if (m_gatedCallback != 0 && m_objAux->ActKey() == m_gatedActKey) {
        (this->*m_gatedCallback)();
        m_gatedCallback = 0;
    }
    (this->*m_deferredCallback)();
    m_deferredCallback = 0;
    m_gatedActKey = 0x3e9;
}

// @early-stop
RVA(0x00008c00, 0x152)
i32 CWapX::Chain(CFileMemBase* arc, SerialMode mode, LogicTypeId unused, CGameObject* obj) {
    char name[0x80];

    if (arc == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            arc->Read(name, 0x80);
            arc->Read(m_blob, 0x10);
            m_gameObject = obj;
            m_wwdObject = static_cast<CWwdGameObjectA*>(obj);
            m_animWorker = obj->m_animWorker;
            if (strlen(name) == 0) {
                m_value = 0;
                return 1;
            }

            CMapStringToPtr* map = &m_animWorker->m_ownerCtx->m_animRegistry->m_animations;
            void* val = 0;
            map->Lookup(name, val);
            m_value = static_cast<CAniElement*>(val);
            return 1;
        }
        case SERIAL_SAVE: {

            memset(name, 0, sizeof(name));
            if (m_value != 0) {
                CString nm = m_animWorker->m_ownerCtx->m_animRegistry->KeyOfValue(m_value);
                strcpy(name, static_cast<const char*>(nm));
            }
            arc->Write(name, 0x80);
            arc->Write(m_blob, 0x10);
            break;
        }
    }
    return 1;
}
