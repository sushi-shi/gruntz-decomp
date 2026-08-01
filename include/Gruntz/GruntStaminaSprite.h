#ifndef GRUNTZ_CGRUNTSTAMINASPRITE_H
#define GRUNTZ_CGRUNTSTAMINASPRITE_H

#include <Gruntz/GruntHealthSprite.h>
#include <rva.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/LogicTypeId.h>

class CGruntStaminaSprite : public CGruntHealthSprite {
public:
    CGruntStaminaSprite() {}
    CGruntStaminaSprite(CGameObject* obj);

    RVA(0x00012020, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTSTAMINASPRITE;
    }

    virtual i32 GetDisplayedValue(CGrunt* grunt) OVERRIDE;
};
SIZE(0x64);

#endif // GRUNTZ_CGRUNTSTAMINASPRITE_H
