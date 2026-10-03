#ifndef GRUNTZ_CGRUNTCREATIONPOINT_H
#define GRUNTZ_CGRUNTCREATIONPOINT_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGruntCreationPoint : public CUserLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GRUNTCREATIONPOINT;
    }

public:
    CGruntCreationPoint() {}
    CGruntCreationPoint(CGameObject* obj);

    static void RegisterActs();

    virtual void FireActivation(i32 id)  ;
    i32 AdvanceAnim();
};

#endif
