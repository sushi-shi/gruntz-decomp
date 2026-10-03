#ifndef GRUNTZ_CGRUNTSTARTINGPOINT_H
#define GRUNTZ_CGRUNTSTARTINGPOINT_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CGruntStartingPoint : public CUserLogic, public CWapX {
public:

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
         {
            SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
        }
    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_GRUNTSTARTINGPOINT;
    }

public:
    CGruntStartingPoint() {}
    CGruntStartingPoint(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;

    i32 Idle();
};

#endif
