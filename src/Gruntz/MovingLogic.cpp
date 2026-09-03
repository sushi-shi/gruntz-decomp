#include <rva.h>

#include <Gruntz/MovingLogic.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <Lith/BDefs.h>
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

    m_previousScreenPosition = Motion()->m_position.ToCoord();
    Motion()->Step(static_cast<double>(g_frameTime) * g_motionTimeScale - Motion()->m_time);

    if ((m_object->m_flags & IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER)) && m_object->m_carrier != NULL) {
        Coord position = m_object->ScreenPos();
        position += m_object->m_carrier->m_delta;
        m_object->SetScreenPos(position);
        Motion()->m_position.SetXY(position);
    }

    if (m_object->m_moveMode == MOVE_GROUNDED) {
        m_collisionFlags = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_position.x),
            m_object->m_screenPosition.m_y,
            IDX(m_moveFlags)
        );
        Motion()->m_velocity.y = 0.0;
    } else {
        m_object->m_flags &= ~IDX(WWD_GAME_OBJECT_FLAG_ON_CARRIER);
        m_collisionFlags = m_object->OwnerMgr()->m_level->MoveToward(
            m_object,
            static_cast<i32>(Motion()->m_position.x),
            static_cast<i32>(Motion()->m_position.y),
            IDX(m_moveFlags)
        );
    }

    CMotionState* ms = Motion();
    Coord screenPosition = m_object->ScreenPos();
    if (static_cast<i32>(Motion()->m_position.x) != screenPosition.m_x) {
        double d = static_cast<double>(screenPosition.m_x);
        ms->m_velocity.x = ms->ArrivalVelX(d);
        double a0new = ms->m_step.x - (ms->m_position.x - d);
        ms->m_position.x = d;
        ms->m_step.x = a0new;
    }

    if (static_cast<i32>(Motion()->m_position.y) != screenPosition.m_y) {
        double d = static_cast<double>(screenPosition.m_y);
        ms->m_velocity.y = ms->ArrivalVelY(d);
        double a8new = ms->m_step.y - (ms->m_position.y - d);
        ms->m_position.y = d;
        ms->m_step.y = a8new;
    }

    if (m_object->m_moveMode != MOVE_DIRECT) {
        i32 f = IDX(m_collisionFlags);
        if (f & IDX(MOVE_RESULT_TILE_TOP)) {
            Motion()->m_velocity.y = -Motion()->m_velocity.y;
            return;
        }
        if (f & IDX(MOVE_RESULT_TILE_RIGHT)) {
            Motion()->m_maxBounds.x = static_cast<double>(m_previousScreenPosition.m_x);
            Motion()->m_velocity.x = Motion()->m_velocity.x * g_motionNegHalf;
            return;
        }
        if (f & IDX(MOVE_RESULT_TILE_LEFT)) {
            Motion()->m_minBounds.x = static_cast<double>(m_previousScreenPosition.m_x);
            Motion()->m_velocity.x = Motion()->m_velocity.x * g_motionNegHalf;
        }
    }
}

static inline double
ArrivalVelocity(double velocity, double acceleration, double position, double target) {
    if (acceleration == g_motionZero) {
        return velocity;
    }
    double discriminant = SQR(velocity) - (target - position) * acceleration * g_motionNegTwo;
    if (g_motionZero > discriminant) {
        discriminant = g_motionZero;
    }
    double root = sqrt(discriminant);
    return (velocity > g_motionZero) ? root : -root;
}

static inline void StepMotionAxis(
    double& velocity,
    double acceleration,
    double& position,
    double maxStep,
    double lowerBound,
    double upperBound,
    double positiveVelocityClamp,
    double& step,
    double dt
) {
    double accelerationStep = dt * acceleration;
    double proposedStep = (velocity - accelerationStep * g_motionNegHalf) * dt;
    step = proposedStep;
    do {
        double clampedStep;
        if (proposedStep > maxStep) {
            clampedStep = maxStep;
        } else if (proposedStep < -maxStep) {
            clampedStep = -maxStep;
        } else {
            break;
        }
        step = clampedStep;
        velocity = ArrivalVelocity(velocity, acceleration, position, clampedStep + position);
    } while (0);
    double oldPosition = position;
    double newPosition = step + position;
    position = newPosition;
    if (newPosition > upperBound) {
        velocity = ArrivalVelocity(velocity, acceleration, position, upperBound);
        step = upperBound - oldPosition;
        position = upperBound;
    } else if (newPosition < lowerBound) {
        velocity = ArrivalVelocity(velocity, acceleration, position, lowerBound);
        step = lowerBound - oldPosition;
        position = lowerBound;
    } else {
        velocity += accelerationStep;
    }
    if (velocity > positiveVelocityClamp) {
        velocity = positiveVelocityClamp;
    }
}

// @early-stop
RVA(0x0016ecd0, 0x6e6)
void CMotionState::Step(double dt) {
    m_previousPosition = m_position;
    m_deltaTime = dt;
    m_time = dt + m_time;
    if (m_stepDisabled != false) {
        return;
    }
    StepMotionAxis(
        m_velocity.x,
        m_acceleration.x,
        m_position.x,
        m_maxStep.x,
        m_minBounds.x,
        m_maxBounds.x,
        m_maxVelocity.x,
        m_step.x,
        dt
    );
    StepMotionAxis(
        m_velocity.y,
        m_acceleration.y,
        m_position.y,
        m_maxStep.y,
        m_minBounds.y,
        m_maxBounds.y,
        m_maxVelocity.y,
        m_step.y,
        dt
    );
    StepMotionAxis(
        m_velocity.z,
        m_acceleration.z,
        m_position.z,
        m_maxStep.z,
        m_minBounds.z,
        m_maxBounds.z,
        m_maxVelocity.z,
        m_step.z,
        dt
    );
}

RVA(0x0016f3c0, 0x61)
double CMotionState::ArrivalVelX(double target) {
    return ArrivalVelocity(m_velocity.x, m_acceleration.x, m_position.x, target);
}

RVA(0x0016f430, 0x61)
double CMotionState::ArrivalVelY(double target) {
    return ArrivalVelocity(m_velocity.y, m_acceleration.y, m_position.y, target);
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
