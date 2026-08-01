#ifndef GRUNTZ_GRUNTZ_ROLLINGBALL_H
#define GRUNTZ_GRUNTZ_ROLLINGBALL_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/UserLogic.h>

#include <Gruntz/SerialArchive.h>
#include <Gruntz/ActReg.h>

class CFileMemBase;

class CRollingBall : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
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

    union {
        double m_subX;
        struct {
            i32 m_subXLo;
            i32 m_subXHi;
        };
    };
    union {
        double m_subY;
        struct {
            i32 m_subYLo;
            i32 m_subYHi;
        };
    };
    i32 m_stepDirX;
    i32 m_stepDirY;
    i32 m_targetX;
    i32 m_targetY;
    i32 m_explodeLatch;
    i32 m_fallLatch;

    union {
        i64 m_explodeStart64;
        struct {
            i32 m_explodeStartLo;
            i32 m_explodeStartHi;
        };
    };
    union {
        i64 m_explodeWindow64;
        struct {
            i32 m_explodeWindowLo;
            i32 m_explodeWindowHi;
        };
    };

    union {
        double m_moveDelta;
        struct {
            i32 m_moveDeltaLo;
            i32 m_moveDeltaHi;
        };
    };
};
SIZE(0xa0);

extern "C" i32 __ftol(double x);

#endif // GRUNTZ_GRUNTZ_ROLLINGBALL_H
