#ifndef GRUNTZ_CSIMPLEANIMATION_H
#define GRUNTZ_CSIMPLEANIMATION_H

#include <rva.h>

#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSimpleAnimation : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x0000f910, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_SIMPLEANIMATION;
    }

public:
    CSimpleAnimation() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CSimpleAnimation(CGameObject* obj);
    i32 AdvanceAnim();

    virtual void FireActivation(i32 id) OVERRIDE;
};

#endif // GRUNTZ_CSIMPLEANIMATION_H
