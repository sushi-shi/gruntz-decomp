#ifndef GRUNTZ_MOTIONSTATE_H
#define GRUNTZ_MOTIONSTATE_H

#include <Ints.h>
#include <rva.h>

extern const double g_movingLogicMin; // 0x5f04b0 (-2147483647.0)
extern const double g_movingLogicMax; // 0x5f04b8 (2147483646.0)

extern const double g_motionNegHalf; // 0x5f04f8 (-0.5)
extern const double g_motionZero;    // 0x5f0500 (0.0)
extern const double g_motionNegTwo;  // 0x5f0508 (-2.0)

class CMotionState {
public:
    CMotionState(); // 0x136d0
    // 0x58bc0 - seed all three axes at once. The role of each slot is fixed by Step's
    // integrator below (STEP_AXIS(v=m_28.., a=m_10.., s=m_40..)) and corroborated by
    // ArrivalVelX/Y, which solve the textbook v^2 = v0^2 + 2*a*(target - s) with
    // exactly m_28 as v, m_10 as a and m_40 as s.
    i32 SetParams(
        double posX,
        double posY,
        double posZ,
        double velX,
        double velY,
        double velZ,
        double accelX,
        double accelY,
        double accelZ,
        double clock,
        double dt
    );
    // 0x58ca0 - NOT a Z setter: it writes the per-axis max |step| (m_d8/m_e0/m_e8, the
    // `vmax` STEP_AXIS clamps each axis's per-frame travel against) on all three axes.
    void SetZ(double z);
    void Step(double dt);              // 0x16ecd0
    double ArrivalVelX(double target); // 0x16f3c0
    double ArrivalVelY(double target); // 0x16f430

    // Layout roles read off Step's STEP_AXIS expansion (the old comments here had
    // rate/position/accel rotated one group out of place).
    double m_00, m_08;       // accumulated clock, last dt
    double m_10, m_18, m_20; // per-axis ACCELERATION (the `a` of STEP_AXIS)
    double m_28, m_30, m_38; // per-axis VELOCITY     (the `v`)
    double m_40, m_48, m_50; // per-axis POSITION     (the `s`)
    double m_58, m_60, m_68; // previous-frame position snapshot (Step's first 3 stores)
    double m_70, m_78, m_80; // per-axis position lower band (MIN)
    double m_88, m_90, m_98; // per-axis position upper band (MAX)
    double m_a0, m_a8, m_b0; // per-axis per-step travel written back (the `scr`)
    i32 m_b8;                // freeze flag (Step returns early when set)
    char m_padbc[0xc0 - 0xbc];
    double m_c0, m_c8, m_d0;  // spare (zeroed)
    double m_d8, m_e0, m_e8;  // per-axis max |step| per frame (the `vmax`; SetZ writes it)
    double m_f0, m_f8, m_100; // per-axis velocity clamp (the `posClamp`)
};
SIZE_UNKNOWN();

#ifndef CMOTIONSTATE_STANDALONE_CTOR
inline CMotionState::CMotionState() {
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
#endif

extern const double g_motionNegHalf;
extern const double g_motionNegTwo;
#endif // GRUNTZ_MOTIONSTATE_H
