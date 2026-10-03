#ifndef GRUNTZ_CGRUNTSELECTEDSPRITE_H
#define GRUNTZ_CGRUNTSELECTEDSPRITE_H

#include <Ints.h>

#include <Gruntz/GruntIdentity.h>
#include <Gruntz/GruntIndicatorSprite.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>

class CGruntSelectedSprite : public CUserLogic, public CWapX {
public:
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GRUNTSELECTEDSPRITE;
    }
    CGruntSelectedSprite() {}
    CGruntSelectedSprite(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();

    i32 BindToGrunt(i32 playerIndex, i32 unitIndex);
    i32 Update();
    GruntIdentity m_gruntIdentity;
};

#endif
