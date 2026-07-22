#include <Gruntz/MovingLogic.h> // own extern surface
#include <Gruntz/MotionState.h>
#include <math.h>
#include <rva.h>
#include <Gruntz/Projectile.h> // g_movingLogicMax (ex .cpp extern)

DATA(0x001f0500)
const double g_motionZero = 0.0; // 0x5f0500 (owner-TU def; decl in MotionState.h)
DATA(0x001f0508)
const double g_motionNegTwo = -2.0; // 0x5f0508  discriminant term (owner-TU def)

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
    m_70 = g_movingLogicMin;
    m_88 = g_movingLogicMax;
    m_78 = g_movingLogicMin;
    m_90 = g_movingLogicMax;
    m_80 = g_movingLogicMin;
    m_98 = g_movingLogicMax;
    m_d8 = g_movingLogicMax;
    m_e0 = g_movingLogicMax;
    m_e8 = g_movingLogicMax;
    m_f0 = g_movingLogicMax;
    m_f8 = g_movingLogicMax;
    m_100 = g_movingLogicMax;
}

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

RVA(0x00058ca0, 0x19)
void CMotionState::SetZ(double z) {
    m_d8 = z;
    m_e0 = z;
    m_e8 = z;
}

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
