#ifndef GRUNTZ_CPARTICLEZ_H
#define GRUNTZ_CPARTICLEZ_H

#include <rva.h>
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CParticlez : CUserLogic)

class CParticlez : public CUserLogic, public CWapX {
public:
    // +0x40..+0x53 unmodeled tail (sizeof proven 0x54 by the worker-pump new-site
    // `push 0x54` @0x46850 LogicDispatchC, FortressFlag.cpp).

public:
    CParticlez(CGameObject* obj); // 0x046ad0 (ctor body in UserLogic.cpp)
    // 0x00012cd0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00012cd0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PARTICLEZ;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1
    virtual void FireActivation(i32 id) OVERRIDE;                     // 0x046d30
    // Bind the per-frame handler (Update) to the activation key "A" via the shared
    // name registry (the same archetype as CSecretTeleporterTrigger::RegisterActs).
    static void RegisterActs(); // 0x046e90
    i32 Update();               // 0x047090 (advance anim + on-screen latch; ret 0)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
};
SIZE_UNKNOWN();

typedef void (CUserLogic::*PartHandler)();
struct CPartEntry {
    PartHandler m_fn; // [entry]
};
SIZE_UNKNOWN();
typedef i32 (CUserLogic::*PartHandlerI32)();
struct CPartEntryI32 {
    PartHandlerI32 m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CPARTICLEZ_H
