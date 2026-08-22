#include <rva.h>

#include <Gruntz/GruntPowerupSprite.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Io/FileMem.h>
#include <Rez/FrameClock.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x0007ff80, 0xa, CActRegPool<CGruntPowerupSprite>::s_table)
RVA_DYNINIT(0x0007ffa0, 0x15, CActRegPool<CGruntPowerupSprite>::s_table)
RVA_DYNINIT(0x0007ffd0, 0xe, CActRegPool<CGruntPowerupSprite>::s_table)
RVA_DYNINIT(0x0007fff0, 0x1f, CActRegPool<CGruntPowerupSprite>::s_table)
template<> DATA(0x00244d30)
CActReg CActRegPool<CGruntPowerupSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00012340, 0x1e, ??_GCGruntPowerupSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00012370, 0x44, ??1CGruntPowerupSprite@@UAE@XZ)

RVA(0x0007fdb0, 0x166)
CGruntPowerupSprite::CGruntPowerupSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    ApplyName("GAME_LIGHTING_POWERUP");
    SwitchGeometry("GAME_CYCLE100", 0);
    CWwdGameObjectA* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_POWERUP)
    Hide();
}

RVA(0x00080020, 0x102)
void CGruntPowerupSprite::FireActivation(i32 id) {
    if ((*((CActRegPool<CGruntPowerupSprite>::s_table.ResolveEntry(id)))) != 0) {
        (this->*(*((CActRegPool<CGruntPowerupSprite>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x00080180, 0x18d)
void CGruntPowerupSprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CGruntPowerupSprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntPowerupSprite::Update);
}

RVA(0x00080380, 0x6c)
i32 CGruntPowerupSprite::SetCell(i32 x, i32 y, i32 powerup) {
    m_cell.m_x = x;
    m_cell.m_y = y;
    m_powerupId = powerup;
    CShadeTable* rec = g_gameReg->m_logicPump->m_tables[powerup];
    CWwdGameObjectA* r = m_object;
    SET_DRAW_FILL(r, SHADE_DST_BY_SRC_16, rec);
    m_wwdObject->m_stateFlags &= ~SPRITE_STATE_HIDDEN;
    SET_ANIMATION_ACT("A");
    return 1;
}

RVA(0x00080410, 0x51)
i32 CGruntPowerupSprite::Update() {
    m_wwdObject->m_animCursor.Advance(g_engineFrameDelta);
    CGrunt* e = g_gameReg->m_cmdGrid->m_grid[m_cell.m_x * 15 + m_cell.m_y];
    if (e != NULL) {
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

RVA(0x00080490, 0xbe)
i32 CGruntPowerupSprite::SerializeMove(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (CUserLogic::SerializeMove(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    if (Chain(ar, mode, typeId, pObj) == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_cell, sizeof(m_cell));
            ar->Write(&m_powerupId, sizeof(m_powerupId));
            break;
        case SERIAL_LOAD: {
            ar->Read(&m_cell, sizeof(m_cell));
            ar->Read(&m_powerupId, sizeof(m_powerupId));
            i32 id = m_powerupId;
            CWwdGameObjectA* r = m_object;
            CShadeTable* v = g_gameReg->m_logicPump->m_tables[id];
            SET_DRAW_FILL_REVERSED(r, SHADE_DST_BY_SRC_16, v);
            break;
        }
    }
    return 1;
}
