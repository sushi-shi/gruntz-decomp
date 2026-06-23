#ifndef GRUNTZ_STUB_CEXPLOSION_H
#define GRUNTZ_STUB_CEXPLOSION_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CExplosion : CUserLogic (RTTI). sizeof 0x54.
class CExplosion : public CUserLogic {
public:
    CExplosion(int);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CExplosion, 0x54);
#endif
