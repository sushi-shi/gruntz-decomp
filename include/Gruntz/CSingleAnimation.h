// CSingleAnimation.h - a single-shot eyecandy animation game-object
// (C:\Proj\Gruntz).
//
// CSingleAnimation : CUserLogic - a tile-logic leaf in the same game-object
// hierarchy as CSimpleAnimation / CBehindCandyAni (proven by its dtor @0x010540
// stamping the CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4,
// tearing down the +0x18 link via the embedded ~EngStr at 0x16d2a0 -
// byte-identical in shape to ~CSimpleAnimation @0x00f9d0 / the established
// leaf-dtor archetype). The leaf adds no destructible members beyond CUserLogic,
// so its dtor folds the bare CUserLogic teardown (the /GX leaf-dtor archetype).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CSINGLEANIMATION_H
#define GRUNTZ_CSINGLEANIMATION_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CSingleAnimation : CUserLogic)

class CSingleAnimation : public CUserLogic {
public:
    ~CSingleAnimation(); // 0x010540 (folds the CUserLogic teardown)
};

#endif // GRUNTZ_CSINGLEANIMATION_H
