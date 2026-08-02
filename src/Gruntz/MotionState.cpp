#define CMOTIONSTATE_STANDALONE_CTOR
#include <Gruntz/MovingLogic.h>
#include <Gruntz/MotionState.h>
#include <math.h>
#include <rva.h>
#include <Gruntz/Projectile.h>

DATA(0x001f0500)
const double g_motionZero = 0.0;
DATA(0x001f0508)
const double g_motionNegTwo = -2.0;

RVA(0x000136d0, 0x184)
CMotionState::CMotionState() {
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
    m_c0.x = 0.0;
    m_c0.y = 0.0;
    m_c0.z = 0.0;
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

RVA(0x00058bc0, 0xa1)
i32 CMotionState::SetParams(
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
) {
    m_position.x = posX;
    m_position.y = posY;
    m_position.z = posZ;
    m_velocity.x = velX;
    m_velocity.y = velY;
    m_velocity.z = velZ;
    m_acceleration.x = accelX;
    m_acceleration.y = accelY;
    m_acceleration.z = accelZ;
    m_time = clock;
    m_deltaTime = dt;
    return 1;
}

RVA(0x00058ca0, 0x19)
void CMotionState::SetZ(double z) {
    m_maxStep.x = z;
    m_maxStep.y = z;
    m_maxStep.z = z;
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

// @early-stop
RVA(0x0016ecd0, 0x6e6)
void CMotionState::Step(double dt) {
    m_previousPosition.x = m_position.x;
    m_previousPosition.y = m_position.y;
    m_previousPosition.z = m_position.z;
    m_deltaTime = dt;
    m_time += dt;
    if (m_stepDisabled != 0) {
        return;
    }
    STEP_AXIS(
        m_velocity.x,
        m_acceleration.x,
        m_position.x,
        m_maxStep.x,
        m_minBounds.x,
        m_maxBounds.x,
        m_maxVelocity.x,
        m_step.x
    );
    STEP_AXIS(
        m_velocity.y,
        m_acceleration.y,
        m_position.y,
        m_maxStep.y,
        m_minBounds.y,
        m_maxBounds.y,
        m_maxVelocity.y,
        m_step.y
    );
    STEP_AXIS(
        m_velocity.z,
        m_acceleration.z,
        m_position.z,
        m_maxStep.z,
        m_minBounds.z,
        m_maxBounds.z,
        m_maxVelocity.z,
        m_step.z
    );
}

RVA(0x0016f3c0, 0x61)
double CMotionState::ArrivalVelX(double target) {
    if (m_acceleration.x == 0.0) {
        return m_velocity.x;
    }
    double disc =
        m_velocity.x * m_velocity.x - (target - m_position.x) * m_acceleration.x * g_motionNegTwo;
    if (0.0 > disc) {
        disc = 0.0;
    }
    double r = sqrt(disc);
    return (m_velocity.x > 0.0) ? r : -r;
}

RVA(0x0016f430, 0x61)
double CMotionState::ArrivalVelY(double target) {
    if (m_acceleration.y == 0.0) {
        return m_velocity.y;
    }
    double disc =
        m_velocity.y * m_velocity.y - (target - m_position.y) * m_acceleration.y * g_motionNegTwo;
    if (0.0 > disc) {
        disc = 0.0;
    }
    double r = sqrt(disc);
    return (m_velocity.y > 0.0) ? r : -r;
}
