#ifndef GRUNTZ_STUB_CTOYPEEK_H
#define GRUNTZ_STUB_CTOYPEEK_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CToyPeek : CUserLogic (RTTI). sizeof 0x68.
class CToyPeek : public CUserLogic {
public:
    CToyPeek(int);
    char m_size_pad[0x28]; // own region over CUserLogic (0x40)
};
SIZE(CToyPeek, 0x68);
#endif
