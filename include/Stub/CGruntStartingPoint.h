#ifndef GRUNTZ_STUB_CGRUNTSTARTINGPOINT_H
#define GRUNTZ_STUB_CGRUNTSTARTINGPOINT_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CGruntStartingPoint : CUserLogic (RTTI). sizeof 0x54.
class CGruntStartingPoint : public CUserLogic {
public:
    CGruntStartingPoint(int);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CGruntStartingPoint, 0x54);
#endif
