#ifndef GRUNTZ_STUB_CEXITTRIGGER_H
#define GRUNTZ_STUB_CEXITTRIGGER_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CExitTrigger : CUserLogic (RTTI). sizeof 0x5c.
class CExitTrigger : public CUserLogic {
public:
    CExitTrigger(i32);
    char m_size_pad[0x1c]; // own region over CUserLogic (0x40)
};
SIZE(CExitTrigger, 0x5c);
#endif
