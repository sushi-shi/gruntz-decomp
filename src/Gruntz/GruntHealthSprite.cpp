#include <rva.h>

#include <Gruntz/GruntHealthSprite.h>

#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntCellInline.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/TypeKeyColl.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Lith/BDefs.h>
#include <Wap32/ZVec.h>

#include <stddef.h>

RVA_DYNINIT(0x0007ecd0, 0xa, CActRegPool<CGruntHealthSprite>::s_table)
RVA_DYNINIT(0x0007ecf0, 0x15, CActRegPool<CGruntHealthSprite>::s_table)
RVA_DYNINIT(0x0007ed20, 0xe, CActRegPool<CGruntHealthSprite>::s_table)
RVA_DYNINIT(0x0007ed40, 0x1f, CActRegPool<CGruntHealthSprite>::s_table)
template<> DATA(0x00244d80)
CActReg CActRegPool<CGruntHealthSprite>::s_table(ACT_ID_FIRST, ACT_ID_LAST);
RVA_COMPGEN(0x00011f80, 0x1e, ??_GCGruntHealthSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00011fb0, 0x44, ??1CGruntHealthSprite@@UAE@XZ)

RVA(0x0007eb00, 0x170)
CGruntHealthSprite::CGruntHealthSprite(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageFrameByName("GAME_GRUNTHEALTHSPRITE", 1);
    SET_ANIMATION_ACT("A");
    m_displayedValue = HEALTH_FULL;
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_GRUNT_HUD);
    m_yOffset = -0x19;
}

RVA(0x0007ed70, 0x102)
void CGruntHealthSprite::FireActivation(i32 id) {
    if ((*((CActRegPool<CGruntHealthSprite>::s_table.ResolveEntry(id)))) != NULL) {
        (this->*(*((CActRegPool<CGruntHealthSprite>::s_table.ResolveEntry(id)))))();
    }
}

RVA(0x0007eed0, 0x18d)
void CGruntHealthSprite::RegisterActs() {
    ACT_NAME_ID(id, "A")
    (*((CActRegPool<CGruntHealthSprite>::s_table.ResolveEntry(id)))) =
        static_cast<i32 (CUserLogic::*)()>(&CGruntHealthSprite::HealthUpdate);
}

RVA(0x0007f0d0, 0x6e)
i32 CGruntHealthSprite::BindToGrunt(i32 playerIndex, i32 unitIndex, i32 displayedValue) {
    m_gruntIdentity.m_playerIndex = playerIndex;
    m_gruntIdentity.m_unitIndex = unitIndex;
    i32 slot = 0x15 - ROUND(static_cast<double>(displayedValue) * 0.2);
    CWwdSpriteObject* obj = m_object;
    CDDrawWorker* map = obj->m_imageSet;
    if (map) {
        CImage* glyph = map->GetAt(slot);
        obj->m_frameImage = glyph;
        obj->m_frameIndex = slot;
    }
    m_displayedValue = displayedValue;
    return 1;
}

RVA(0x0007f160, 0xd)
i32 CGruntHealthSprite::GetDisplayedValue(CGrunt* g) {
    return g->m_health;
}

RVA(0x0007f180, 0xb4)
i32 CGruntHealthSprite::HealthUpdate() {

    CGrunt* e = FindGruntByIdentity(g_gameReg, m_gruntIdentity);
    if (e == NULL) {
        return 0;
    }
    i32 result = GetDisplayedValue(e);
    if (m_displayedValue != result) {
        i32 slot = 0x15 - ROUND(static_cast<double>(result) * 0.2);
        CWwdSpriteObject* obj = m_object;
        CDDrawWorker* holder = obj->m_imageSet;
        if (holder != NULL) {
            CImage* glyph = holder->GetAt(slot);
            obj->m_frameImage = glyph;
            obj->m_frameIndex = slot;
        }
        m_displayedValue = result;
    }
    Coord position = e->m_object->ScreenPos() + Coord(0, m_yOffset);
    m_object->SetScreenPos(position);
    return 0;
}

RVA(0x0007f270, 0xa3)
i32 CGruntHealthSprite::SerializeDispatch(
    CFileMemBase* ar,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
) {
    switch (mode) {
        case SERIAL_SAVE:
            ar->Write(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Write(&m_displayedValue, sizeof(m_displayedValue));
            ar->Write(&m_yOffset, sizeof(m_yOffset));
            break;
        case SERIAL_LOAD:
            ar->Read(&m_gruntIdentity, sizeof(m_gruntIdentity));
            ar->Read(&m_displayedValue, sizeof(m_displayedValue));
            ar->Read(&m_yOffset, sizeof(m_yOffset));
            break;
    }
    SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
}
