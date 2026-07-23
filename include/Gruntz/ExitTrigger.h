#ifndef GRUNTZ_CEXITTRIGGER_H
#define GRUNTZ_CEXITTRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CExitTrigger : CUserLogic)

class CWarlord;

class CExitTrigger : public CUserLogic, public CWapX {
public:
public:
    CExitTrigger(CGameObject* obj); // 0x03ecf0 (1-arg leaf ctor)
    // 0x00010870 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x00010870, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_EXITTRIGGER;
    }
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, i32) OVERRIDE; // slot 1 (0x3f040)
    // slot 4 (0x3f290, body in WormholeActs.cpp): the act-registry PMF dispatcher over
    // g_exitTriggerActReg. RE-ATTRIBUTED from the ex "CWormhole act cluster" - RTTI:
    // CExitTrigger[4] override -> ILT 0x0042e6, and 0x0042e6 -> jmp 0x3f290 (sema
    // xref); the cluster's band (0x3f210..0x3f77d) also continues this class's own
    // ExitTrigger.cpp band (0x3ecf0..0x3f187) contiguously.
    virtual void FireActivation(i32 id) OVERRIDE; // 0x3f290
    static void RegisterActs();                   // 0x03f3f0 (binds the "A" activation handler)
    i32 AdvanceAnim(); // 0x03f5f0 (the per-frame handler PMF; declared-only)
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; RVA_COMPGEN pin in the home TU).
    CWarlord* m_warlordLogic; // +0x54  the resolved warlord's bound logic (obj->m_7c->m_logic)
    i32 m_resolved;           // +0x58  resolved gate (1 = warlord bound, 0 = inactive slot)
};
SIZE(0x5c);

typedef i32 (CUserLogic::*ExitTriggerHandler)();
struct CExitActEntry {
    ExitTriggerHandler m_fn;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CEXITTRIGGER_H
