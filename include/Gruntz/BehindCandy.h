// BehindCandy.h - the behind-candy eyecandy tile-logic game-object (C:\Proj\Gruntz).
//
// CBehindCandy : CUserLogic - a tile-logic leaf in the same game-object hierarchy
// as CTimeBomb (proven by its dtor @0x00fc30 stamping the CUserLogic vftable
// 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down the +0x18 link via
// the embedded ~EngStr at 0x16d2a0 - byte-identical in shape to ~CTimeBomb
// @0x012a70). The leaf adds no destructible members beyond CUserLogic, so its
// dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// GetTypeTag (0x00fb70) is the per-class logic-type id accessor (returns 0x3f0),
// the SAME 6-byte `mov eax,<id>; ret` virtual as CTileTriggerTransition::
// GetTypeTag (0x011730, 0x405).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CBEHINDCANDY_H
#define GRUNTZ_CBEHINDCANDY_H

#include <rva.h>

#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)
#include <Gruntz/UserLogic.h>   // CUserLogic base (CBehindCandy : CUserLogic)

SIZE_UNKNOWN(CBehindCandy);
class CBehindCandy : public CTileLogic {
public:
    CBehindCandy(CGameObject* obj); // 0x0ac3f0 (ctor body in UserLogic.cpp)
    LogicTypeId GetTypeTag();       // 0x00fb70 (returns the class logic-type id 0x3f0)
    i32 Serialize(i32 ar, i32 tag, i32 c, i32 d); // 0x00fb90 (vtable slot 1: two-chain Serialize)
    virtual ~CBehindCandy() OVERRIDE;             // 0x00fc30 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CBEHINDCANDY_H
