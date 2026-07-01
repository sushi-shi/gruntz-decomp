// MotionState.h - the per-object 3-axis kinematic state embedded at
// CMovingLogic+0x38 (C:\Proj\Gruntz). A standalone (non-polymorphic) helper the
// moving-object family owns: every projectile/grunt-projectile leaf ctor does
// `lea reg,[this+0x38]; call CMotionState::CMotionState` on this subobject, then
// drives it through SetParams (0x58bc0) / SetZ (0x58ca0) and steps it each frame
// via Step (0x16ecd0).
//
// Layout: a band of `double`s for three axes (X/Y/Z), interleaved as columns:
//   +0x00/+0x08  the per-step time delta (dt) + its running copy
//   +0x10/+0x18/+0x20  per-axis rate coefficient
//   +0x28/+0x30/+0x38  per-axis position
//   +0x40/+0x48/+0x50  per-axis acceleration/target
//   +0x58/+0x60/+0x68  per-axis acceleration snapshot (latched at Step entry)
//   +0x70/+0x78/+0x80  per-axis lower target band   (seeded MIN)
//   +0x88/+0x90/+0x98  per-axis upper target band   (seeded MAX)
//   +0xa0/+0xa8/+0xb0  per-axis output scratch
//   +0xb8              int "snap" flag
//   +0xc0/+0xc8/+0xd0  per-axis spare double (zeroed)
//   +0xd8/+0xe0/+0xe8  per-axis position upper clamp (seeded MAX)
//   +0xf0/+0xf8/+0x100 per-axis position lower/upper clamp (seeded MAX)
// The clamp/bound defaults are copied from two shared .rdata doubles
// (0x5f04b0 = MIN = -2147483647.0, 0x5f04b8 = MAX = 2147483646.0) via plain
// integer dword moves, so they are modeled as named double constants the ctor
// reads (a literal would materialize 32-bit immediates, not the dword loads).
#ifndef GRUNTZ_MOTIONSTATE_H
#define GRUNTZ_MOTIONSTATE_H

#include <Ints.h>
#include <rva.h>

extern const double g_motionMin; // 0x5f04b0 (-2147483647.0)
extern const double g_motionMax; // 0x5f04b8 (2147483646.0)

// The shared easing constants the Step integrator folds in.
extern const double g_motionNegHalf; // 0x5f04f8 (-0.5)
extern const double g_motionZero;    // 0x5f0500 (0.0)
extern const double g_motionNegTwo;  // 0x5f0508 (-2.0)

SIZE_UNKNOWN(CMotionState);
class CMotionState {
public:
    CMotionState(); // 0x136d0
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

#endif // GRUNTZ_MOTIONSTATE_H
