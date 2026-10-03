#ifndef GRUNTZ_CGRUNTTOYTIMESPRITE_H
#define GRUNTZ_CGRUNTTOYTIMESPRITE_H

#include <Ints.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/LogicTypeId.h>

class CGruntToyTimeSprite : public CGruntHealthSprite {
public:
    CGruntToyTimeSprite() {}
    CGruntToyTimeSprite(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GRUNTTOYTIMESPRITE;
    }

    virtual i32 GetDisplayedValue(CGrunt* grunt)  ;
};

#endif
