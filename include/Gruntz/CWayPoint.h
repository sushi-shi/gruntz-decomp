// CWayPoint.h - the waypoint marker (C:\Proj\Gruntz), a CUserLogic tile-logic leaf
// (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed here; the ctor
// (0xae3f0) remains the @stub backlog in src/Stub/CWayPoint.cpp. Offsets + code
// bytes are load-bearing.
#ifndef GRUNTZ_CWAYPOINT_H
#define GRUNTZ_CWAYPOINT_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CWayPoint : public CUserLogic {
public:
    CWayPoint(CGameObject* obj); // 0xae3f0
    ~CWayPoint();                // 0x102e0 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];
};
SIZE(CWayPoint, 0x54);

#endif // GRUNTZ_CWAYPOINT_H
