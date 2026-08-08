#ifndef GRUNTZ_CMOVINGLOGIC_H
#define GRUNTZ_CMOVINGLOGIC_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Wwd/MoveMode.h>

extern const double g_movingLogicMin;
extern const double g_movingLogicMax;

extern "C" u32 g_frameTime;
extern const double g_motionZScale;
extern u32 g_defaultZ;

class CMovingLogic : public CUserLogic {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_NONE;
    }

    virtual void FinalizeStep(char* unused) OVERRIDE;

    char m_pad34[0x38 - 0x34];

public:
    CMovingLogic();

    CMovingLogic(CGameObject* owner);
    virtual ~CMovingLogic() OVERRIDE;

    virtual void AdvanceMotion();

    CMotionState* Motion() {
        return &m_motion;
    }

    CMotionState m_motion;
    Coord m_previousScreenPosition;
    i32 m_collisionFlags;
    i32 m_moveFlags;
};
SIZE_UNKNOWN();

// The default ctor expands CMotionState's body; the owner-taking one below leaves it
// a call, which is the 0x13940-vs-0x47a10 split retail shows.
inline CMovingLogic::CMovingLogic() : m_motion(CMotionState::INLINE_BASE) {}

inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {
    i32 lo0 = m_objAux->m_minX;
    if (lo0 == 0) {
        m_motion.m_minBounds.x = g_movingLogicMin;
    } else {
        m_motion.m_minBounds.x = static_cast<double>(lo0);
    }
    i32 lo1 = m_objAux->m_minY;
    if (lo1 == 0) {
        m_motion.m_minBounds.y = g_movingLogicMin;
    } else {
        m_motion.m_minBounds.y = static_cast<double>(lo1);
    }
    i32 hi0 = m_objAux->m_maxX;
    if (hi0 == 0) {
        m_motion.m_maxBounds.x = g_movingLogicMax;
    } else {
        m_motion.m_maxBounds.x = static_cast<double>(hi0);
    }
    i32 hi1 = m_objAux->m_maxY;
    if (hi1 == 0) {
        m_motion.m_maxBounds.y = g_movingLogicMax;
    } else {
        m_motion.m_maxBounds.y = static_cast<double>(hi1);
    }
    m_motion.SetParams(
        static_cast<double>(m_object->m_screenX),
        static_cast<double>(m_object->m_screenY),
        0.0,
        static_cast<double>(m_object->m_speedX),
        static_cast<double>(m_object->m_speedY),
        0.0,
        0.0,
        0.0,
        0.0,
        static_cast<double>(g_frameTime) * g_motionZScale,
        0.0
    );
    m_motion.SetZ(static_cast<double>(g_defaultZ));
    m_collisionFlags = 0;
    m_moveFlags = 0;
    m_object->m_moveMode = MOVE_DIRECT;
    CMovingLogic::AdvanceMotion();
}

inline CMovingLogic::~CMovingLogic() {}

extern const double g_motionTimeScale;
#endif // GRUNTZ_CMOVINGLOGIC_H
