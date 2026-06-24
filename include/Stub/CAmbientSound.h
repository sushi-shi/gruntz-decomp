#ifndef GRUNTZ_STUB_CAMBIENTSOUND_H
#define GRUNTZ_STUB_CAMBIENTSOUND_H
#include <rva.h>
#include <Stub/CUserBase.h>
// CAmbientSound : CUserBase (RTTI). sizeof 0x40.
class CAmbientSound : public CUserBase {
public:
    void Stub_00b6a0(i32, i32, i32, i32, i32);
    void Stub_00b7b0(i32, i32, i32, i32, i32);
    char m_size_pad[0x3c]; // own region over CUserBase (0x4)
};
SIZE(CAmbientSound, 0x40);
#endif
