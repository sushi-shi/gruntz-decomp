#include <rva.h>

#include <Gruntz/MovingLogic.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <Wwd/MoveFlags.h>

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

    m_previousScreenPosition.m_x = static_cast<i32>(Motion()->m_position.m_x);
    m_previousScreenPosition.m_y = static_cast<i32>(Motion()->m_position.m_y);
    Motion()->Step(static_cast<double>(g_frameTime) * g_motionTimeScale - Motion()->m_time);

    if ((m_object->m_flags & IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER)) && m_object->m_carrier != NULL) {
        m_object->m_screenX += m_object->m_carrier->m_deltaX;
        Motion()->m_position.m_x = static_cast<double>(m_object->m_screenX);
        m_object->m_screenY += m_object->m_carrier->m_deltaY;
        Motion()->m_position.m_y = static_cast<double>(m_object->m_screenY);
    }

    if (m_object->m_moveMode == MOVE_GROUNDED) {
        m_collisionFlags = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_position.m_x),
            m_object->m_screenY,
            IDX(m_moveFlags)
        );
        Motion()->m_velocity.m_y = 0.0;
    } else {
        m_object->m_flags &= ~IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER);
        m_collisionFlags = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_position.m_x),
            static_cast<i32>(Motion()->m_position.m_y),
            IDX(m_moveFlags)
        );
    }

    CMotionState* ms = Motion();
    i32 sx = m_object->m_screenX;
    if (static_cast<i32>(Motion()->m_position.m_x) != sx) {
        double d = static_cast<double>(sx);
        ms->m_velocity.m_x = ms->ArrivalVelX(d);
        double correctedStepX = ms->m_step.m_x - (ms->m_position.m_x - d);
        ms->m_position.m_x = d;
        ms->m_step.m_x = correctedStepX;
    }

    i32 sy = m_object->m_screenY;
    if (static_cast<i32>(Motion()->m_position.m_y) != sy) {
        double d = static_cast<double>(sy);
        ms->m_velocity.m_y = ms->ArrivalVelY(d);
        double correctedStepY = ms->m_step.m_y - (ms->m_position.m_y - d);
        ms->m_position.m_y = d;
        ms->m_step.m_y = correctedStepY;
    }

    if (m_object->m_moveMode != MOVE_DIRECT) {
        i32 f = IDX(m_collisionFlags);
        if (f & IDX(MOVE_RESULT_TILE_TOP)) {
            Motion()->m_velocity.m_y = -Motion()->m_velocity.m_y;
            return;
        }
        if (f & IDX(MOVE_RESULT_TILE_RIGHT)) {
            Motion()->m_maxBounds.m_x = static_cast<double>(m_previousScreenPosition.m_x);
            Motion()->m_velocity.m_x = Motion()->m_velocity.m_x * g_motionNegHalf;
            return;
        }
        if (f & IDX(MOVE_RESULT_TILE_LEFT)) {
            Motion()->m_minBounds.m_x = static_cast<double>(m_previousScreenPosition.m_x);
            Motion()->m_velocity.m_x = Motion()->m_velocity.m_x * g_motionNegHalf;
        }
    }
}

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
        scr = t;                                                                                   \
        do {                                                                                       \
            double c;                                                                              \
            if (t > vmax) {                                                                        \
                c = vmax;                                                                          \
            } else if (t < -vmax) {                                                                \
                c = -vmax;                                                                         \
            } else {                                                                               \
                break;                                                                             \
            }                                                                                      \
            scr = c;                                                                               \
            ARRIVAL_V(v, a, s, c + s);                                                             \
        } while (0);                                                                               \
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
    m_previousPosition.m_x = m_position.m_x;
    m_previousPosition.m_y = m_position.m_y;
    m_previousPosition.m_z = m_position.m_z;
    m_deltaTime = dt;
    m_time = dt + m_time;
    if (m_stepDisabled != false) {
        return;
    }
    STEP_AXIS(
        m_velocity.m_x,
        m_acceleration.m_x,
        m_position.m_x,
        m_maxStep.m_x,
        m_minBounds.m_x,
        m_maxBounds.m_x,
        m_maxVelocity.m_x,
        m_step.m_x
    );
    STEP_AXIS(
        m_velocity.m_y,
        m_acceleration.m_y,
        m_position.m_y,
        m_maxStep.m_y,
        m_minBounds.m_y,
        m_maxBounds.m_y,
        m_maxVelocity.m_y,
        m_step.m_y
    );
    STEP_AXIS(
        m_velocity.m_z,
        m_acceleration.m_z,
        m_position.m_z,
        m_maxStep.m_z,
        m_minBounds.m_z,
        m_maxBounds.m_z,
        m_maxVelocity.m_z,
        m_step.m_z
    );
}

RVA(0x0016f3c0, 0x61)
double CMotionState::ArrivalVelX(double target) {
    if (m_acceleration.m_x == 0.0) {
        return m_velocity.m_x;
    }
    double disc = m_velocity.m_x * m_velocity.m_x
                  - (target - m_position.m_x) * m_acceleration.m_x * g_motionNegTwo;
    if (0.0 > disc) {
        disc = 0.0;
    }
    double r = sqrt(disc);
    return (m_velocity.m_x > 0.0) ? r : -r;
}

RVA(0x0016f430, 0x61)
double CMotionState::ArrivalVelY(double target) {
    if (m_acceleration.m_y == 0.0) {
        return m_velocity.m_y;
    }
    double disc = m_velocity.m_y * m_velocity.m_y
                  - (target - m_position.m_y) * m_acceleration.m_y * g_motionNegTwo;
    if (0.0 > disc) {
        disc = 0.0;
    }
    double r = sqrt(disc);
    return (m_velocity.m_y > 0.0) ? r : -r;
}

RVA(0x0016f4a0, 0x1da)
i32 CMovingLogic::SerializeDispatch(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* object
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
    return CUserLogic::SerializeDispatch(arc, mode, typeId, object) != 0;
}
