#ifndef GRUNTZ_CLEVELTIME_H
#define GRUNTZ_CLEVELTIME_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CLevelTime : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_LEVELTIME;
    }

public:
    CLevelTime() {}

    CLevelTime(CGameObject* obj);
};

#endif
