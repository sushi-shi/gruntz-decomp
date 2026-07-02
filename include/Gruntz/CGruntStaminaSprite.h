// CGruntStaminaSprite.h - the grunt stamina-bar eyecandy sprite (C:\Proj\Gruntz).
//
// CGruntStaminaSprite : CUserLogic (RTTI .?AVCGruntStaminaSprite@@) - a tile-logic
// leaf in the same HUD-sprite family as CGruntHealthSprite. Owner recovered by
// caller-trace: the scalar-deleting-destructor @0x00012040 (CGruntStaminaSprite
// vftable slot 0) tail-calls this plain dtor @0x00012070. The dtor stamps the
// CUserLogic vftable 0x5e705c then the CUserBase vftable 0x5e70b4, tearing down
// the +0x18 link via the embedded ~EngStr @0x16d2a0 - byte-identical in shape to
// ~CGruntHealthSprite @0x00011fb0 / the established leaf-dtor archetype. The leaf
// adds no destructible members beyond CUserLogic, so its dtor folds the bare
// CUserLogic teardown (the /GX leaf-dtor archetype).
//
// Field names are placeholders; only OFFSETS + the inheritance chain are
// load-bearing.
#ifndef GRUNTZ_CGRUNTSTAMINASPRITE_H
#define GRUNTZ_CGRUNTSTAMINASPRITE_H

#include <rva.h>
#include <Gruntz/Grunt.h> // CUserLogic base + CGrunt (the accessor's bound grunt)

class CGruntStaminaSprite : public CUserLogic {
public:
    // GetTypeTag (0x12020): the 6-byte per-class logic-type id accessor (0x410).
    i32 GetTypeTag();
    ~CGruntStaminaSprite(); // 0x00012070 (folds the CUserLogic teardown)
};

// GetStaminaTime (0x07fbb0): free __stdcall accessor (ret 4) reading the bound
// CGrunt's m_stamina (+0x3f0), the sibling of GetWingzTime (m_wingzTime +0x3f8).
// Standalone helper, not a sprite member - stale-ecx trace mis-homing (the
// __stdcall callee reads a foreign CGrunt, no fn-pointer storage).
i32 __stdcall GetStaminaTime(CGrunt* o);

#endif // GRUNTZ_CGRUNTSTAMINASPRITE_H
