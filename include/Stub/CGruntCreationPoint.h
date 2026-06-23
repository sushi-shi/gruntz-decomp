#ifndef GRUNTZ_STUB_CGRUNTCREATIONPOINT_H
#define GRUNTZ_STUB_CGRUNTCREATIONPOINT_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CGruntCreationPoint : CUserLogic (RTTI). sizeof 0x54.
class CGruntCreationPoint : public CUserLogic {
public:
    CGruntCreationPoint(int);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CGruntCreationPoint, 0x54);
#endif
