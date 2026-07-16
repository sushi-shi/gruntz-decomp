// GruntWingzTimeSprite.h - the grunt "wingz" time eyecandy sprite (C:\Proj\Gruntz).
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

#include <Gruntz/GruntHealthSprite.h>
#include <rva.h>

#include <Gruntz/Grunt.h>       // CUserLogic base + CGrunt (the accessor's bound grunt)
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)

// NOTE: modeled against Grunt.h's true-0x30 CUserLogic (this header includes Grunt.h
// for CGrunt); it is NOT part of the canonical-world CTileLogic reparent. Adds no members +
// tail, so 0x30-vs-0x40 is a no-op for its GetTypeTag + link-teardown dtor.
//
// STALE CLAIM REMOVED (2026-07-13): the parenthetical used to read "(that world's
// UserLogic.h cannot coexist with Grunt.h in one TU)". False - Grunt.h #includes
// <Gruntz/UserLogic.h> itself, and a TU with UserLogic.h + this header + the other leaf
// headers compiles clean under the real MSVC 5.0 (that is how the AnimWorkerSpriteLeaves.h
// size-views were dissolved). Sizeof here is 0x64, matching the retail operator-new
// immediate exactly (compile-time asserted).
class CGruntWingzTimeSprite : public CGruntHealthSprite {
public:
    CGruntWingzTimeSprite(CGameObject* obj); // 0x0007fcc0 (body in GruntWingzTimeSprite.cpp)
    // GetTypeTag (0x121a0, slot 2): inline body + RVA in the header so cl emits the
    // COMDAT wherever the ctor's vtable is emitted (GruntWingzTimeSprite.cpp).
    RVA(0x000121a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTWINGZTIMESPRITE;
    }
    virtual ~CGruntWingzTimeSprite() OVERRIDE;        // 0x0121f0 (folds the CUserLogic teardown)
    virtual i32 Vslot16(CGrunt* grunt) OVERRIDE; // slot 16 (stat-time getter)
};
SIZE(CGruntWingzTimeSprite, 0x64);       // recovered from operator-new sites (gruntz.analysis.news)
VTBL(CGruntWingzTimeSprite, 0x001e77cc); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_CGRUNTWINGZTIMESPRITE_H
