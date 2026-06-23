#ifndef GRUNTZ_STUB_CINGAMEICON_H
#define GRUNTZ_STUB_CINGAMEICON_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CInGameIcon : CUserLogic (RTTI). sizeof 0x80.
class CInGameIcon : public CUserLogic {
public:
    CInGameIcon(int);
    char m_size_pad[0x40]; // own region over CUserLogic (0x40)
};
SIZE(CInGameIcon, 0x80);
#endif
