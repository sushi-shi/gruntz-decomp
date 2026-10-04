#ifndef GRUNTZ_TILETRIGGERTRANSITION_H
#define GRUNTZ_TILETRIGGERTRANSITION_H

#include <string>

#include <Ints.h>

#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

class CTileTriggerTransition : public CUserLogic, public CWapX {
public:

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
          {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }

public:
    CTileTriggerTransition() {}
    CTileTriggerTransition(CGameObject* obj);

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_TILETRIGGERTRANSITION;
    }
    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();
    i32 ApplyAnimation(const std::string& sprite, const std::string& geom);
    i32 TransitionAct();
};

#endif
