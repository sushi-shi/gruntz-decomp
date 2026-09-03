#include <rva.h>

#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/Explosion.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/UserLogic.h>
#include <Rez/FrameClock.h>

// @early-stop
RVA(0x000476b0, 0x69)
i32 CExplosion::Update() {
    if (m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta) == 1) {
        CWwdSpriteObject* t = m_object;
        if (t->m_score == 1) {
            g_gameReg->m_triggerMgr->BuildRockBreakParticles(
                t->m_screenPosition.m_x,
                t->m_screenPosition.m_y,
                1,
                t->m_smarts
            );
        }
    }
    MARK_OBJECT_COMPLETE_IF(IsAniCursorComplete(&m_wwdObject->m_animationCursor))
    return 0;
}
