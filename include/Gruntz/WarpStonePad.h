#ifndef GRUNTZ_CWARPSTONEPAD_H
#define GRUNTZ_CWARPSTONEPAD_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CWarpStonePad : public CUserLogic, public CWapX {

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
         {SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)}

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_WARPSTONEPAD;
    }

public:
public:
    CWarpStonePad() {}
    CWarpStonePad(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif
