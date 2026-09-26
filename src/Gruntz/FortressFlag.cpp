#include <rva.h>

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

RVA_DYNINIT(0x00046580, 0x5, s_gruntDirNorth)
RVA_DYNINIT(0x000465a0, 0x1a, s_gruntDirNorth)
RVA_DYNINIT(0x000465d0, 0x5, s_gruntDirNorthEast)
RVA_DYNINIT(0x000465f0, 0x1a, s_gruntDirNorthEast)
RVA_DYNINIT(0x00046620, 0x5, s_gruntDirEast)
RVA_DYNINIT(0x00046640, 0x1f, s_gruntDirEast)
RVA_DYNINIT(0x00046670, 0x5, s_gruntDirSouthEast)
RVA_DYNINIT(0x00046690, 0x1a, s_gruntDirSouthEast)
RVA_DYNINIT(0x000466c0, 0x5, s_gruntDirSouth)
RVA_DYNINIT(0x000466e0, 0x1f, s_gruntDirSouth)
RVA_DYNINIT(0x00046710, 0x5, s_gruntDirSouthWest)
RVA_DYNINIT(0x00046730, 0x1f, s_gruntDirSouthWest)
RVA_DYNINIT(0x00046760, 0x5, s_gruntDirWest)
RVA_DYNINIT(0x00046780, 0x1f, s_gruntDirWest)
RVA_DYNINIT(0x000467b0, 0x5, s_gruntDirNorthWest)
RVA_DYNINIT(0x000467d0, 0x17, s_gruntDirNorthWest)
RVA_DYNINIT(0x00046800, 0x5, s_gruntDirCenter)
RVA_DYNINIT(0x00046820, 0x1a, s_gruntDirCenter)

RVA_DYNINIT(0x00045fe0, 0xa, CActRegPool<CFortressFlag>::s_table)
RVA_DYNINIT(0x00046000, 0x15, CActRegPool<CFortressFlag>::s_table)
RVA_DYNINIT(0x00046030, 0xe, CActRegPool<CFortressFlag>::s_table)
RVA_DYNINIT(0x00046050, 0x1f, CActRegPool<CFortressFlag>::s_table)
template<> DATA(0x00244638)
CActReg CActRegPool<CFortressFlag>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x00046c90, 0xa, CActRegPool<CParticlez>::s_table)
RVA_DYNINIT(0x00046cb0, 0x15, CActRegPool<CParticlez>::s_table)
RVA_DYNINIT(0x00046ce0, 0xe, CActRegPool<CParticlez>::s_table)
RVA_DYNINIT(0x00046d00, 0x1f, CActRegPool<CParticlez>::s_table)
template<> DATA(0x00244870)
CActReg CActRegPool<CParticlez>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_DYNINIT(0x000472b0, 0xa, CActRegPool<CExplosion>::s_table)
RVA_DYNINIT(0x000472d0, 0x15, CActRegPool<CExplosion>::s_table)
RVA_DYNINIT(0x00047300, 0xe, CActRegPool<CExplosion>::s_table)
RVA_DYNINIT(0x00047320, 0x1f, CActRegPool<CExplosion>::s_table)
template<> DATA(0x002447f8)
CActReg CActRegPool<CExplosion>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00010e60, 0x1e, ??_GCFortressFlag@@UAEPAXI@Z)
RVA_COMPGEN(0x00010e90, 0x44, ??1CFortressFlag@@UAE@XZ)

RVA_COMPGEN(0x00012d60, 0x1e, ??_GCParticlez@@UAEPAXI@Z)
RVA_COMPGEN(0x00012d90, 0x44, ??1CParticlez@@UAE@XZ)

RVA_COMPGEN(0x00012e90, 0x1e, ??_GCExplosion@@UAEPAXI@Z)
RVA_COMPGEN(0x00012ec0, 0x44, ??1CExplosion@@UAE@XZ)

// @early-stop
RVA(0x00045d30, 0x220)
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

RVA(0x00046080, 0x102)
void CFortressFlag::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x000461e0, 0x18d)
void CFortressFlag::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (CActRegPool<CFortressFlag>::s_table[id]) =
        static_cast<i32 (CUserLogic::*)()>(&CFortressFlag::AdvanceAnim);
}

RVA(0x000463e0, 0x17)
i32 CFortressFlag::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}

RVA(0x00046410, 0x92)
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
RVA_COMPGEN(0x000464e0, 0x74, ??A?$zDArray@P8CUserLogic@@AEHXZ@@QAEAAP8CUserLogic@@AEHXZH@Z)

RVA(0x00046850, 0xf1)
i32 DispatchParticlezLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CParticlez)
}

RVA(0x00046990, 0xf1)
i32 DispatchExplosionLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CExplosion)
}

// @early-stop
RVA(0x00046ad0, 0x15e)
CParticlez::CParticlez(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_ACTOR_BEHIND)
    m_object->m_dirty.m_armed = 0;
}

RVA(0x00046d30, 0x102)
void CParticlez::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x00046e90, 0x18d)
void CParticlez::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CParticlez>::s_table[id] = static_cast<i32 (CUserLogic::*)()>(&CParticlez::Update);
}

RVA(0x00047090, 0x39)
i32 CParticlez::Update() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    CWwdSpriteObject* o = m_wwdObject;
    if (o->m_animationCursor.IsComplete()) {
        o->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
    }
    return 0;
}

// @early-stop
RVA(0x000470e0, 0x16b)
CExplosion::CExplosion(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_EXPLOSION");
    SET_ANIMATION_ACT("A");
    SetObjectFlags(WWD_GAME_OBJECT_FLAGS_CULL_SOUND_KEEP_ACTIVE);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_OVERLAY)
    m_object->m_dirty.m_armed = 0;
}

RVA(0x00047350, 0x102)
void CExplosion::FireActivation(i32 id) {
    DispatchRegisteredAct(this, id);
}

RVA(0x000474b0, 0x18d)
void RegisterExplosionActions() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CExplosion>::s_table[id] = static_cast<CActHandler>(&CExplosion::Update);
}
