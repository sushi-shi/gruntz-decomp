#ifndef GRUNTZ_STUB_CBRICKZ_H
#define GRUNTZ_STUB_CBRICKZ_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CBrickz : CUserLogic (RTTI). sizeof 0x54.
class CBrickz : public CUserLogic {
public:
    void LoadAttributes(i32, i32);
    CBrickz(i32);
    i32 winapi_09ea60_IntersectRect(i32, i32, i32);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CBrickz, 0x54);
#endif
