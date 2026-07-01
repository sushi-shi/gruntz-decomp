// CGruntWingzTimeSprite.h - the grunt "wingz" time eyecandy sprite (C:\Proj\Gruntz).
//
// CGruntWingzTimeSprite : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CSimpleAnimation (proven by its dtor @0x0121f0 stamping the
// CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down
// the +0x18 link via the embedded ~EngStr at 0x16d2a0 - byte-identical in shape
// to the established leaf-dtor archetype). The leaf adds no destructible members
// beyond CUserLogic, so its dtor folds the bare CUserLogic teardown (the /GX
// leaf-dtor archetype).
//
// GetWingzTime (0x07fd90) is a tiny __stdcall accessor: read the bound object's
// +0x3f8 wingz-timer field and return it (ret 4 -> callee cleanup -> one stack
// arg, NOT __thiscall).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CGRUNTWINGZTIMESPRITE_H
#define GRUNTZ_CGRUNTWINGZTIMESPRITE_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CGruntWingzTimeSprite : CUserLogic)

// The bound grunt/game-object the accessor reads the +0x3f8 wingz-timer out of.
// Only that touched offset is load-bearing; modeled minimally here.
SIZE_UNKNOWN(CWingzTimeHost);
struct CWingzTimeHost {
    char m_pad0[0x3f8];
    i32 m_3f8; // +0x3f8  wingz timer value
};

class CGruntWingzTimeSprite : public CUserLogic {
public:
    // GetTypeTag (0x121a0): the 6-byte per-class logic-type id accessor (0x417).
    i32 GetTypeTag();
    static i32 __stdcall GetWingzTime(CWingzTimeHost* o); // 0x07fd90
    ~CGruntWingzTimeSprite(); // 0x0121f0 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CGRUNTWINGZTIMESPRITE_H
