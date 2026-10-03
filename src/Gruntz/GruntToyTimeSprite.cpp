#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GruntToyTimeSprite.h>

#include <Gruntz/ActRegistry.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>

CGruntToyTimeSprite::CGruntToyTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    SetImageFrameByName("GAME_GRUNTTOYTIMESPRITE", 1);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_displayedValue = 0;
    m_yOffset = -0x20;
}

i32 CGruntToyTimeSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_toyTime;
}
