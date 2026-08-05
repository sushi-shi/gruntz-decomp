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
i32 CGrunt::LoadGruntDecayConfig() {
    if (m_deathType == DEATH_DROP) {
        return 0;
    }
    if (m_wwdObject->m_animCursor.Advance(g_engineFrameDelta) == 1) {
        if (m_entranceReason == PICKUP_BOMB && m_deathType != DEATH_MELT) {
            m_tileMgr->BuildRockBreakParticles(
                m_object->m_screenX,
                m_object->m_screenY,
                1,
                m_tileOwnerHi
            );
        } else {
            m_tileMgr->SpawnPuddle(
                m_object->m_screenX,
                m_object->m_screenY,
                m_tileOwnerHi,
                IDX(m_moveIcon),
                m_deathType != DEATH_MELT,
                0x19
            );
        }
    }
    CAniAdvanceCursor* sub = &m_wwdObject->m_animCursor;
    if (sub->m_finished == 0) {
        return 0;
    }
    if (sub->m_frameTicksLeft != 0) {
        return 0;
    }
    GruntDeathType mode = m_deathType;
    if (mode == DEATH_NORMAL || mode == DEATH_SQUASH || mode == DEATH_EXPLODE
        || mode == DEATH_SHATTER) {
        m_prevAnimSetNode = m_objAux->m_actKey;
        m_objAux->m_actKey = ActFindId("R");
        if (m_cellRemovalNotified == 0) {
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
        }
        i32 dt = static_cast<i32>(g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
        if (m_object->m_drawFillCmd == SHADE_PAL_ALPHA_16) {
            m_idleWindowLo = dt;
            m_idleTimerLo = static_cast<i32>(g_frameTime) - m_object->m_fillFraction * dt / 256;
            m_idleWindowHi = 0;
        } else {
            m_idleWindowLo = dt;
            m_idleWindowHi = 0;
            m_idleTimerLo = static_cast<i32>(g_frameTime);
        }
        m_idleTimerHi = 0;
        i64 e = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_idleTimer;
        u32 elapsed = e < 0 ? 0 : static_cast<u32>(e);
        i32 r = static_cast<i32>(
            (static_cast<double>(elapsed) * 256.0
             / static_cast<double>(g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8)))
        );
        m_object->m_drawActive = 1;
        m_object->m_drawFillCmd = SHADE_PAL_ALPHA_16;
        m_object->m_fillFraction = r;
        return 0;
    }
    if (m_cellRemovalNotified == 0) {
        m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
    }
    m_wwdObject->m_flags |= 0x10000;
    return 0;
}

// @early-stop
RVA(0x00061570, 0x11d)
i32 CGrunt::LoadGruntDecayConfig2() {
    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_idleTimer >= m_idleWindow) {
        m_wwdObject->m_stateFlags |= IDX(SPRITE_STATE_HIDDEN);
        m_wwdObject->m_frameSet->SetAllTypes(SHADE_COPY);
        if (m_cellRemovalNotified == 0) {
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
        }
        m_wwdObject->m_flags |= 0x10000;
        return 0;
    }
    i64 e = static_cast<i64>(static_cast<u32>(g_frameTime)) - m_idleTimer;
    u32 elapsed = e < 0 ? 0 : static_cast<u32>(e);
    i32 r = static_cast<i32>(
        (static_cast<double>(elapsed) * 256.0
         / static_cast<double>(g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8)))
    );
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = SHADE_PAL_ALPHA_16;
    m_object->m_fillFraction = r;
    return 0;
}
