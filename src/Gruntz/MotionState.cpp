#include <rva.h>

#include <Gruntz/MotionState.h>

#include <Gruntz/MovingLogic.h>
#include <Gruntz/Projectile.h>

#include <math.h>

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

// Out of line: retail's CGrunt / CProjectile ctors CALL this (through the ILT
// thunk at 0x3828), they do not expand it - at 405 bytes it is far past cl's
// inline budget, so leaving it inline in UserLogic.h let OUR cl expand a much
// smaller body into every derived ctor.
RVA(0x00058cd0, 0x195)
CUserLogic::CUserLogic(CGameObject* obj) {
    m_logicObject = obj;
    m_object = static_cast<CWwdGameObjectA*>(obj);
    m_objAux = obj->m_animWorker;
    {
        zBitVec tmp(g_emptyString, 0);
        m_link.m_str = tmp;
    }
    RegisterLogicTypesOnce();
    m_object->AddLogicHit("LogicHit");
    m_object->AddLogicAttack("LogicAttack");
    m_object->AddLogicBump("LogicBump");
    m_deferredCallback = 0;
    m_gatedCallback = 0;
    m_gatedActKey = 0x3e9;
    m_reserved2c = 2;
}
