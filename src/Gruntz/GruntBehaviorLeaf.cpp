// GruntBehaviorLeaf.cpp - the per-frame grunt "decay/wand" AI (0x612a0 / 0x61570 /
// 0x65a60), re-homed out of the UserLogic god-TU (C:\Proj\Gruntz).
//
// These run on CGruntBehaviorLeaf (a placeholder-identity CUserLogic leaf; see
// <Gruntz/GruntBehaviorLeaf.h>). The decay timer (m_830 start / m_838 duration, both
// hi=0) drives a 0..256 fixed-point fill bar on the bound object's draw command;
// m_object is the real inherited CGameObject. The bute/anim callees are reloc-masked
// __thiscall externs. g_frameTime = running ms clock. Only offsets / code bytes are
// load-bearing.
#include <Gruntz/GruntBehaviorLeaf.h>
#include <Gruntz/TriggerMgr.h>   // the real owner of NotifyCell/SpawnPuddle (the +0x260 slot)
#include <Gruntz/RockBreakMgr.h> // the real owner of BuildRockBreakParticles
#include <Image/ImageSet.h>      // CImageSet::SetAllTypes (m_drawState->m_194)
#include <Bute/ButeTree.h>       // CButeTree::Find (the "R" animset key)
#include <Bute/ButeMgr.h>        // CButeMgr getters (Grunt/DecayTime, WANDGRUNT/HealthLoss)

extern "C" u32 g_frameTime;   // 0x645588  running game clock (ms)
extern "C" i32 g_engineFrameDelta;   // 0x6bf3bc  per-frame draw-delta (arrival probe ctx)
extern CButeMgr g_buteMgr; // 0x6453d8 - getters reloc-mask
extern CButeTree g_buteTree;
extern char k_60bebc[]; // interned bute-node name "R"

// LoadGruntDecayConfig (0x612a0): advance the arrival probe, drive the walk/idle
// anim by grunt mode, then (once arrived + not busy) latch the decay timer + fill.
// @early-stop
// 90.5%: logic byte-faithful. Residual is CSE/regalloc of the 64-bit timer delta -
// retail keeps g_frameTime pinned in eax and re-does the m_decayTimerLo subtraction in the fill
// tail, while cl shares the whole `(i64)clock - m_decayTimerLo` delta, shifting the lo dword
// off eax and cascading register names + the epilogue merge. Not source-steerable.
RVA(0x000612a0, 0x23c)
i32 CGruntBehaviorLeaf::LoadGruntDecayConfig() {
    if (m_gruntMode == 0) {
        return 0;
    }
    if (m_drawState->m_1a0.Advance_15c360(g_engineFrameDelta) == 1) {
        if (m_gruntSubState == 1 && m_gruntMode != 5) {
            ((CRockBreakMgr*)m_260)
                ->BuildRockBreakParticles(m_object->m_screenX, m_object->m_screenY, 1, m_animArg0);
        } else {
            ((CTriggerMgr*)m_260)
                ->SpawnPuddle(
                    m_object->m_screenX,
                    m_object->m_screenY,
                    m_animArg0,
                    m_animArg2,
                    m_gruntMode != 5,
                    0x19
                );
        }
    }
    CAniAdvanceCursor* sub = &m_drawState->m_1a0;
    if (sub->m_28 == 0) {
        return 0;
    }
    if (sub->m_20 != 0) {
        return 0;
    }
    i32 mode = m_gruntMode;
    if (mode == 1 || mode == 2 || mode == 0xb || mode == 6) {
        m_prevAnimSetNode = m_objAux->m_1c;
        m_objAux->m_1c = g_buteTree.Find(k_60bebc);
        if (m_animSuppress == 0) {
            ((CTriggerMgr*)m_260)->NotifyCell(m_animArg0, m_animArg1, 0);
        }
        i32 dt = (i32)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8);
        if (m_object->m_drawFillCmd == 0xb) {
            m_decayDurationLo = dt;
            m_decayTimerLo = (i32)g_frameTime - m_object->m_fillFraction * dt / 256;
            m_decayDurationHi = 0;
        } else {
            m_decayDurationLo = dt;
            m_decayDurationHi = 0;
            m_decayTimerLo = (i32)g_frameTime;
        }
        m_decayTimerHi = 0;
        i64 e = (i64)(u32)g_frameTime - *(i64*)&m_decayTimerLo;
        u32 elapsed = e < 0 ? 0 : (u32)e;
        i32 r = (i32)((double)elapsed * 256.0
                      / (double)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
        m_object->m_drawActive = 1;
        m_object->m_drawFillCmd = 0xb;
        m_object->m_fillFraction = r;
        return 0;
    }
    if (m_animSuppress == 0) {
        ((CTriggerMgr*)m_260)->NotifyCell(m_animArg0, m_animArg1, 0);
    }
    m_drawState->m_8 |= 0x10000;
    return 0;
}

// LoadGruntDecayConfig2 (0x61570): if the timer has fully elapsed, fire the finish
// (flag + finish anim); else refresh the 0..256 fill fraction on the draw command.
// @early-stop
// ~77%: logic byte-faithful. Same CSE/regalloc wall as LoadGruntDecayConfig -
// retail loads g_frameTime once into eax and recomputes the m_decayTimerLo subtraction in the
// fill branch (eax preserved -> lo stays in eax); cl CSEs the whole 64-bit delta and
// pins lo in ecx, cascading a register-name mismatch through the tail. Hoisting the
// clock into a local regressed it. Not source-steerable.
RVA(0x00061570, 0x11d)
i32 CGruntBehaviorLeaf::LoadGruntDecayConfig2() {
    if ((i64)(u32)g_frameTime - *(i64*)&m_decayTimerLo >= *(i64*)&m_decayDurationLo) {
        m_drawState->m_40 |= 1;
        m_drawState->m_194->SetAllTypes(1);
        if (m_animSuppress == 0) {
            ((CTriggerMgr*)m_260)->NotifyCell(m_animArg0, m_animArg1, 0);
        }
        m_drawState->m_8 |= 0x10000;
        return 0;
    }
    i64 e = (i64)(u32)g_frameTime - *(i64*)&m_decayTimerLo;
    u32 elapsed = e < 0 ? 0 : (u32)e;
    i32 r =
        (i32)((double)elapsed * 256.0 / (double)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0xb;
    m_object->m_fillFraction = r;
    return 0;
}

// LoadWandGruntItemConfig @0x65a60 was re-homed to GruntEntranceArrival.cpp
// (wave3-I): its retail body + private .data cells sit inside the 0x616e0 obj.
