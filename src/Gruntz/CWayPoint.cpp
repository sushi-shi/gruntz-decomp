// CWayPoint.cpp - the waypoint marker (C:\Proj\Gruntz), a CUserLogic leaf. Only
// the /GX leaf dtor is reconstructed here.
#include <Gruntz/CWayPoint.h>

// CWayPoint::~CWayPoint (0x102e0) - the /GX leaf dtor folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// leaf vptr store is dead-eliminated.
RVA(0x000102e0, 0x44)
CWayPoint::~CWayPoint() {}
