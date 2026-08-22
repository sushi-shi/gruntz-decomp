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
    if (m_wwdObject->m_animCursor.Advance(g_engineFrameDelta) == 1) {
        CWwdGameObjectA* t = m_object;
        if (t->m_score == 1) {
            g_gameReg->m_cmdGrid
                ->BuildRockBreakParticles(t->m_screenX, t->m_screenY, 1, t->m_smarts);
        }
    }
    MARK_OBJECT_COMPLETE_IF(IsAniCursorComplete(&m_wwdObject->m_animCursor))
    return 0;
}
