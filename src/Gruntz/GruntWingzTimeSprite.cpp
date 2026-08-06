#include <rva.h>

#include <Gruntz/GruntWingzTimeSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SortKeyLayer.h>

RVA_COMPGEN(0x000121c0, 0x1e, ??_GCGruntWingzTimeSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x000121f0, 0x44, ??1CGruntWingzTimeSprite@@UAE@XZ)

// @early-stop
RVA(0x0007fcc0, 0xa0)
CGruntWingzTimeSprite::CGruntWingzTimeSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    m_wwdObject->ApplyLookupSprite("GAME_GRUNTWINGZTIMESPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_GRUNT_HUD) {
        o->m_sortKey = SORTKEY_GRUNT_HUD;
        o->m_flags |= 0x20000;
    }
    m_health = 0;
    m_yOffset = -0x26;
}

// @interleaver GetDisplayedValue - fixed-size generated body (13 B, byte-identical across
// 4 classes), so every TU emits one and the linker folds them to first use.
RVA(0x0007fd90, 0xd)
i32 CGruntWingzTimeSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_wingzTime;
}
