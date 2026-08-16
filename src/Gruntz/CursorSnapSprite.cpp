#include <rva.h>

#include <Gruntz/CursorSnapSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/AnimWorker.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/AnimWorkerAct.h>

RVA(0x00011880, 0x47)
i32 CCursorSnapSprite::SerializeMove(
    CFileMemBase* ar,
    SerialMode tag,
    LogicTypeId c,
    CGameObject* d
) {
    if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
        return 0;
    }
    return Chain(ar, tag, c, d) != 0;
}

RVA_COMPGEN(0x000118f0, 0x1e, ??_GCCursorSnapSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011920, 0x44, ??1CCursorSnapSprite@@UAE@XZ)

// @identity-TODO _CreateCursorSnapSprite (241 B) sits outside this TU's block at 0x3a200, between
// BltSelf (ddrawsurfacepair) and ?0CCursorSnapSprite (cursorsnapsprite). No size-family and too
// large for a dtor pool - the placement is UNEXPLAINED; find its real owner.
RVA(0x0003a200, 0xf1)
i32 CreateCursorSnapSprite(CGameObject* owner) {
    AnimWorkerObj* rec = owner->m_animWorker;
    switch (rec->WorkerAct()) {
        case ACT_UNINITIALISED: {
            rec->SetWorkerAct(ACT_LIVE);
            CUserLogic* sub = new CCursorSnapSprite(owner);
            sub->Activate();
            rec->m_logic = sub;
            break;
        }
        case ACT_OBJECT_REMOVED:
            rec->m_logic->OnObjectRemoved();
            break;
        case ACT_LEAVE_ACTIVE_REGION:
            rec->m_logic->OnLeaveActiveRegion();
            break;
        case ACT_PREPARE_SAVE:
            rec->m_logic->PrepareSave();
            break;
        case ACT_AFTER_LOAD_REFERENCES:
            rec->m_logic->AfterLoadReferences();
            break;
        case ACT_AFTER_LOAD:
            rec->m_logic->AfterLoad();
            break;
        case ACT_AFTER_SAVE:
            rec->m_logic->AfterSave();
            break;
        case ACT_LIVE:
            break;
        default:
            Worker_DefaultPump(rec->m_logic);
            break;
    }
    return 1;
}

RVA(0x0003a340, 0x16e)
CCursorSnapSprite::CCursorSnapSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    ApplyName("GAME_CURSORSNAPSPRITE");
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("GAME_SINGLEIMAGEANI", 0);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    SetObjectFlags(2);
    Hide();
}

RVA(0x0003a5b0, 0x102)
void CCursorSnapSprite::FireActivation(i32 id) {
    CActHandler* e = (CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id));
    if ((*e) != 0) {
        CActHandler* e2 = (CActRegPool<CCursorSnapSprite>::s_table.ResolveEntry(id));
        (this->*((*e2)))();
    }
}
