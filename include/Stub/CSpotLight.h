#ifndef GRUNTZ_STUB_CSPOTLIGHT_H
#define GRUNTZ_STUB_CSPOTLIGHT_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CSpotLight : CUserLogic (RTTI). sizeof 0xa8.
class CSpotLight : public CUserLogic {
public:
    CSpotLight(int);
    char m_size_pad[0x68]; // own region over CUserLogic (0x40)
};
SIZE(CSpotLight, 0xa8);
#endif
