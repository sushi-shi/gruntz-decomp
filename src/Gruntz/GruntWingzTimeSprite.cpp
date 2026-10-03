#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntWingzTimeSprite.h>

#include <Gruntz/ActRegistry.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>

CGruntWingzTimeSprite::CGruntWingzTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    SetImageFrameByName("GAME_GRUNTWINGZTIMESPRITE", 1);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_displayedValue = 0;
    m_yOffset = -0x26;
}

i32 CGruntWingzTimeSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_wingzTime;
}
