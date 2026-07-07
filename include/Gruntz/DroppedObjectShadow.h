// DroppedObjectShadow.h - the dropped-object shadow eyecandy (C:\Proj\Gruntz),
// a CUserLogic tile-logic leaf (RTTI .?AVCUserLogic@@). The /GX leaf dtor + the
// 1-arg ctor (0xc7490) are reconstructed here. NOTE: 0xc62e0 (Ghidra-labeled
// "LoadAttributes@CDroppedObjectShadow") was a trace mis-attribution - it is
// really CObjectDropper::Update (a LARGER sibling class); reconstructed byte-exact
// in src/Gruntz/ObjectDropper.cpp. Offsets + code bytes are the load-bearing
// facts; field names are placeholders.
#ifndef GRUNTZ_CDROPPEDOBJECTSHADOW_H
#define GRUNTZ_CDROPPEDOBJECTSHADOW_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CDroppedObjectShadow : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CDroppedObjectShadow(CGameObject* obj);   // 0xc7490 (1-arg leaf ctor)
    virtual ~CDroppedObjectShadow() OVERRIDE; // 0x12670 (folds the CUserLogic teardown)
    i32 m_savedGeoId;                         // +0x40  m_38->m_geoId snapshot
    char m_pad44[0x54 - 0x44];
};
VTBL(CDroppedObjectShadow, 0x1e787c);
SIZE(CDroppedObjectShadow, 0x54);

#endif // GRUNTZ_CDROPPEDOBJECTSHADOW_H
