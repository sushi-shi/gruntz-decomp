// EyeCandy.h - the eyecandy tile-logic game-object (C:\Proj\Gruntz).
//
// CEyeCandy : CUserLogic - a tile-logic leaf in the same game-object hierarchy
// as CTimeBomb (proven by its dtor @0x00fd60 stamping the CUserLogic vftable
// 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18 link via
// the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to ~CTimeBomb
// @0x012a70). The leaf adds no destructible members beyond CUserLogic, so its
// dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// GetTypeTag (0x00fca0) is the per-class logic-type id accessor (returns 0x3f1),
// the SAME 6-byte `mov eax,<id>; ret` virtual as CTileTriggerTransition::
// GetTypeTag (0x011730, 0x405).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CEYECANDY_H
#define GRUNTZ_CEYECANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CEyeCandy : CUserLogic)

SIZE_UNKNOWN(CEyeCandy);
class CEyeCandy : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CEyeCandy(CGameObject* obj);               // 0x0ac620 (ctor body in UserLogic.cpp)
    virtual LogicTypeId GetTypeTag() OVERRIDE; // 0x00fca0 (returns the class logic-type id 0x3f1)
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    virtual i32 UserLogicVfunc2() OVERRIDE;                            // slot 4
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x00fcc0 (vtable slot 1: two-chain Serialize)
    virtual ~CEyeCandy() OVERRIDE;                // 0x00fd60 (folds the CUserLogic teardown)
};
VTBL(CEyeCandy, 0x001e843c);

#endif // GRUNTZ_CEYECANDY_H
