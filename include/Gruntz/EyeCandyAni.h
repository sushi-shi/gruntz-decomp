#ifndef GRUNTZ_CEYECANDYANI_H
#define GRUNTZ_CEYECANDYANI_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CEyeCandyAni : public CUserLogic, public CWapX {
public:
public:
    CEyeCandyAni() {}
    CEyeCandyAni(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_EYECANDYANI;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual void FireActivation(i32 id)  ;

    static void RegisterActs();

    i32 AdvanceAnim();
};

#endif
