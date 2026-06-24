#ifndef GRUNTZ_STUB_CLEVELTIME_H
#define GRUNTZ_STUB_CLEVELTIME_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CLevelTime : CUserLogic (RTTI). sizeof 0x54.
class CLevelTime : public CUserLogic {
public:
    CLevelTime(i32);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CLevelTime, 0x54);
#endif
