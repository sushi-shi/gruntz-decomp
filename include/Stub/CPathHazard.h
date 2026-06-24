#ifndef GRUNTZ_STUB_CPATHHAZARD_H
#define GRUNTZ_STUB_CPATHHAZARD_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CPathHazard : CUserLogic (RTTI). sizeof 0x130.
class CPathHazard : public CUserLogic {
public:
    void CPathHazard_0b35a0(i32);
    char m_size_pad[0xf0]; // own region over CUserLogic (0x40)
};
SIZE(CPathHazard, 0x130);
#endif
