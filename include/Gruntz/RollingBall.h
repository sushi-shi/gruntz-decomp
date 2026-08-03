#ifndef GRUNTZ_GRUNTZ_ROLLINGBALL_H
#define GRUNTZ_GRUNTZ_ROLLINGBALL_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/CoordNode.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CFileMemBase;

class CRollingBall : public CUserLogic, public CWapX {
public:
    RVA(0x000b0fe0, 0x1ab)
    virtual i32 SerializeMove(CFileMemBase* ar, SerialMode tag, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(ar, tag, c, d)) {
            return 0;
        }
        if (!Chain(ar, tag, c, d)) {
            return 0;
        }

        switch (tag) {
            case SERIAL_SAVE:
                ar->Write(&m_explodeStart, sizeof(m_explodeStart));
                ar->Write(&m_explodeWindow, sizeof(m_explodeWindow));
                break;
            case SERIAL_LOAD:
                ar->Read(&m_explodeStart, sizeof(m_explodeStart));
                ar->Read(&m_explodeWindow, sizeof(m_explodeWindow));
                break;
        }

        switch (tag) {
            case SERIAL_SAVE:
                ar->Write(&m_moveSpeed, 8);
                ar->Write(&m_subX, 8);
                ar->Write(&m_subY, 8);
                ar->Write(&m_stepDirX, 4);
                ar->Write(&m_stepDirY, 4);
                ar->Write(&m_target, 8);
                ar->Write(&m_explodeLatch, 4);
                ar->Write(&m_fallLatch, 4);
                ar->Write(&m_moveDelta, sizeof(m_moveDelta));
                break;
            case SERIAL_LOAD:
                ar->Read(&m_moveSpeed, 8);
                ar->Read(&m_subX, 8);
                ar->Read(&m_subY, 8);
                ar->Read(&m_stepDirX, 4);
                ar->Read(&m_stepDirY, 4);
                ar->Read(&m_target, 8);
                ar->Read(&m_explodeLatch, 4);
                ar->Read(&m_fallLatch, 4);
                ar->Read(&m_moveDelta, sizeof(m_moveDelta));
                break;
        }
        return 1;
    }
    RVA(0x00012f30, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_ROLLINGBALL;
    }

public:
    CRollingBall() {}
    CRollingBall(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    static void RegisterActs();

    i32 Update();

    char m_pad54[0x58 - 0x54];
    double m_moveSpeed;

    double m_subX;
    double m_subY;
    i32 m_stepDirX;
    i32 m_stepDirY;
    Coord m_target;
    i32 m_explodeLatch;
    i32 m_fallLatch;

    i64 m_explodeStart;
    i64 m_explodeWindow;
    double m_moveDelta;
};
SIZE(0xa0);

extern "C" i32 __ftol(double x);

#endif // GRUNTZ_GRUNTZ_ROLLINGBALL_H
