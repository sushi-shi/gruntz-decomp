#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDeathType.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TriggerMgr.h>
#include <Image/ImageSet.h>
#include <Rez/FrameClock.h>

// @early-stop
RVA(0x000612a0, 0x23c)
i32 CGrunt::UpdateDeathAnimation() {
    if (m_deathType == DEATH_DROP) {
        return 0;
    }
    if (m_wwdObject->m_animationCursor.Advance(g_engineFrameDelta) == 1) {
        if (m_entranceReason == PICKUP_BOMB && m_deathType != DEATH_MELT) {
            m_triggerMgr->BuildRockBreakParticles(
                m_object->m_screenX,
                m_object->m_screenY,
                1,
                m_playerIndex
            );
        } else {
            m_triggerMgr->SpawnPuddle(
                m_object->m_screenX,
                m_object->m_screenY,
                m_playerIndex,
                IDX(m_moveIcon),
                m_deathType != DEATH_MELT,
                0x19
            );
        }
    }
    CAniAdvanceCursor* sub = &m_wwdObject->m_animationCursor;
    if (sub->m_finished == false) {
        return 0;
    }
    if (sub->m_frameTicksLeft != 0) {
        return 0;
    }
    GruntDeathType mode = m_deathType;
    if (mode == DEATH_NORMAL || mode == DEATH_SQUASH || mode == DEATH_EXPLODE
        || mode == DEATH_SHATTER) {
        SET_ANIMATION_ACT("R");
        if (m_cellRemovalNotified == false) {
            m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);
        }
        i32 dt = static_cast<i32>(g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
        i32 epoch;
        if (m_object->m_drawFillCmd == SHADE_PAL_ALPHA_16) {
            epoch = static_cast<i32>(g_frameTime) - m_object->m_fillFraction * dt / 256;
            m_idleWindowLo = dt;
            m_idleWindowHi = 0;
        } else {
            m_idleWindowLo = dt;
            m_idleWindowHi = 0;
            epoch = static_cast<i32>(g_frameTime);
        }
        m_idleTimerLo = epoch;
        m_idleTimerHi = 0;
        i64 e = static_cast<i64>(g_frameTime) - m_idleTimer;
        u32 elapsed = e < 0 ? 0 : static_cast<u32>(e);
        i32 r = static_cast<i32>(
            (static_cast<double>(elapsed) * 256.0
             / static_cast<double>(g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8)))
        );
        CWwdSpriteObject* o = m_object;
        SET_DRAW_FILL_FRACTION(o, SHADE_PAL_ALPHA_16, r);
        return 0;
    }
    if (m_cellRemovalNotified == false) {
        m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);
    }
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
    return 0;
}

// @early-stop
RVA(0x00061570, 0x11d)
i32 CGrunt::UpdateDecayFade() {
    i64 now = static_cast<i64>(g_frameTime);
    if (now - m_idleTimer >= m_idleWindow) {
        Hide();
        m_wwdObject->m_imageSet->SetAllTypes(SHADE_COPY);
        if (m_cellRemovalNotified == false) {
            m_triggerMgr->UnregisterUnit(m_playerIndex, m_unitIndex, 0);
        }
        SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE));
        return 0;
    }
    i64 e = now - m_idleTimer;
    u32 elapsed = e < 0 ? 0 : static_cast<u32>(e);
    CWwdSpriteObject* o = m_object;
    i32 r = static_cast<i32>(
        (static_cast<double>(elapsed) * 256.0
         / static_cast<double>(g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8)))
    );
    SET_DRAW_FILL_FRACTION(o, SHADE_PAL_ALPHA_16, r);
    return 0;
}
