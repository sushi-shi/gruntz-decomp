// DoNothing.h - the inert "do nothing" tile-logic game-object (C:\Proj\Gruntz).
//
// CDoNothing : CUserLogic - a tile-logic leaf in the same game-object hierarchy
// as CTimeBomb (proven by its dtor @0x00f770 stamping the CUserLogic vftable
// 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18 link via
// the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to ~CTimeBomb
// @0x012a70). The leaf adds no destructible members beyond CUserLogic, so its
// dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// GetTypeTag (0x00f6b0) is the per-class logic-type id accessor (returns 0x3ec),
// the SAME 6-byte `mov eax,<id>; ret` virtual as CTileTriggerTransition::
// GetTypeTag (0x011730, 0x405).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CDONOTHING_H
#define GRUNTZ_CDONOTHING_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CDoNothing : CUserLogic)

SIZE_UNKNOWN(CDoNothing);
class CDoNothing : public CUserLogic {
public:
    CDoNothing(CGameObject* obj);   // 0xac1d0
    LogicTypeId GetTypeTag();       // 0x00f6b0 (returns the class logic-type id 0x3ec)
    virtual ~CDoNothing() OVERRIDE; // 0x00f770 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CDONOTHING_H
