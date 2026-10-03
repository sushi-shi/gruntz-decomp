#ifndef GRUNTZ_CBRICKZ_H
#define GRUNTZ_CBRICKZ_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CBrickz : public CUserLogic, public CWapX {
public:
public:
    CBrickz() {}
    CBrickz(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_BRICKZ;
    }

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
          {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }

    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();
    i32 Trigger();
};

#endif
