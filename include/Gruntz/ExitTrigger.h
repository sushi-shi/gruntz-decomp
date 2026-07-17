// ExitTrigger.h - the level-exit trigger tile-logic game-object (C:\Proj\Gruntz).
//
// CExitTrigger : CUserLogic - a tile-logic leaf in the same game-object hierarchy
// as CTimeBomb (proven by its dtor @0x0108c0 stamping the CUserLogic vftable
// 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18 link via
// the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to ~CTimeBomb
// @0x012a70). The leaf adds no destructible members beyond CUserLogic, so its
// dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// GetTypeTag (0x010870) is the per-class logic-type id accessor (returns 0x3f7),
// the SAME 6-byte `mov eax,<id>; ret` virtual as CTileTriggerTransition::
// GetTypeTag (0x011730, 0x405).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CEXITTRIGGER_H
#define GRUNTZ_CEXITTRIGGER_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CExitTrigger : CUserLogic)

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
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1 (0x3f040)
    // slot 4: RTTI proves this class overrides (its own slot rva, not the base 0x246e),
    // but the body is NOT reconstructed yet - declared-only, deliberately no definition.
    virtual void FireActivation(i32 id) OVERRIDE;
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    CUserLogic* m_warlordLogic; // +0x54  the resolved warlord's bound logic (obj->m_7c->m_logic)
    i32 m_resolved;             // +0x58  resolved gate (1 = warlord bound, 0 = inactive slot)
};
VTBL(CExitTrigger, 0x001e822c);
SIZE(CExitTrigger, 0x5c);

#endif // GRUNTZ_CEXITTRIGGER_H
