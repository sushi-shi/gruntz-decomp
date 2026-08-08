#ifndef GRUNTZ_CGRUNTHEALTHSPRITE_H
#define GRUNTZ_CGRUNTHEALTHSPRITE_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGrunt;

class CGruntHealthSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00011f60, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTHEALTHSPRITE;
    }
    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 HealthUpdate();
    i32 SetHealthGlyph(i32 x, i32 y, i32 health);

    virtual i32 GetDisplayedValue(CGrunt* grunt);
    RVA(0x00011ef0, 0x4b)
    CGruntHealthSprite() {}
    CGruntHealthSprite(CGameObject* obj);

    Coord m_cell;
    i32 m_health;
    i32 m_yOffset;
};
SIZE(0x64);

#endif // GRUNTZ_CGRUNTHEALTHSPRITE_H
