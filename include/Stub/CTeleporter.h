#ifndef GRUNTZ_STUB_CTELEPORTER_H
#define GRUNTZ_STUB_CTELEPORTER_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CTeleporter : CUserLogic (RTTI). sizeof 0x70.
class CTeleporter : public CUserLogic {
public:
    void LoadAttributes();
    char m_size_pad[0x30]; // own region over CUserLogic (0x40)
};
SIZE(CTeleporter, 0x70);
#endif
