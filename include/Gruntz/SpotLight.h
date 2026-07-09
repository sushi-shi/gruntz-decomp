// SpotLight.h - the spotlight eyecandy game-object (C:\Proj\Gruntz), a CUserLogic
// leaf (vftable 0x5e75bc). Own fields begin past the CUserLogic base (+0x40): the
// rotation/offset doubles + the per-tick state ints. Only offsets / code bytes are
// load-bearing; names are placeholders for the recovered engine identities.
#ifndef GRUNTZ_CSPOTLIGHT_H
#define GRUNTZ_CSPOTLIGHT_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (+ CGruntArchive / CGameObject)

class CSpotLight : public CUserLogic {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual LogicTypeId GetTypeTag() OVERRIDE;                         // slot 2
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    TILE_LOGIC_TAIL
public:
    CSpotLight(CGameObject* obj); // 0xb1200
    virtual ~CSpotLight() OVERRIDE;
    // The vtable slot-4 (UserLogicVfunc2) activation dispatcher body (0x0b1630);
    // a plain method - the base placeholder blocks the int-arg OVERRIDE spelling.
    i32 RunAct(i32 id);

    char m_pad40[0x58 - 0x40];
    double m_58; // +0x58  per-tick rate
    double m_60; // +0x60
    double m_68; // +0x68
    double m_70; // +0x70
    double m_78; // +0x78
    double m_80; // +0x80
    double m_88; // +0x88
    double m_90; // +0x90  pi or 0
    i32 m_98;    // +0x98
    i32 m_9c;    // +0x9c
    i32 m_a0;    // +0xa0
    i32 m_a4;    // +0xa4
};
VTBL(CSpotLight, 0x1e75bc);
SIZE(CSpotLight, 0xa8);

#endif // GRUNTZ_CSPOTLIGHT_H
