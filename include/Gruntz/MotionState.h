#ifndef GRUNTZ_MOTIONSTATE_H
#define GRUNTZ_MOTIONSTATE_H

#include <Ints.h>
#include <rva.h>

extern const double g_motionMin; // 0x5f04b0 (-2147483647.0)
extern const double g_motionMax; // 0x5f04b8 (2147483646.0)

extern const double g_motionNegHalf; // 0x5f04f8 (-0.5)
extern const double g_motionZero;    // 0x5f0500 (0.0)
extern const double g_motionNegTwo;  // 0x5f0508 (-2.0)

class CMotionState {
public:
    CMotionState(); // 0x136d0
    // The out-of-line ctor entry (0x136d0) the fat-world CMovingLogic leaf ctors
    // invoke via a `+0x38` cast (the band is not a direct member of a fat-CUserLogic
    // CMovingLogic; see Gruntz/MovingLogic.h). Same address as the ctor; reloc-masked.
    void Init(); // 0x136d0
    i32 SetParams(
        double a0,
        double a1,
        double a2,
        double a3,
        double a4, // 0x58bc0
        double a5,
        double a6,
        double a7,
        double a8,
        double a9,
        double a10
    );
    void SetZ(double z);               // 0x58ca0
    void Step(double dt);              // 0x16ecd0
    double ArrivalVelX(double target); // 0x16f3c0
    double ArrivalVelY(double target); // 0x16f430

    double m_00, m_08;       // dt + copy
    double m_10, m_18, m_20; // per-axis rate
    double m_28, m_30, m_38; // per-axis position
    double m_40, m_48, m_50; // per-axis accel/target
    double m_58, m_60, m_68; // per-axis accel snapshot
    double m_70, m_78, m_80; // per-axis lower band (MIN)
    double m_88, m_90, m_98; // per-axis upper band (MAX)
    double m_a0, m_a8, m_b0; // per-axis output scratch
    i32 m_b8;                // snap flag
    char m_padbc[0xc0 - 0xbc];
    double m_c0, m_c8, m_d0;  // spare (zeroed)
    double m_d8, m_e0, m_e8;  // per-axis upper clamp (MAX)
    double m_f0, m_f8, m_100; // per-axis clamp (MAX)
};
SIZE_UNKNOWN();

extern const double g_motionNegHalf;
extern const double g_motionNegTwo;
#endif // GRUNTZ_MOTIONSTATE_H
