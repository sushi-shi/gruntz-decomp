#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/FortressFlag.h>

#include <Enums.h>
#include <Globals.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/AnimSink.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/Particlez.h>
#include <Gruntz/ResolveNodeInline.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/WarlordOwner.h>
#include <Gruntz/WwdGameReg.h>
#include <Image/CImage.h>
#include <Rez/FrameClock.h>
#include <Wwd/LogicRecordEvent.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

template<>
CActReg CActRegPool<CFortressFlag>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

template<>
CActReg CActRegPool<CParticlez>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

template<>
CActReg CActRegPool<CExplosion>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CFortressFlag::CFortressFlag(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    CWwdSpriteObject* o = m_object;
    i32 v = o->m_frameImage->m_anchorY + o->m_screenY + 0x186a0;
    SET_SORT_KEY_IF_CHANGED(o, v)
    switch (static_cast<WarlordOwner>(m_object->m_smarts)) {
        case WARLORDZ_KING:
            SetImageSetByName("GAME_FORTRESSFLAGZ_KING");
            break;
        case WARLORDZ_NAPOLEAN:
            SetImageSetByName("GAME_FORTRESSFLAGZ_NAPOLEAN");
            break;
        case WARLORDZ_PATTON:
            SetImageSetByName("GAME_FORTRESSFLAGZ_PATTON");
            break;
        case WARLORDZ_VIKING:
            SetImageSetByName("GAME_FORTRESSFLAGZ_VIKING");
            break;
        default:
            SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
            return;
    }
    SET_ANIMATION_ACT("A");
    SwitchAnimationByName("GAME_CYCLE100", 0);
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_SKIP_COLLISION_KEEP_ACTIVE);
    i32 idx = IDX(g_gameReg->m_players[m_object->m_smarts].m_color);
    CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
    CWwdSpriteObject* spr = m_object;
    spr->SetDrawFill(SHADE_PAL_16, sel);
}

void CFortressFlag::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CFortressFlag::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CFortressFlag>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CFortressFlag::AdvanceAnim);
}

i32 CFortressFlag::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}

i32 CFortressFlag::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE_OR_RETURN(ar, mode, typeId, object)
    if (mode == SERIAL_POSTLOAD) {
        CWwdSpriteObject* spr = m_object;
        i32 idx = IDX(g_gameReg->m_players[spr->m_smarts].m_color);
        CShadeTable* sel = g_gameReg->m_spriteFactory->GetSel(idx, 0);
        spr = m_object;
        spr->SetDrawFill(SHADE_PAL_16, sel);
    }
    return 1;
}

template CActHandler& zDArray<CActHandler>::operator[](i32 id);

i32 DispatchParticlezLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CParticlez)
}

i32 DispatchExplosionLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CExplosion)
}

CParticlez::CParticlez(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_BEHIND)
    m_object->m_dirty.m_armed = 0;
}

void CParticlez::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CParticlez::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CParticlez>::s_table[id] = static_cast<i32 (CUserLogic::*)()>(&CParticlez::Update);
}

i32 CParticlez::Update() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* o = m_wwdObject;
    if (o->m_animationCursor.IsComplete()) {
        o->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
    }
    return 0;
}

CExplosion::CExplosion(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_EXPLOSION");
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
    m_object->m_dirty.m_armed = 0;
}

void CExplosion::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

void RegisterExplosionActions() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CExplosion>::s_table[id] = static_cast<CActHandler>(&CExplosion::Update);
}
