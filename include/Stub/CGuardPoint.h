#ifndef GRUNTZ_STUB_CGUARDPOINT_H
#define GRUNTZ_STUB_CGUARDPOINT_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CGuardPoint : CUserLogic (RTTI). sizeof 0x54.
class CGuardPoint : public CUserLogic {
public:
    CGuardPoint(i32);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CGuardPoint, 0x54);
#endif
