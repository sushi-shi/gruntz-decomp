// GruntDecay.cpp - CGrunt's decay handlers from the original grunt-region TU
// currently tracked as the `gruntbehaviorleaf` unit. The old
// CGruntBehaviorLeaf owner was a layout-identical placeholder disproved by the
// action registry, receiver offsets, ILT callees, and absence of RTTI/vtable
// evidence.
#include <Gruntz/Grunt.h>
#include <Rez/FrameClock.h> // frame-clock band (g_frameDelta/g_frameTime/g_killCueClock/g_engineFrameDelta)
#include <Gruntz/TriggerMgr.h> // the real owner of NotifyCell/SpawnPuddle (the +0x260 slot)
#include <Image/ImageSet.h>    // CDDrawWorker::SetAllTypes (m_drawState->m_194)
#include <Bute/ButeTree.h>     // CButeTree::Find (the "R" animset key)
#include <Bute/ButeMgr.h>      // CButeMgr getters (Grunt/DecayTime, WANDGRUNT/HealthLoss)

// LoadGruntDecayConfig (0x612a0): advance the arrival probe, drive the walk/idle
// anim by grunt mode, then (once arrived + not busy) latch the decay timer + fill.
// @early-stop
// 90.5%: logic byte-faithful. Residual is CSE/regalloc of the 64-bit timer delta -
// retail keeps g_frameTime pinned in eax and re-does the m_idleTimerLo subtraction in the fill
// tail, while cl shares the whole `(i64)clock - m_idleTimerLo` delta, shifting the lo dword
// off eax and cascading register names + the epilogue merge. Not source-steerable.
RVA(0x000612a0, 0x23c)
i32 CGrunt::LoadGruntDecayConfig() {
    if (m_deathType == 0) {
        return 0;
    }
    if (m_38->m_1a0.Advance(g_engineFrameDelta) == 1) {
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
    CAniAdvanceCursor* sub = &m_38->m_1a0;
    if (sub->m_28 == 0) {
        return 0;
    }
    if (sub->m_20 != 0) {
        return 0;
    }
    i32 mode = m_deathType;
    if (mode == 1 || mode == 2 || mode == 0xb || mode == 6) {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find("R");
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
        i64 e = static_cast<i64>(static_cast<u32>(g_frameTime))
                - *reinterpret_cast<i64*>(&m_idleTimerLo);
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
    m_38->m_flags |= 0x10000;
    return 0;
}

// LoadGruntDecayConfig2 (0x61570): if the timer has fully elapsed, fire the finish
// (flag + finish anim); else refresh the 0..256 fill fraction on the draw command.
// @early-stop
// ~77%: logic byte-faithful. Same CSE/regalloc wall as LoadGruntDecayConfig -
// retail loads g_frameTime once into eax and recomputes the m_idleTimerLo subtraction in the
// fill branch (eax preserved -> lo stays in eax); cl CSEs the whole 64-bit delta and
// pins lo in ecx, cascading a register-name mismatch through the tail. Hoisting the
// clock into a local regressed it. Not source-steerable.
RVA(0x00061570, 0x11d)
i32 CGrunt::LoadGruntDecayConfig2() {
    if (static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&m_idleTimerLo)
        >= *reinterpret_cast<i64*>(&m_idleWindowLo)) {
        m_38->m_stateFlags |= 1;
        m_38->m_imageSet->SetAllTypes(1);
        if (m_36c == 0) {
            m_tileMgr->NotifyCell(m_tileOwnerHi, m_tileOwnerLo, 0);
        }
        m_38->m_flags |= 0x10000;
        return 0;
    }
    i64 e =
        static_cast<i64>(static_cast<u32>(g_frameTime)) - *reinterpret_cast<i64*>(&m_idleTimerLo);
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
