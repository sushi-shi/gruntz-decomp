// GruntStaminaSprite.h - the grunt stamina-bar eyecandy sprite (C:\Proj\Gruntz).
//
// RTTI (.?AVCGruntStaminaSprite@@, vtbl 0x1e7a44) gives the true base as
// CGruntHealthSprite (17 slots, slot 16 = origin CGruntHealthSprite overridden).
// We DELIBERATELY model it as `: CUserLogic` DIRECTLY, not `: CGruntHealthSprite`
// - a documented held-base (the CLightningHazard precedent). Reason: the leaf dtor
// @0x00012070 (0x44 B, 100%) stamps the CUserLogic vftable 0x5e705c then the
// CUserBase vftable 0x5e70b4 and tears the +0x18 link via ~EngStr @0x16d2a0 - it
// INLINES the full CUserLogic teardown and DEAD-STORE-ELIMINATES the intermediate
// CGruntHealthSprite/leaf vptr stamps (dump 0x12130/0x12070: `[esi]=0x5e705c`
// first, NO CGruntHealthSprite vtable ref, NO ~CGruntHealthSprite call). Deriving
// CGruntHealthSprite (whose dtor @0x00011fb0 is out-of-line in another TU, hence
// not inlinable here) would force cl to emit a CALL to ~CGruntHealthSprite instead
// of the inlined teardown, regressing the byte-exact dtor. The intermediate base
// adds NO destructible members, so direct-CUserLogic is byte-identical to the true
// chain. (Also blocked from a shared model by the fat-0x40 vs true-0x30 CUserLogic
// dual-model + the GruntIndicatorSprite-vs-CGrunt sub-world split.)
//
// Owner recovered by caller-trace: the scalar-deleting-destructor @0x00012040
// (vftable slot 0) tail-calls this plain dtor @0x00012070.
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CGRUNTSTAMINASPRITE_H
#define GRUNTZ_CGRUNTSTAMINASPRITE_H

#include <Gruntz/GruntHealthSprite.h>
#include <rva.h>

#include <Gruntz/Grunt.h>       // CUserLogic base + CGrunt (the accessor's bound grunt)
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)

// Grunt.h-world class (includes Grunt.h for CGrunt); NOT part of the canonical-world
// CTileLogic reparent - stays `: CUserLogic` (Grunt.h's true-0x30) until stage 5.
class CGruntStaminaSprite : public CGruntHealthSprite {
public:
    CGruntStaminaSprite(CGameObject* obj); // 0x0007fae0 (body in GruntStaminaSprite.cpp)
    // GetTypeTag (0x12020, slot 2): inline body + RVA in the header so cl emits the
    // COMDAT wherever the ctor's vtable is emitted (GruntStaminaSprite.cpp).
    RVA(0x00012020, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSTAMINASPRITE;
    }
    virtual ~CGruntStaminaSprite() OVERRIDE;          // 0x00012070 (folds the CUserLogic teardown)
    virtual i32 Vslot16(CGruntEntry* grunt) OVERRIDE; // slot 16 (stat-time getter)
};
SIZE(CGruntStaminaSprite, 0x64);       // recovered from operator-new sites (gruntz.analysis.news)
VTBL(CGruntStaminaSprite, 0x001e7a44); // vtable_names -> code (RTTI game class)

// GetStaminaTime (0x07fbb0): free __stdcall accessor (ret 4) reading the bound
// CGrunt's m_stamina (+0x3f0), the sibling of GetWingzTime (m_wingzTime +0x3f8).
// Standalone helper, not a sprite member - stale-ecx trace mis-homing (the
// __stdcall callee reads a foreign CGrunt, no fn-pointer storage).
i32 __stdcall GetStaminaTime(CGrunt* o);

#endif // GRUNTZ_CGRUNTSTAMINASPRITE_H
