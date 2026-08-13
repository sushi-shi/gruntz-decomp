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

extern const double g_motionZScale;
extern const u32 g_defaultZ;

class CMovingLogic : public CUserLogic {
public:
    enum EGruntScale {
        GRUNT_SCALE,
    };

    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_NONE;
    }

    RVA(0x00013c70, 0x47)
    virtual void FinalizeStep(char*) OVERRIDE {
        if (m_deferredCallback != 0) {
            if (m_gatedCallback != 0 && m_objAux->ActKey() == m_gatedActKey) {
                (this->*m_gatedCallback)();
                m_gatedCallback = 0;
            }
            (this->*m_deferredCallback)();
            m_deferredCallback = 0;
            m_gatedActKey = IDX(ACT_NONE);
        }
        AdvanceMotion();
    }

    char m_pad34[0x38 - 0x34];

public:
    // Two entities, same tag type.  The out-of-line 0x13940 EXPANDS both its
    // CUserLogic base (??_7CUserBase stamp + the single `call ??0zBitVec`) and
    // its CMotionState member; CProjectile `call`s it.  The inline sibling leaves
    // both a `call`, which is what CGrunt's construction shows in retail's
    // SerialObjectFactory: `call ??0CUserLogic`, `call ??0CMotionState`, then the
    // ??_7CMovingLogic stamp.
    CMovingLogic();
    CMovingLogic(CUserLogic::EInlineBase);
    // The same body as the out-of-line 0x13940, as an inline sibling: retail's
    // ??0CProjectile (0x126e0) expands the whole chain down to the single
    // `call ??0zBitVec`.
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
};

// The untagged default ctor is out of line in SerialObjectFactory.cpp (0x13940); it
// expands CMotionState's body, while the owner-taking one below leaves it a call -
// the 0x13940-vs-0x47a10 split retail shows.
inline CMovingLogic::CMovingLogic(CUserLogic::EInlineBase) {}

inline CMovingLogic::CMovingLogic(CMotionState::EInlineBase)
    : CUserLogic(CUserLogic::INLINE_BASE), m_motion(CMotionState::INLINE_BASE) {}

inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {
    InitOwner(g_motionZScale);
}

inline void CMovingLogic::InitOwner(const double& timeScale) {
    i32 lo0 = m_objAux->m_minX;
    if (lo0 == 0) {
        Motion()->m_minBounds.x = g_movingLogicMin;
    } else {
        Motion()->m_minBounds.x = static_cast<double>(lo0);
    }
    i32 lo1 = m_objAux->m_minY;
    if (lo1 == 0) {
        Motion()->m_minBounds.y = g_movingLogicMin;
    } else {
        Motion()->m_minBounds.y = static_cast<double>(lo1);
    }
    i32 hi0 = m_objAux->m_maxX;
    if (hi0 == 0) {
        Motion()->m_maxBounds.x = g_movingLogicMax;
    } else {
        Motion()->m_maxBounds.x = static_cast<double>(hi0);
    }
    i32 hi1 = m_objAux->m_maxY;
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
    m_motion.SetZ(static_cast<double>(g_defaultZ));
    m_collisionFlags = 0;
    m_moveFlags = 0;
    m_object->m_moveMode = MOVE_DIRECT;
    CMovingLogic::AdvanceMotion();
}

inline CMovingLogic::~CMovingLogic() {}

extern const double g_motionTimeScale;
#endif // GRUNTZ_CMOVINGLOGIC_H
