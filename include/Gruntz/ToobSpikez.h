#ifndef GRUNTZ_CTOOBSPIKEZ_H
#define GRUNTZ_CTOOBSPIKEZ_H

#include <Ints.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>

class CToobSpikez : public CUserLogic, public CWapX {
public:
public:
    CToobSpikez() {}
    CToobSpikez(CGameObject* obj);

    i32 AdvanceAnim();

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_TOOBSPIKEZ;
    }

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, CGameObject* object)
          {
        SERIALIZE_USER_LOGIC_AND_ANIMATION_STATE(ar, mode, typeId, object)
    }
    virtual void FireActivation(i32 id)  ;
    static void RegisterActs();
};

#endif
