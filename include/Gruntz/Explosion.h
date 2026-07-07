// Explosion.h - the explosion eyecandy (C:\Proj\Gruntz), a CUserLogic tile-logic
// leaf (RTTI .?AVCUserLogic@@). Only the /GX leaf dtor is reconstructed here; the
// ctor (0x470e0) remains the @stub backlog in src/Stub/Explosion.cpp. Offsets +
// code bytes are the load-bearing facts; field names are placeholders.
#ifndef GRUNTZ_CEXPLOSION_H
#define GRUNTZ_CEXPLOSION_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CExplosion : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CExplosion(CGameObject* obj);   // 0x470e0
    virtual ~CExplosion() OVERRIDE; // 0x12ec0 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];
};
VTBL(CExplosion, 0x1e766c);
SIZE(CExplosion, 0x54);

#endif // GRUNTZ_CEXPLOSION_H
