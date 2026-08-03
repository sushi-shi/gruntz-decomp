#include <rva.h>

#include <Gruntz/Ufo.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/LightFxMgr.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpotLight.h>
#include <Io/FileMem.h>

VTBL(CUFO, 0x001e72b4);
RVA_COMPGEN(0x000133d0, 0x1e, ??_GCUFO@@UAEPAXI@Z)
RVA(0x000b4330, 0x8)
i32 CUFO::Tick() {
    CPathHazard::Tick();
    return 0;
}

// @early-stop
RVA(0x000b4a90, 0x145)
CUFO::CUFO(CGameObject* obj) : CPathHazard(obj) {
    CWwdGameObjectA* o = m_object;
    i32 sx = o->m_screenX;
    i32 sy = o->m_screenY;
    m_value = m_wwdObject->m_animCursor.m_animation;
    m_wwdObject->ApplyLookupGeometry("LEVEL_UFO", 0);
    for (i32 i = 0; i < 2; ++i) {
        CWwdGameObjectA* sl =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, sx, 0, 0, "SpotLight", 0x40003);
        if (sl != 0) {
            sl->ApplyName("LEVEL_SPOTLIGHT");
            AnimWorkerObj* sub = sl->m_animWorker;
            sl->m_score = 1;
            sl->m_direction = 0;
            sl->m_smarts = 2;
            sl->m_powerup = 0;
            sl->m_points = i;
            sl->m_damage = m_object->m_faceDirection;
            sub->m_notify(sl);

            (static_cast<CSpotLight*>(sl->m_animWorker->m_logic))->m_focus = m_object;
        }
    }
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = SHADE_ALPHA_16;
    m_object->m_fillFraction = 0x80;
    m_object->m_area.left = 0;
    m_object->m_area.right = 0;
    m_object->m_area.top = 0;
    m_object->m_area.bottom = 0;
}
