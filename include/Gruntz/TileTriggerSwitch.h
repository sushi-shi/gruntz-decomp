#ifndef GRUNTZ_CTILETRIGGERSWITCH_H
#define GRUNTZ_CTILETRIGGERSWITCH_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CTileTriggerSwitch : public CUserLogic, public CWapX {

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_TILETRIGGERSWITCH;
    }

public:

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
          {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }

public:
    CTileTriggerSwitch() {}
    CTileTriggerSwitch(CGameObject* obj);

    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif
