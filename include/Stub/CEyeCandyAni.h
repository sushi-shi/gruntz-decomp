#ifndef GRUNTZ_STUB_CEYECANDYANI_H
#define GRUNTZ_STUB_CEYECANDYANI_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CEyeCandyAni : CUserLogic (RTTI). sizeof 0x54.
class CEyeCandyAni : public CUserLogic {
public:
    CEyeCandyAni(i32);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CEyeCandyAni, 0x54);
#endif
