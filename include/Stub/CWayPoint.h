#ifndef GRUNTZ_STUB_CWAYPOINT_H
#define GRUNTZ_STUB_CWAYPOINT_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CWayPoint : CUserLogic (RTTI). sizeof 0x54.
class CWayPoint : public CUserLogic {
public:
    CWayPoint(int);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CWayPoint, 0x54);
#endif
