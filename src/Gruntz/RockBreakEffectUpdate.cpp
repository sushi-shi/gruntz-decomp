#include <StdAfx.h>

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
    if (m_wwdObject->GetAnimationCursor().Advance(g_engineFrameDelta) == 1) {
        CWwdSpriteObject* t = m_object;
        if (t->GetScore() == 1) {
            g_gameReg->GetTriggerMgr()
                ->ApplyExplosion(t->m_screenX, t->m_screenY, 1, t->GetSmarts());
        }
    }
    MARK_OBJECT_COMPLETE_IF(m_wwdObject->GetAnimationCursor().IsComplete())
    return 0;
}
