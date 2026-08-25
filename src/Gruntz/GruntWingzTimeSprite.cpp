#include <rva.h>

#include <Gruntz/GruntWingzTimeSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>

RVA_COMPGEN(0x000121c0, 0x1e, ??_GCGruntWingzTimeSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x000121f0, 0x44, ??1CGruntWingzTimeSprite@@UAE@XZ)

RVA(0x0007fcc0, 0xa0)
CGruntWingzTimeSprite::CGruntWingzTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    SetImageFrameByName("GAME_GRUNTWINGZTIMESPRITE", 1);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_displayedValue = 0;
    m_yOffset = -0x26;
}

RVA(0x0007fd90, 0xd)
i32 CGruntWingzTimeSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_wingzTime;
}
