#include <rva.h>

#include <Gruntz/StatusBarSpriteActs.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/StatusBarSprite.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/UserLogic.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>
#include <Wwd/LogicRecordEvent.h>

#include <stddef.h>

RVA_DYNINIT(0x0010c410, 0xa, CActRegPool<CStatusBarSprite>::s_table)
RVA_DYNINIT(0x0010c430, 0x15, CActRegPool<CStatusBarSprite>::s_table)
RVA_DYNINIT(0x0010c460, 0xe, CActRegPool<CStatusBarSprite>::s_table)
RVA_DYNINIT(0x0010c480, 0x1f, CActRegPool<CStatusBarSprite>::s_table)
template<> DATA(0x0024e670)
CActReg CActRegPool<CStatusBarSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00011b50, 0x1e, ??_GCStatusBarSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011b80, 0x44, ??1CStatusBarSprite@@UAE@XZ)

RVA(0x0010c0f0, 0xf1)
i32 DispatchStatusBarSpriteLogic(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    switch (record->LogicEvent()) {
        case ACT_UNINITIALISED: {
            record->SetLogicEvent(ACT_LIVE);
            CStatusBarSprite* t = new CStatusBarSprite(obj);
            t->Activate();
            record->m_userLogic = t;
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

RVA(0x0010c230, 0x178)
CStatusBarSprite::CStatusBarSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    ApplyName("GAME_STATUSBARSPRITE");
    SwitchGeometry("GAME_SINGLEIMAGEANI", 0);
    SET_ANIMATION_ACT("A");
    CWwdGameObjectA* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
}

RVA(0x0010c4b0, 0x102)
void CStatusBarSprite::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(coord));
    if ((*e) != NULL) {
        CActHandler* e2 = (CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010c610, 0x18d)
void CStatusBarSprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CStatusBarSprite::AdvanceAnim);
}

RVA(0x0010c810, 0x17)
i32 CStatusBarSprite::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}
