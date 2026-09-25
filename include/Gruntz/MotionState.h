#ifndef GRUNTZ_MOTIONSTATE_H
#define GRUNTZ_MOTIONSTATE_H

#include <rva.h>

#include <Gruntz/DoubleVector.h>
#include <Ints.h>

extern const double g_movingLogicMin;
extern const double g_movingLogicMax;

extern const double g_motionNegHalf;
extern const double g_motionZero;
extern const double g_motionNegTwo;

class CMotionState {
public:
    enum EInlineBase {
        INLINE_BASE
    };

    CMotionState();
    CMotionState(EInlineBase);
    ~CMotionState();

    void InitBounds();

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

    double m_time;
    double m_deltaTime;
    DoubleVector3 m_acceleration;
    DoubleVector3 m_velocity;
    DoubleVector3 m_position;
    DoubleVector3 m_previousPosition;
    DoubleVector3 m_minBounds;
    DoubleVector3 m_maxBounds;
    DoubleVector3 m_step;
    b32 m_stepDisabled;
    DoubleVector3 m_reservedc0; // zeroed + save-streamed; Step never reads it
    DoubleVector3 m_maxStep;
    DoubleVector3 m_maxVelocity;
};

inline CMotionState::~CMotionState() {}

inline CMotionState::CMotionState(EInlineBase) {
    InitBounds();
}

inline void CMotionState::InitBounds() {
    VEC3_SET(m_position, 0.0, 0.0, 0.0);
    VEC3_SET(m_velocity, 0.0, 0.0, 0.0);
    VEC3_SET(m_acceleration, 0.0, 0.0, 0.0);
    m_time = 0.0;
    m_deltaTime = 0.0;
    VEC3_SET(m_reservedc0, 0.0, 0.0, 0.0);
    m_stepDisabled = false;
    m_minBounds.m_x = g_movingLogicMin;
    m_maxBounds.m_x = g_movingLogicMax;
    m_minBounds.m_y = g_movingLogicMin;
    m_maxBounds.m_y = g_movingLogicMax;
    m_minBounds.m_z = g_movingLogicMin;
    m_maxBounds.m_z = g_movingLogicMax;
    VEC3_SET(m_maxStep, g_movingLogicMax, g_movingLogicMax, g_movingLogicMax);
    VEC3_SET(m_maxVelocity, g_movingLogicMax, g_movingLogicMax, g_movingLogicMax);
}

#define INITIALIZE_MOTION_BOUNDS(state)                                                            \
    (state).m_position.m_x = 0.0;                                                                  \
    (state).m_position.m_y = 0.0;                                                                  \
    (state).m_position.m_z = 0.0;                                                                  \
    (state).m_velocity.m_x = 0.0;                                                                  \
    (state).m_velocity.m_y = 0.0;                                                                  \
    (state).m_velocity.m_z = 0.0;                                                                  \
    (state).m_acceleration.m_x = 0.0;                                                              \
    (state).m_acceleration.m_y = 0.0;                                                              \
    (state).m_acceleration.m_z = 0.0;                                                              \
    (state).m_time = 0.0;                                                                          \
    (state).m_deltaTime = 0.0;                                                                     \
    (state).m_reservedc0.m_x = 0.0;                                                                \
    (state).m_reservedc0.m_y = 0.0;                                                                \
    (state).m_reservedc0.m_z = 0.0;                                                                \
    (state).m_stepDisabled = false;                                                                \
    SET_VECTOR3_BOUNDS(                                                                            \
        (state).m_minBounds,                                                                       \
        (state).m_maxBounds,                                                                       \
        g_movingLogicMin,                                                                          \
        g_movingLogicMax                                                                           \
    );                                                                                             \
    (state).m_maxStep.m_x = g_movingLogicMax;                                                      \
    (state).m_maxStep.m_y = g_movingLogicMax;                                                      \
    (state).m_maxStep.m_z = g_movingLogicMax;                                                      \
    (state).m_maxVelocity.m_x = g_movingLogicMax;                                                  \
    (state).m_maxVelocity.m_y = g_movingLogicMax;                                                  \
    (state).m_maxVelocity.m_z = g_movingLogicMax;

#endif // GRUNTZ_MOTIONSTATE_H
