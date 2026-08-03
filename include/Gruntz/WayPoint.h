#ifndef GRUNTZ_CWAYPOINT_H
#define GRUNTZ_CWAYPOINT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWayPoint : public CUserLogic, public CWapX {
public:
public:
    RVA(0x00010220, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_WAYPOINT;
    }
    RVA(0x00010240, 0x47)
    virtual i32 SerializeMove(CFileMemBase* a, SerialMode b, LogicTypeId c, CGameObject* d)
        OVERRIDE {
        if (!CUserLogic::SerializeMove(a, b, c, d)) {
            return 0;
        }
        return Chain(a, b, c, d) != 0;
    }
    CWayPoint() {}
    CWayPoint(CGameObject* obj);
};
SIZE(0x54);

#endif // GRUNTZ_CWAYPOINT_H
