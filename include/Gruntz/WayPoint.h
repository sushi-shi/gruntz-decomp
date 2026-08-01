#ifndef GRUNTZ_CWAYPOINT_H
#define GRUNTZ_CWAYPOINT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/UserLogic.h>

class CWayPoint : public CUserLogic, public CWapX {
public:
public:
    RVA(0x00010220, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WAYPOINT;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    CWayPoint() {}
    CWayPoint(CGameObject* obj);
};
SIZE(0x54);

#endif // GRUNTZ_CWAYPOINT_H
