#ifndef GRUNTZ_CGRUNTSTAMINASPRITE_H
#define GRUNTZ_CGRUNTSTAMINASPRITE_H

#include <Gruntz/GruntHealthSprite.h>
#include <rva.h>

#include <Gruntz/Grunt.h>       // CUserLogic base + CGrunt (the accessor's bound grunt)
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)

class CGruntStaminaSprite : public CGruntHealthSprite {
public:
    CGruntStaminaSprite(CGameObject* obj); // 0x0007fae0 (body in GruntStaminaSprite.cpp)
    // GetTypeTag (0x12020, slot 2): inline body + RVA in the header so cl emits the
    // COMDAT wherever the ctor's vtable is emitted (GruntStaminaSprite.cpp).
    RVA(0x00012020, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSTAMINASPRITE;
    }
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    virtual i32 Vslot16(CGrunt* grunt) OVERRIDE; // slot 16 (stat-time getter)
};
SIZE(0x64); // recovered from operator-new sites (gruntz.analysis.news)
VTBL(CGruntStaminaSprite, 0x001e7a44); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_CGRUNTSTAMINASPRITE_H
