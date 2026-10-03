#ifndef GRUNTZ_CEYECANDY_H
#define GRUNTZ_CEYECANDY_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CEyeCandy : public CUserLogic, public CWapX {
public:
public:
    CEyeCandy() : CUserLogic(CUserLogic::INLINE_BASE) {}
    CEyeCandy(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_EYECANDY;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;
};

#endif
