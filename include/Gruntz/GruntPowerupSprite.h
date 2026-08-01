#ifndef GRUNTZ_CGRUNTPOWERUPSPRITE_H
#define GRUNTZ_CGRUNTPOWERUPSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/GruntIndicatorSprite.h>

class CGruntPowerupSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
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

    i32 m_cellX;
    i32 m_cellY;
    i32 m_powerupId;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTPOWERUPSPRITE_H
