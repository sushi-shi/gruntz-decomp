#include <rva.h>

#include <Gruntz/StateDispatch.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LevelTime.h>
#include <Gruntz/LogicTypeTableInline.h>
#include <Wwd/LogicRecordEvent.h>

class CUserLogic;

RVA_COMPGEN(0x00011a30, 0x1e, ??_GCLevelTime@@UAEPAXI@Z)
RVA_COMPGEN(0x00011a60, 0x44, ??1CLevelTime@@UAE@XZ)

RVA(0x0009b690, 0xf1)
i32 DispatchLevelTimeLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CLevelTime* h = new CLevelTime(obj);
            h->Activate();
            record->m_userLogic = h;
            break;
        }
        case ACT_OBJECT_REMOVED:
            record->m_userLogic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            record->m_userLogic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            record->m_userLogic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            record->m_userLogic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            record->m_userLogic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            record->m_userLogic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            DispatchLogicEvent(record->m_userLogic);
            break;
    }
    return 1;
}

RVA(0x0009b7d0, 0x18f)
CLevelTime::CLevelTime(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
}
