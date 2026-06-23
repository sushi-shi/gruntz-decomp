#ifndef GRUNTZ_STUB_CWARLORD_H
#define GRUNTZ_STUB_CWARLORD_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CWarlord : CUserLogic (RTTI). sizeof 0xb0.
class CWarlord : public CUserLogic {
public:
    CWarlord(int);
    void LoadAttributes();
    void LoadAttributes2();
    char m_size_pad[0x70]; // own region over CUserLogic (0x40)
};
SIZE(CWarlord, 0xb0);
#endif
