#ifndef GRUNTZ_STUB_CDONOTHING_H
#define GRUNTZ_STUB_CDONOTHING_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CDoNothing : CUserLogic (RTTI). sizeof 0x54.
class CDoNothing : public CUserLogic {
public:
    CDoNothing(i32);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CDoNothing, 0x54);
#endif
