#include <rva.h>

#include <Gruntz/MovingLogic.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

#include <math.h>
#include <stddef.h>
#include <strstrea.h>

DATA(0x001f04f0)
const double g_motionTimeScale = 0.001;

DATA(0x001f04f8)
const double g_motionNegHalf = -0.5;

DATA(0x001f0500)
const double g_motionZero = 0.0;
DATA(0x001f0508)
const double g_motionNegTwo = -2.0;

// @early-stop
RVA(0x0016ea90, 0x234)
void CMovingLogic::AdvanceMotion() {

    m_previousScreenPosition.m_x = static_cast<i32>(Motion()->m_position.x);
    m_previousScreenPosition.m_y = static_cast<i32>(Motion()->m_position.y);
    Motion()->Step(static_cast<double>(g_frameTime) * g_motionTimeScale - Motion()->m_time);

    if ((m_object->m_flags & 0x10) && m_object->m_carrier != NULL) {
        m_object->m_screenX += m_object->m_carrier->m_deltaX;
        Motion()->m_position.x = static_cast<double>(m_object->m_screenX);
        m_object->m_screenY += m_object->m_carrier->m_deltaY;
        Motion()->m_position.y = static_cast<double>(m_object->m_screenY);
    }

    if (m_object->m_moveMode == MOVE_GROUNDED) {
        m_collisionFlags = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_position.x),
            m_object->m_screenY,
            m_moveFlags
        );
        Motion()->m_velocity.y = 0.0;
    } else {
        m_object->m_flags &= ~0x10;
        m_collisionFlags = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_position.x),
            static_cast<i32>(Motion()->m_position.y),
            m_moveFlags
        );
    }

    CMotionState* ms = Motion();
    i32 sx = m_object->m_screenX;
    if (static_cast<i32>(Motion()->m_position.x) != sx) {
        double d = static_cast<double>(sx);
        ms->m_velocity.x = ms->ArrivalVelX(d);
        double a0new = ms->m_step.x - (ms->m_position.x - d);
        ms->m_position.x = d;
        ms->m_step.x = a0new;
    }

    i32 sy = m_object->m_screenY;
    if (static_cast<i32>(Motion()->m_position.y) != sy) {
        double d = static_cast<double>(sy);
        ms->m_velocity.y = ms->ArrivalVelY(d);
        double a8new = ms->m_step.y - (ms->m_position.y - d);
        ms->m_position.y = d;
        ms->m_step.y = a8new;
    }

    if (m_object->m_moveMode != MOVE_DIRECT) {
        i32 f = m_collisionFlags;
        if (f & 0x800000) {
            Motion()->m_velocity.y = -Motion()->m_velocity.y;
            return;
        }
        if (f & 0x40000) {
            Motion()->m_maxBounds.x = static_cast<double>(m_previousScreenPosition.m_x);
            Motion()->m_velocity.x = Motion()->m_velocity.x * g_motionNegHalf;
            return;
        }
        if (f & 0x80000) {
            Motion()->m_minBounds.x = static_cast<double>(m_previousScreenPosition.m_x);
            Motion()->m_velocity.x = Motion()->m_velocity.x * g_motionNegHalf;
        }
    }
}

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

RVA(0x0016f4a0, 0x1da)
i32 CMovingLogic::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (arc == NULL) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            i32 len;
            arc->Read(&len, sizeof(len));
            char* buf = new char[len];
            arc->Read(buf, len);
            istrstream accum(buf, len);
            ReadCurve(accum, *Motion());
            delete[] buf;
            arc->Read(&m_previousScreenPosition.m_x, sizeof(m_previousScreenPosition.m_x));
            arc->Read(&m_previousScreenPosition.m_y, sizeof(m_previousScreenPosition.m_y));
            arc->Read(&m_collisionFlags, sizeof(m_collisionFlags));
            arc->Read(&m_moveFlags, sizeof(m_moveFlags));

            break;
        }
        case SERIAL_SAVE: {

            char buf[0x100];
            ostrstream accum(buf, 0x100);
            WriteCurve(accum, *Motion());
            i32 len = accum.pcount();
            arc->Write(&len, sizeof(len));
            arc->Write(accum.str(), len);
            arc->Write(&m_previousScreenPosition.m_x, sizeof(m_previousScreenPosition.m_x));
            arc->Write(&m_previousScreenPosition.m_y, sizeof(m_previousScreenPosition.m_y));
            arc->Write(&m_collisionFlags, sizeof(m_collisionFlags));
            arc->Write(&m_moveFlags, sizeof(m_moveFlags));

            break;
        }
    }
    return CUserLogic::SerializeMove(arc, mode, typeId, pObj) != 0;
}
