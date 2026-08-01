#ifndef GRUNTZ_CGRUNTWINGZTIMESPRITE_H
#define GRUNTZ_CGRUNTWINGZTIMESPRITE_H

#include <Gruntz/GruntHealthSprite.h>
#include <rva.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/LogicTypeId.h>

class CGruntWingzTimeSprite : public CGruntHealthSprite {
public:
    CGruntWingzTimeSprite() {}
    CGruntWingzTimeSprite(CGameObject* obj);

    RVA(0x000121a0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTWINGZTIMESPRITE;
    }

    virtual i32 Vslot16(CGrunt* grunt) OVERRIDE;
};
SIZE(0x64);

#endif // GRUNTZ_CGRUNTWINGZTIMESPRITE_H
