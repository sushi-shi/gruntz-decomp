#ifndef GRUNTZ_CBEHINDCANDY_H
#define GRUNTZ_CBEHINDCANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CBehindCandy : CUserLogic)

class CBehindCandy : public CUserLogic, public CWapX {
public:
public:
    CBehindCandy(CGameObject* obj); // 0x0ac3f0 (ctor body in UserLogic.cpp)
    // 0x0000fb70 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x0000fb70, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_BEHINDCANDY;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
VTBL(CBehindCandy, 0x001e8494);
SIZE(CBehindCandy, 0x54);

#endif // GRUNTZ_CBEHINDCANDY_H
