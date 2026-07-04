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

class CExitTrigger : public CUserLogic {
public:
    CExitTrigger(CGameObject* obj);   // 0x03ecf0 (1-arg leaf ctor)
    LogicTypeId GetTypeTag();         // 0x010870 (returns the class logic-type id 0x3f7)
    virtual ~CExitTrigger() OVERRIDE; // 0x0108c0 (folds the CUserLogic teardown)

    i32 m_savedGeoId; // +0x40  saved m_38->m_1b4 geometry id
    char m_pad44[0x54 - 0x44];
    i32 m_warlordId; // +0x54  resolved warlord id
    i32 m_resolved;  // +0x58  resolved gate (1 = warlord bound, 0 = inactive slot)
};
SIZE(CExitTrigger, 0x5c);

#endif // GRUNTZ_CEXITTRIGGER_H
