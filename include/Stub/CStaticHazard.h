#ifndef GRUNTZ_STUB_CSTATICHAZARD_H
#define GRUNTZ_STUB_CSTATICHAZARD_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CStaticHazard : CUserLogic (RTTI). sizeof 0x6c.
class CStaticHazard : public CUserLogic {
public:
    CStaticHazard(int);
    void LoadAttributes2();
    void LoadAttributes();
    char m_size_pad[0x2c]; // own region over CUserLogic (0x40)
};
SIZE(CStaticHazard, 0x6c);
#endif
