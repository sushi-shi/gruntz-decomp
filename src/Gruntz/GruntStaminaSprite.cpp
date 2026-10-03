#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntStaminaSprite.h>

#include <Gruntz/ActRegistry.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>

CGruntStaminaSprite::CGruntStaminaSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    SetImageFrameByName("GAME_GRUNTSTAMINASPRITE", 1);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_displayedValue = HEALTH_FULL;
    m_yOffset = -0x20;
}

i32 CGruntStaminaSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_stamina;
}
