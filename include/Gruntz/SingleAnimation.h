#ifndef GRUNTZ_CSINGLEANIMATION_H
#define GRUNTZ_CSINGLEANIMATION_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CSingleAnimation : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_SINGLEANIMATION;
    }

public:
    CSingleAnimation() {}
    CSingleAnimation(CGameObject* obj);
    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();

    i32 AdvanceAnim();
};

#endif
