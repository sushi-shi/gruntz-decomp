#ifndef GRUNTZ_CGRUNTPOWERUPSPRITE_H
#define GRUNTZ_CGRUNTPOWERUPSPRITE_H

#include <rva.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

class CGruntPowerupSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00012320, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTPOWERUPSPRITE;
    }
    CGruntPowerupSprite() {}
    CGruntPowerupSprite(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 SetCell(i32 x, i32 y, i32 powerup);
    i32 Update();

    Coord m_cell;
    i32 m_powerupId;
};
SIZE(0x60);

#endif // GRUNTZ_CGRUNTPOWERUPSPRITE_H
