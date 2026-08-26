#include <rva.h>

#include <Gruntz/GruntToyTimeSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/SortKeyLayer.h>
#include <Gruntz/SortKeyMacros.h>

RVA_COMPGEN(0x00012110, 0x1e, ??_GCGruntToyTimeSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00012140, 0x44, ??1CGruntToyTimeSprite@@UAE@XZ)

RVA(0x0007faf0, 0xa0)
CGruntToyTimeSprite::CGruntToyTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    SetImageFrameByName("GAME_GRUNTTOYTIMESPRITE", 1);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    SET_SORT_KEY_IF_CHANGED(o, SORTKEY_GRUNT_HUD)
    m_displayedValue = 0;
    m_yOffset = -0x20;
}

RVA(0x0007fbc0, 0xd)
i32 CGruntToyTimeSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_toyTime;
}
