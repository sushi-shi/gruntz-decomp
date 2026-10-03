#ifndef GRUNTZ_CGRUNTWINGZTIMESPRITE_H
#define GRUNTZ_CGRUNTWINGZTIMESPRITE_H

#include <Ints.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/LogicTypeId.h>

class CGruntWingzTimeSprite : public CGruntHealthSprite {
public:
    CGruntWingzTimeSprite() {}
    CGruntWingzTimeSprite(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GRUNTWINGZTIMESPRITE;
    }

    virtual i32 GetDisplayedValue(CGrunt* grunt)  ;
};

#endif
