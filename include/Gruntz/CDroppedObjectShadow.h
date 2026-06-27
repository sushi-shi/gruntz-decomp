// CDroppedObjectShadow.h - the dropped-object shadow eyecandy (C:\Proj\Gruntz),
// a CUserLogic tile-logic leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is
// reconstructed here; the ctor (0xc7490) + LoadAttributes (0xc62e0) remain the
// @stub backlog in src/Stub/CDroppedObjectShadow.cpp. Offsets + code bytes are
// the load-bearing facts; field names are placeholders.
#ifndef GRUNTZ_CDROPPEDOBJECTSHADOW_H
#define GRUNTZ_CDROPPEDOBJECTSHADOW_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CDroppedObjectShadow : public CUserLogic {
public:
    CDroppedObjectShadow(CGameObject* obj); // 0xc7490 (1-arg leaf ctor)
    ~CDroppedObjectShadow();                // 0x12670 (folds the CUserLogic teardown)
    i32 m_40;                               // +0x40
    char m_pad44[0x54 - 0x44];
};
SIZE(CDroppedObjectShadow, 0x54);

#endif // GRUNTZ_CDROPPEDOBJECTSHADOW_H
