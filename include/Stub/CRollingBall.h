#ifndef GRUNTZ_STUB_CROLLINGBALL_H
#define GRUNTZ_STUB_CROLLINGBALL_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CRollingBall : CUserLogic (RTTI). sizeof 0xa0.
class CRollingBall : public CUserLogic {
public:
    CRollingBall(int);
    char m_size_pad[0x60]; // own region over CUserLogic (0x40)
};
SIZE(CRollingBall, 0xa0);
#endif
