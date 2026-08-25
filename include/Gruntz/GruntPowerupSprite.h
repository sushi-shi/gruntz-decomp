#ifndef GRUNTZ_CGRUNTPOWERUPSPRITE_H
#define GRUNTZ_CGRUNTPOWERUPSPRITE_H

#include <rva.h>

#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

class CGruntPowerupSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00012320, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_GRUNTPOWERUPSPRITE;
    }
    CGruntPowerupSprite() {}
    CGruntPowerupSprite(CGameObject* obj);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterActs();

    i32 BindToGrunt(i32 playerIndex, i32 unitIndex, i32 powerupId);
    i32 Update();

    GruntIdentity m_gruntIdentity;
    i32 m_powerupId;
};

#endif // GRUNTZ_CGRUNTPOWERUPSPRITE_H
