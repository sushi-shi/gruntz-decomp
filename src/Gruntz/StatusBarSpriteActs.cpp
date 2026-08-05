#include <rva.h>

#include <Gruntz/StatusBarSpriteActs.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/StatusBarSprite.h>
#include <Gruntz/TileTriggerTransition.h>
#include <Gruntz/UserLogic.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>
#include <Wwd/AnimWorkerAct.h>

#include <stddef.h>

template<> DATA(0x0024e670)
CActReg CActRegPool<CStatusBarSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

VTBL(CStatusBarSprite, 0x001e7fc4);

// @interleaver GetTypeTag - fixed-size generated body (6 B, byte-identical across
// 67 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011ac0, 0x6)
LogicTypeId CStatusBarSprite::GetTypeTag() {
    return LOGIC_STATUSBARSPRITE;
}

// @interleaver SerializeMove - fixed-size generated body (71 B, byte-identical across
// 29 classes), so every TU emits one and the linker folds them to first use.
RVA(0x00011ae0, 0x47)
i32 CStatusBarSprite::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (!CUserLogic::SerializeMove(ar, mode, typeId, pObj)) {
        return 0;
    }
    return Chain(ar, mode, typeId, pObj) != 0;
}
RVA_COMPGEN(0x00011b50, 0x1e, ??_GCStatusBarSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011b80, 0x44, ??1CStatusBarSprite@@UAE@XZ)

RVA(0x0010c0f0, 0xf1)
i32 CreateStatusBarSprite(CGameObject* obj) {
    AnimWorkerObj* ctl = obj->m_animWorker;
    AnimWorkerAct act = static_cast<AnimWorkerAct>(ctl->ActKey());
    switch (act) {
        case ACT_UNINITIALISED: {
            ctl->SetActKey(IDX(ACT_LIVE));
            CStatusBarSprite* t = new CStatusBarSprite(obj);
            t->Activate();
            ctl->m_logic = t;
            break;
        }
        case ACT_OBJECT_REMOVED:
            ctl->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            ctl->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            ctl->m_logic->PrepareSave();
            break;
        case ACT_AFTER_SAVE:
            ctl->m_logic->AfterSave();
            break;
        case ACT_AFTER_LOAD:
            ctl->m_logic->AfterLoad();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            ctl->m_logic->AfterLoadReferences();
            break;
        case ACT_LIVE:
            break;
        default:
            ProjTypeXfer(ctl->m_logic);
            break;
    }
    return 1;
}

// @early-stop
RVA(0x0010c230, 0x178)
CStatusBarSprite::CStatusBarSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_wwdObject->ApplyName("GAME_STATUSBARSPRITE");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_SINGLEIMAGEANI", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    if (m_object->m_sortKey != SORTKEY_OVERLAY) {
        m_object->m_sortKey = SORTKEY_OVERLAY;
        m_object->m_flags |= 0x20000;
    }
}

RVA(0x0010c4b0, 0x102)
void CStatusBarSprite::FireActivation(i32 coord) {
    CActHandler* e = (CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(coord));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(coord));
        (this->*((*e2)))();
    }
}

RVA(0x0010c610, 0x18d)
void CStatusBarSprite::RegisterActs() {
    i32 id = ActFindId("A");
    if (id == 0) {
        ActInsertId("A", g_typeCounter);
        id = g_typeCounter;
        CString* slot = ActNameLookup(g_typeCounter);
        i32 n = g_typeColl.m_grown;
        CString* list = ActNameSlots();
        while (n-- != 0) {
            if (list != NULL) {
                list->CString::~CString();
            }
            list++;
        }
        *slot = "A";
        g_typeCounter++;
    }
    (*((CActRegPool<CStatusBarSprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CStatusBarSprite::AdvanceAnim);
}

RVA(0x0010c810, 0x17)
i32 CStatusBarSprite::AdvanceAnim() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    return 0;
}
