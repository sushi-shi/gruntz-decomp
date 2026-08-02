#ifndef GRUNTZ_CGRUNTSTARTINGPOINT_H
#define GRUNTZ_CGRUNTSTARTINGPOINT_H

#include <rva.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/UserLogic.h>

class CGruntStartingPoint : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x000105b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSTARTINGPOINT;
    }

public:
    CGruntStartingPoint() {}
    CGruntStartingPoint(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;

    i32 Idle();
};
SIZE(0x54);

SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTSTARTINGPOINT_H
