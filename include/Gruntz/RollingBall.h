#ifndef GRUNTZ_GRUNTZ_ROLLINGBALL_H
#define GRUNTZ_GRUNTZ_ROLLINGBALL_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/DoubleVector.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CRollingBall : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00012f30, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ROLLINGBALL;
    }

public:
    CRollingBall() : CUserLogic(CUserLogic::INLINE_BASE), m_explodeStart(0), m_explodeWindow(0) {}
    CRollingBall(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 Update();

    char m_pad54[0x58 - 0x54];
    double m_moveSpeed;

    DoubleVector2 m_subPosition;
    Coord m_stepDirection;
    Coord m_target;
    b32 m_explodeLatch;
    i32 m_fallLatch;

    i64 m_explodeStart;
    i64 m_explodeWindow;
    double m_moveDelta;
};

#endif // GRUNTZ_GRUNTZ_ROLLINGBALL_H
