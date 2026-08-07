#include <rva.h>

#include <Gruntz/MotionState.h>
#include <Gruntz/MovingLogic.h>
#include <Gruntz/Projectile.h>

#include <math.h>

DATA(0x001f0500)
const double g_motionZero = 0.0;
DATA(0x001f0508)
const double g_motionNegTwo = -2.0;

// The velocity update is the ArrivalVel<axis> shape written INLINE: an if/else
// that YIELDS a value, so retail's `a == 0` arm reloads and re-stores the
// velocity in place (`fld [ecx+0x38]; fstp [ecx+0x38]`).  It is fed an ABSOLUTE
// target and subtracts the position back off - the first site builds
// `s + c` and the macro immediately computes `(target) - s` again
// (`fadd [ecx+0x50]` ... `fsub [ecx+0x50]`); FP is not associative so cl keeps
// both.  ArrivalVelX/Y/Z sit BELOW Step at 0x16f3c0/0x16f430, so retail's cl
// could not have inlined them either - this is the same body, spelled out.
#define ARRIVAL_V(v, a, s, target)                                                                 \
    do {                                                                                           \
        double nv;                                                                                 \
        if (a == g_motionZero) {                                                                   \
            nv = v;                                                                                \
        } else {                                                                                   \
            double disc = v * v - ((target) - (s)) * a * g_motionNegTwo;                           \
            if (g_motionZero > disc) {                                                             \
                disc = g_motionZero;                                                               \
            }                                                                                      \
            double r = sqrt(disc);                                                                 \
            nv = (v > g_motionZero) ? r : -r;                                                      \
        }                                                                                          \
        v = nv;                                                                                    \
    } while (0)

#define STEP_AXIS(v, a, s, vmax, loBand, hiBand, posClamp, scr)                                    \
    do {                                                                                           \
        double step0 = dt * a;                                                                     \
        double t = (v - step0 * g_motionNegHalf) * dt;                                             \
        double c;                                                                                  \
        scr = t;                                                                                   \
        if (t > vmax) {                                                                            \
            c = vmax;                                                                              \
            scr = c;                                                                               \
            ARRIVAL_V(v, a, s, c + s);                                                             \
        } else if (t < -vmax) {                                                                    \
            c = -vmax;                                                                             \
            scr = c;                                                                               \
            ARRIVAL_V(v, a, s, c + s);                                                             \
        }                                                                                          \
        double oldS = s;                                                                           \
        double newS = scr + s;                                                                     \
        s = newS;                                                                                  \
        if (newS > hiBand) {                                                                       \
            ARRIVAL_V(v, a, s, hiBand);                                                            \
            scr = hiBand - oldS;                                                                   \
            s = hiBand;                                                                            \
        } else if (newS < loBand) {                                                                \
            ARRIVAL_V(v, a, s, loBand);                                                            \
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
    m_time = dt + m_time;
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
