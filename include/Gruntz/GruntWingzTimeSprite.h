#ifndef GRUNTZ_CGRUNTWINGZTIMESPRITE_H
#define GRUNTZ_CGRUNTWINGZTIMESPRITE_H

#include <Gruntz/GruntHealthSprite.h>
#include <rva.h>

#include <Gruntz/Grunt.h>       // CUserLogic base + CGrunt (the accessor's bound grunt)
#include <Gruntz/LogicTypeId.h> // LogicTypeId (GetTypeTag return type)

class CGruntWingzTimeSprite : public CGruntHealthSprite {
public:
    CGruntWingzTimeSprite(CGameObject* obj); // 0x0007fcc0 (body in GruntWingzTimeSprite.cpp)
    // GetTypeTag (0x121a0, slot 2): inline body + RVA in the header so cl emits the
    // COMDAT wherever the ctor's vtable is emitted (GruntWingzTimeSprite.cpp).
    RVA(0x000121a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTWINGZTIMESPRITE;
    }
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
    virtual i32 Vslot16(CGrunt* grunt) OVERRIDE; // slot 16 (stat-time getter)
};
SIZE(0x64); // recovered from operator-new sites (gruntz.analysis.news)
VTBL(CGruntWingzTimeSprite, 0x001e77cc); // vtable_names -> code (RTTI game class)

#endif // GRUNTZ_CGRUNTWINGZTIMESPRITE_H
