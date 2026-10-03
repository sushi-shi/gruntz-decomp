#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/ToobSpikez.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Rez/FrameClock.h>
#include <Wap32/TileGeometry.h>
#include <ZTools/ZDArray.h>

#include <stddef.h>

i32 DispatchToobSpikezLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CToobSpikez)
}

template<>
CActReg CActRegPool<CToobSpikez>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SwitchAnimationByName("GAME_CYCLE100", 0);
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    m_object->m_speedX = m_object->m_screenX >> TILE_SHIFT_PX;
    m_object->m_speedY = m_object->m_screenY >> TILE_SHIFT_PX;
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_TOOB_SPIKE)
}

void CToobSpikez::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void CToobSpikez::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CToobSpikez>::s_table[id] = static_cast<CActHandler>(&CToobSpikez::AdvanceAnim);
}

i32 CToobSpikez::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
