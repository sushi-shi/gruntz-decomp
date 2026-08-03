#include <rva.h>

#include <Gruntz/MovingLogic.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>
#include <strstrea.h>

DATA(0x001f04f8)
const double g_motionNegHalf = -0.5;

DATA(0x001f04f0)
const double g_motionTimeScale = 0.001;

RVA(0x0016cdd0, 0x22f)
ostream& WriteCurve(ostream& accum, const CMotionState& c) {
    accum << c.m_time;
    accum << c.m_deltaTime;
    accum << c.m_acceleration.x;
    accum << c.m_acceleration.y;
    accum << c.m_acceleration.z;
    accum << c.m_velocity.x;
    accum << c.m_velocity.y;
    accum << c.m_velocity.z;
    accum << c.m_position.x;
    accum << c.m_position.y;
    accum << c.m_position.z;
    accum << c.m_minBounds.x;
    accum << c.m_minBounds.y;
    accum << c.m_minBounds.z;
    accum << c.m_maxBounds.x;
    accum << c.m_maxBounds.y;
    accum << c.m_maxBounds.z;
    accum << c.m_step.x;
    accum << c.m_step.y;
    accum << c.m_step.z;
    accum << c.m_stepDisabled;
    accum << c.m_reservedc0.x;
    accum << c.m_reservedc0.y;
    accum << c.m_reservedc0.z;
    accum << c.m_maxStep.x;
    accum << c.m_maxStep.y;
    accum << c.m_maxStep.z;
    accum << c.m_maxVelocity.x;
    accum << c.m_maxVelocity.y;
    accum << c.m_maxVelocity.z;
    return accum;
}

RVA(0x0016d000, 0x189)
istream& ReadCurve(istream& accum, CMotionState& c) {
    accum >> c.m_time;
    accum >> c.m_deltaTime;
    accum >> c.m_acceleration.x;
    accum >> c.m_acceleration.y;
    accum >> c.m_acceleration.z;
    accum >> c.m_velocity.x;
    accum >> c.m_velocity.y;
    accum >> c.m_velocity.z;
    accum >> c.m_position.x;
    accum >> c.m_position.y;
    accum >> c.m_position.z;
    accum >> c.m_minBounds.x;
    accum >> c.m_minBounds.y;
    accum >> c.m_minBounds.z;
    accum >> c.m_maxBounds.x;
    accum >> c.m_maxBounds.y;
    accum >> c.m_maxBounds.z;
    accum >> c.m_step.x;
    accum >> c.m_step.y;
    accum >> c.m_step.z;
    accum >> c.m_stepDisabled;
    accum >> c.m_reservedc0.x;
    accum >> c.m_reservedc0.y;
    accum >> c.m_reservedc0.z;
    accum >> c.m_maxStep.x;
    accum >> c.m_maxStep.y;
    accum >> c.m_maxStep.z;
    accum >> c.m_maxVelocity.x;
    accum >> c.m_maxVelocity.y;
    accum >> c.m_maxVelocity.z;
    return accum;
}

// @early-stop
RVA(0x0016e7f0, 0x1cf)
i32 CUserLogic::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (arc == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            i32 len;
            arc->Read(&len, 4);
            void* buf = ::operator new(len);
            arc->Read(buf, len);
            istrstream accum(static_cast<char*>(buf), len);
            accum >> m_link.m_str;
            ::operator delete(buf);
            arc->Read(&m_gatedActKey, 4);
            arc->Read(&m_reserved2c, 4);
            arc->Read(&g_logicTypesRegistered, 4);
            arc->Read(&m_prevAnimSetNode, 4);
            m_logicObject = pObj;
            m_object = static_cast<CWwdGameObjectA*>(pObj);
            m_objAux = (pObj)->m_animWorker;
            m_deferredCallback = 0;
            m_gatedCallback = 0;
            m_gatedActKey = 0x3e9;

            break;
        }
        case SERIAL_SAVE: {

            char buf[0x100];
            ostrstream accum(buf, 0x100);
            accum << m_link.m_str;
            i32 len = accum.pcount();
            arc->Write(&len, 4);
            arc->Write(accum.str(), len);
            arc->Write(&m_gatedActKey, 4);
            arc->Write(&m_reserved2c, 4);
            arc->Write(&g_logicTypesRegistered, 4);
            arc->Write(&m_prevAnimSetNode, 4);

            break;
        }
    }
    return 1;
}

// @early-stop
RVA(0x0016ea90, 0x234)
void CMovingLogic::AdvanceMotion() {

    m_previousScreenPosition.m_x = static_cast<i32>(Motion()->m_position.x);
    m_previousScreenPosition.m_y = static_cast<i32>(Motion()->m_position.y);
    Motion()->Step(static_cast<double>(g_frameTime) * g_motionTimeScale - Motion()->m_time);

    if ((m_object->m_flags & 0x10) && m_object->m_carrier != 0) {
        m_object->m_screenX += m_object->m_carrier->m_deltaX;
        Motion()->m_position.x = static_cast<double>(m_object->m_screenX);
        m_object->m_screenY += m_object->m_carrier->m_deltaY;
        Motion()->m_position.y = static_cast<double>(m_object->m_screenY);
    }

    if (m_object->m_moveMode == 1) {
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

    if (m_object->m_moveMode != 7) {
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

// @early-stop
RVA(0x0016f4a0, 0x1da)
i32 CMovingLogic::SerializeMove(
    CFileMemBase* arc,
    SerialMode mode,
    LogicTypeId typeId,
    CGameObject* pObj
) {
    if (arc == 0) {
        return 0;
    }
    switch (mode) {
        case SERIAL_LOAD: {

            i32 len;
            arc->Read(&len, 4);
            void* buf = ::operator new(len);
            arc->Read(buf, len);
            istrstream accum(static_cast<char*>(buf), len);
            ReadCurve(accum, *Motion());
            ::operator delete(buf);
            arc->Read(&m_previousScreenPosition.m_x, 4);
            arc->Read(&m_previousScreenPosition.m_y, 4);
            arc->Read(&m_collisionFlags, 4);
            arc->Read(&m_moveFlags, 4);

            break;
        }
        case SERIAL_SAVE: {

            char buf[0x100];
            ostrstream accum(buf, 0x100);
            WriteCurve(accum, *Motion());
            i32 len = accum.pcount();
            arc->Write(&len, 4);
            arc->Write(accum.str(), len);
            arc->Write(&m_previousScreenPosition.m_x, 4);
            arc->Write(&m_previousScreenPosition.m_y, 4);
            arc->Write(&m_collisionFlags, 4);
            arc->Write(&m_moveFlags, 4);

            break;
        }
    }
    return CUserLogic::SerializeMove(arc, mode, typeId, pObj) != 0;
}
