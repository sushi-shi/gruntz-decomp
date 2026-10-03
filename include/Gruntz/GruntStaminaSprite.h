#ifndef GRUNTZ_CGRUNTSTAMINASPRITE_H
#define GRUNTZ_CGRUNTSTAMINASPRITE_H

#include <Ints.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/LogicTypeId.h>

class CGruntStaminaSprite : public CGruntHealthSprite {
public:
    CGruntStaminaSprite() {}
    CGruntStaminaSprite(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GRUNTSTAMINASPRITE;
    }

    virtual i32 GetDisplayedValue(CGrunt* grunt)  ;
};

#endif
