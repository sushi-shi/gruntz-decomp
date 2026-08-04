#include <rva.h>

#include <Gruntz/GruntStaminaSprite.h>

#include <Bute/ButeTree.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SortKeyLayer.h>

RVA_COMPGEN(0x00012040, 0x1e, ??_GCGruntStaminaSprite@@UAEPAXI@Z)
RVA_COMPGEN(0x00012070, 0x44, ??1CGruntStaminaSprite@@UAE@XZ)

// @early-stop
RVA(0x0007fae0, 0xa0)
CGruntStaminaSprite::CGruntStaminaSprite(CGameObject* obj) : CGruntHealthSprite(obj) {
    m_wwdObject->ApplyLookupSprite("GAME_GRUNTSTAMINASPRITE", 1);
    m_prevAnimSetNode = m_objAux->m_actKey;
    m_objAux->m_actKey = ActFindId("A");
    CWwdGameObjectA* o = m_object;
    if (o->m_sortKey != SORTKEY_GRUNT_HUD) {
        o->m_sortKey = SORTKEY_GRUNT_HUD;
        o->m_flags |= 0x20000;
    }
    m_health = 0x64;
    m_yOffset = -0x20;
}

VTBL(CGruntStaminaSprite, 0x001e7a44);
// @interleaver GetDisplayedValue - fixed-size generated body (13 B, byte-identical across
// 4 classes), so every TU emits one and the linker folds them to first use.
RVA(0x0007fbb0, 0xd)
i32 CGruntStaminaSprite::GetDisplayedValue(CGrunt* grunt) {
    return grunt->m_stamina;
}
