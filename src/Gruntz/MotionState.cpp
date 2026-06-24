// MotionState.cpp - CMotionState, the 3-axis kinematic subobject embedded at
// CMovingLogic+0x38 (see include/Gruntz/MotionState.h). Standalone helper: a
// plain ctor (no vptr), two parameter setters, and a per-frame easing integrator.
#include <Gruntz/MotionState.h>
#include <rva.h>

// The shared .rdata bound/easing doubles the methods read by plain dword loads.
DATA(0x001f04b0)
extern const double g_motionMin;
DATA(0x001f04b8)
extern const double g_motionMax;
DATA(0x001f04f8)
extern const double g_motionNegHalf;
DATA(0x001f0500)
extern const double g_motionZero;
DATA(0x001f0508)
extern const double g_motionNegTwo;

// ---------------------------------------------------------------------------
RVA(0x000136d0, 0x184)
CMotionState::CMotionState() {
    m_40 = 0.0;
    m_48 = 0.0;
    m_50 = 0.0;
    m_28 = 0.0;
    m_30 = 0.0;
    m_38 = 0.0;
    m_10 = 0.0;
    m_18 = 0.0;
    m_20 = 0.0;
    m_00 = 0.0;
    m_08 = 0.0;
    m_c0 = 0.0;
    m_c8 = 0.0;
    m_d0 = 0.0;
    m_b8 = 0;
    m_70 = g_motionMin;
    m_88 = g_motionMax;
    m_78 = g_motionMin;
    m_90 = g_motionMax;
    m_80 = g_motionMin;
    m_98 = g_motionMax;
    m_d8 = g_motionMax;
    m_e0 = g_motionMax;
    m_e8 = g_motionMax;
    m_f0 = g_motionMax;
    m_f8 = g_motionMax;
    m_100 = g_motionMax;
}

// ---------------------------------------------------------------------------
RVA(0x00058bc0, 0xa1)
i32 CMotionState::SetParams(
    double a0,
    double a1,
    double a2,
    double a3,
    double a4,
    double a5,
    double a6,
    double a7,
    double a8,
    double a9,
    double a10
) {
    m_40 = a0;
    m_48 = a1;
    m_50 = a2;
    m_28 = a3;
    m_30 = a4;
    m_38 = a5;
    m_10 = a6;
    m_18 = a7;
    m_20 = a8;
    m_00 = a9;
    m_08 = a10;
    return 1;
}

// ---------------------------------------------------------------------------
RVA(0x00058ca0, 0x19)
void CMotionState::SetZ(double z) {
    m_d8 = z;
    m_e0 = z;
    m_e8 = z;
}

// Step (0x16ecd0, 1766 B) - the per-frame 3-axis easing integrator - stays a
// stub in src/Stub/Discovered.cpp for the final sweep. It is a single large,
// hand-scheduled x87 routine (fcom/fnstsw/test-ah min-max/conditional-negate +
// fsqrt chains repeated per axis); a partial reconstruction would diverge its
// own regalloc and under-count, so per the big-FP STOP-EARLY rule it is left
// for a dedicated leaf-first pass.
