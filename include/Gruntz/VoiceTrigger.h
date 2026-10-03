#ifndef GRUNTZ_CVOICETRIGGER_H
#define GRUNTZ_CVOICETRIGGER_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CVoiceTrigger : public CUserLogic, public CWapX {
public:
public:
    CVoiceTrigger();
    CVoiceTrigger(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_VOICETRIGGER;
    }

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
          {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }
    virtual void FireActivation(i32 actionId)  ;
    static void RegisterActs();

    i32 Tick();
};

#endif
