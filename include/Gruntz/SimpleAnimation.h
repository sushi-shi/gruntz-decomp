#ifndef GRUNTZ_CSIMPLEANIMATION_H
#define GRUNTZ_CSIMPLEANIMATION_H

#include <rva.h>
#include <Gruntz/UserLogic.h>
#include <Gruntz/LogicFnTable.h>

class CSimpleAnimation : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x0000f910, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SIMPLEANIMATION;
    }

public:
    CSimpleAnimation() {}
    CSimpleAnimation(CGameObject* obj);
    i32 AdvanceAnim();

    virtual void FireActivation(i32 id) OVERRIDE;
};
SIZE(0x54);

#endif // GRUNTZ_CSIMPLEANIMATION_H
