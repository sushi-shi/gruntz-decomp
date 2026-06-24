#ifndef GRUNTZ_STUB_CDROPPEDOBJECTSHADOW_H
#define GRUNTZ_STUB_CDROPPEDOBJECTSHADOW_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CDroppedObjectShadow : CUserLogic (RTTI). sizeof 0x54.
class CDroppedObjectShadow : public CUserLogic {
public:
    void LoadAttributes();
    CDroppedObjectShadow(i32);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CDroppedObjectShadow, 0x54);
#endif
