#ifndef GRUNTZ_CMOVINGLOGIC_H
#define GRUNTZ_CMOVINGLOGIC_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Rez/FrameClock.h>
#include <Wwd/MoveMode.h>

#include <stddef.h>

extern const u32 g_defaultZ;

class CMovingLogic : public CUserLogic {
public:
    enum EGruntScale {
        GRUNT_SCALE,
    };

    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00013bb0, 0x4)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_NONE;
    }

    RVA(0x00013c70, 0x47)
    virtual void FinalizeStep(char*) OVERRIDE {
        if (m_deferredCallback != NULL) {
            if (m_gatedCallback != NULL && m_logicRecord->EventCode() == m_gatedCallbackCode) {
                (this->*m_gatedCallback)();
                m_gatedCallback = NULL;
            }
            (this->*m_deferredCallback)();
            m_deferredCallback = NULL;
            m_gatedCallbackCode = IDX(ACT_NONE);
        }
        AdvanceMotion();
    }

    char m_pad34[0x38 - 0x34];

public:
    CMovingLogic();
    CMovingLogic(CUserLogic::EInlineBase);
    CMovingLogic(CMotionState::EInlineBase);

    CMovingLogic(CGameObject* owner);
    CMovingLogic(CGameObject* owner, EGruntScale);
    virtual ~CMovingLogic() OVERRIDE;

    virtual void AdvanceMotion();

    CMotionState* Motion() {
        return &m_motion;
    }

    CMotionState m_motion;
    Coord m_previousScreenPosition;
    i32 m_collisionFlags;
    i32 m_moveFlags;

private:
    void InitOwner(const double& timeScale);
    void BeginMotion();
};

inline CMovingLogic::CMovingLogic(CUserLogic::EInlineBase) {}

inline CMovingLogic::CMovingLogic(CMotionState::EInlineBase)
    : CUserLogic(CUserLogic::INLINE_BASE), m_motion(CMotionState::INLINE_BASE) {}

inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {
    InitOwner(0.001);
    CMotionState* m = Motion();
    double z = static_cast<double>(g_defaultZ);
    m->m_maxStep.x = z;
    m->m_maxStep.y = z;
    m->m_maxStep.z = z;
    BeginMotion();
}

inline CMovingLogic::CMovingLogic(CGameObject* owner, EGruntScale) : CUserLogic(owner) {
    InitOwner(0.001);
    m_motion.SetZ(static_cast<double>(g_defaultZ));
    BeginMotion();
}

inline void CMovingLogic::InitOwner(const double& timeScale) {
    i32 lo0 = m_logicRecord->m_minX;
    if (lo0 == 0) {
        Motion()->m_minBounds.x = g_movingLogicMin;
    } else {
        Motion()->m_minBounds.x = static_cast<double>(lo0);
    }
    i32 lo1 = m_logicRecord->m_minY;
    if (lo1 == 0) {
        Motion()->m_minBounds.y = g_movingLogicMin;
    } else {
        Motion()->m_minBounds.y = static_cast<double>(lo1);
    }
    i32 hi0 = m_logicRecord->m_maxX;
    if (hi0 == 0) {
        Motion()->m_maxBounds.x = g_movingLogicMax;
    } else {
        Motion()->m_maxBounds.x = static_cast<double>(hi0);
    }
    i32 hi1 = m_logicRecord->m_maxY;
    if (hi1 == 0) {
        Motion()->m_maxBounds.y = g_movingLogicMax;
    } else {
        Motion()->m_maxBounds.y = static_cast<double>(hi1);
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
        static_cast<double>(g_frameTime) * timeScale,
        0.0
    );
}

inline void CMovingLogic::BeginMotion() {
    m_collisionFlags = 0;
    m_moveFlags = 0;
    m_object->m_moveMode = MOVE_DIRECT;
    CMovingLogic::AdvanceMotion();
}

inline CMovingLogic::~CMovingLogic() {}

extern const double g_motionTimeScale;
#endif // GRUNTZ_CMOVINGLOGIC_H
