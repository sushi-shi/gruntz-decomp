#ifndef GRUNTZ_CFRONTCANDY_H
#define GRUNTZ_CFRONTCANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CFrontCandy : CUserLogic)

class CFrontCandy : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    // slot 2: per-class logic-type id, inline (emitted with the ctor's vtable in UserLogic.cpp)
    RVA(0x0000fa40, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_FRONTCANDY;
    }
public:
    CFrontCandy(CGameObject* obj); // 0x0abfa0
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
                               // logic-worker pump @0xaa1e0, pushes 0x54)
};
SIZE_UNKNOWN();
VTBL(CFrontCandy, 0x1e84ec);

#endif // GRUNTZ_CFRONTCANDY_H
