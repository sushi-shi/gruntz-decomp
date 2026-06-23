#ifndef GRUNTZ_STUB_CKITCHENSLIME_H
#define GRUNTZ_STUB_CKITCHENSLIME_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CKitchenSlime : CUserLogic (RTTI). sizeof 0x90.
class CKitchenSlime : public CUserLogic {
public:
    CKitchenSlime(int);
    char m_size_pad[0x50]; // own region over CUserLogic (0x40)
};
SIZE(CKitchenSlime, 0x90);
#endif
