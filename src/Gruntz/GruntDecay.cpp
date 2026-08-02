

#include <Gruntz/Grunt.h>
#include <Rez/FrameClock.h>
#include <Gruntz/TriggerMgr.h>
#include <Image/ImageSet.h>
#include <Bute/ButeTree.h>
#include <Bute/ButeMgr.h>

// @early-stop
RVA(0x000612a0, 0x23c)
i32 CGrunt::LoadGruntDecayConfig() {
    if (m_deathType == 0) {
        return 0;
    }
    if (m_wwdObject->m_1a0.Advance(g_engineFrameDelta) == 1) {
        if (m_entranceReason == 1 && m_deathType != 5) {
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
                m_1f4_moveIcon,
                m_deathType != 5,
                0x19
            );
        }
    }
    CAniAdvanceCursor* sub = &m_wwdObject->m_1a0;
    if (sub->m_finished == 0) {
        return 0;
    }
    if (sub->m_frameTicksLeft != 0) {
        return 0;
    }
    i32 mode = m_deathType;
    if (mode == 1 || mode == 2 || mode == 0xb || mode == 6) {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = ActFindId("R");
        if (m_36c == 0) {
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
        }
        i32 dt = static_cast<i32>(g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
        if (m_object->m_drawFillCmd == 0xb) {
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
        m_object->m_drawFillCmd = 0xb;
        m_object->m_fillFraction = r;
        return 0;
    }
    if (m_36c == 0) {
        m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
    }
    m_wwdObject->m_flags |= 0x10000;
    return 0;
}

// @early-stop
RVA(0x00061570, 0x11d)
i32 CGrunt::LoadGruntDecayConfig2() {
    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - m_idleTimer >= m_idleWindow) {
        m_wwdObject->m_stateFlags |= 1;
        m_wwdObject->m_imageSet->SetAllTypes(1);
        if (m_36c == 0) {
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
    m_object->m_drawFillCmd = 0xb;
    m_object->m_fillFraction = r;
    return 0;
}
