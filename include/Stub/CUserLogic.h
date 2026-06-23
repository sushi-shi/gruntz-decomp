#ifndef GRUNTZ_STUB_CUSERLOGIC_H
#define GRUNTZ_STUB_CUSERLOGIC_H
#include <rva.h>
#include <Stub/CUserBase.h>
// CUserLogic : CUserBase - stub-world spine for the un-graduated game-object
// family. Real field layout in Gruntz/UserLogic.h; here we only need the correct
// size 0x40 (proven by CPathHazard's byte-exact ctor) so leaves compose, plus this
// stub's RVA-labeled methods. (A thin leaf new's 0x54 = 0x40 base + 0x14 leaf data.)
class CUserLogic : public CUserBase {
public:
    void CUserLogic_0138d0();      // RVA 0x0138d0 (the shared base ctor)
    void CUserLogic_058cd0(int);   // RVA 0x058cd0
    char m_pad[0x40 - 0x04];       // own region over CUserBase(0x04); fields in UserLogic.h
};
SIZE(CUserLogic, 0x40);
#endif // GRUNTZ_STUB_CUSERLOGIC_H
