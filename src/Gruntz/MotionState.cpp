// MotionState.cpp - CMotionState, the 3-axis kinematic subobject embedded at
// CMovingLogic+0x38 (see include/Gruntz/MotionState.h). Standalone helper: a
// plain ctor (no vptr), two parameter setters, and a per-frame easing integrator.
#include <Gruntz/MotionState.h>
#include <math.h>
#include <rva.h>
#include <Globals.h>

// The shared .rdata bound/easing doubles the methods read by plain dword loads.
DATA(0x001f04b0)
extern const double g_motionMin;
DATA(0x001f04b8)
extern const double g_motionMax;
DATA(0x001f0500)
extern const double g_motionZero;

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

// CMotionState::SetZ (0x00058ca0) is now an inline member in the header.


// ---------------------------------------------------------------------------
// STEP_AXIS - the per-axis uniformly-accelerated 1D integrator, copy-pasted 3x
// down the X/Y/Z columns (the retail body is three literal blocks, each with a
// shallow x87 stack carrying `dt`). Per axis:
//   v = velocity (m_28/30/38), a = acceleration (m_10/18/20),
//   s = displacement (m_40/48/50) accumulated by the half-step `(v+0.5*dt*a)*dt`,
//   vmax = velocity-magnitude clamp (m_d8/e0/e8), [loBand,hiBand] = displacement
//   band (m_70/78/80, m_88/90/98), posClamp = max velocity (m_f0/f8/100),
//   scr = output scratch (m_a0/a8/b0). When the half-step or the running
//   displacement leaves its band the velocity is re-solved from v^2 = v0^2 + 2*a*ds.
#define STEP_AXIS(v, a, s, vmax, loBand, hiBand, posClamp, scr)                                    \
    do {                                                                                           \
        double step0 = dt * a;                                                                     \
        double t = (v - step0 * g_motionNegHalf) * dt;                                             \
        scr = t;                                                                                   \
        if (t > vmax || t < -vmax) {                                                               \
            double c = (t > vmax) ? vmax : -vmax;                                                  \
            scr = c;                                                                               \
            if (a != 0.0) {                                                                        \
                double disc = v * v - c * a * g_motionNegTwo;                                      \
                if (disc < 0.0)                                                                    \
                    disc = 0.0;                                                                    \
                double r = sqrt(disc);                                                             \
                v = (v > 0.0) ? r : -r;                                                            \
            }                                                                                      \
        }                                                                                          \
        double oldS = s;                                                                           \
        double newS = scr + s;                                                                     \
        s = newS;                                                                                  \
        if (newS > hiBand) {                                                                       \
            if (a != 0.0) {                                                                        \
                double disc = v * v - (hiBand - newS) * a * g_motionNegTwo;                        \
                if (disc < 0.0)                                                                    \
                    disc = 0.0;                                                                    \
                double r = sqrt(disc);                                                             \
                v = (v > 0.0) ? r : -r;                                                            \
            }                                                                                      \
            scr = hiBand - oldS;                                                                   \
            s = hiBand;                                                                            \
        } else if (newS < loBand) {                                                                \
            if (a != 0.0) {                                                                        \
                double disc = v * v - (loBand - newS) * a * g_motionNegTwo;                        \
                if (disc < 0.0)                                                                    \
                    disc = 0.0;                                                                    \
                double r = sqrt(disc);                                                             \
                v = (v > 0.0) ? r : -r;                                                            \
            }                                                                                      \
            scr = loBand - oldS;                                                                   \
            s = loBand;                                                                            \
        } else {                                                                                   \
            v += step0;                                                                            \
        }                                                                                          \
        if (v > posClamp)                                                                          \
            v = posClamp;                                                                          \
    } while (0)

// ---------------------------------------------------------------------------
// @early-stop
// x87 stack-schedule wall (~65%): math + control flow + every member store are
// byte-exact; the residual is MSVC5's fld-st(0)-vs-fld-[mem] / fxch choreography
// in the per-axis quadratic-solve + clamp blocks, which is not source-steerable
// (docs/patterns/x87-fp-stack-schedule.md, x87-copypaste-vs-inline-fp-block.md).
RVA(0x0016ecd0, 0x6e6)
void CMotionState::Step(double dt) {
    m_58 = m_40;
    m_60 = m_48;
    m_68 = m_50;
    m_08 = dt;
    m_00 += dt;
    if (m_b8 != 0) {
        return;
    }
    STEP_AXIS(m_28, m_10, m_40, m_d8, m_70, m_88, m_f0, m_a0);
    STEP_AXIS(m_30, m_18, m_48, m_e0, m_78, m_90, m_f8, m_a8);
    STEP_AXIS(m_38, m_20, m_50, m_e8, m_80, m_98, m_100, m_b0);
}

// ---------------------------------------------------------------------------
// Per-axis arrival-velocity solve: with zero rate the current X velocity is
// returned unchanged; otherwise v_final = signed sqrt(v^2 + 2*a*ds) where
// ds = target - m_40 (same v^2 = v0^2 + 2*a*ds root used inside STEP_AXIS).
RVA(0x0016f3c0, 0x61)
double CMotionState::ArrivalVelX(double target) {
    if (m_10 == 0.0) {
        return m_28;
    }
    double disc = m_28 * m_28 - (target - m_40) * m_10 * g_motionNegTwo;
    if (0.0 > disc) {
        disc = 0.0;
    }
    double r = sqrt(disc);
    return (m_28 > 0.0) ? r : -r;
}

// ---------------------------------------------------------------------------
RVA(0x0016f430, 0x61)
double CMotionState::ArrivalVelY(double target) {
    if (m_18 == 0.0) {
        return m_30;
    }
    double disc = m_30 * m_30 - (target - m_48) * m_18 * g_motionNegTwo;
    if (0.0 > disc) {
        disc = 0.0;
    }
    double r = sqrt(disc);
    return (m_30 > 0.0) ? r : -r;
}
