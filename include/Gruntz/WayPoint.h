#ifndef GRUNTZ_CWAYPOINT_H
#define GRUNTZ_CWAYPOINT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWayPoint : public CUserLogic, public CWapX {
public:
public:
    RVA(0x00010230, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WAYPOINT;
    }
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    CWayPoint() {}
    CWayPoint(CGameObject* obj);
};

#endif // GRUNTZ_CWAYPOINT_H
