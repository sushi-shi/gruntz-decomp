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

VTBL(CDoNothing, 0x001e85f4); // vtable_names -> code (RTTI game class)
class CDoNothing : public CUserLogic {
public:
    TILE_LOGIC_TAIL
public:
    CDoNothing(CGameObject* obj); // 0xac1d0
    // 0x0000f6b0 vtable slot 2: per-class logic-type id, inline (one
    // deduped COMDAT copy in retail; see docs on header-inline members).
    RVA(0x0000f6b0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DONOTHING;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1 (0x2b26)
    virtual ~CDoNothing() OVERRIDE; // 0x00f770 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40]; // +0x40..0x53 (leaf tail; sizeof from `new CDoNothing` @0xa9cc0)
};
SIZE(CDoNothing, 0x54);

#endif // GRUNTZ_CDONOTHING_H
