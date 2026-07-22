#ifndef GRUNTZ_CTOOBSPIKEZ_H
#define GRUNTZ_CTOOBSPIKEZ_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CToobSpikez : CUserLogic)

class CToobSpikez : public CUserLogic, public CWapX {
public:
public:
    CToobSpikez(CGameObject* obj); // 0x1145c0 (ctor body in UserLogic.cpp)
    // The class's own CUserLogic slot overrides, reconstructed as regular methods
    // (the fat base models slots 1/2 with placeholder signatures; see the .cpp).
    // 0x00012ba0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00012ba0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_TOOBSPIKEZ;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    void Register();                       // 0x1147e0 (reserve the activation range)
    virtual void FireActivation(i32 id) OVERRIDE; // 0x114860 (vtable slot 4)
    static void RegisterActs();                   // 0x1149c0 (binds the logic handler to key "A";
    //  static: no this, called this-less by the factory)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
SIZE(0x54);

#endif // GRUNTZ_CTOOBSPIKEZ_H
