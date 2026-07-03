// CGruntWingzTimeSprite.h - the grunt "wingz" time eyecandy sprite (C:\Proj\Gruntz).
//
// RTTI (.?AVCGruntWingzTimeSprite@@, vtbl 0x1e77cc) gives the true base as
// CGruntHealthSprite (17 slots, slot 16 = origin CGruntHealthSprite overridden).
// We DELIBERATELY model it as `: CUserLogic` DIRECTLY (documented held-base, the
// CLightningHazard precedent). Reason: the leaf dtor @0x0121f0 (0x44 B, 100%)
// stamps the CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4 and
// tears the +0x18 link via ~EngStr @0x16d2a0 - it INLINES the full CUserLogic
// teardown and DEAD-STORE-ELIMINATES the intermediate CGruntHealthSprite/leaf vptr
// stamps (no CGruntHealthSprite vtable ref, no ~CGruntHealthSprite call). Deriving
// CGruntHealthSprite (out-of-line dtor @0x00011fb0 in another TU) would force cl to
// emit a CALL instead of the inlined teardown, regressing the byte-exact dtor. The
// intermediate base adds NO destructible members, so direct-CUserLogic is
// byte-identical to the true chain.
//
// GetWingzTime (0x07fd90) is a free __stdcall accessor: read the bound grunt's
// +0x3f8 wingz-timer field and return it (ret 4 -> callee cleanup -> one stack
// arg, NOT __thiscall). It is a standalone `int __stdcall(CGrunt*)` helper, not a
// member of the sprite - the this/ecx trace only mis-homed it here (stale-ecx
// attribution is unreliable for __stdcall callees): it reads a foreign CGrunt,
// not this sprite, and has no fn-pointer storage. Declared free below.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CGRUNTWINGZTIMESPRITE_H
#define GRUNTZ_CGRUNTWINGZTIMESPRITE_H

#include <rva.h>

#include <Gruntz/Grunt.h>       // CUserLogic base + CGrunt (the accessor's bound grunt)
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)

class CGruntWingzTimeSprite : public CUserLogic {
public:
    // GetTypeTag (0x121a0): the 6-byte per-class logic-type id accessor (0x417).
    LogicTypeId GetTypeTag();
    ~CGruntWingzTimeSprite(); // 0x0121f0 (folds the CUserLogic teardown)
};

// GetWingzTime (0x07fd90): free __stdcall accessor (ret 4) reading the bound
// CGrunt's m_wingzTime (+0x3f8).
i32 __stdcall GetWingzTime(CGrunt* o);

#endif // GRUNTZ_CGRUNTWINGZTIMESPRITE_H
