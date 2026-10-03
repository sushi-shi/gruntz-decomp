#ifndef GRUNTZ_CWAYPOINT_H
#define GRUNTZ_CWAYPOINT_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWayPoint : public CUserLogic, public CWapX {
public:
public:

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_WAYPOINT;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;
    CWayPoint() {}
    CWayPoint(CGameObject* obj);
};

#endif
