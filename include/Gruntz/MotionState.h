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
    // Tag type: picks the inline sibling of the out-of-line 0x136d0 ctor.
    enum EInlineBase {
        INLINE_BASE
    };

    // Out of line at 0x136d0.  Retail `call`s it from CGrunt::CGrunt,
    // CProjectile::CProjectile and SerialObjectFactory.
    CMotionState();
    // The expanded sibling: CMovingLogic's and CProjectile's default ctors carry
    // this body inline.
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
    i32 m_stepDisabled;
    char m_padbc[0xc0 - 0xbc];
    DoubleVector3 m_reservedc0; // zeroed + save-streamed; Step never reads it
    DoubleVector3 m_maxStep;
    DoubleVector3 m_maxVelocity;
};

inline CMotionState::~CMotionState() {}

inline CMotionState::CMotionState(EInlineBase) {
    InitBounds();
}

// The one textual copy of the ctor body.  Both CMotionState ctor entities expand it.
inline void CMotionState::InitBounds() {
    m_position.x = 0.0;
    m_position.y = 0.0;
    m_position.z = 0.0;
    m_velocity.x = 0.0;
    m_velocity.y = 0.0;
    m_velocity.z = 0.0;
    m_acceleration.x = 0.0;
    m_acceleration.y = 0.0;
    m_acceleration.z = 0.0;
    m_time = 0.0;
    m_deltaTime = 0.0;
    m_reservedc0.x = 0.0;
    m_reservedc0.y = 0.0;
    m_reservedc0.z = 0.0;
    m_stepDisabled = 0;
    m_minBounds.x = g_movingLogicMin;
    m_maxBounds.x = g_movingLogicMax;
    m_minBounds.y = g_movingLogicMin;
    m_maxBounds.y = g_movingLogicMax;
    m_minBounds.z = g_movingLogicMin;
    m_maxBounds.z = g_movingLogicMax;
    m_maxStep.x = g_movingLogicMax;
    m_maxStep.y = g_movingLogicMax;
    m_maxStep.z = g_movingLogicMax;
    m_maxVelocity.x = g_movingLogicMax;
    m_maxVelocity.y = g_movingLogicMax;
    m_maxVelocity.z = g_movingLogicMax;
}

#endif // GRUNTZ_MOTIONSTATE_H
