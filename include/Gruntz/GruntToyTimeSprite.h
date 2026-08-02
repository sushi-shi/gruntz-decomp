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

    RVA(0x000120e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTTOYTIMESPRITE;
    }

    virtual i32 GetDisplayedValue(CGrunt* grunt) OVERRIDE;
};
SIZE(0x64);

#endif // GRUNTZ_CGRUNTTOYTIMESPRITE_H
