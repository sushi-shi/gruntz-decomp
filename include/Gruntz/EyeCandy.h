#ifndef GRUNTZ_CEYECANDY_H
#define GRUNTZ_CEYECANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CEyeCandy : CUserLogic)

class CEyeCandy : public CUserLogic, public CWapX {
public:
public:
    CEyeCandy(CGameObject* obj); // 0x0ac620 (ctor body in UserLogic.cpp)
    // 0x0000fca0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x0000fca0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EYECANDY;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CEYECANDY_H
