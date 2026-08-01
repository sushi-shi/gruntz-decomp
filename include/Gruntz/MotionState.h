#ifndef GRUNTZ_MOTIONSTATE_H
#define GRUNTZ_MOTIONSTATE_H

#include <Ints.h>
#include <rva.h>

extern const double g_movingLogicMin;
extern const double g_movingLogicMax;

extern const double g_motionNegHalf;
extern const double g_motionZero;
extern const double g_motionNegTwo;

class CMotionState {
public:
    CMotionState();

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

    void SetZ(double z);
    void Step(double dt);
    double ArrivalVelX(double target);
    double ArrivalVelY(double target);

    double m_00, m_08;
    double m_10, m_18, m_20;
    double m_28, m_30, m_38;
    double m_40, m_48, m_50;
    double m_58, m_60, m_68;
    double m_70, m_78, m_80;
    double m_88, m_90, m_98;
    double m_a0, m_a8, m_b0;
    i32 m_b8;
    char m_padbc[0xc0 - 0xbc];
    double m_c0, m_c8, m_d0;
    double m_d8, m_e0, m_e8;
    double m_f0, m_f8, m_100;
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
