#include <rva.h>

#include <Gruntz/GruntToyTimeSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/SortKeyLayer.h>

RVA_COMPGEN(0x00012100, 0x1e, ??_GCGruntToyTimeSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00012130, 0x44, ??1CGruntToyTimeSprite@@UAE@XZ)

RVA(0x0007fbd0, 0xa0)
CGruntToyTimeSprite::CGruntToyTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    ApplyLookupSprite("GAME_GRUNTTOYTIMESPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_GRUNT_HUD) {
        o->m_sortKey = SORTKEY_GRUNT_HUD;
        o->m_flags |= 0x20000;
    }
    m_health = 0;
    m_yOffset = -0x20;
}

RVA(0x0007fca0, 0xd)
i32 CGruntToyTimeSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_toyTime;
}
