#ifndef GRUNTZ_STUB_CSTATUSBARSPRITE_H
#define GRUNTZ_STUB_CSTATUSBARSPRITE_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CStatusBarSprite : CUserLogic (RTTI). sizeof 0x54.
class CStatusBarSprite : public CUserLogic {
public:
    CStatusBarSprite(int);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CStatusBarSprite, 0x54);
#endif
