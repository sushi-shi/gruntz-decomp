#include <Gruntz/GruntWingzTimeSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Bute/ButeTree.h>
#include <rva.h>

RVA_COMPGEN(0x000121c0, 0x1e, ??_GCGruntWingzTimeSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x000121f0, 0x44, ??1CGruntWingzTimeSprite@@UAE@XZ)

// @early-stop
RVA(0x0007fcc0, 0xa0)
CGruntWingzTimeSprite::CGruntWingzTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTWINGZTIMESPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = ActFindId("A");
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != 0xdbba0) {
        o->m_sortKey = 0xdbba0;
        o->m_flags |= 0x20000;
    }
    m_health = 0;
    m_60 = -0x26;
}

VTBL(CGruntWingzTimeSprite, 0x001e77cc);
RVA(0x0007fd90, 0xd)
i32 CGruntWingzTimeSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_wingzTime;
}
