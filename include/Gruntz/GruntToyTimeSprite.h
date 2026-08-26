#ifndef GRUNTZ_CGRUNTTOYTIMESPRITE_H
#define GRUNTZ_CGRUNTTOYTIMESPRITE_H

#include <rva.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/GruntHealthSprite.h>
#include <Gruntz/LogicTypeId.h>

class CGruntToyTimeSprite : public CGruntHealthSprite {
public:
    CGruntToyTimeSprite() {}
    CGruntToyTimeSprite(CGameObject* obj);

    RVA(0x000120f0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTTOYTIMESPRITE;
    }

    virtual i32 GetDisplayedValue(CGrunt* grunt) OVERRIDE;
};

#endif // GRUNTZ_CGRUNTTOYTIMESPRITE_H
