#ifndef GRUNTZ_STUB_CTIMEBOMB_H
#define GRUNTZ_STUB_CTIMEBOMB_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CTimeBomb : CUserLogic (RTTI). sizeof 0x68.
class CTimeBomb : public CUserLogic {
public:
    CTimeBomb(int);
    void LoadAttributes();
    char m_size_pad[0x28]; // own region over CUserLogic (0x40)
};
SIZE(CTimeBomb, 0x68);
#endif
