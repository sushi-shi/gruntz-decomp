#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/GruntStaminaSprite.h>

#include <Gruntz/ActRegistry.h>
#include <Gruntz/HealthPct.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SortKeyLayer.h>

RVA_COMPGEN(0x00012040, 0x1e, ??_GCGruntStaminaSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00012070, 0x44, ??1CGruntStaminaSprite@@UAE@XZ)

RVA(0x0007fae0, 0xa0)
CGruntStaminaSprite::CGruntStaminaSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    SetImageFrameByName("GAME_GRUNTSTAMINASPRITE", 1);
    SET_ANIMATION_ACT("A");
    CWwdSpriteObject* o = m_object;
    o->SetSortKey(SORTKEY_GRUNT_HUD);
    m_displayedValue = HEALTH_FULL;
    m_yOffset = -0x20;
}

RVA(0x0007fbb0, 0xd)
i32 CGruntStaminaSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->GetStamina();
}
