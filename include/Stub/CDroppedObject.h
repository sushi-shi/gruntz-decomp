#ifndef GRUNTZ_STUB_CDROPPEDOBJECT_H
#define GRUNTZ_STUB_CDROPPEDOBJECT_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CDroppedObject : CUserLogic (RTTI). sizeof 0x70.
class CDroppedObject : public CUserLogic {
public:
    CDroppedObject(i32);
    void LoadDroppedObjectEffects();
    char m_size_pad[0x30]; // own region over CUserLogic (0x40)
};
SIZE(CDroppedObject, 0x70);
#endif
