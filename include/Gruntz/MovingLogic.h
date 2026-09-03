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
#include <Wwd/MoveFlags.h>
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
    GZ_ENUM_STORAGE(MoveResultFlags, i32) m_collisionFlags;
    GZ_ENUM_STORAGE(MoveRequestFlags, i32) m_moveFlags;

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
    m->m_maxStep.Init(z, z, z);
    BeginMotion();
}

inline CMovingLogic::CMovingLogic(CGameObject* owner, EGruntScale) : CUserLogic(owner) {
    InitOwner(0.001);
    m_motion.SetZ(static_cast<double>(g_defaultZ));
    BeginMotion();
}

inline void CMovingLogic::InitOwner(const double& timeScale) {
    Coord lowerBounds(m_logicRecord->m_minX, m_logicRecord->m_minY);
    Motion()->m_minBounds.Init(
        lowerBounds.m_x == 0 ? g_movingLogicMin : static_cast<double>(lowerBounds.m_x),
        lowerBounds.m_y == 0 ? g_movingLogicMin : static_cast<double>(lowerBounds.m_y),
        Motion()->m_minBounds.z
    );
    Coord upperBounds(m_logicRecord->m_maxX, m_logicRecord->m_maxY);
    Motion()->m_maxBounds.Init(
        upperBounds.m_x == 0 ? g_movingLogicMax : static_cast<double>(upperBounds.m_x),
        upperBounds.m_y == 0 ? g_movingLogicMax : static_cast<double>(upperBounds.m_y),
        Motion()->m_maxBounds.z
    );
    m_motion.SetParams(
        static_cast<double>(m_object->m_screenPosition.m_x),
        static_cast<double>(m_object->m_screenPosition.m_y),
        0.0,
        static_cast<double>(m_object->m_speed.m_x),
        static_cast<double>(m_object->m_speed.m_y),
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
