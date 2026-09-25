#include <rva.h>

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

RVA_COMPGEN(0x00012c30, 0x1e, ??_GCToobSpikez@@UAEPAXI@Z)
RVA_COMPGEN(0x00012c60, 0x44, ??1CToobSpikez@@UAE@XZ)

RVA(0x00114480, 0xf1)
i32 DispatchToobSpikezLogic(CGameObject* obj) {
    TILE_LOGIC_RECORD_DISPATCH(CToobSpikez)
}

RVA_DYNINIT(0x001147c0, 0xa, CActRegPool<CToobSpikez>::s_table)
RVA_DYNINIT(0x001147e0, 0x15, CActRegPool<CToobSpikez>::s_table)
RVA_DYNINIT(0x00114810, 0xe, CActRegPool<CToobSpikez>::s_table)
RVA_DYNINIT(0x00114830, 0x1f, CActRegPool<CToobSpikez>::s_table)
template<> DATA(0x0024e978)
CActReg CActRegPool<CToobSpikez>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

// @early-stop
RVA(0x001145c0, 0x18e)
CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SwitchAnimationByName("GAME_CYCLE100", 0);
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    m_object->m_speed = m_object->ScreenPos();
    ScreenTile(&m_object->m_speed);
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_TOOB_SPIKE);
}

RVA(0x00114860, 0x102)
void CToobSpikez::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x001149c0, 0x18d)
void CToobSpikez::RegisterActs() {
    ACT_NAME_ID(id, "A")
    CActRegPool<CToobSpikez>::s_table[id] = static_cast<CActHandler>(&CToobSpikez::AdvanceAnim);
}

RVA(0x00114bc0, 0x17)
i32 CToobSpikez::AdvanceAnim() {
    m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta);
    return 0;
}
