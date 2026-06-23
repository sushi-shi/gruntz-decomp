#ifndef GRUNTZ_STUB_CFORTRESSFLAG_H
#define GRUNTZ_STUB_CFORTRESSFLAG_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CFortressFlag : CUserLogic (RTTI). sizeof 0x54.
class CFortressFlag : public CUserLogic {
public:
    CFortressFlag(int);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CFortressFlag, 0x54);
#endif
