// GruntBehaviorLeaf.cpp - the per-frame grunt "decay/wand" AI (0x612a0 / 0x61570 /
// 0x65a60), re-homed out of the UserLogic god-TU (C:\Proj\Gruntz).
//
// These run on CGruntBehaviorLeaf (a placeholder-identity CUserLogic leaf; see
// <Gruntz/GruntBehaviorLeaf.h>). The decay timer (m_830 start / m_838 duration, both
// hi=0) drives a 0..256 fixed-point fill bar on the bound object's draw command;
// m_object is the real inherited CGameObject. The bute/anim callees are reloc-masked
// __thiscall externs. g_645588 = running ms clock. Only offsets / code bytes are
// load-bearing.
#include <Gruntz/GruntBehaviorLeaf.h>
#include <Image/ImageSet.h>   // CImageSet::SetAllTypes (m_drawState->m_194)
#include <Bute/ButeTree.h>    // CButeTree::Find (the "R" animset key)
#include <Bute/ButeMgr.h>     // CButeMgr getters (Grunt/DecayTime, WANDGRUNT/HealthLoss)

extern "C" u32 g_645588;   // 0x645588  running game clock (ms)
extern "C" i32 g_6bf3bc;   // 0x6bf3bc  per-frame draw-delta (arrival probe ctx)
extern CButeMgr g_buteMgr; // 0x6453d8 - getters reloc-mask
extern CButeTree g_buteTree;
extern char k_60bebc[]; // interned bute-node name "R"

// LoadGruntDecayConfig (0x612a0): advance the arrival probe, drive the walk/idle
// anim by grunt mode, then (once arrived + not busy) latch the decay timer + fill.
// @early-stop
// 90.5%: logic byte-faithful. Residual is CSE/regalloc of the 64-bit timer delta -
// retail keeps g_645588 pinned in eax and re-does the m_decayTimerLo subtraction in the fill
// tail, while cl shares the whole `(i64)clock - m_decayTimerLo` delta, shifting the lo dword
// off eax and cascading register names + the epilogue merge. Not source-steerable.
RVA(0x000612a0, 0x23c)
i32 CGruntBehaviorLeaf::LoadGruntDecayConfig() {
    if (m_gruntMode == 0) {
        return 0;
    }
    if (m_drawState->m_1a0.Advance_15c360(g_6bf3bc) == 1) {
        if (m_gruntSubState == 1 && m_gruntMode != 5) {
            m_animCtrl->DrawAnimAt(m_object->m_screenX, m_object->m_screenY, 1, m_animArg0);
        } else {
            m_animCtrl->PlayAnimEx(
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
            m_animCtrl->Anim2a72(m_animArg0, m_animArg1, 0);
        }
        i32 dt = (i32)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8);
        if (m_object->m_drawFillCmd == 0xb) {
            m_decayDurationLo = dt;
            m_decayTimerLo = (i32)g_645588 - m_object->m_fillFraction * dt / 256;
            m_decayDurationHi = 0;
        } else {
            m_decayDurationLo = dt;
            m_decayDurationHi = 0;
            m_decayTimerLo = (i32)g_645588;
        }
        m_decayTimerHi = 0;
        i64 e = (i64)(u32)g_645588 - *(i64*)&m_decayTimerLo;
        u32 elapsed = e < 0 ? 0 : (u32)e;
        i32 r = (i32)((double)elapsed * 256.0
                      / (double)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
        m_object->m_drawActive = 1;
        m_object->m_drawFillCmd = 0xb;
        m_object->m_fillFraction = r;
        return 0;
    }
    if (m_animSuppress == 0) {
        m_animCtrl->Anim2a72(m_animArg0, m_animArg1, 0);
    }
    m_drawState->m_8 |= 0x10000;
    return 0;
}

// LoadGruntDecayConfig2 (0x61570): if the timer has fully elapsed, fire the finish
// (flag + finish anim); else refresh the 0..256 fill fraction on the draw command.
// @early-stop
// ~77%: logic byte-faithful. Same CSE/regalloc wall as LoadGruntDecayConfig -
// retail loads g_645588 once into eax and recomputes the m_decayTimerLo subtraction in the
// fill branch (eax preserved -> lo stays in eax); cl CSEs the whole 64-bit delta and
// pins lo in ecx, cascading a register-name mismatch through the tail. Hoisting the
// clock into a local regressed it. Not source-steerable.
RVA(0x00061570, 0x11d)
i32 CGruntBehaviorLeaf::LoadGruntDecayConfig2() {
    if ((i64)(u32)g_645588 - *(i64*)&m_decayTimerLo >= *(i64*)&m_decayDurationLo) {
        m_drawState->m_40 |= 1;
        m_drawState->m_194->SetAllTypes(1);
        if (m_animSuppress == 0) {
            m_animCtrl->Anim2a72(m_animArg0, m_animArg1, 0);
        }
        m_drawState->m_8 |= 0x10000;
        return 0;
    }
    i64 e = (i64)(u32)g_645588 - *(i64*)&m_decayTimerLo;
    u32 elapsed = e < 0 ? 0 : (u32)e;
    i32 r =
        (i32)((double)elapsed * 256.0 / (double)g_buteMgr.GetDwordDef("Grunt", "DecayTime", 0xbb8));
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0xb;
    m_object->m_fillFraction = r;
    return 0;
}

// LoadWandGruntItemConfig (0x65a60): per-frame wand-grunt item logic. Advance the
// arrival probe; on the peak phase (0x63) latch the item downtime timer, tick the
// wand health loss, and fire the depletion anim; every active frame run the wand
// projectile step; finally, once arrived + idle, clear the latch + run the reset.
// @early-stop
// ~95%: whole body byte-identical (incl. the branchless max(0,hp) sub/sets/dec/and
// idiom) except cl schedules the `if (m_1c4)` load a few slots earlier than retail
// (which interleaves it among the timer zero-stores). Pure scheduling; not steerable.
RVA(0x00065a60, 0x159)
i32 CGruntBehaviorLeaf::LoadWandGruntItemConfig() {
    i32 phase = m_drawState->m_1a0.Advance_15c360(g_6bf3bc);
    if (phase > 0) {
        if (phase == 0x63) {
            m_downtimeLatch = 1;
            u32 downtime = g_buteMgr.GetDword(m_gruntTypeTag, "ItemDowntime");
            if (m_typeDisc == 0x3b) {
                downtime = 0;
            }
            m_wandDowntimeLo = downtime;
            m_wandDowntimeHi = 0;
            m_wandTimerLo = g_645588;
            m_wandTimerHi = 0;
            m_460 = 0;
            m_3f0 = 0;
            if (m_1c4 != 0) {
                RefreshDecay();
            }
            if (m_gruntSubState == 0x13) {
                SetDecayTarget(m_380);
                i32 hp = m_health - g_buteMgr.GetIntDef("WANDGRUNT", "HealthLoss", 0x19);
                m_health = hp < 0 ? 0 : hp;
                if (m_health <= 0) {
                    m_animCtrl->SetAnim(m_animArg0, m_animArg1, 1, -1);
                }
            }
        }
        m_animCtrl->PlayStateAnim(m_animArg0, m_animArg1, m_3e4, m_3e8, m_gruntSubState, phase);
    }
    CAniAdvanceCursor* sub = &m_drawState->m_1a0;
    if (sub->m_28 != 0 && sub->m_20 == 0) {
        m_downtimeLatch = 0;
        InitAnimState(1, 0, 0);
    }
    return 0;
}
