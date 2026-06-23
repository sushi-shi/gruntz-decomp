#ifndef GRUNTZ_STUB_CINGAMETEXT_H
#define GRUNTZ_STUB_CINGAMETEXT_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CInGameText : CUserLogic (RTTI). sizeof 0x5c.
class CInGameText : public CUserLogic {
public:
    CInGameText(int);
    char m_size_pad[0x1c]; // own region over CUserLogic (0x40)
};
SIZE(CInGameText, 0x5c);
#endif
