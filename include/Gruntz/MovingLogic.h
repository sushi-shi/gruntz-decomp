#ifndef GRUNTZ_CMOVINGLOGIC_H
#define GRUNTZ_CMOVINGLOGIC_H

#include <Mfc.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/MotionState.h>
#include <Gruntz/SerialArchive.h>
#include <rva.h>

extern const double g_movingLogicMin;
extern const double g_movingLogicMax;

extern "C" u32 g_frameTime;
extern const double g_motionZScale;
extern u32 g_defaultZ;

class CMovingLogic : public CUserLogic {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
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

inline CMovingLogic::CMovingLogic() {}

inline CMovingLogic::CMovingLogic(CGameObject* owner) : CUserLogic(owner) {}

inline CMovingLogic::~CMovingLogic() {}

extern const double g_motionTimeScale;
#endif // GRUNTZ_CMOVINGLOGIC_H
