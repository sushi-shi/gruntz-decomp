// TileTrigger.h - the CTileTrigger tile-logic game-object family (C:\Proj\Gruntz).
//
// CTileTrigger : CUserLogic (vftable 0x5e7f14) is the shared base (declared in
// <Gruntz/UserLogic.h>). Three RTTI-named leaves derive from it, each adding no
// data members - the ctor chains CTileTrigger(obj) (out-of-line) and the leaf vptr
// auto-stamps; the empty dtor folds the bare CUserLogic teardown (a manual-vptr
// model would be frameless - see docs/patterns/eh-dtor-needs-base-subobject.md).
// Their ctors/dtors + the two per-class activation registries live in
// src/Gruntz/TileTrigger.cpp; the state pumps that `new` them are in
// src/Gruntz/TileLogicPump.cpp. Only offsets / code bytes are load-bearing; field
// names are placeholders.
#ifndef GRUNTZ_TILETRIGGER_H
#define GRUNTZ_TILETRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CTileTrigger base (CUserLogic hierarchy + CGameObject)

SIZE_UNKNOWN(CTileSecretTrigger);
VTBL(CTileSecretTrigger, 0x001e7e64); // vtable_names -> code (RTTI game class)
class CTileSecretTrigger : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
    CTileSecretTrigger(CGameObject* obj);      // 0x10fa60
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    // NO own slot-4 body and no act cluster: RTTI says this class's vtable (0x1e7e64)
    // slot 4 is INHERITED from CTileTrigger (-> 0x0034fe -> jmp 0x10e4a0). The
    // 0x10f160.. cluster it used to claim is CCheckpointTrigger's (the shift-by-one).
};

SIZE_UNKNOWN(CGiantRock);
VTBL(CGiantRock, 0x001e7d5c); // vtable_names -> code (RTTI game class)
class CGiantRock : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
    CGiantRock(CGameObject* obj);              // 0x10fa90
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};

SIZE_UNKNOWN(CCoveredPowerup);
VTBL(CCoveredPowerup, 0x001e7e0c); // vtable_names -> code (RTTI game class)
class CCoveredPowerup : public CTileTrigger {
public:
    virtual LogicTypeId GetTypeTag() OVERRIDE; // slot 2
    CCoveredPowerup(CGameObject* obj);         // 0x10fac0
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};

// The activation-registry entry records (the .data CActReg rows; 4-byte PMFs).
typedef i32 (CUserLogic::*TileTriggerHandler)();
struct CTileTriggerActEntry {
    TileTriggerHandler m_fn;
};


#endif // GRUNTZ_TILETRIGGER_H
