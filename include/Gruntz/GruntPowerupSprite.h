#ifndef GRUNTZ_CGRUNTPOWERUPSPRITE_H
#define GRUNTZ_CGRUNTPOWERUPSPRITE_H

#include <Ints.h>

#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

class CGruntPowerupSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GRUNTPOWERUPSPRITE;
    }
    CGruntPowerupSprite() {}
    CGruntPowerupSprite(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();

    i32 BindToGrunt(i32 playerIndex, i32 unitIndex, i32 powerupId);
    i32 Update();

    GruntIdentity m_gruntIdentity;
    i32 m_powerupId;
};

#endif
