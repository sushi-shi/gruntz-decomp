#ifndef GRUNTZ_STUB_COBJECTDROPPER_H
#define GRUNTZ_STUB_COBJECTDROPPER_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CObjectDropper : CUserLogic (RTTI). sizeof 0x98.
class CObjectDropper : public CUserLogic {
public:
    CObjectDropper(i32);
    char m_size_pad[0x58]; // own region over CUserLogic (0x40)
};
SIZE(CObjectDropper, 0x98);
#endif
