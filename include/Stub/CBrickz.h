#ifndef GRUNTZ_STUB_CBRICKZ_H
#define GRUNTZ_STUB_CBRICKZ_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CBrickz : CUserLogic (RTTI). sizeof 0x54. The ctor + the grid allocator have
// graduated (src/Gruntz/CBrickz.cpp, src/Gruntz/Brickz.cpp); only the big
// LoadAttributes parser remains here.
class CBrickz : public CUserLogic {
public:
    void LoadAttributes(i32, i32);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CBrickz, 0x54);
#endif
