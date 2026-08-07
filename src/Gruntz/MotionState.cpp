#include <rva.h>

#include <Gruntz/MotionState.h>

#include <Gruntz/LogicTypeTableInline.h>
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
