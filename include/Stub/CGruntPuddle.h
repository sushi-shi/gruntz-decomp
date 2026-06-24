#ifndef GRUNTZ_STUB_CGRUNTPUDDLE_H
#define GRUNTZ_STUB_CGRUNTPUDDLE_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CGruntPuddle : CUserLogic (RTTI). sizeof 0x70.
class CGruntPuddle : public CUserLogic {
public:
    CGruntPuddle(i32);
    char m_size_pad[0x30]; // own region over CUserLogic (0x40)
};
SIZE(CGruntPuddle, 0x70);
#endif
