#ifndef GRUNTZ_STUB_CLIGHTFX_H
#define GRUNTZ_STUB_CLIGHTFX_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CLightFx : CUserLogic (RTTI). sizeof 0x5c.
class CLightFx : public CUserLogic {
public:
    CLightFx(int);
    char m_size_pad[0x1c]; // own region over CUserLogic (0x40)
};
SIZE(CLightFx, 0x5c);
#endif
