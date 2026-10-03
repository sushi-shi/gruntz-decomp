#ifndef GRUNTZ_CSTATUSBARSPRITE_H
#define GRUNTZ_CSTATUSBARSPRITE_H

#include <Ints.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CStatusBarSprite : public CUserLogic, public CWapX {
public:

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
         {
            SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
        }
    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_STATUSBARSPRITE;
    }

public:
    CStatusBarSprite() {}
    CStatusBarSprite(CGameObject* obj);
    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();
    i32 AdvanceAnim();
};

#endif
