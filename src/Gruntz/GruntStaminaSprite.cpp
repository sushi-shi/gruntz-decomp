#include <rva.h>

#include <Gruntz/GruntStaminaSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>

RVA_COMPGEN(0x00012050, 0x1e, ??_GCGruntStaminaSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00012080, 0x44, ??1CGruntStaminaSprite@@UAE@XZ)

RVA(0x0007fa00, 0xa0)
CGruntStaminaSprite::CGruntStaminaSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    SetImageFrameByName("GAME_GRUNTSTAMINASPRITE", 1);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_displayedValue = HEALTH_FULL;
    m_yOffset = -0x20;
}

RVA(0x0007fad0, 0xd)
i32 CGruntStaminaSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_stamina;
}
