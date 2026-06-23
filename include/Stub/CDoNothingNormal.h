#ifndef GRUNTZ_STUB_CDONOTHINGNORMAL_H
#define GRUNTZ_STUB_CDONOTHINGNORMAL_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CDoNothingNormal : CUserLogic (RTTI). sizeof 0x54.
class CDoNothingNormal : public CUserLogic {
public:
    CDoNothingNormal();
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CDoNothingNormal, 0x54);
#endif
