#ifndef GRUNTZ_MOTIONSTATEINLINE_H
#define GRUNTZ_MOTIONSTATEINLINE_H

#include <Gruntz/MotionState.h>

inline void CMotionState::CorrectX(double position) {
    m_velocity.m_x = ArrivalVelX(position);
    double correctedStep = m_step.m_x - (m_position.m_x - position);
    m_position.m_x = position;
    m_step.m_x = correctedStep;
}

inline void CMotionState::CorrectY(double position) {
    m_velocity.m_y = ArrivalVelY(position);
    double correctedStep = m_step.m_y - (m_position.m_y - position);
    m_position.m_y = position;
    m_step.m_y = correctedStep;
}

#endif // GRUNTZ_MOTIONSTATEINLINE_H
