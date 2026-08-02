

#include <Rez/FrameClock.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/TriggerMgr.h>
#include <Gruntz/Explosion.h>

// @early-stop
RVA(0x000476b0, 0x69)
i32 CExplosion::Update() {
    if (m_wwdObject->m_1a0.Advance(g_engineFrameDelta) == 1) {
        CWwdGameObjectA* t = m_object;
        if (t->m_114 == 1) {
            g_gameReg->m_cmdGrid->BuildRockBreakParticles(t->m_screenX, t->m_screenY, 1, t->m_124);
        }
    }
    if (m_wwdObject->m_1a0.m_finished != 0 && m_wwdObject->m_1a0.m_frameTicksLeft == 0) {
        m_wwdObject->m_flags |= 0x10000;
    }
    return 0;
}
