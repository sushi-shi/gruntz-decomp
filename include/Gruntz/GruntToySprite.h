#ifndef GRUNTZ_CGRUNTTOYSPRITE_H
#define GRUNTZ_CGRUNTTOYSPRITE_H

#include <Ints.h>

#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>

class CGruntToySprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GRUNTTOYSPRITE;
    }
    CGruntToySprite() {}
    CGruntToySprite(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();

    i32 BindToGrunt(i32 playerIndex, i32 unitIndex);
    i32 Update();

    GruntIdentity m_gruntIdentity;
    PickupType m_lastLayer;
};

#endif
