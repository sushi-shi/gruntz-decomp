// CExitTrigger.h - the level-exit trigger tile-logic game-object (C:\Proj\Gruntz).
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
#include <Gruntz/UserLogic.h> // CUserLogic base (CExitTrigger : CUserLogic)

class CExitTrigger : public CUserLogic {
public:
    i32 GetTypeTag(); // 0x010870 (returns the class logic-type id 0x3f7)
    ~CExitTrigger();  // 0x0108c0 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CEXITTRIGGER_H
