#ifndef GRUNTZ_CSIMPLEANIMATION_H
#define GRUNTZ_CSIMPLEANIMATION_H

#include <Ints.h>

#include <Gruntz/LogicFnTable.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSimpleAnimation : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_SIMPLEANIMATION;
    }

public:
    CSimpleAnimation() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CSimpleAnimation(CGameObject* obj);
    i32 AdvanceAnim();

    virtual void FireActivation(i32 id)  ;
};

#endif
