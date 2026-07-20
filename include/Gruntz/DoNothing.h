#ifndef GRUNTZ_CDONOTHING_H
#define GRUNTZ_CDONOTHING_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CDoNothing : CUserLogic)

VTBL(CDoNothing, 0x001e85f4); // vtable_names -> code (RTTI game class)
class CDoNothing : public CUserLogic, public CWapX {
public:
public:
    CDoNothing(CGameObject* obj); // 0xac1d0
    // 0x0000f6b0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x0000f6b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DONOTHING;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1 (0x2b26)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
SIZE(CDoNothing, 0x54);

#endif // GRUNTZ_CDONOTHING_H
